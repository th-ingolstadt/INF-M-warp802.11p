/** @file wlan_mac_bss_info.c
 *  @brief BSS Info Subsystem
 *
 *  This contains code tracking BSS information. It also contains code for managing
 *  the active scan state machine.
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/***************************** Include Files *********************************/

#include "wlan_mac_high_sw_config.h"

#include "xil_types.h"
#include "stdlib.h"
#include "stdio.h"
#include "wlan_platform_common.h"
#include "wlan_platform_high.h"
#include "xparameters.h"
#include "string.h"

#include "wlan_mac_pkt_buf_util.h"
#include "wlan_mac_high.h"
#include "wlan_mac_network_info.h"
#include "wlan_mac_station_info.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_mgmt_tags.h"

/*********************** Global Variable Definitions *************************/

extern platform_high_dev_info_t	 platform_high_dev_info;

/*************************** Variable Definitions ****************************/

static dl_list               network_info_free;

/// The network_info_list is stored chronologically from .first being oldest
/// and .last being newest. The "find" function search from last to first
/// to minimize search time for new BSSes you hear from often.
static dl_list               network_info_list;               	 		///< Filled network info

static dl_list               network_info_matching_ssid_list;           ///< Filled network info that match the SSID provided to wlan_mac_high_find_network_info_SSID


/*************************** Functions Prototypes ****************************/

network_info_entry_t* wlan_mac_high_find_network_info_oldest();


/******************************** Functions **********************************/

void network_info_init() {

	u32 i;
	u32 num_network_info;
	network_info_entry_t* network_info_entry_base;

	dl_list_init(&network_info_free);
	dl_list_init(&network_info_list);
	dl_list_init(&network_info_matching_ssid_list);

	// Clear the memory in the dram used for bss_infos
	bzero((void*)BSS_INFO_BUFFER_BASE, BSS_INFO_BUFFER_SIZE);

	// The number of BSS info elements we can initialize is limited by the smaller of two values:
	//     (1) The number of network_info_entry_t structs we can squeeze into BSS_INFO_DL_ENTRY_MEM_SIZE
	//     (2) The number of bss_info structs we can squeeze into BSS_INFO_BUFFER_SIZE
	num_network_info = min(BSS_INFO_DL_ENTRY_MEM_SIZE/sizeof(network_info_entry_t), BSS_INFO_BUFFER_SIZE/sizeof(network_info_t));

	// At boot, every dl_entry buffer descriptor is free
	// To set up the doubly linked list, we exploit the fact that we know the starting state is sequential.
	// This matrix addressing is not safe once the queue is used. The insert/remove helper functions should be used
	network_info_entry_base = (network_info_entry_t*)(BSS_INFO_DL_ENTRY_MEM_BASE);

	for (i = 0; i < num_network_info; i++) {
		network_info_entry_base[i].data = (void*)(BSS_INFO_BUFFER_BASE + (i*sizeof(network_info_t)));
		dl_entry_insertEnd(&network_info_free, (dl_entry*)&(network_info_entry_base[i]));
	}

	xil_printf("Network Info list (len %d) placed in DRAM: using %d kB\n", num_network_info, (num_network_info*sizeof(network_info_t))/1024);

	return;
}



void network_info_init_finish(){
	//Will be called after interrupts have been started. Safe to use scheduler now.
	wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, 10000000, SCHEDULE_REPEAT_FOREVER, (void*)network_info_timestamp_check);
}



