/** @file wlan_mac_ibss.c
 *  @brief Station
 *
 *  This contains code for the 802.11 IBSS node (ad hoc).
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

// Xilinx SDK includes
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "xil_cache.h"

// WLAN includes
#include "wlan_platform_common.h"
#include "wlan_platform_high.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_entries.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_scan.h"
#include "ascii_characters.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_network_info.h"
#include "wlan_mac_station_info.h"
#include "wlan_mac_sniffer.h"
#include "wlan_mac_mgmt_tags.h"
#include "wlan_mac_common.h"
#include "wlan_mac_pkt_buf_util.h"

// own includes
#include "rftap.h"


/*************************** Constant Definitions ****************************/

#define  WLAN_EXP_ETH TRANSPORT_ETH_B


#define  WLAN_DEFAULT_CHANNEL 48

// The WLAN_DEFAULT_BSS_CONFIG_HT_CAPABLE define will set the default
// unicast TX phy mode to:  1 --> HTMF  or  0 --> NONHT.
#define  WLAN_DEFAULT_BSS_CONFIG_HT_CAPABLE 0

#define  WLAN_DEFAULT_TX_PWR 15
#define  WLAN_DEFAULT_TX_ANTENNA TX_ANTMODE_SISO_ANTA
#define  WLAN_DEFAULT_RX_ANTENNA RX_ANTMODE_SISO_ANTA

// WLAN_DEFAULT_USE_HT
//
// The WLAN_DEFAULT_USE_HT define will set the default unicast TX phy mode
// to:  1 --> HTMF  or  0 --> NONHT.  It will also be used as the default
// value for the HT_CAPABLE capability of the BSS in configure_bss() when
// moving from a NULL to a non-NULL BSS and the ht_capable parameter is not
// specified.  This parameter only affects how the MAC selects the phy_mode
// value for transmissions. It does not affect the underlying PHY support for
// Tx/Rx of HTMF waveforms.
#define  WLAN_DEFAULT_USE_HT                     0


/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/


// Common TX header for 802.11 packets
mac_header_80211_common tx_header_common;

// Tx queue variables;
static u32 max_queue_size;
volatile u8 pause_data_queue;

// MAC address
static u8 wlan_mac_addr[MAC_ADDR_LEN];

// Common Platform Device Info
platform_common_dev_info_t platform_common_dev_info;


/*************************** Functions Prototypes ****************************/


/******************************** Functions **********************************/

