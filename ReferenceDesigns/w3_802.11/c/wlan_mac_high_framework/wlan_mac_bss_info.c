/** @file wlan_mac_bss_info.c
 *  @brief BSS Info Subsystem
 *
 *  This contains code tracking BSS information. It also contains code for managing
 *  the active scan state machine.
 *
 *  @copyright Copyright 2014-2015, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

/***************************** Include Files *********************************/

#include "xil_types.h"
#include "stdlib.h"
#include "stdio.h"
#include "xparameters.h"
#include "string.h"

#include "wlan_mac_pkt_buf_util.h"
#include "wlan_mac_time_util.h"
#include "wlan_mac_high.h"
#include "wlan_mac_bss_info.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_schedule.h"

/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/

static dl_list               bss_info_free;                			///< Free BSS info

/// The bss_info_list is stored chronologically from .first being oldest
/// and .last being newest. The "find" function search from last to first
/// to minimize search time for new BSSes you hear from often.
static dl_list               bss_info_list;               	 		///< Filled BSS info

static dl_list               bss_info_matching_ssid_list;           ///< Filled BSS info that match the SSID provided to wlan_mac_high_find_bss_info_SSID


/*************************** Functions Prototypes ****************************/

dl_entry* wlan_mac_high_find_bss_info_oldest();


/******************************** Functions **********************************/

void bss_info_init(u8 dram_present){

	u32       i;
	u32       num_bss_info;
	dl_entry* dl_entry_base;

	dl_list_init(&bss_info_free);
	dl_list_init(&bss_info_list);
	dl_list_init(&bss_info_matching_ssid_list);

	if(dram_present){
		// Clear the memory in the dram used for bss_infos
		bzero((void*)BSS_INFO_BUFFER_BASE, BSS_INFO_BUFFER_SIZE);

		// The number of BSS info elements we can initialize is limited by the smaller of two values:
		//     (1) The number of dl_entry structs we can squeeze into BSS_INFO_DL_ENTRY_MEM_SIZE
		//     (2) The number of bss_info structs we can squeeze into BSS_INFO_BUFFER_SIZE
		num_bss_info = min(BSS_INFO_DL_ENTRY_MEM_SIZE/sizeof(dl_entry), BSS_INFO_BUFFER_SIZE/sizeof(bss_info));

		// At boot, every dl_entry buffer descriptor is free
		// To set up the doubly linked list, we exploit the fact that we know the starting state is sequential.
		// This matrix addressing is not safe once the queue is used. The insert/remove helper functions should be used
		dl_entry_base = (dl_entry*)(BSS_INFO_DL_ENTRY_MEM_BASE);

		for (i = 0; i < num_bss_info; i++) {
			dl_entry_base[i].data = (void*)(BSS_INFO_BUFFER_BASE + (i*sizeof(bss_info)));
			dl_entry_insertEnd(&bss_info_free, &(dl_entry_base[i]));
		}

		xil_printf("BSS Info list (len %d) placed in DRAM: using %d kB\n", num_bss_info, (num_bss_info*sizeof(bss_info))/1024);

	} else {
		xil_printf("Error initializing BSS info subsystem\n");
	}
}



void bss_info_init_finish(){
	//Will be called after interrupts have been started. Safe to use scheduler now.
	wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, 10000000, SCHEDULE_REPEAT_FOREVER, (void*)bss_info_timestamp_check);
}



