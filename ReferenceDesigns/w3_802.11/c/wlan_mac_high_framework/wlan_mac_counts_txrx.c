/** @file wlan_mac_counts_txrx.c
 *  @brief Tx/Rx Counts Subsytem
 *
 *  This contains code tracking transmissions and reception counts.
 *
 *  @copyright Copyright 2014-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
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
#include "wlan_mac_counts_txrx.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_schedule.h"

/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/

static dl_list               counts_txrx_free;                		///< Free Counts

/// The counts_txrx_list is stored chronologically from .first being oldest
/// and .last being newest. The "find" function search from last to first
/// to minimize search time for new BSSes you hear from often.
static dl_list               counts_txrx_list;               	 	///< Filled Counts


/*************************** Functions Prototypes ****************************/

dl_entry* find_counts_txrx_oldest();


/******************************** Functions **********************************/

void counts_txrx_init(u8 dram_present){

	u32       i;
	u32       num_counts_txrx;
	dl_entry* dl_entry_base;

	dl_list_init(&counts_txrx_free);
	dl_list_init(&counts_txrx_list);

	if(dram_present){
		// Clear the memory in the dram used for bss_infos
		bzero((void*)COUNTS_TXRX_BUFFER_BASE, COUNTS_TXRX_BUFFER_SIZE);

		// The number of elements we can initialize is limited by the smaller of two values:
		//     (1) The number of dl_entry structs we can squeeze into COUNTS_TXRX_DL_ENTRY_MEM_SIZE
		//     (2) The number of counts_txrx structs we can squeeze into COUNTS_TXRX_BUFFER_SIZE
		num_counts_txrx = min(COUNTS_TXRX_DL_ENTRY_MEM_SIZE/sizeof(dl_entry), COUNTS_TXRX_BUFFER_SIZE/sizeof(counts_txrx_t));

		// At boot, every dl_entry buffer descriptor is free
		// To set up the doubly linked list, we exploit the fact that we know the starting state is sequential.
		// This matrix addressing is not safe once the queue is used. The insert/remove helper functions should be used
		dl_entry_base = (dl_entry*)(COUNTS_TXRX_DL_ENTRY_MEM_BASE);

		for (i = 0; i < num_counts_txrx; i++) {
			dl_entry_base[i].data = (void*)(COUNTS_TXRX_BUFFER_BASE + (i*sizeof(counts_txrx_t)));
			dl_entry_insertEnd(&counts_txrx_free, &(dl_entry_base[i]));
		}

		xil_printf("Counts Tx/Rx list (len %d) placed in DRAM: using %d kB\n", num_counts_txrx, (num_counts_txrx*sizeof(counts_txrx_t))/1024);

	} else {
		xil_printf("Error initializing Counts Tx/Rx subsystem\n");
	}
}



void counts_txrx_init_finish(){
	//Will be called after interrupts have been started. Safe to use scheduler now.
	wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, 10000000, SCHEDULE_REPEAT_FOREVER, (void*)counts_txrx_timestamp_check);
}