int main() {
	// Initialize Microblaze --
	//  these functions should be called before anything
	//  else is executed
	Xil_DCacheDisable();
	Xil_ICacheDisable();
	microblaze_enable_exceptions();


	compilation_details_t compilation_details;
	bzero(&compilation_details, sizeof(compilation_details_t));

	// Print initial message to UART
	xil_printf("\f");
	xil_printf("----- Mango 802.11 Reference Design -----\n");
	xil_printf("----- v1.7.2 ----------------------------\n");
	xil_printf("----- wlan_mac_sniffer ------------------\n");
	xil_printf("Compiled %s %s\n\n", __DATE__, __TIME__);
	strncpy(compilation_details.compilation_date, __DATE__, 12);
	strncpy(compilation_details.compilation_time, __TIME__, 9);

	wlan_mac_common_malloc_init();

	// Initialize the maximum TX queue size
	max_queue_size = MAX_TX_QUEUE_LEN;

	// Unpause the queue
	pause_data_queue       = 0;

	// Initialize the utility library
    wlan_mac_high_init();

    // Get the device info
	platform_common_dev_info = wlan_platform_common_get_dev_info();

    wlan_platform_high_userio_disp_status(USERIO_DISP_STATUS_APPLICATION_ROLE, APPLICATION_ROLE_IBSS);

	// Initialize hex display to "No BSS"
    wlan_platform_high_userio_disp_status(USERIO_DISP_STATUS_MEMBER_LIST_UPDATE, 0xFF);

	// Set default Tx params
	// Set sane default Tx params. These will be overwritten by the user application
	tx_params_t	tx_params = { .phy = { .mcs = 3, .phy_mode = PHY_MODE_NONHT, .antenna_mode = WLAN_DEFAULT_TX_ANTENNA, .power = WLAN_DEFAULT_TX_PWR },
							  .mac = { .flags = 0 } };

	wlan_mac_set_default_tx_params(unicast_data, &tx_params);

	tx_params.phy.mcs = 0;
	tx_params.phy.phy_mode = PHY_MODE_NONHT;

	wlan_mac_set_default_tx_params(unicast_mgmt, &tx_params);
	wlan_mac_set_default_tx_params(mcast_data, &tx_params);
	wlan_mac_set_default_tx_params(mcast_mgmt, &tx_params);

	// Re-apply the defaults to any existing station_info_t structs that this AP
	// knows about
	wlan_mac_reapply_default_tx_params();

	// Initialize callbacks
#if WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE
	wlan_mac_util_set_eth_rx_callback((void*) ethernet_receive);
#endif
	wlan_mac_high_set_mpdu_rx_callback((void*) mpdu_rx_process);
	wlan_mac_high_set_uart_rx_callback((void*) uart_rx);
	wlan_mac_high_set_poll_tx_queues_callback((void*) poll_tx_queues);

	wlan_mac_high_set_cpu_low_reboot_callback((void*) handle_cpu_low_reboot);

#if WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE
	// Set the Ethernet ecapsulation mode
//	wlan_mac_util_set_eth_encap_mode(APPLICATION_ROLE_IBSS);
#endif

    wlan_mac_hw_info_t * hw_info;
    hw_info = get_mac_hw_info();

	// CPU Low will pass HW information to CPU High as part of the boot process
	//   - Get necessary HW information
	memcpy((void*) &(wlan_mac_addr[0]), (void*) get_mac_hw_addr_wlan(), MAC_ADDR_LEN);

    // Set Header information
	tx_header_common.address_2 = &(wlan_mac_addr[0]);

	// Set the at-boot MAC Time to 0 usec
	set_mac_time_usec(0);

	wlan_mac_high_set_radio_channel(WLAN_DEFAULT_CHANNEL);
	wlan_mac_high_set_rx_ant_mode(WLAN_DEFAULT_RX_ANTENNA);
	wlan_mac_high_set_tx_ctrl_power(WLAN_DEFAULT_TX_PWR);
	wlan_mac_high_set_radio_tx_power(WLAN_DEFAULT_TX_PWR);

#if WLAN_SW_CONFIG_ENABLE_LOGGING
	// Reset the event log
	event_log_reset();
#endif //WLAN_SW_CONFIG_ENABLE_LOGGING


	// Print Station information to the terminal
    xil_printf("------------------------\n");
    xil_printf("WLAN MAC IBSS boot complete: \n");
    xil_printf("  Serial Number : %s-%05d\n", hw_info->serial_number_prefix, hw_info->serial_number);
	xil_printf("  MAC Addr      : %02x:%02x:%02x:%02x:%02x:%02x\n\n", wlan_mac_addr[0], wlan_mac_addr[1], wlan_mac_addr[2], wlan_mac_addr[3], wlan_mac_addr[4], wlan_mac_addr[5]);

#ifdef WLAN_USE_UART_MENU
	xil_printf("\nPress the Esc key in your terminal to access the UART menu\n");
#endif

	xil_printf("Start sniffing \n");
	// Start the interrupts
	wlan_mac_high_interrupt_restore_state(INTERRUPTS_ENABLED);


	// Schedule Events


	while(1){
	}

	// Unreachable, but non-void return keeps the compiler happy
	return -1;
}



/*****************************************************************************/
/**
 * @brief Poll Tx queues to select next available packet to transmit
 *
 *****************************************************************************/
#define NUM_QUEUE_GROUPS 2
typedef enum queue_group_t{
	MGMT_QGRP,
	DATA_QGRP
} queue_group_t;