inline void bss_info_rx_process(void* pkt_buf_addr) {

	rx_frame_info*      mpdu_info                = (rx_frame_info*)pkt_buf_addr;
	void*               mpdu                     = (u8*)pkt_buf_addr + PHY_RX_PKT_BUF_MPDU_OFFSET;
	u8*                 mpdu_ptr_u8              = (u8*)mpdu;
	char*               ssid;
	u8                  ssid_length;
	mac_header_80211*   rx_80211_header          = (mac_header_80211*)((void *)mpdu_ptr_u8);
	dl_entry*			curr_dl_entry;
	bss_info*			curr_bss_info;
	u32 				i;

	u16 				length					 = mpdu_info->phy_details.length;

	if( (mpdu_info->state == RX_MPDU_STATE_FCS_GOOD)){
		switch(rx_80211_header->frame_control_1) {
			case (MAC_FRAME_CTRL1_SUBTYPE_BEACON):
			case (MAC_FRAME_CTRL1_SUBTYPE_PROBE_RESP):

				curr_dl_entry = wlan_mac_high_find_bss_info_BSSID(rx_80211_header->address_3);

				if(curr_dl_entry != NULL){
					curr_bss_info = (bss_info*)(curr_dl_entry->data);
					dl_entry_remove(&bss_info_list, curr_dl_entry);
				} else {
					// We haven't seen this BSS ID before, so we'll attempt to checkout a new dl_entry
					// struct from the free pool
					curr_dl_entry = bss_info_checkout();

					if (curr_dl_entry == NULL){
						// No free dl_entry!
						// We'll have to reallocate the oldest entry in the filled list
						curr_dl_entry = wlan_mac_high_find_bss_info_oldest();

						if (curr_dl_entry != NULL) {
							dl_entry_remove(&bss_info_list, curr_dl_entry);
						} else {
							xil_printf("Cannot create bss_info.\n");
							return;
						}
					}

					curr_bss_info = (bss_info*)(curr_dl_entry->data);

					// Clear any old information from the BSS info
					wlan_mac_high_clear_bss_info(curr_bss_info);

					curr_bss_info->last_join_attempt_result = NEVER_ATTEMPTED;

					// Initialize the associated stations list
					dl_list_init(&(curr_bss_info->associated_stations));

					// Copy BSSID into bss_info struct
					memcpy(curr_bss_info->bssid, rx_80211_header->address_3, BSSID_LEN);

					// Set the state to BSS_STATE_UNAUTHENTICATED since we have not seen this BSS info before
				    curr_bss_info->state     = BSS_STATE_UNAUTHENTICATED;

				    // Default the PHY mode to 802.11g/a. This will be overwritten if a beacon/probe resp is
				    // observed that contains HT fields
				    curr_bss_info->phy_mode = PHY_MODE_NONHT;
				}

				// Update the AP information
				curr_bss_info->num_basic_rates = 0;

				// Move the packet pointer to after the header
				mpdu_ptr_u8 += sizeof(mac_header_80211);

				// Copy capabilities into bss_info struct
				curr_bss_info->capabilities = ((beacon_probe_frame*)mpdu_ptr_u8)->capabilities;

				// Copy beacon interval into bss_info struct
				curr_bss_info->beacon_interval = ((beacon_probe_frame*)mpdu_ptr_u8)->beacon_interval;

				// Copy the channel on which this packet was received into the bss_info struct
				curr_bss_info->chan = mpdu_info->channel;

				// Copy the Rx power with which this packet was received into the bss_info stuct
				curr_bss_info->rx_power_dBm = mpdu_info->rx_power;

				// Move the packet pointer to after the beacon/probe frame
				mpdu_ptr_u8 += sizeof(beacon_probe_frame);

				// Parse the tagged parameters
				while( (((u32)mpdu_ptr_u8) - ((u32)mpdu)) < (length - WLAN_PHY_FCS_NBYTES)) {

					// Parse each of the tags
					switch(mpdu_ptr_u8[0]){

						//-------------------------------------------------
						case TAG_SSID_PARAMS:
							// SSID parameter set
							//
							ssid        = (char*)(&(mpdu_ptr_u8[2]));
							ssid_length = min(mpdu_ptr_u8[1],SSID_LEN_MAX);

							memcpy(curr_bss_info->ssid, ssid, ssid_length);

							// Terminate the string
							(curr_bss_info->ssid)[ssid_length] = 0;
						break;

						//-------------------------------------------------
						case TAG_SUPPORTED_RATES:
						case TAG_EXT_SUPPORTED_RATES:
							// Supported rates / Extended supported rates
							//
							for( i = 0; i < mpdu_ptr_u8[1]; i++){
								if( (mpdu_ptr_u8[2+i] & RATE_BASIC) == RATE_BASIC ) {

									// This is a basic rate. It is required by the AP in order to associate.
									if((curr_bss_info->num_basic_rates) < NUM_BASIC_RATES_MAX){
										if( wlan_mac_high_valid_tagged_rate(mpdu_ptr_u8[2+i]) ){
											(curr_bss_info->basic_rates)[(curr_bss_info->num_basic_rates)] = mpdu_ptr_u8[2+i];
											(curr_bss_info->num_basic_rates)++;
										} else {
											// xil_printf("Invalid Tag Parameter rate: %d\n", mpdu_ptr_u8[2+i]);
										}
									} else {
										// xil_printf("Number of basic rates exceeded.\n");
									}
								}
							}
						break;

						//-------------------------------------------------
						case TAG_HT_CAPABILITIES:
							curr_bss_info->phy_mode = PHY_MODE_HTMF;
						break;
						//-------------------------------------------------
						case TAG_HT_INFORMATION: //TODO -- there is more to pull from HT information than a channel once we add HT support
						case TAG_DS_PARAMS:
							// DS Parameter set (e.g. channel)
							//
							// Note: this overrides the the rx_frame_info channel
							// in the case of DSSS since DSSS receptions are prone to
							// being received off-channel
							curr_bss_info->chan = mpdu_ptr_u8[2];
						break;
					}

					// Increment packet pointer to the next tag
					mpdu_ptr_u8 += mpdu_ptr_u8[1]+2;
				}

				curr_bss_info->latest_activity_timestamp = get_system_time_usec();
				dl_entry_insertEnd(&bss_info_list,curr_dl_entry);
			break;


			//---------------------------------------------------------------------
			default:
                // Only need to process MAC_FRAME_CTRL1_SUBTYPE_BEACON, MAC_FRAME_CTRL1_SUBTYPE_PROBE_RESP
				// to update BSS information.
			break;
		}
	}
}