inline void network_info_rx_process(void* pkt_buf_addr) {

	rx_frame_info_t*    	rx_frame_info   	     = (rx_frame_info_t*)pkt_buf_addr;
	void*               	mac_payload              = (u8*)pkt_buf_addr + PHY_RX_PKT_BUF_MPDU_OFFSET;
	u8*                 	mac_payload_ptr_u8       = (u8*)mac_payload;
	char*               	ssid;
	u8                  	ssid_length;
	mac_header_80211*   	rx_80211_header          = (mac_header_80211*)((void *)mac_payload_ptr_u8);
	network_info_entry_t*   curr_network_info_entry;
	network_info_t*			curr_network_info;
	u8                  	unicast_to_me;
	u8                  	to_multicast;
	u8						update_rx_power 		 = 0;
	u8						update_timestamp 		 = 0;

	u16 					length					 = rx_frame_info->phy_details.length;

	// Determine destination of packet
	unicast_to_me = wlan_addr_eq(rx_80211_header->address_1, get_mac_hw_addr_wlan());
	to_multicast  = wlan_addr_mcast(rx_80211_header->address_1);

	if ((rx_frame_info->flags & RX_FRAME_INFO_FLAGS_FCS_GOOD) && (unicast_to_me || to_multicast) ) {
		switch(rx_80211_header->frame_control_1) {
			case (MAC_FRAME_CTRL1_SUBTYPE_BEACON):
				update_rx_power = 1;
				update_timestamp = 1;
			case (MAC_FRAME_CTRL1_SUBTYPE_PROBE_RESP):
			case (MAC_FRAME_CTRL1_SUBTYPE_ASSOC_RESP):

			curr_network_info_entry = wlan_mac_high_find_network_info_BSSID(rx_80211_header->address_3);

				if(curr_network_info_entry != NULL){
					curr_network_info = curr_network_info_entry->data;

					// Remove entry from bss_info_list; Will be added back at the bottom of the function
					dl_entry_remove(&network_info_list, (dl_entry*)curr_network_info_entry);
				} else {
					// We haven't seen this BSS ID before, so we'll attempt to checkout a new dl_entry
					// struct from the free pool
					curr_network_info_entry = network_info_checkout();

					if (curr_network_info_entry == NULL){
						// No free dl_entry!
						// We'll have to reallocate the oldest entry in the filled list
						curr_network_info_entry = wlan_mac_high_find_network_info_oldest();

						if (curr_network_info_entry != NULL) {
							dl_entry_remove(&network_info_list, (dl_entry*)curr_network_info_entry);
						} else {
							xil_printf("Cannot create network_info_t.\n");
							return;
						}
					}

					curr_network_info = curr_network_info_entry->data;

					// Clear any old information from the network info
					wlan_mac_high_clear_network_info(curr_network_info);

					// Initialize the members stations list
					dl_list_init(&(curr_network_info->members));

					// Copy BSSID into network_info_t struct
					memcpy(curr_network_info->bss_config.bssid, rx_80211_header->address_3, MAC_ADDR_LEN);

					// Copy BSSID into network_info_entry_t struct
					memcpy(curr_network_info_entry->bssid, rx_80211_header->address_3, MAC_ADDR_LEN);

				}

				// Move the packet pointer to after the header
				mac_payload_ptr_u8 += sizeof(mac_header_80211);

				if((rx_80211_header->frame_control_1 == MAC_FRAME_CTRL1_SUBTYPE_BEACON) || (rx_80211_header->frame_control_1 == MAC_FRAME_CTRL1_SUBTYPE_PROBE_RESP)){
					//Set values to zero that will be updated when processing tags
					curr_network_info->capabilities = 0;
					curr_network_info->bss_config.ht_capable = 0;
					curr_network_info->bss_config.dtim_period = 0;

					// Copy beacon capabilities into bss_info struct
					//     - Only a subset of beacon capabilities are recorded
					curr_network_info->capabilities |= (((beacon_probe_frame*)mac_payload_ptr_u8)->capabilities & BSS_CAPABILITIES_BEACON_MASK);

					// Copy beacon interval into bss_info struct
					curr_network_info->bss_config.beacon_interval = ((beacon_probe_frame*)mac_payload_ptr_u8)->beacon_interval;

					// Move the packet pointer to after the beacon/probe frame
					mac_payload_ptr_u8 += sizeof(beacon_probe_frame);
				} else {
					// This must be an association response
					mac_payload_ptr_u8 += sizeof(association_response_frame);
				}

				// Copy the channel on which this packet was received into the network_info struct
				//     - chan_spec will be overwritten later in this function if a HT
				//       capabilities tag is discovered
				curr_network_info->bss_config.chan_spec.chan_pri = rx_frame_info->channel;
				curr_network_info->bss_config.chan_spec.chan_type = CHAN_TYPE_BW20;


				// Parse the tagged parameters
				while( (((u32)mac_payload_ptr_u8) - ((u32)mac_payload)) < (length - WLAN_PHY_FCS_NBYTES)) {

					// Parse each of the tags
					switch(mac_payload_ptr_u8[0]){

						//-------------------------------------------------
						case MGMT_TAG_SSID:
							// SSID parameter set
							//
							ssid        = (char*)(&(mac_payload_ptr_u8[2]));
							ssid_length = min(mac_payload_ptr_u8[1],SSID_LEN_MAX);

							memcpy(curr_network_info->bss_config.ssid, ssid, ssid_length);

							// Terminate the string
							(curr_network_info->bss_config.ssid)[ssid_length] = 0;
						break;

						//-------------------------------------------------
						case MGMT_TAG_HT_CAPABILITIES:
							curr_network_info->bss_config.ht_capable = 1;
						break;

						//-------------------------------------------------
						case MGMT_TAG_HT_OPERATION:
							curr_network_info->bss_config.chan_spec.chan_pri = mac_payload_ptr_u8[2];
							if(mac_payload_ptr_u8[2] & 0x4){
								// Channel widths larger than 20MHz are supported by this BSS
								if((mac_payload_ptr_u8[2] & 0x3) == 0x3){
									//Secondary Channel is below primary channel
									curr_network_info->bss_config.chan_spec.chan_type = CHAN_TYPE_BW40_SEC_BELOW;
								} else if((mac_payload_ptr_u8[2] & 0x3) == 0x1){
									//Secondary Channel is above primary channel
									curr_network_info->bss_config.chan_spec.chan_type = CHAN_TYPE_BW40_SEC_ABOVE;
								}
							}
						break;

						//-------------------------------------------------
						case MGMT_TAG_DSSS_PARAMETER_SET:
							// DS Parameter set (e.g. channel)
							curr_network_info->bss_config.chan_spec.chan_pri = mac_payload_ptr_u8[2];
						break;

						//-------------------------------------------------
						case MGMT_TAG_TIM:
							curr_network_info->bss_config.dtim_period = mac_payload_ptr_u8[3];
						break;
					}

					// Increment packet pointer to the next tag
					mac_payload_ptr_u8 += mac_payload_ptr_u8[1]+2;
				}

				// Update the beacon Rx power
				if (update_rx_power) curr_network_info->latest_beacon_rx_power = rx_frame_info->rx_power;

				// Update the beacon timestamp
				if (update_timestamp) curr_network_info->latest_beacon_rx_time = get_system_time_usec();

				// TODO: Potential here for a application-specific callback on new BSS capabilities

				// Add network info into bss_info_list
				dl_entry_insertEnd(&network_info_list, (dl_entry*)curr_network_info_entry);
			break;


			//---------------------------------------------------------------------
			default:
                // Only need to process MAC_FRAME_CTRL1_SUBTYPE_BEACON, MAC_FRAME_CTRL1_SUBTYPE_PROBE_RESP
				// to update BSS information.
			break;
		}
	}
}