/*****************************************************************************/
/**
 * @brief Poll Tx queues to select next available packet to transmit
 *
 * This function will attempt to completely fill all Tx packet buffers in the
 * PKT_BUF_GROUP_GENERAL group. Dequeueing occurs with a nested round-robing policy:
 * 	1) The function will alternate between dequeueing management and data frames in order
 * 	   to prioritize time-critical management responses probe responses.
 * 	2) Data frames will be dequeued round-robin for station for which packets are enqueued.
 * 	   Multicast frames are treated like their own station for the purposes of this policy.
 *
 *****************************************************************************/
void poll_tx_queues(){
	interrupt_state_t curr_interrupt_state;
	dl_entry* tx_queue_buffer_entry;
	u32 i;

	int num_pkt_bufs_avail;
	int poll_loop_cnt;

	// Remember the next group to poll between calls to this function
	//   This implements the ping-pong poll between the MGMT_QGRP and DATA_QGRP groups
	static queue_group_t next_queue_group = MGMT_QGRP;
	queue_group_t curr_queue_group;

	// Stop interrupts for all processing below - this avoids many possible race conditions,
	//  like new packets being enqueued or stations joining/leaving the BSS
	curr_interrupt_state = wlan_mac_high_interrupt_stop();

	// First handle the general packet buffer group
	num_pkt_bufs_avail = wlan_mac_num_tx_pkt_buf_available(PKT_BUF_GROUP_GENERAL);

	// This loop will (at most) check every queue twice
	//  This handles the case of a single non-empty queue needing to supply packets
	//  for both GENERAL packet buffers
	poll_loop_cnt = 0;
	while((num_pkt_bufs_avail > 0) && (poll_loop_cnt < (2*NUM_QUEUE_GROUPS))) {
		poll_loop_cnt++;
		curr_queue_group = next_queue_group;

		if(curr_queue_group == MGMT_QGRP) {
			// Poll the management queue on this loop, data queues on next loop
			next_queue_group = DATA_QGRP;
			tx_queue_buffer_entry = dequeue_from_head(MANAGEMENT_QID);
			if(tx_queue_buffer_entry) {
				// Update the packet buffer group
				((tx_queue_buffer_t*)(tx_queue_buffer_entry->data))->queue_info.pkt_buf_group = PKT_BUF_GROUP_GENERAL;
				// Successfully dequeued a management packet - transmit and checkin
				transmit_checkin(tx_queue_buffer_entry);
				//continue the loop
				num_pkt_bufs_avail--;
				continue;
			}
		} else {
			// Poll the data queues on this loop, management queue on next loop
			next_queue_group = MGMT_QGRP;

			if(pause_data_queue) {
				// Data queues are paused - skip any dequeue attempts and continue the loop
				continue;
			}

#if 0
			for(i = 0; i < (active_network_info->members.length + 1); i++) {
				// Resume polling data queues from where we stopped on the previous call
				curr_station_info_entry = next_station_info_entry;

				// Loop through all associated stations' queues and the broadcast queue
				if(curr_station_info_entry == NULL) {
					next_station_info_entry = (station_info_entry_t*)(active_network_info->members.first);

					tx_queue_buffer_entry = dequeue_from_head(MCAST_QID);
					if(tx_queue_buffer_entry) {
						// Update the packet buffer group
						((tx_queue_buffer_t*)(tx_queue_buffer_entry->data))->queue_info.pkt_buf_group = PKT_BUF_GROUP_GENERAL;
						// Successfully dequeued a management packet - transmit and checkin
						transmit_checkin(tx_queue_buffer_entry);
						// Successfully dequeued a multicast packet - end the DATA_QGRP loop
						num_pkt_bufs_avail--;
						break;
					}
				} else {
					// Check the queue for an associated station
					curr_station_info = (station_info_t*)(curr_station_info_entry->data);

					if( station_info_is_member(&active_network_info->members, curr_station_info)) {
						if(curr_station_info_entry == (station_info_entry_t*)(active_network_info->members.last)){
							// We've reached the end of the table, so we wrap around to the beginning
							next_station_info_entry = NULL;
						} else {
							next_station_info_entry = dl_entry_next(curr_station_info_entry);
						}

						tx_queue_buffer_entry = dequeue_from_head(STATION_ID_TO_QUEUE_ID(curr_station_info_entry->id));
						if(tx_queue_buffer_entry) {
							// Update the packet buffer group
							((tx_queue_buffer_t*)(tx_queue_buffer_entry->data))->queue_info.pkt_buf_group = PKT_BUF_GROUP_GENERAL;
							// Successfully dequeued a management packet - transmit and checkin
							transmit_checkin(tx_queue_buffer_entry);
							// Successfully dequeued a unicast packet for this station - end the DATA_QGRP loop
							num_pkt_bufs_avail--;
							break;
						}

					} else {
						// This curr_station_info is invalid. Perhaps it was removed from
						// the association table before poll_tx_queues was called. We will
						// start the round robin checking back at broadcast.
						next_station_info_entry = NULL;
					} // END if(curr_station_info is BSS member)
				} // END if(multicast queue or station queue)
			} // END for loop over association table
#endif
		} // END if(MGMT or DATA queue group)
	} // END while(buffers available && keep polling)

	wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
}