inline void counts_txrx_tx_process(void* pkt_buf_addr) {
	tx_frame_info_t*      	tx_frame_info  	  = (tx_frame_info_t*)pkt_buf_addr;
	mac_header_80211* 		tx_80211_header   = (mac_header_80211*)((u8*)tx_frame_info + PHY_TX_PKT_BUF_MPDU_OFFSET);
	dl_entry*				curr_dl_entry;
	counts_txrx_t*			curr_counts_txrx;
	frame_counts_txrx_t*	frame_counts_txrx;
	u8						pkt_type;

	pkt_type = (tx_80211_header->frame_control_1 & MAC_FRAME_CTRL1_MASK_TYPE);

	curr_dl_entry = wlan_mac_high_find_counts_txrx_addr(tx_80211_header->address_1);

	if(curr_dl_entry != NULL){
		curr_counts_txrx = (counts_txrx_t*)(curr_dl_entry->data);

		// Remove entry from counts_txrx_list; Will be added back at the bottom of the function
		// This serves to sort the list and keep the most recently updated entries at the tail.
		dl_entry_remove(&counts_txrx_list, curr_dl_entry);
	} else {
		// We haven't seen this addr before, so we'll attempt to checkout a new dl_entry
		// struct from the free pool
		curr_dl_entry = counts_txrx_checkout();

		if (curr_dl_entry == NULL){
			// No free dl_entry!
			// We'll have to reallocate the oldest entry in the filled list
			curr_dl_entry = find_counts_txrx_oldest();

			if (curr_dl_entry != NULL) {
				dl_entry_remove(&counts_txrx_list, curr_dl_entry);
			} else {
				xil_printf("Cannot create counts_txrx.\n");
				return;
			}
		}

		curr_counts_txrx = (counts_txrx_t*)(curr_dl_entry->data);

		// Clear any old information from the Tx/Rx counts
		wlan_mac_high_clear_counts_txrx(curr_counts_txrx);

		// Copy BSSID into bss_info struct
		memcpy(curr_counts_txrx->addr, tx_80211_header->address_1, MAC_ADDR_LEN);
	}

	// By this point in the function, curr_counts_txrx is guaranteed to be pointing to a valid counts_txrx_t struct
	// that we should update with this reception.

	// Update the latest TXRX time
	curr_counts_txrx->latest_txrx_timestamp = get_system_time_usec();


	switch(pkt_type){
		default:
			//Unknown type
			return;
		break;
		case MAC_FRAME_CTRL1_TYPE_DATA:
			frame_counts_txrx = &(curr_counts_txrx->data);
		break;
		case MAC_FRAME_CTRL1_TYPE_MGMT:
			frame_counts_txrx = &(curr_counts_txrx->mgmt);
		break;
	}

	(frame_counts_txrx->tx_num_packets_total)++;
	(frame_counts_txrx->tx_num_bytes_total) += (tx_frame_info->length);
	(frame_counts_txrx->tx_num_attempts)    += (tx_frame_info->num_tx_attempts);

	if((tx_frame_info->tx_result) == TX_MPDU_RESULT_SUCCESS){
		(frame_counts_txrx->tx_num_packets_success)++;
		(frame_counts_txrx->tx_num_bytes_success) += tx_frame_info->length;
	}

	// Add Tx/Rx Counts into counts_txrx_list
	dl_entry_insertEnd(&counts_txrx_list, curr_dl_entry);

}

inline void counts_txrx_rx_process(void* pkt_buf_addr) {
	rx_frame_info_t*    	rx_frame_info   	     = (rx_frame_info_t*)pkt_buf_addr;
	void*               	mac_payload              = (u8*)pkt_buf_addr + PHY_RX_PKT_BUF_MPDU_OFFSET;
	mac_header_80211*   	rx_80211_header          = (mac_header_80211*)((void *)mac_payload);
	dl_entry*				curr_dl_entry;
	counts_txrx_t*			curr_counts_txrx;
	frame_counts_txrx_t*	frame_counts_txrx;
	u8						pkt_type;
	u8						perform_duplicate_check  = 0;
	u16						rx_seq;
	u16 					length   = rx_frame_info->phy_details.length;

	pkt_type = (rx_80211_header->frame_control_1 & MAC_FRAME_CTRL1_MASK_TYPE);

	if ( (rx_frame_info->flags & RX_FRAME_INFO_FLAGS_FCS_GOOD) &&
			(pkt_type !=  MAC_FRAME_CTRL1_TYPE_CTRL)) {
		// We will only consider good FCS receptions in our counts. We cannot
		// trust the address bytes themselves if the FCS is in error. Furthermore,
		// control frames will not be considered for counts either since the CTS
		// and ACK frames have no addr2 field.

		// Sequence number is 12 MSB of seq_control field
		rx_seq         = ((rx_80211_header->sequence_control) >> 4) & 0xFFF;

		curr_dl_entry = wlan_mac_high_find_counts_txrx_addr(rx_80211_header->address_2);

		if(curr_dl_entry != NULL){
			perform_duplicate_check = 1;
			curr_counts_txrx = (counts_txrx_t*)(curr_dl_entry->data);

			// Remove entry from counts_txrx_list; Will be added back at the bottom of the function
			// This serves to sort the list and keep the most recently updated entries at the tail.
			dl_entry_remove(&counts_txrx_list, curr_dl_entry);
		} else {
			// We haven't seen this addr before, so we'll attempt to checkout a new dl_entry
			// struct from the free pool
			curr_dl_entry = counts_txrx_checkout();

			if (curr_dl_entry == NULL){
				// No free dl_entry!
				// We'll have to reallocate the oldest entry in the filled list
				curr_dl_entry = find_counts_txrx_oldest();

				if (curr_dl_entry != NULL) {
					dl_entry_remove(&counts_txrx_list, curr_dl_entry);
				} else {
					xil_printf("Cannot create counts_txrx.\n");
					return;
				}
			}

			curr_counts_txrx = (counts_txrx_t*)(curr_dl_entry->data);

			// Clear any old information from the Tx/Rx counts
			wlan_mac_high_clear_counts_txrx(curr_counts_txrx);

			// Copy BSSID into bss_info struct
			memcpy(curr_counts_txrx->addr, rx_80211_header->address_2, MAC_ADDR_LEN);
		}

		// By this point in the function, curr_counts_txrx is guaranteed to be pointing to a valid counts_txrx_t struct
		// that we should update with this reception.

		// Update the latest TXRX time
		curr_counts_txrx->latest_txrx_timestamp = get_system_time_usec();

		switch(pkt_type){
			default:
				//Unknown type
				return;
			break;
			case MAC_FRAME_CTRL1_TYPE_DATA:
				frame_counts_txrx = &(curr_counts_txrx->data);
			break;
			case MAC_FRAME_CTRL1_TYPE_MGMT:
				frame_counts_txrx = &(curr_counts_txrx->mgmt);
			break;
		}

		(frame_counts_txrx->rx_num_packets_total)++;
		(frame_counts_txrx->rx_num_bytes_total) += (length - WLAN_PHY_FCS_NBYTES - sizeof(mac_header_80211));

		if(perform_duplicate_check){
			// Check if this was a duplicate reception
			//	 - Packet has the RETRY bit set to 1 in the second frame control byte
			//   - Received seq num matched previously received seq num for this station
			if( ((rx_80211_header->frame_control_2) & MAC_FRAME_CTRL2_FLAG_RETRY) && (curr_counts_txrx->rx_latest_seq == rx_seq) ) {
				//Duplicate reception

			} else {
				//Unique reception
				(frame_counts_txrx->rx_num_packets)++;
				(frame_counts_txrx->rx_num_bytes) += (length - WLAN_PHY_FCS_NBYTES - sizeof(mac_header_80211));
			}
		}
		curr_counts_txrx->rx_latest_seq = rx_seq;

		// Add Tx/Rx Counts into counts_txrx_list
		dl_entry_insertEnd(&counts_txrx_list, curr_dl_entry);
	}
}