void print_bss_info(){
	int       iter;
	u32       i;
	dl_entry* curr_dl_entry;
	bss_info* curr_bss_info;


	i = 0;
	iter          = bss_info_list.length;
	curr_dl_entry = bss_info_list.last;

	// Print the header
	xil_printf("************************ BSS Info *************************\n");

	while ((curr_dl_entry != NULL) && (iter-- > 0)) {
		curr_bss_info = (bss_info*)(curr_dl_entry->data);

		xil_printf("[%d] SSID:     %s ", i, curr_bss_info->ssid);

		if(curr_bss_info->capabilities & CAPABILITIES_PRIVACY){
			xil_printf("(*)");
		}
		if(curr_bss_info->capabilities & CAPABILITIES_IBSS){
			xil_printf("(I)");
		}
		xil_printf("\n");

		xil_printf("    BSSID:         %02x-%02x-%02x-%02x-%02x-%02x\n", curr_bss_info->bssid[0],curr_bss_info->bssid[1],curr_bss_info->bssid[2],curr_bss_info->bssid[3],curr_bss_info->bssid[4],curr_bss_info->bssid[5]);
		xil_printf("    Channel:       %d\n",curr_bss_info->chan);

		if((curr_bss_info->flags & BSS_FLAGS_KEEP) == 0){
			xil_printf("    Last update:   %d msec ago\n", (u32)((get_system_time_usec() - curr_bss_info->latest_activity_timestamp)/1000));
		}
		xil_printf("    Capabilities:  0x%04x\n", curr_bss_info->capabilities);
		curr_dl_entry = dl_entry_prev(curr_dl_entry);
		i++;
	}
}



void bss_info_timestamp_check() {
	dl_entry* curr_dl_entry;
	bss_info* curr_bss_info;

	curr_dl_entry = bss_info_list.first;

	while(curr_dl_entry != NULL){
		curr_bss_info = (bss_info*)(curr_dl_entry->data);

		if((get_system_time_usec() - curr_bss_info->latest_activity_timestamp) > BSS_INFO_TIMEOUT_USEC){
			if((curr_bss_info->flags & BSS_FLAGS_KEEP) == 0){
				wlan_mac_high_clear_bss_info(curr_bss_info);
				dl_entry_remove(&bss_info_list, curr_dl_entry);
				bss_info_checkin(curr_dl_entry);
			}
		} else {
			// Nothing after this entry is older, so it's safe to quit
			return;
		}

		curr_dl_entry = dl_entry_next(curr_dl_entry);
	}
}