/*****************************************************************************/
/**
 * @brief Purges all packets from all Tx queues
 *
 * This function discards all currently en-queued packets awaiting transmission and returns all
 * queue entries to the free pool.
 *
 * This function does not discard packets already submitted to the lower-level MAC for transmission
 *
 * @param None
 * @return None
 *****************************************************************************/
void purge_all_data_tx_queue(){
	// Purge all data transmit queues
	purge_queue(MCAST_QID);                                    		// Broadcast Queue
}

/*****************************************************************************/
/**
 * @brief Callback to handle insertion of an Ethernet reception into the corresponding wireless Tx queue
 *
 * This function is called when a new Ethernet packet is received that must be transmitted via the wireless interface.
 * The packet must be encapsulated before it is passed to this function. Ethernet encapsulation is implemented in the mac_high framework.
 *
 * The tx_queue_list argument is a DL list, but must contain exactly one queue entry which contains the encapsulated packet
 * A list container is used here to ease merging of the list with the target queue.
 *
 * @param dl_entry* curr_tx_queue_element
 *  - A single queue element containing the packet to transmit
 * @param u8* eth_dest
 *  - 6-byte destination address from original Ethernet packet
 * @param u8* eth_src
 *  - 6-byte source address from original Ethernet packet
 * @param u16 tx_length
 *  - Length (in bytes) of the packet payload
 * @return 1 for successful enqueuing of the packet, 0 otherwise
 *****************************************************************************/