void print_network_info(){
	int iter;
	u32 i;
	network_info_entry_t* curr_network_info_entry;
	network_info_t* curr_network_info;

	i = 0;
	iter = network_info_list.length;
	curr_network_info_entry = (network_info_entry_t*)network_info_list.last;

	// Print the header
	xil_printf("************************ Network Info *************************\n");

	while ((curr_network_info_entry != NULL) && (iter-- > 0)) {
		curr_network_info = curr_network_info_entry->data;

		xil_printf("[%d] SSID:     %s ", i, curr_network_info->bss_config.ssid);

		if(curr_network_info->capabilities & BSS_CAPABILITIES_PRIVACY){
			xil_printf("(*)");
		}
		if(curr_network_info->capabilities & BSS_CAPABILITIES_IBSS){
			xil_printf("(I)");
		}
		if(curr_network_info->flags & NETWORK_FLAGS_KEEP){
			xil_printf("(my BSS)");
		}
		xil_printf("\n");

		xil_printf("    BSSID:         %02x-%02x-%02x-%02x-%02x-%02x\n", curr_network_info->bss_config.bssid[0], curr_network_info->bss_config.bssid[1],
																		 curr_network_info->bss_config.bssid[2], curr_network_info->bss_config.bssid[3],
																		 curr_network_info->bss_config.bssid[4], curr_network_info->bss_config.bssid[5]);
		xil_printf("    Channel:       %d\n", wlan_mac_high_bss_channel_spec_to_radio_chan(curr_network_info->bss_config.chan_spec));


		xil_printf("    Last update:   %d msec ago\n", (u32)((get_system_time_usec() - curr_network_info->latest_beacon_rx_time)/1000));

		xil_printf("    Capabilities:  0x%04x\n", curr_network_info->capabilities);
		curr_network_info_entry = dl_entry_prev(curr_network_info_entry);
		i++;
	}
}