void counts_txrx_print_all(){
	int       			iter;
	u32       			i;
	dl_entry* 			curr_dl_entry;
	counts_txrx_t* 		curr_counts_txrx;

	i = 0;
	iter          = counts_txrx_list.length;
	curr_dl_entry = counts_txrx_list.last;

	// Print the header
	xil_printf("************************ Tx/Rx Counts *************************\n");

	while ((curr_dl_entry != NULL) && (iter-- > 0)) {
		curr_counts_txrx = (counts_txrx_t*)(curr_dl_entry->data);
		xil_printf("[%d] %02x-%02x-%02x-%02x-%02x-%02x ", i, curr_counts_txrx->addr[0],curr_counts_txrx->addr[1],curr_counts_txrx->addr[2],curr_counts_txrx->addr[3],curr_counts_txrx->addr[4],curr_counts_txrx->addr[5]);

		if(curr_counts_txrx->flags & COUNTS_TXRX_FLAGS_KEEP){
			xil_printf("(KEEP)\n");
		} else {
			xil_printf("\n");
		}

		xil_printf("  Data   Rx Num Bytes:           %d\n", (u32)(curr_counts_txrx->data.rx_num_bytes));
		xil_printf("  Data   Rx Num Bytes Total:     %d\n", (u32)(curr_counts_txrx->data.rx_num_bytes_total));
		xil_printf("  Data   Tx Num Bytes Success:   %d\n", (u32)(curr_counts_txrx->data.tx_num_bytes_success));
		xil_printf("  Data   Tx Num Bytes Total:     %d\n", (u32)(curr_counts_txrx->data.tx_num_bytes_total));
		xil_printf("  Data   Rx Num Packets:         %d\n", (u32)(curr_counts_txrx->data.rx_num_packets));
		xil_printf("  Data   Rx Num Packets Total:   %d\n", (u32)(curr_counts_txrx->data.rx_num_packets_total));
		xil_printf("  Data   Tx Num Packets Success: %d\n", (u32)(curr_counts_txrx->data.tx_num_packets_success));
		xil_printf("  Data   Tx Num Packets Total:   %d\n", (u32)(curr_counts_txrx->data.tx_num_packets_total));
		xil_printf("  Data   Tx Num Attempts:        %d\n", (u32)(curr_counts_txrx->data.tx_num_attempts));
		xil_printf("  Mgmt.  Rx Num Bytes:           %d\n", (u32)(curr_counts_txrx->mgmt.rx_num_bytes));
		xil_printf("  Mgmt.  Rx Num Bytes Total:     %d\n", (u32)(curr_counts_txrx->mgmt.rx_num_bytes_total));
		xil_printf("  Mgmt.  Tx Num Bytes Success:   %d\n", (u32)(curr_counts_txrx->mgmt.tx_num_bytes_success));
		xil_printf("  Mgmt.  Tx Num Bytes Total:     %d\n", (u32)(curr_counts_txrx->mgmt.tx_num_bytes_total));
		xil_printf("  Mgmt.  Rx Num Packets:         %d\n", (u32)(curr_counts_txrx->mgmt.rx_num_packets));
		xil_printf("  Mgmt.  Rx Num Packets Total:   %d\n", (u32)(curr_counts_txrx->mgmt.rx_num_packets_total));
		xil_printf("  Mgmt.  Tx Num Packets Success: %d\n", (u32)(curr_counts_txrx->mgmt.tx_num_packets_success));
		xil_printf("  Mgmt.  Tx Num Packets Total:   %d\n", (u32)(curr_counts_txrx->mgmt.tx_num_packets_total));
		xil_printf("  Mgmt.  Tx Num Attempts:        %d\n", (u32)(curr_counts_txrx->mgmt.tx_num_attempts));
		xil_printf("    Last update:   %d msec ago\n", (u32)((get_system_time_usec() - curr_counts_txrx->latest_txrx_timestamp)/1000));


		curr_dl_entry = dl_entry_prev(curr_dl_entry);
		i++;
	}
}