int ethernet_receive(dl_entry* curr_tx_queue_element, u8* eth_dest, u8* eth_src, u16 tx_length){

	tx_queue_buffer_t* curr_tx_queue_buffer;
	station_info_t* station_info = NULL;
	station_info_entry_t* station_info_entry;
	u32 queue_sel;

#if 0
	if(0){

		// Send the pre-encapsulated Ethernet frame over the wireless interface
		//     NOTE:  The queue element has already been provided, so we do not need to check if it is NULL
		curr_tx_queue_buffer = (tx_queue_buffer_t*)(curr_tx_queue_element->data);

		// Setup the TX header
		wlan_mac_high_setup_tx_header( &tx_header_common, eth_dest,active_network_info->bss_config.bssid);

		// Fill in the data
		wlan_create_data_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, 0);

		if( wlan_addr_mcast(eth_dest) ){
			// Fill in metadata
			queue_sel = MCAST_QID;
			curr_tx_queue_buffer->flags = 0;
			curr_tx_queue_buffer->length = tx_length;

			// Assign a station_info_t struct
			//  Notes: (1) if one exists already, it will be adopted.
			//		   (2) if no heap exists for creating one, NULL is a valid
			//             value for the station_info_t*
			curr_tx_queue_buffer->station_info = station_info_create(eth_dest);
		} else {

			station_info_entry = station_info_find_by_addr(eth_dest, &active_network_info->members);

			if(station_info_entry != NULL){
				station_info = (station_info_t*)station_info_entry->data;
			} else {
				// Add station info
				//     - Set ht_capable argument to the HT_CAPABLE capability of the BSS.  Given that the node does not know
				//       the HT capabilities of the new station, it is reasonable to assume that they are the same as the BSS.
				//
				station_info = station_info_add(&(active_network_info->members), eth_dest, ADD_STATION_INFO_ANY_ID, active_network_info->bss_config.ht_capable);
				if(station_info != NULL){
					station_info->flags |= STATION_INFO_FLAG_KEEP;
					time_hr_min_sec_t time_hr_min_sec = wlan_mac_time_to_hr_min_sec(get_system_time_usec());
					xil_printf("*%dh:%02dm:%02ds* IBSS 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x added to BSS\n",
							time_hr_min_sec.hr, time_hr_min_sec.min, time_hr_min_sec.sec,
							eth_dest[0], eth_dest[1], eth_dest[2],
							eth_dest[3], eth_dest[4], eth_dest[5]);
				}

				wlan_platform_high_userio_disp_status(USERIO_DISP_STATUS_MEMBER_LIST_UPDATE, active_network_info->members.length);
			}

			if( station_info == NULL ){
				queue_sel = MCAST_QID;
			} else {
				queue_sel = STATION_ID_TO_QUEUE_ID(station_info->ID);
			}

			// Fill in metadata
			curr_tx_queue_buffer->flags = TX_QUEUE_BUFFER_FLAGS_FILL_DURATION;
			curr_tx_queue_buffer->length = tx_length;
			curr_tx_queue_buffer->station_info = station_info;
		}

		if(queue_num_queued(queue_sel) < max_queue_size){
			// Put the packet in the queue
			enqueue_after_tail(queue_sel, curr_tx_queue_element);

		} else {
			// Packet was not successfully enqueued
			return 0;
		}

		// Packet was successfully enqueued
		return 1;
	} else {
		return 0;
	}
#endif
	return 0;
}



/*****************************************************************************/
/**
 * @brief Process received MPDUs
 *
 * This callback function will process all the received MPDUs..
 *
 * @param  void* pkt_buf_addr
 *     - Packet buffer address;  Contains the contents of the MPDU as well as other packet information from CPU low
 * @param  station_info_t * station_info
 *     - Pointer to metadata about the station from which this frame was received
 * @param  rx_common_entry* rx_event_log_entry
 * 	   - Pointer to the log entry created for this reception by the MAC High Framework
 * @return u32 flags
 *
 *****************************************************************************/