void network_info_timestamp_check() {
	network_info_entry_t* curr_network_info_entry;
	network_info_t* curr_network_info;

	curr_network_info_entry = (network_info_entry_t*)network_info_list.first;

	while(curr_network_info_entry != NULL){
		curr_network_info = curr_network_info_entry->data;

		if((get_system_time_usec() - curr_network_info->latest_beacon_rx_time) > NETWORK_INFO_TIMEOUT_USEC){
			if((curr_network_info->flags & NETWORK_FLAGS_KEEP) == 0){
				wlan_mac_high_clear_network_info(curr_network_info);
				dl_entry_remove(&network_info_list, (dl_entry*)curr_network_info_entry);
				network_info_checkin(curr_network_info_entry);
			}
		} else {
			// Nothing after this entry is older, so it's safe to quit
			return;
		}

		curr_network_info_entry = dl_entry_next(curr_network_info_entry);
	}
}



network_info_entry_t* network_info_checkout(){
	network_info_entry_t* network_info_entry;
	network_info_t* curr_network_info;

	if(network_info_free.length > 0){
		network_info_entry = ((network_info_entry_t*)(network_info_free.first));
		dl_entry_remove(&network_info_free,network_info_free.first);

		curr_network_info = network_info_entry->data;
		dl_list_init(&(curr_network_info->members));
		return network_info_entry;
	} else {
		return NULL;
	}
}

void network_info_checkin(network_info_entry_t* network_info_entry){
	dl_entry_insertEnd(&network_info_free, (dl_entry*)network_info_entry);
	return;
}