dl_entry* bss_info_checkout(){
	dl_entry* bsi;
	bss_info* curr_bss_info;

	if(bss_info_free.length > 0){
		bsi = ((dl_entry*)(bss_info_free.first));
		dl_entry_remove(&bss_info_free,bss_info_free.first);

		curr_bss_info = (bss_info*)(bsi->data);
		dl_list_init(&(curr_bss_info->associated_stations));
		return bsi;
	} else {
		return NULL;
	}
}



void bss_info_checkin(dl_entry* bsi){
	dl_entry_insertEnd(&bss_info_free, (dl_entry*)bsi);
	return;
}


dl_list* wlan_mac_high_find_bss_info_SSID(char* ssid){
	//This function will return a pointer to a dl_list that contains every
	//bss_info struct that matches the SSID argument.

    int       iter;
	dl_entry* curr_dl_entry_primary_list;
	dl_entry* curr_dl_entry_match_list;
	dl_entry* next_dl_entry_match_list;
	bss_info* curr_bss_info;

	// Remove/free any members of bss_info_matching_ssid_list that exist since the last time
	// this function was called

	iter          = bss_info_matching_ssid_list.length;
	curr_dl_entry_match_list = bss_info_matching_ssid_list.first;
	while ((curr_dl_entry_match_list != NULL) && (iter-- > 0)) {
		next_dl_entry_match_list = dl_entry_next(curr_dl_entry_match_list);
		wlan_mac_high_free(curr_dl_entry_match_list);
		curr_dl_entry_match_list = next_dl_entry_match_list;
	}

	// At this point in the code, bss_info_matching_ssid_list is guaranteed to be empty.
	// We will fill it with new dl_entry that point to existing bss_info structs that
	// match the SSID argument to this function. Note: these bss_info structs will continue
	// to be pointed to by the dl_entry

	iter          = bss_info_list.length;
	curr_dl_entry_primary_list = bss_info_list.last;
	while ((curr_dl_entry_primary_list != NULL) && (iter-- > 0)) {
		curr_bss_info = (bss_info*)(curr_dl_entry_primary_list->data);

		if (strcmp(ssid,curr_bss_info->ssid) == 0) {
			curr_dl_entry_match_list = wlan_mac_high_malloc(sizeof(dl_entry));
			curr_dl_entry_match_list->data = (void*)(curr_bss_info);
			dl_entry_insertEnd(&bss_info_matching_ssid_list, curr_dl_entry_match_list);
		}

		curr_dl_entry_primary_list = dl_entry_prev(curr_dl_entry_primary_list);
	}

	return &bss_info_matching_ssid_list;
}


dl_entry* wlan_mac_high_find_bss_info_BSSID(u8* bssid){
	int       iter;
	dl_entry* curr_dl_entry;
	bss_info* curr_bss_info;

	iter          = bss_info_list.length;
	curr_dl_entry = bss_info_list.last;

	while ((curr_dl_entry != NULL) && (iter-- > 0)) {
		curr_bss_info = (bss_info*)(curr_dl_entry->data);

		if (wlan_addr_eq(bssid,curr_bss_info->bssid)) {
			return curr_dl_entry;
		}

		curr_dl_entry = dl_entry_prev(curr_dl_entry);
	}
	return NULL;
}


dl_entry* wlan_mac_high_find_bss_info_oldest(){
	int       iter;
	dl_entry* curr_dl_entry;
	bss_info* curr_bss_info;

	iter          = bss_info_list.length;
	curr_dl_entry = bss_info_list.first;

	while ((curr_dl_entry != NULL) && (iter-- > 0)) {
		curr_bss_info = (bss_info*)(curr_dl_entry->data);

		if ((curr_bss_info->flags & BSS_FLAGS_KEEP) == 0) {
			return curr_dl_entry;
		}

		curr_dl_entry = dl_entry_next(curr_dl_entry);
	}
	return NULL;
}