void counts_txrx_zero_all(){
	//This function will return all counts to 0 without affecting the list
	//of count structs.
	int       			iter;
	u32       			i;
	dl_entry* 			curr_dl_entry;
	counts_txrx_t* 		curr_counts_txrx;

	i = 0;
	iter          = counts_txrx_list.length;
	curr_dl_entry = counts_txrx_list.last;


	while ((curr_dl_entry != NULL) && (iter-- > 0)) {
		curr_counts_txrx = (counts_txrx_t*)(curr_dl_entry->data);

		bzero(&(curr_counts_txrx->data), sizeof(frame_counts_txrx_t));
		bzero(&(curr_counts_txrx->mgmt), sizeof(frame_counts_txrx_t));

		curr_dl_entry = dl_entry_prev(curr_dl_entry);
		i++;
	}
}



void counts_txrx_timestamp_check() {
	dl_entry* curr_dl_entry;
	counts_txrx_t* curr_counts_txrx;

	curr_dl_entry = counts_txrx_list.first;

	while(curr_dl_entry != NULL){
		curr_counts_txrx = (counts_txrx_t*)(curr_dl_entry->data);

		if((get_system_time_usec() - curr_counts_txrx->latest_txrx_timestamp) > COUNTS_TXRX_TIMEOUT_USEC){
			if((curr_counts_txrx->flags & COUNTS_TXRX_FLAGS_KEEP) == 0){
				wlan_mac_high_clear_counts_txrx(curr_counts_txrx);
				dl_entry_remove(&counts_txrx_list, curr_dl_entry);
				counts_txrx_checkin(curr_dl_entry);
			}
		} else {
			// Nothing after this entry is older, so it's safe to quit
			return;
		}

		curr_dl_entry = dl_entry_next(curr_dl_entry);
	}
}

dl_entry* counts_txrx_checkout(){
	dl_entry* entry;

	if(counts_txrx_free.length > 0){
		entry = ((dl_entry*)(counts_txrx_free.first));
		dl_entry_remove(&counts_txrx_free,counts_txrx_free.first);
		return entry;
	} else {
		return NULL;
	}
}

void counts_txrx_checkin(dl_entry* entry){
	dl_entry_insertEnd(&counts_txrx_free, (dl_entry*)entry);
	return;
}