dl_list* wlan_mac_high_find_network_info_SSID(char* ssid){
	// This function will return a pointer to a dl_list that contains every
	// network_info_t struct that matches the SSID argument.

    int       iter;
	network_info_entry_t* curr_network_info_entry_primary_list;
	network_info_entry_t* curr_network_info_entry_match_list;
	network_info_entry_t* next_network_info_entry_match_list;
	network_info_t* curr_network_info;

	// Remove/free any members of network_info_matching_ssid_list that exist since the last time
	// this function was called

	iter = network_info_matching_ssid_list.length;
	curr_network_info_entry_match_list = (network_info_entry_t*)network_info_matching_ssid_list.first;

	while ((curr_network_info_entry_match_list != NULL) && (iter-- > 0)) {
		next_network_info_entry_match_list = dl_entry_next(curr_network_info_entry_match_list);
		dl_entry_remove(&network_info_matching_ssid_list, (dl_entry*)curr_network_info_entry_match_list);
		wlan_mac_high_free(curr_network_info_entry_match_list);
		curr_network_info_entry_match_list = next_network_info_entry_match_list;
	}

	// At this point in the code, network_info_matching_ssid_list is guaranteed to be empty.
	// We will fill it with new network_info_entry_t that point to existing network_info_t structs that
	// match the SSID argument to this function. Note: these network_info_t structs will continue
	// to be pointed to by the existing network_info_entry_t

	iter = network_info_list.length;
	curr_network_info_entry_primary_list = (network_info_entry_t*)network_info_list.last;

	while ((curr_network_info_entry_primary_list != NULL) && (iter-- > 0)) {
		curr_network_info = curr_network_info_entry_primary_list->data;

		if (strcmp(ssid, curr_network_info->bss_config.ssid) == 0) {
			curr_network_info_entry_match_list = wlan_mac_high_malloc(sizeof(network_info_entry_t));
			curr_network_info_entry_match_list->data = curr_network_info;
			memcpy(curr_network_info_entry_match_list->bssid, curr_network_info->bss_config.bssid, MAC_ADDR_LEN);
			dl_entry_insertEnd(&network_info_matching_ssid_list, (dl_entry*)curr_network_info_entry_match_list);
		}

		curr_network_info_entry_primary_list = dl_entry_prev(curr_network_info_entry_primary_list);
	}

	return &network_info_matching_ssid_list;
}


network_info_entry_t* wlan_mac_high_find_network_info_BSSID(u8* bssid){
	int						iter;
	network_info_entry_t* 	curr_network_info_entry;

	iter = network_info_list.length;
	curr_network_info_entry = (network_info_entry_t*)network_info_list.last;

	while ((curr_network_info_entry != NULL) && (iter-- > 0)) {
		if (wlan_addr_eq(bssid,curr_network_info_entry->bssid)) {
			return curr_network_info_entry;
		}
		curr_network_info_entry = dl_entry_prev(curr_network_info_entry);
	}
	return NULL;
}

network_info_entry_t* wlan_mac_high_find_network_info_oldest(){
	int iter;
	network_info_entry_t* curr_network_info_entry;
	network_info_t* curr_network_info;

	iter = network_info_list.length;
	curr_network_info_entry = (network_info_entry_t*)network_info_list.first;

	while ((curr_network_info_entry != NULL) && (iter-- > 0)) {
		curr_network_info = curr_network_info_entry->data;

		if ((curr_network_info->flags & NETWORK_FLAGS_KEEP) == 0) {
			return curr_network_info_entry;
		}

		curr_network_info_entry = dl_entry_next(curr_network_info_entry);
	}
	return NULL;
}