// Function will create a bss_info and make sure that the BSS ID is unique
// in the bss_info list.
//
bss_info* wlan_mac_high_create_bss_info(u8* bssid, char* ssid, u8 chan){
	dl_entry * curr_dl_entry;
	bss_info * curr_bss_info = NULL;

	curr_dl_entry = wlan_mac_high_find_bss_info_BSSID(bssid);

	if (curr_dl_entry != NULL){
		curr_bss_info = (bss_info*)(curr_dl_entry->data);
		dl_entry_remove(&bss_info_list, curr_dl_entry);
	} else {
		// We haven't seen this BSS ID before, so we'll attempt to grab a new dl_entry
		// struct from the free pool
		curr_dl_entry = bss_info_checkout();

		if (curr_dl_entry == NULL){
			// No free dl_entry!
			// We'll have to reallocate the oldest entry in the filled list
			curr_dl_entry = wlan_mac_high_find_bss_info_oldest();

			if (curr_dl_entry != NULL) {
				dl_entry_remove(&bss_info_list, curr_dl_entry);
			} else {
				xil_printf("Cannot create bss_info.\n");
				return NULL;
			}
		}

		curr_bss_info = (bss_info*)(curr_dl_entry->data);

		// Clear any old information from the BSS info
		wlan_mac_high_clear_bss_info(curr_bss_info);

		curr_bss_info->last_join_attempt_result = NEVER_ATTEMPTED;

		// Initialize the associated stations list
		dl_list_init(&(curr_bss_info->associated_stations));

		// Copy the BSS ID to the entry
		memcpy(curr_bss_info->bssid, bssid, BSSID_LEN);
	}

	// Update the fields of the BSS Info
	strcpy(curr_bss_info->ssid,ssid);
	curr_bss_info->chan                      = chan;
	curr_bss_info->latest_activity_timestamp = get_system_time_usec();
    curr_bss_info->state                     = BSS_STATE_UNAUTHENTICATED;

	dl_entry_insertEnd(&bss_info_list, curr_dl_entry);

	return curr_bss_info;
}

/**
 * @brief Reset List of Networks
 *
 * Reset all BSS Info except ones flagged to be kept
 *
 * @param  None
 * @return None
 */
void wlan_mac_high_reset_network_list(){
	dl_list  * bss_info_list = wlan_mac_high_get_bss_info_list();
	dl_entry * next_dl_entry = bss_info_list->first;
	dl_entry * curr_dl_entry;
    bss_info * curr_bss_info;
    int		   iter = bss_info_list->length;

	while( (next_dl_entry != NULL) && (iter-- > 0) ){
		curr_dl_entry = next_dl_entry;
		next_dl_entry = dl_entry_next(curr_dl_entry);
		curr_bss_info = (bss_info *)(curr_dl_entry->data);

		if( (curr_bss_info->flags & BSS_FLAGS_KEEP) == 0){
			wlan_mac_high_clear_bss_info(curr_bss_info);
			dl_entry_remove(bss_info_list, curr_dl_entry);
			bss_info_checkin(curr_dl_entry);
		}
	}
}

void wlan_mac_high_clear_bss_info(bss_info * info){
	int            iter;
	station_info * curr_station_info;
	dl_entry     * next_station_info_entry;
	dl_entry     * curr_station_info_entry;

	if (info != NULL){
        // Remove any station infos
		iter                    = info->associated_stations.length;
		next_station_info_entry = info->associated_stations.first;

		if (((info->flags & BSS_FLAGS_KEEP) == 0) && (next_station_info_entry != NULL)) {
            xil_printf("WARNING:  BSS info not flagged to be kept but contained station_info entries.\n");
		}

		while ((next_station_info_entry != NULL) && (iter-- > 0)) {
			curr_station_info_entry = next_station_info_entry;
			next_station_info_entry = dl_entry_next(curr_station_info_entry);
			curr_station_info       = (station_info*)(curr_station_info_entry->data);
			wlan_mac_high_remove_association(&info->associated_stations, get_counts(), curr_station_info->addr);
		}

		// Clear the bss_info
        bzero(info, sizeof(bss_info));
	}
}



inline dl_list* wlan_mac_high_get_bss_info_list(){
	return &bss_info_list;
}