dl_entry* wlan_mac_high_find_counts_txrx_addr(u8* addr){
	int       		iter;
	dl_entry* 		curr_dl_entry;
	counts_txrx_t* 	curr_counts_txrx;

	iter          = counts_txrx_list.length;
	curr_dl_entry = counts_txrx_list.last;

	while ((curr_dl_entry != NULL) && (iter-- > 0)) {
		curr_counts_txrx = (counts_txrx_t*)(curr_dl_entry->data);

		if (wlan_addr_eq(addr,curr_counts_txrx->addr)) {
			return curr_dl_entry;
		}

		curr_dl_entry = dl_entry_prev(curr_dl_entry);
	}
	return NULL;
}

dl_entry* find_counts_txrx_oldest(){
	int       iter;
	dl_entry* curr_dl_entry;
	counts_txrx_t* counts_txrx_info;

	iter          = counts_txrx_list.length;
	curr_dl_entry = counts_txrx_list.first;

	while ((curr_dl_entry != NULL) && (iter-- > 0)) {
		counts_txrx_info = (counts_txrx_t*)(curr_dl_entry->data);

		if ((counts_txrx_info->flags & COUNTS_TXRX_FLAGS_KEEP) == 0) {
			return curr_dl_entry;
		}

		curr_dl_entry = dl_entry_next(curr_dl_entry);
	}
	return NULL;
}



// Function will create a counts_txrx_t and make sure that the address is unique
// in the counts_txrx list.
//
counts_txrx_t* wlan_mac_high_create_counts_txrx(u8* addr){
	dl_entry * 		curr_dl_entry;
	counts_txrx_t * curr_counts_txrx = NULL;

	curr_dl_entry = wlan_mac_high_find_counts_txrx_addr(addr);

	if (curr_dl_entry != NULL){
		// Get the Tx/Rx Counts from the entry
		curr_counts_txrx = (counts_txrx_t*)(curr_dl_entry->data);

		// Remove the entry from the info list so it can be added back later
		dl_entry_remove(&counts_txrx_list, curr_dl_entry);
	} else {
		// Have not seen this addr before; attempt to grab a new dl_entry
		// struct from the free pool
		curr_dl_entry = counts_txrx_checkout();

		if (curr_dl_entry == NULL){
			// No free dl_entry; Re-allocate the oldest entry in the filled list
			curr_dl_entry = find_counts_txrx_oldest();

			if (curr_dl_entry != NULL) {
				dl_entry_remove(&counts_txrx_list, curr_dl_entry);
			} else {
				xil_printf("Cannot create counts_txrx.\n");
				return NULL;
			}
		}

		// Get the Tx/Rx Counts from the entry
		curr_counts_txrx = (counts_txrx_t*)(curr_dl_entry->data);

		// Clear any old information from the Tx/Rx Counts
		wlan_mac_high_clear_counts_txrx(curr_counts_txrx);

		// Copy the BSS ID to the entry
		memcpy(curr_counts_txrx->addr, addr, MAC_ADDR_LEN);
	}

	// Update the fields of the Tx/Rx Counts
	curr_counts_txrx->latest_txrx_timestamp = get_system_time_usec();

	// Insert the updated entry into the network list
	dl_entry_insertEnd(&counts_txrx_list, curr_dl_entry);

	return curr_counts_txrx;
}

/**
 * @brief Reset List of Tx/Rx Counts
 *
 * Reset all Tx/Rx Counts except ones flagged to be kept
 *
 * @param  None
 * @return None
 */
void wlan_mac_high_reset_counts_txrx_list(){
	dl_entry * 		next_dl_entry = counts_txrx_list.first;
	dl_entry * 		curr_dl_entry;
    counts_txrx_t * curr_counts_txrx;
    int		   		iter = counts_txrx_list.length;

	while( (next_dl_entry != NULL) && (iter-- > 0) ){
		curr_dl_entry = next_dl_entry;
		next_dl_entry = dl_entry_next(curr_dl_entry);
		curr_counts_txrx = (counts_txrx_t*)(curr_dl_entry->data);

		if( (curr_counts_txrx->flags & COUNTS_TXRX_FLAGS_KEEP) == 0){
			wlan_mac_high_clear_counts_txrx(curr_counts_txrx);
			dl_entry_remove(&counts_txrx_list, curr_dl_entry);
			bss_info_checkin(curr_dl_entry);
		}
	}
}

void wlan_mac_high_clear_counts_txrx(counts_txrx_t * counts_txrx){
	if (counts_txrx != NULL){
		// Clear the Tx/Rx Counts
        bzero(counts_txrx, sizeof(counts_txrx_t));
	}
}



inline dl_list* wlan_mac_high_get_counts_txrx_list(){
	return &counts_txrx_list;
}