// Function will create a bss_info and make sure that the BSS ID is unique
// in the bss_info list.
//
network_info_t* wlan_mac_high_create_network_info(u8* bssid, char* ssid, u8 chan){
	network_info_entry_t* curr_network_info_entry;
	network_info_t* curr_network_info = NULL;

	curr_network_info_entry = wlan_mac_high_find_network_info_BSSID(bssid);

	if (curr_network_info_entry != NULL){
		// Get the BSS info from the entry
		curr_network_info = curr_network_info_entry->data;

		// Remove the entry from the info list so it can be added back later
		dl_entry_remove(&network_info_list, (dl_entry*)curr_network_info_entry);
	} else {
		// Have not seen this BSS ID before; attempt to grab a new network_info_entry_t
		// struct from the free pool
		curr_network_info_entry = network_info_checkout();

		if (curr_network_info_entry == NULL){
			// No free network_info_entry_t; Re-allocate the oldest entry in the filled list
			curr_network_info_entry = wlan_mac_high_find_network_info_oldest();

			if (curr_network_info_entry != NULL) {
				dl_entry_remove(&network_info_list, (dl_entry*)curr_network_info_entry);
			} else {
				xil_printf("Cannot create network_info_t.\n");
				return NULL;
			}
		}

		// Get the network_info_t from the entry
		curr_network_info = curr_network_info_entry->data;

		// Clear any old information from the network info
		wlan_mac_high_clear_network_info(curr_network_info);

		// Initialize the associated stations list
		dl_list_init(&(curr_network_info->members));

		// Copy the BSS ID to the network_info_t struct
		memcpy(curr_network_info->bss_config.bssid, bssid, MAC_ADDR_LEN);

		// Copy the BSS ID to the network_info_entry_t struct
		memcpy(curr_network_info_entry->bssid, bssid, MAC_ADDR_LEN);
	}

	// Update the fields of the BSS Info
	strncpy(curr_network_info->bss_config.ssid, ssid, SSID_LEN_MAX);
	curr_network_info->bss_config.chan_spec.chan_pri       = chan;
	curr_network_info->bss_config.chan_spec.chan_type      = CHAN_TYPE_BW20;
	curr_network_info->latest_beacon_rx_time    = get_system_time_usec();

	//
	// The following fields have their previous value retained if the were
	// in the network list (i.e. wlan_mac_high_find_bss_info_BSSID() returned
	// a non-NULL entry):
	//    - latest_beacon_rx_power
	//    - capabilities
	//    - beacon_interval
	//    - flags
	//    - station_info_list
	//

	// Insert the updated entry into the network list
	dl_entry_insertEnd(&network_info_list, (dl_entry*)curr_network_info_entry);

	return curr_network_info;
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
	network_info_entry_t* next_network_info_entry = (network_info_entry_t*)network_info_list.first;
	network_info_entry_t* curr_network_info_entry;
    network_info_t * curr_network_info;
    int		   iter = network_info_list.length;

	while( (next_network_info_entry != NULL) && (iter-- > 0) ){
		curr_network_info_entry = next_network_info_entry;
		next_network_info_entry = dl_entry_next(curr_network_info_entry);
		curr_network_info = curr_network_info_entry->data;

		if( (curr_network_info->flags & NETWORK_FLAGS_KEEP) == 0){
			wlan_mac_high_clear_network_info(curr_network_info);
			dl_entry_remove(&network_info_list, (dl_entry*)curr_network_info_entry);
			network_info_checkin(curr_network_info_entry);
		}
	}
}

void wlan_mac_high_clear_network_info(network_info_t* info){
	int            iter;
	station_info_t * curr_station_info;
	station_info_entry_t* next_station_info_entry;
	station_info_entry_t* curr_station_info_entry;

	if (info != NULL){
        // Remove any station infos
		iter                    = info->members.length;
		next_station_info_entry = (station_info_entry_t*)(info->members.first);

		if (((info->flags & NETWORK_FLAGS_KEEP) == 0) && (next_station_info_entry != NULL)) {
            xil_printf("WARNING:  network_info_t not flagged to be kept but contained station_info_t entries.\n");
		}

		// Set any members to ~STATION_INFO_FLAG_KEEP so the framework can remove them
		while ((next_station_info_entry != NULL) && (iter-- > 0)) {
			curr_station_info_entry = next_station_info_entry;
			next_station_info_entry = dl_entry_next(curr_station_info_entry);

			curr_station_info       = curr_station_info_entry->data;

			// Remove the "keep" flag for this station_info so the framework can cleanup later.
			curr_station_info->flags &= ~STATION_INFO_FLAG_KEEP;
			station_info_remove(&info->members, curr_station_info->addr);
		}


		// Clear the bss_info
        bzero(info, sizeof(network_info_t));

        // Set beacon_interval to "unknown"
        info->bss_config.beacon_interval = BEACON_INTERVAL_UNKNOWN;
	}
}



inline dl_list* wlan_mac_high_get_network_info_list(){
	return &network_info_list;
}