u32 mpdu_rx_process(void* pkt_buf_addr, station_info_t* station_info, rx_common_entry* rx_event_log_entry)  {

	rx_frame_info_t* rx_frame_info = (rx_frame_info_t*)pkt_buf_addr;
	void* mac_payload = (u8*)pkt_buf_addr + PHY_RX_PKT_BUF_MPDU_OFFSET;
	u8* mac_payload_ptr_u8 = (u8*)mac_payload;
	mac_header_80211* rx_80211_header = (mac_header_80211*)((void *)mac_payload_ptr_u8);

	u16 rx_seq;

	u8 unicast_to_me;
	u8 to_multicast;
	u8 send_response = 0;
	u32 tx_length;
#if WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE
	u8 pre_llc_offset = 0;
#endif
	u32 return_val = 0;
	u16 length = rx_frame_info->phy_details.length;

	// (debug) UART display of packet
	time_hr_min_sec_t time_hr_min_sec = wlan_mac_time_to_hr_min_sec(get_system_time_usec());
	xil_printf("*%dh:%02dm:%02ds* mpdu: src=0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x, length=%d\n",
								time_hr_min_sec.hr, time_hr_min_sec.min, time_hr_min_sec.sec,
								rx_80211_header->address_2[0], rx_80211_header->address_2[1], rx_80211_header->address_2[2],
								rx_80211_header->address_2[3], rx_80211_header->address_2[4], rx_80211_header->address_2[5],
								length);

	// If this function was passed a CTRL frame (e.g., CTS, ACK), then we should just quit.
	// The only reason this occured was so that it could be logged in the line above.
//	if((rx_80211_header->frame_control_1 & 0xF) == MAC_FRAME_CTRL1_TYPE_CTRL){
//		goto mpdu_rx_process_end;
//	}

	// Determine destination of packet
	unicast_to_me = wlan_addr_eq(rx_80211_header->address_1, wlan_mac_addr);
	to_multicast  = wlan_addr_mcast(rx_80211_header->address_1);

	// mirror frame to ethernet interface
	// memory needs to be accessible for the ethernet_dma (e.g. AUX_BRAM or DDR) - use the wifi tx buffer for now
	dl_entry* curr_tx_queue_element = queue_checkout();
	void* curr_tx_queue_buffer = memset(curr_tx_queue_element->data, 0, QUEUE_BUFFER_SIZE);

	size_t pkt_size = 0;

	ethernet_header_t 	*eth_frame = ethernet_frame_init(curr_tx_queue_buffer, &pkt_size);
	eth_frame->ethertype = ETH_TYPE_IP;

	ipv4_header_t 		*ipv4_frame = ip_frame_init(curr_tx_queue_buffer, &pkt_size);
	udp_header_t 		*udp_frame = udp_frame_init(curr_tx_queue_buffer, &pkt_size);
	rftap_header_t 		*rftap_frame = rftap_frame_init(curr_tx_queue_buffer, &pkt_size);
	// radiotap_header_t 	*radiotap_frame = radiotap_frame_init(curr_tx_queue_buffer, &pkt_size);

	void *pkt = memmove(curr_tx_queue_buffer + pkt_size, mac_payload_ptr_u8, length);
	pkt_size += length;

	// radiotap_frame->it_len = Xil_Htons(length + sizeof(radiotap_header_t));

	rftap_frame->len32 = 3;
	rftap_frame->flags = 1;
	rftap_frame->dlt = 105; // DLT_IEEE802_11

	udp_frame->length = Xil_Htons(length + sizeof(rftap_header_t) + sizeof(udp_header_t));
	udp_frame->src_port = Xil_Htons(1);
	udp_frame->dest_port = Xil_Htons(52001);

	// ipv4_frame->total_length = Xil_Htons(sizeof(ipv4_header_t) + length + sizeof(rftap_header_t));
	ip_frame_calc_checksum(ipv4_frame);

	xil_printf("-> resulting ethernet frame, ptr = %x, l = %u\n", pkt, pkt_size);

	u8 packet[] = {
			0x0a, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0a, 0x01, 0x01, 0x01, 0x01, 0x01, 0x08, 0x00, 0x45, 0x00,
			0x00, 0x75, 0x12, 0x34, 0x00, 0x00, 0xff, 0x11, 0x92, 0x3e, 0x0a, 0x01, 0x01, 0x01, 0x0a, 0x02,
			0x02, 0x02, 0x00, 0x01, 0xcb, 0x21, 0x00, 0x61, 0x33, 0x19, 0x52, 0x46, 0x74, 0x61, 0x08, 0x00,
			0x8d, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x2e, 0x72, 0xf3, 0x41, 0x00, 0x00,
			0xa8, 0xb9, 0xf1, 0x52, 0xad, 0x40, 0x14, 0xae, 0x98, 0xc2, 0x00, 0x00, 0x18, 0x00, 0x2e, 0x40,
			0x00, 0xa0, 0x20, 0x08, 0x00, 0x00, 0x00, 0x0c, 0x64, 0x14, 0x40, 0x01, 0xb4, 0x00, 0x00, 0x00,
			0xb4, 0x00, 0xd0, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x50, 0xa5, 0x03, 0x00, 0x02, 0x02, 0x10, 0x88,
			0x13, 0x80, 0x00 };

	//uint8_t *buffer = malloc(512);
	// memcpy(pkt_buf_addr, packet, sizeof(packet));
	//memcpy(buffer, mac_payload_ptr_u8, 2);
	//memcpy(pkt_buf_addr, mac_payload_ptr_u8,2);
	int ret = 0;
	if ((ret = wlan_platform_ethernet_send(curr_tx_queue_buffer, pkt_size)) != XST_SUCCESS) {
		xil_printf("Error: wlan_platform_ethernet_send() failed: %d\n", ret);
	}

	queue_checkin(curr_tx_queue_element);

    // If the packet is good (ie good FCS) and it is destined for me, then process it
	if( (rx_frame_info->flags & RX_FRAME_INFO_FLAGS_FCS_GOOD)){

		// Sequence number is 12 MSB of seq_control field
		rx_seq         = ((rx_80211_header->sequence_control) >> 4) & 0xFFF;

		// Check if this was a duplicate reception
		//   - Packet is unicast and directed towards me
		//	 - Packet has the RETRY bit set to 1 in the second frame control byte
		//   - Received seq num matched previously received seq num for this STA
		if( (station_info != NULL) && unicast_to_me ){
			if( ((rx_80211_header->frame_control_2) & MAC_FRAME_CTRL2_FLAG_RETRY) && (station_info->latest_rx_seq == rx_seq) ) {
				if(rx_event_log_entry != NULL){
					rx_event_log_entry->flags |= RX_FLAGS_DUPLICATE;
					return_val |= MAC_RX_CALLBACK_RETURN_FLAG_DUP;
				}
			} else {
				station_info->latest_rx_seq = rx_seq;
			}
		}

#if 0
		// Update the association information
		if(active_network_info != NULL){
			if(wlan_addr_eq(rx_80211_header->address_3, active_network_info->bss_config.bssid)){
				if( station_info != NULL && station_info_is_member(&active_network_info->members, station_info) == 0 ){
					// Add station info
					//     - Set ht_capable argument to the HT_CAPABLE capability of the BSS.  Given that the node does not know
					//       the HT capabilities of the new station, it is reasonable to assume that they are the same as the BSS.
					//
					// Note: we do not need the returned station_info_t* from this function since it is guaranteed to match
					//  the "station_info" argument to the mpdu_rx_process function
					station_info_add(&(active_network_info->members), rx_80211_header->address_2, ADD_STATION_INFO_ANY_ID, active_network_info->bss_config.ht_capable);
					station_info->flags |= STATION_INFO_FLAG_KEEP;
					xil_printf("*%dh:%02dm:%02ds* IBSS 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x added to BSS\n",
							time_hr_min_sec.hr, time_hr_min_sec.min, time_hr_min_sec.sec,
							rx_80211_header->address_2[0], rx_80211_header->address_2[1], rx_80211_header->address_2[2],
							rx_80211_header->address_2[3], rx_80211_header->address_2[4], rx_80211_header->address_2[5]);

					wlan_platform_high_userio_disp_status(USERIO_DISP_STATUS_MEMBER_LIST_UPDATE, active_network_info->members.length);
				}
			}
		}

		if(station_info != NULL) {


			// Check if this was a duplicate reception
			//   - Received seq num matched previously received seq num for this STA
			if( return_val & MAC_RX_CALLBACK_RETURN_FLAG_DUP) {
				// Finish the function
				goto mpdu_rx_process_end;
			}
		}

		if(unicast_to_me || to_multicast){
			// Process the packet
			switch(rx_80211_header->frame_control_1) {

				//---------------------------------------------------------------------
				case MAC_FRAME_CTRL1_SUBTYPE_QOSDATA:
#if WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE
					pre_llc_offset = sizeof(qos_control);
#endif
				case (MAC_FRAME_CTRL1_SUBTYPE_DATA):
					// Data packet
					//   - If the STA is associated with the AP and this is from the DS, then transmit over the wired network
					//
					if(active_network_info != NULL){
						if(wlan_addr_eq(rx_80211_header->address_3, active_network_info->bss_config.bssid)) {
							// MPDU is flagged as destined to the DS - send it for de-encapsulation and Ethernet Tx (if appropriate)
#if WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE
							wlan_mpdu_eth_send(mac_payload, length, pre_llc_offset);
#endif
						}
					}
				break;

				//---------------------------------------------------------------------
				case (MAC_FRAME_CTRL1_SUBTYPE_PROBE_REQ):
					if(active_network_info != NULL){
						if(wlan_addr_eq(rx_80211_header->address_3, bcast_addr)) {
							mac_payload_ptr_u8 += sizeof(mac_header_80211);

							// Loop through tagged parameters
							while(((u32)mac_payload_ptr_u8 -  (u32)mac_payload)<= (length - WLAN_PHY_FCS_NBYTES)){

								// What kind of tag is this?
								switch(mac_payload_ptr_u8[0]){
									//-----------------------------------------------------
									case MGMT_TAG_SSID:
										// SSID parameter set
										if((mac_payload_ptr_u8[1]==0) || (memcmp(mac_payload_ptr_u8+2, (u8*)default_ssid, mac_payload_ptr_u8[1])==0)) {
											// Broadcast SSID or my SSID - send unicast probe response
											send_response = 1;
										}
									break;

									//-----------------------------------------------------
									case MGMT_TAG_SUPPORTED_RATES:
										// Supported rates
									break;

									//-----------------------------------------------------
									case MGMT_TAG_EXTENDED_SUPPORTED_RATES:
										// Extended supported rates
									break;

									//-----------------------------------------------------
									case MGMT_TAG_DSSS_PARAMETER_SET:
										// DS Parameter set (e.g. channel)
									break;
								}

								// Move up to the next tag
								mac_payload_ptr_u8 += mac_payload_ptr_u8[1]+2;
							}

							if(send_response) {
								// Create a probe response frame
								curr_tx_queue_element = queue_checkout();

								if(curr_tx_queue_element != NULL){
									curr_tx_queue_buffer = (tx_queue_buffer_t*)(curr_tx_queue_element->data);

									// Setup the TX header
									wlan_mac_high_setup_tx_header( &tx_header_common, rx_80211_header->address_2, active_network_info->bss_config.bssid );

									// Fill in the data
									tx_length = wlan_create_probe_resp_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, active_network_info);

									// Fill in metadata
									curr_tx_queue_buffer->flags = TX_QUEUE_BUFFER_FLAGS_FILL_DURATION | TX_QUEUE_BUFFER_FLAGS_FILL_TIMESTAMP;
									curr_tx_queue_buffer->length = tx_length;
									curr_tx_queue_buffer->station_info = station_info;

									// Put the packet in the queue
									enqueue_after_tail(MANAGEMENT_QID, curr_tx_queue_element);
								}

								// Finish the function
								goto mpdu_rx_process_end;
							}
						}
					}
				break;

				//---------------------------------------------------------------------
				default:
					//This should be left as a verbose print. It occurs often when communicating with mobile devices since they tend to send
					//null data frames (type: DATA, subtype: 0x4) for power management reasons.
					wlan_printf(PL_VERBOSE, "Received unknown frame control type/subtype %x\n",rx_80211_header->frame_control_1);
				break;
			}
		}

#endif
		// Finish the function
		goto mpdu_rx_process_end;
	} else {
		// Process any Bad FCS packets
		goto mpdu_rx_process_end;
	}


	// Finish any processing for the RX MPDU process
	mpdu_rx_process_end:

    return return_val;
}


/*****************************************************************************/
/**
 * @brief Handle a reboot of CPU_LOW
 *
 * If CPU_LOW reboots, any parameters we had previously set in it will be lost.
 * This function is called to tell us that we should re-apply any previous
 * parameters we had set.
 *
 * @param  u32 type - type of MAC running in CPU_LOW
 * @return None
 *****************************************************************************/
void handle_cpu_low_reboot(u32 type){

}


/*****************************************************************************/
/**
 * @brief Accessor methods for global variables
 *
 * These functions will return pointers to global variables
 *
 * @param  None
 * @return None
 *****************************************************************************/
dl_list* get_network_member_list(){
	return NULL;
}

