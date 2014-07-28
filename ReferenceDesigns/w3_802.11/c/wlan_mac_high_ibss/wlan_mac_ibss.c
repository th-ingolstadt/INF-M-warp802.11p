/** @file wlan_mac_sta.c
 *  @brief Station
 *
 *  This contains code for the 802.11 Station.
 *
 *  @copyright Copyright 2014, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 *  @bug No known bugs
 */

/***************************** Include Files *********************************/

//Xilinx SDK includes
#include "xparameters.h"
#include "stdio.h"
#include "stdlib.h"
#include "xtmrctr.h"
#include "xio.h"
#include "string.h"
#include "xintc.h"

//WARP includes
#include "w3_userio.h"
#include "wlan_mac_ipc_util.h"
#include "wlan_mac_misc_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_entries.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_ibss_scan_fsm.h"
#include "ascii_characters.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_bss_info.h"
#include "wlan_mac_ibss.h"

/*************************** Constant Definitions ****************************/
#define  WLAN_DEFAULT_CHANNEL                    1
#define  WLAN_DEFAULT_TX_PWR                     5

const u8 max_num_associations                    = 1;


/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/

// If you want this station to try to associate to a known AP at boot, type
//   the string here. Otherwise, let it be an empty string.
char default_ssid[SSID_LEN_MAX + 1] = "WARP-IBSS";
//char default_ssid[SSID_LEN_MAX + 1] = "";


// Common TX header for 802.11 packets
mac_header_80211_common           tx_header_common;

// Default transmission parameters
tx_params                         default_unicast_mgmt_tx_params;
tx_params                         default_unicast_data_tx_params;
tx_params                         default_multicast_mgmt_tx_params;
tx_params                         default_multicast_data_tx_params;

// Top level IBSS state
static u8                         wlan_mac_addr[6];
u8                                uart_mode;                         // Control variable for UART MENU
u32	                              max_queue_size;                    // Maximum transmit queue size
static u32 						  beacon_sched_id = SCHEDULE_FAILURE;

u8                                pause_data_queue;


u32                               mac_param_chan;
bss_info*		   				  ap_bss_info;	//FIXME: This is messy. WN is looking for an ap_bss_info, but I don't plan on having one. WN should use getters.
												//Furthermore, there won't be any notion of bss_info in the example bridge project, so WN can't rely on it being here
												//in every top-level project.
bss_info*						  ibss_info;


// List to hold Tx/Rx statistics
dl_list		                      statistics_table;



/*************************** Functions Prototypes ****************************/

#ifdef WLAN_USE_UART_MENU
void uart_rx(u8 rxByte);                         // Implemented in wlan_mac_sta_uart_menu.c
#else
void uart_rx(u8 rxByte){ };
#endif

u8 tim_bitmap[1] = {0x0};
u8 tim_control = 1;

/******************************** Functions **********************************/

int main() {
	u64 scan_start_timestamp;
	u8 locally_administered_addr[6];
	dl_entry* ibss_info_entry;

	// This function should be executed first. It will zero out memory, and if that
	//     memory is used before calling this function, unexpected results may happen.
	wlan_mac_high_heap_init();

	// Initialize the maximum TX queue size
	max_queue_size = MAX_TX_QUEUE_LEN;

	// Unpause the queue
	pause_data_queue = 0;

	ibss_info = NULL;

	// Print initial message to UART
	xil_printf("\f");
	xil_printf("----- Mango 802.11 Reference Design -----\n");
	xil_printf("----- v0.95 Beta ------------------------\n");
	xil_printf("----- wlan_mac_ibss ----------------------\n");
	xil_printf("Compiled %s %s\n\n", __DATE__, __TIME__);

    // Set default transmission parameters
	default_unicast_data_tx_params.mac.num_tx_max     = MAX_NUM_TX;
	default_unicast_data_tx_params.phy.power          = WLAN_DEFAULT_TX_PWR;
	default_unicast_data_tx_params.phy.rate           = WLAN_MAC_RATE_18M;
	default_unicast_data_tx_params.phy.antenna_mode   = TX_ANTMODE_SISO_ANTA;

	default_unicast_mgmt_tx_params.mac.num_tx_max     = MAX_NUM_TX;
	default_unicast_mgmt_tx_params.phy.power          = WLAN_DEFAULT_TX_PWR;
	default_unicast_mgmt_tx_params.phy.rate           = WLAN_MAC_RATE_6M;
	default_unicast_mgmt_tx_params.phy.antenna_mode   = TX_ANTMODE_SISO_ANTA;

	//All multicast traffic (incl. broadcast) uses these default Tx params
	default_multicast_data_tx_params.mac.num_tx_max   = 1;
	default_multicast_data_tx_params.phy.power        = WLAN_DEFAULT_TX_PWR;
	default_multicast_data_tx_params.phy.rate         = WLAN_MAC_RATE_6M;
	default_multicast_data_tx_params.phy.antenna_mode = TX_ANTMODE_SISO_ANTA;

	default_multicast_mgmt_tx_params.mac.num_tx_max   = 1;
	default_multicast_mgmt_tx_params.phy.power        = WLAN_DEFAULT_TX_PWR;
	default_multicast_mgmt_tx_params.phy.rate         = WLAN_MAC_RATE_6M;
	default_multicast_mgmt_tx_params.phy.antenna_mode = TX_ANTMODE_SISO_ANTA;


	// Initialize the utility library
    wlan_mac_high_init();

#ifdef USE_WARPNET_WLAN_EXP
    // Configure WLAN Exp framework
	wlan_exp_configure(WLAN_EXP_NODE_TYPE, WLAN_EXP_ETH);
#endif

	// Initialize callbacks
	wlan_mac_util_set_eth_rx_callback(       (void*)ethernet_receive);
	wlan_mac_high_set_mpdu_tx_done_callback( (void*)mpdu_transmit_done);
	wlan_mac_high_set_mpdu_dequeue_callback( (void*)mpdu_dequeue);
	wlan_mac_high_set_mpdu_rx_callback(      (void*)mpdu_rx_process);
	wlan_mac_high_set_uart_rx_callback(      (void*)uart_rx);
	wlan_mac_high_set_mpdu_accept_callback(  (void*)poll_tx_queues);
	wlan_mac_ltg_sched_set_callback(         (void*)ltg_event);

	// Set the Ethernet ecapsulation mode
	wlan_mac_util_set_eth_encap_mode(ENCAP_MODE_IBSS);

	// Initialize the association and statistics tables
	dl_list_init(&statistics_table);

	// Ask CPU Low for its status
	//     The response to this request will be handled asynchronously
	wlan_mac_high_request_low_state();

    // Wait for CPU Low to initialize
	while( wlan_mac_high_is_cpu_low_initialized() == 0){
		xil_printf("waiting on CPU_LOW to boot\n");
	};

	// CPU Low will pass HW information to CPU High as part of the boot process
	//   - Get necessary HW information
	memcpy((void*) &(wlan_mac_addr[0]), (void*) wlan_mac_high_get_eeprom_mac_addr(), 6);

    // Set Header information
	tx_header_common.address_2 = &(wlan_mac_addr[0]);

	// Set up channel
	mac_param_chan      = WLAN_DEFAULT_CHANNEL;
	wlan_mac_high_set_channel( mac_param_chan );

	// Set the other CPU low parameters
	wlan_mac_high_set_rx_ant_mode(RX_ANTMODE_SISO_ANTA);
	wlan_mac_high_set_tx_ctrl_pow(WLAN_DEFAULT_TX_PWR);

	// Configure CPU Low's filter for passing Rx packets up to CPU High
	//     Default is "promiscuous" mode - pass all data and management packets with good or bad checksums
	//     This allows logging of all data/management receptions, even if they're not intended for this node
	wlan_mac_high_set_rx_filter_mode(RX_FILTER_FCS_ALL | RX_FILTER_HDR_ALL_MPDU);

    // Initialize interrupts
	wlan_mac_high_interrupt_init();

    // Schedule all events
    //     None at this time



	// Reset the event log
	event_log_reset();

	// Print Station information to the terminal
    xil_printf("WLAN MAC Station boot complete: \n");
	xil_printf("  MAC Addr     : %02x-%02x-%02x-%02x-%02x-%02x\n\n",wlan_mac_addr[0],wlan_mac_addr[1],wlan_mac_addr[2],wlan_mac_addr[3],wlan_mac_addr[4],wlan_mac_addr[5]);

#ifdef WLAN_USE_UART_MENU
	uart_mode = UART_MODE_MAIN;
	xil_printf("\nAt any time, press the Esc key in your terminal to access the AP menu\n");
#endif

#ifdef USE_WARPNET_WLAN_EXP
	// Set AP processing callbacks
	node_set_process_callback( (void *)wlan_exp_node_sta_processCmd );
#endif

	// Start the interrupts
	wlan_mac_high_interrupt_start();

	// Set the default active scan channels
	u8 channel_selections[14] = {1,2,3,4,5,6,7,8,9,10,11,36,44,48};
	wlan_mac_sta_set_scan_channels(channel_selections, sizeof(channel_selections)/sizeof(channel_selections[0]));

	//Search for existing IBSS of default SSID string.

	#define SCAN_TIMEOUT_USEC 5000000
	scan_start_timestamp = get_usec_timestamp();
	wlan_mac_sta_scan_enable((u8*)bcast_addr, default_ssid);

	while((get_usec_timestamp() - scan_start_timestamp) < SCAN_TIMEOUT_USEC){
		ibss_info_entry = wlan_mac_high_find_bss_info_SSID(default_ssid);
		if(ibss_info_entry != NULL){
			break;
		}
	}
	wlan_mac_sta_scan_disable();

	if(ibss_info_entry != NULL){
		xil_printf("Found existing IBSS\n");
		ibss_info = (bss_info*)(ibss_info_entry->data);
		ibss_info->state = BSS_STATE_OWNED;
	} else {
		xil_printf("Creating new IBSS\n");
		memcpy(locally_administered_addr,wlan_mac_addr,6);
		locally_administered_addr[0] |= MAC_ADDR_MSB_MASK_LOCAL; //Raise the bit identifying this address as locally administered
		ibss_info = wlan_mac_high_create_bss_info(locally_administered_addr, default_ssid, WLAN_DEFAULT_CHANNEL);
		ibss_info->beacon_interval = BEACON_INTERVAL_MS;
		ibss_info->state = BSS_STATE_OWNED;
	}

	mac_param_chan      = ibss_info->chan;
	wlan_mac_high_set_channel( mac_param_chan );

	xil_printf("IBSS Details: \n");
	xil_printf("  BSSID           : %02x-%02x-%02x-%02x-%02x-%02x\n",ibss_info->bssid[0],ibss_info->bssid[1],ibss_info->bssid[2],ibss_info->bssid[3],ibss_info->bssid[4],ibss_info->bssid[5]);
	xil_printf("   SSID           : %s\n", ibss_info->ssid);
	xil_printf("   Channel        : %d\n", ibss_info->chan);
	xil_printf("   Beacon Interval: %d ms\n",ibss_info->beacon_interval);



	//802.11-2012 10.1.3.3 Beacon generation in an IBSS
	//Note: Unlike the AP implementation, we need to use the SCHEDULE_FINE scheduler sub-beacon-interval fidelity
	beacon_sched_id = wlan_mac_schedule_event_repeated(SCHEDULE_FINE, (ibss_info->beacon_interval)*1000, 1, (void*)beacon_transmit);


	while(1){
#ifdef USE_WARPNET_WLAN_EXP
		// The wlan_exp Ethernet handling is not interrupt based. Periodic polls of the wlan_exp
		//     transport are required to service new commands. All other node activity (wired/wireless Tx/Rx,
		//     scheduled events, user interaction, etc) are handled via interrupt service routines
		transport_poll( WLAN_EXP_ETH );
#endif
	}

	// Unreachable, but non-void return keeps the compiler happy
	return -1;
}

/**
 * @brief Transmit a beacon
 *
 * This function will create and enqueue a beacon.
 *
 * @param  None
 * @return None
 */
void beacon_transmit(u32 schedule_id) {

 	u16 tx_length;
 	tx_queue_element*	curr_tx_queue_element;
 	tx_queue_buffer* 	curr_tx_queue_buffer;

 	//When an IBSS node receives a beacon, it schedules the call of this beacon_transmit function
 	//for some point in the future that is generally less than the beacon interval to account for
 	//the delay in reception and processing. As such, this function will update the period of this
 	//schedule with the actual beacon interval.

 	beacon_sched_id = wlan_mac_schedule_event_repeated(SCHEDULE_FINE, (ibss_info->beacon_interval)*1000, 1, (void*)beacon_transmit);

 	// Create a beacon
 	curr_tx_queue_element = queue_checkout();

 	if((curr_tx_queue_element != NULL) && (ibss_info != NULL)){
 		curr_tx_queue_buffer = (tx_queue_buffer*)(curr_tx_queue_element->data);

		// Setup the TX header
 		wlan_mac_high_setup_tx_header( &tx_header_common, (u8 *)bcast_addr, ibss_info->bssid );

		// Fill in the data
        tx_length = wlan_create_beacon_frame(
			(void*)(curr_tx_queue_buffer->frame),
			&tx_header_common,
			ibss_info->beacon_interval,
			(CAPABILITIES_SHORT_TIMESLOT | CAPABILITIES_IBSS),
			strlen(default_ssid),
			(u8*)default_ssid,
			mac_param_chan);

		// Setup the TX frame info
 		wlan_mac_high_setup_tx_frame_info ( &tx_header_common, curr_tx_queue_element, tx_length, (TX_MPDU_FLAGS_FILL_TIMESTAMP | TX_MPDU_FLAGS_REQ_BO | TX_MPDU_FLAGS_AUTOCANCEL), MANAGEMENT_QID );

		// Set the information in the TX queue buffer
 		curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
 		curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)(&default_multicast_mgmt_tx_params);
		curr_tx_queue_buffer->frame_info.AID         = 0;

		// Put the packet in the queue
 		enqueue_after_tail(MANAGEMENT_QID, curr_tx_queue_element);

	    // Poll the TX queues to possibly send the packet
 		poll_tx_queues();
 	}

}


/**
 * @brief Poll Tx queues to select next available packet to transmit
 *
 * This function is called whenever the upper MAC is ready to send a new packet
 * to the lower MAC for transmission. The next packet to transmit is selected
 * from one of the currently-enabled Tx queues.
 *
 * The reference implementation uses a simple queue prioritization scheme:
 *   - Two queues are defined: Management (MANAGEMENT_QID) and Data (UNICAST_QID)
 *     - The Management queue is for all management traffic
 *     - The Data queue is for all data to the associated AP
 *   - The code alternates its polling between queues
 *
 * This function uses the framework function dequeue_transmit_checkin() to check individual queues
 * If dequeue_transmit_checkin() is passed a not-empty queue, it will dequeue and transmit a packet, then
 * return a non-zero status. Thus the calls below terminate polling as soon as any call to dequeue_transmit_checkin()
 * returns with a non-zero value, allowing the next call to poll_tx_queues() to continue the queue polling process.
 *
 * @param None
 * @return None
 */
void poll_tx_queues(){
	u8 i;

	#define MAX_NUM_QUEUE 3

	// Are we pausing transmissions?
	if(pause_data_queue == 0){

		static u32 queue_index = 0;

		// Is CPU low ready for a transmission?
		if( wlan_mac_high_is_ready_for_tx() ) {

			// Alternate between checking the management queue and the data queue
			for(i = 0; i < MAX_NUM_QUEUE; i++) {
				queue_index = (queue_index + 1) % MAX_NUM_QUEUE;

				switch(queue_index){
					case 0:  if(dequeue_transmit_checkin(BEACON_QID)) { return; }  break;
					case 1:  if(dequeue_transmit_checkin(MANAGEMENT_QID)) { return; }  break;
					case 2:  if(dequeue_transmit_checkin(UNICAST_QID))    { return; }  break;
				}
			}
		}
	} else {
		// We are only currently allowed to send management frames. Typically this is caused by an ongoing
		// active scan
		if( wlan_mac_high_is_ready_for_tx() ) {
			dequeue_transmit_checkin(MANAGEMENT_QID);
		}
	}
}



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
 */
void purge_all_data_tx_queue(){
	// Purge all data transmit queues
	purge_queue(MCAST_QID);           // Broadcast Queue
	purge_queue(UNICAST_QID);         // Unicast Queue
}




/**
 * @brief Callback to handle a packet after it was transmitted by the lower-level MAC
 *
 * This function is called when CPU Low indicates it has completed the Tx process for a packet previously
 * submitted by CPU High.
 *
 * CPU High has two responsibilities post-Tx:
 *  - Cleanup any resources dedicated to the packet
 *  - Update any statistics and log info to reflect the Tx result
 *
 * @param tx_frame_info* tx_mpdu
 *  - Pointer to the MPDU which was just transmitted
 * @param wlan_mac_low_tx_details* tx_low_details
 *  - Pointer to the array of data recorded by the lower-level MAC about each re-transmission of the MPDU
 * @param u16 num_tx_low_details
 *  - number of elements in array pointed to by previous argument
 * @return None
*/
void mpdu_transmit_done(tx_frame_info* tx_mpdu, wlan_mac_low_tx_details* tx_low_details, u16 num_tx_low_details) {
	u32                    i;
	u64                    ts_old                  = 0;
	station_info*          station 				   = NULL;


	//if(ap_bss_info != NULL) station = (station_info*)(ap_bss_info->associated_stations.first->data);
	//TODO: Search for station info for this Tx in ibss_info->associated_stations

	// Additional variables (Future Use)
	// void*                  mpdu                    = (u8*)tx_mpdu + PHY_TX_PKT_BUF_MPDU_OFFSET;
	// u8*                    mpdu_ptr_u8             = (u8*)mpdu;
	// mac_header_80211*      tx_80211_header         = (mac_header_80211*)((void *)mpdu_ptr_u8);

	// Log all of the TX Low transmissions
	for(i = 0; i < num_tx_low_details; i++) {

		// Log the TX low
		wlan_exp_log_create_tx_low_entry(tx_mpdu, &tx_low_details[i], ts_old, i);

		// Accumulate the time-between-transmissions, used to calculate absolute time of each TX_LOW event above
		ts_old += tx_low_details[i].tx_start_delta;
	}

	// Log the TX MPDU
	wlan_exp_log_create_tx_entry(tx_mpdu, mac_param_chan);

	// Update the statistics for the node to which the packet was just transmitted
	if(tx_mpdu->AID != 0) {
		wlan_mac_high_update_tx_statistics(tx_mpdu, station);
	}

	// Send log entry to wlan_exp controller immediately (not currently supported)
	//
	// if (tx_high_event_log_entry != NULL) {
    //     wn_transmit_log_entry((void *)tx_high_event_log_entry);
	// }
}



/**
 * @brief Callback to handle insertion of an Ethernet reception into the corresponding wireless Tx queue
 *
 * This function is called when a new Ethernet packet is received that must be transmitted via the wireless interface.
 * The packet must be encapsulated before it is passed to this function. Ethernet encapsulation is implemented in the mac_high framework.
 *
 * The tx_queue_list argument is a DL list, but must contain exactly one queue entry which contains the encapsulated packet
 * A list container is used here to ease merging of the list with the target queue.
 *
 * @param tx_queue_element* curr_tx_queue_element
 *  - A single queue element containing the packet to transmit
 * @param u8* eth_dest
 *  - 6-byte destination address from original Ethernet packet
 * @param u8* eth_src
 *  - 6-byte source address from original Ethernet packet
 * @param u16 tx_length
 *  - Length (in bytes) of the packet payload
 * @return 1 for successful enqueuing of the packet, 0 otherwise
 */
int ethernet_receive(tx_queue_element* curr_tx_queue_element, u8* eth_dest, u8* eth_src, u16 tx_length){
	// TODO: We need to create station_info structs for outgoing MAC addresses. Unlike the AP,
	// the IBSS doesn't have the luxury of creating these structures at the time of association since
	// there is no formal association for IBSS. At the moment, we do not employ station_info and cannot
	// support different transmit parameters for different target addresses.

	tx_queue_buffer* 	curr_tx_queue_buffer;

	if(ibss_info != NULL){

		if(queue_num_queued(UNICAST_QID) < max_queue_size){

			// Send the pre-encapsulated Ethernet frame over the wireless interface
			//     NOTE:  The queue element has already been provided, so we do not need to check if it is NULL
			curr_tx_queue_buffer = (tx_queue_buffer*)(curr_tx_queue_element->data);

			// Setup the TX header
			wlan_mac_high_setup_tx_header( &tx_header_common, eth_dest,ibss_info->bssid);

			// Fill in the data
			wlan_create_data_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, 0);

			// Setup the TX frame info
			wlan_mac_high_setup_tx_frame_info ( &tx_header_common, curr_tx_queue_element, tx_length, (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO), UNICAST_QID );

			// Set the information in the TX queue buffer
			curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
			curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)(&default_unicast_data_tx_params);
			curr_tx_queue_buffer->frame_info.AID         = 0;

			// Put the packet in the queue
			enqueue_after_tail(UNICAST_QID, curr_tx_queue_element);

			// Poll the TX queues to possibly send the packet
			poll_tx_queues();

		} else {
			// Packet was not successfully enqueued
			return 0;
		}

		// Packet was successfully enqueued
		return 1;
	} else {
		return 0;
	}
}



/**
 * @brief Process received MPDUs
 *
 * This callback function will process all the received MPDUs.
 *
 * This function must implement the state machine that will allow a station to join the AP.
 *
 * @param  void * pkt_buf_addr
 *     - Packet buffer address;  Contains the contents of the MPDU as well as other packet information from CPU low
 * @param  u8 rate
 *     - Rate that the packet was transmitted
 * @param  u16 length
 *     - Length of the MPDU
 * @return None
 */
void mpdu_rx_process(void* pkt_buf_addr, u8 rate, u16 length) {

	rx_frame_info*      mpdu_info                = (rx_frame_info*)pkt_buf_addr;
	void*               mpdu                     = (u8*)pkt_buf_addr + PHY_RX_PKT_BUF_MPDU_OFFSET;
	u8*                 mpdu_ptr_u8              = (u8*)mpdu;
	mac_header_80211*   rx_80211_header          = (mac_header_80211*)((void *)mpdu_ptr_u8);

	u16                 rx_seq;
	rx_common_entry*    rx_event_log_entry       = NULL;

	dl_entry*	        associated_station_entry;
	station_info*       associated_station       = NULL;
	statistics_txrx*    station_stats            = NULL;

	tx_queue_element*   curr_tx_queue_element;
	tx_queue_buffer*    curr_tx_queue_buffer;

	u8                  unicast_to_me;
	u8                  to_multicast;
	s64                 timestamp_diff;
	dl_entry*			bss_info_entry;
	bss_info*			curr_bss_info;
	u8					send_response			 = 0;
	u32					tx_length;


	// Log the reception
	rx_event_log_entry = wlan_exp_log_create_rx_entry(mpdu_info, mac_param_chan, rate);

	// Determine destination of packet
	unicast_to_me = wlan_addr_eq(rx_80211_header->address_1, wlan_mac_addr);
	to_multicast  = wlan_addr_mcast(rx_80211_header->address_1);

    // If the packet is good (ie good FCS) and it is destined for me, then process it
	if( (mpdu_info->state == RX_MPDU_STATE_FCS_GOOD) && (unicast_to_me || to_multicast)){

		// Update the association information
		if(ibss_info != NULL){
			associated_station_entry = wlan_mac_high_find_station_info_ADDR(&(ibss_info->associated_stations), (rx_80211_header->address_2));
		} else {
			associated_station_entry = NULL;
		}

		if(associated_station_entry != NULL) {
			associated_station = (station_info*)(associated_station_entry->data);

			// Update station information
			associated_station->rx.last_timestamp = get_usec_timestamp();
			associated_station->rx.last_power     = mpdu_info->rx_power;
			associated_station->rx.last_rate      = mpdu_info->rate;

			rx_seq        = ((rx_80211_header->sequence_control)>>4)&0xFFF;
			station_stats = associated_station->stats;

			// Check if this was a duplicate reception
			//   - Received seq num matched previously received seq num for this STA
			if( (associated_station->rx.last_seq != 0)  && (associated_station->rx.last_seq == rx_seq) ) {
				if(rx_event_log_entry != NULL){
					rx_event_log_entry->flags |= RX_ENTRY_FLAGS_IS_DUPLICATE;
				}

				// Finish the function
				goto mpdu_rx_process_end;
			} else {
				associated_station->rx.last_seq = rx_seq;
			}
		} else {
			station_stats = wlan_mac_high_add_statistics(&statistics_table, NULL, rx_80211_header->address_2);
		}

        // Update receive statistics
		if(station_stats != NULL){
			station_stats->last_rx_timestamp = get_usec_timestamp();
			if((rx_80211_header->frame_control_1 & 0xF) == MAC_FRAME_CTRL1_TYPE_DATA){
				((station_stats)->data.rx_num_packets)++;
				((station_stats)->data.rx_num_bytes) += mpdu_info->length;
			} else if((rx_80211_header->frame_control_1 & 0xF) == MAC_FRAME_CTRL1_TYPE_MGMT) {
				((station_stats)->mgmt.rx_num_packets)++;
				((station_stats)->mgmt.rx_num_bytes) += mpdu_info->length;
			}
		}

		// Process the packet
		switch(rx_80211_header->frame_control_1) {

			//---------------------------------------------------------------------
			case (MAC_FRAME_CTRL1_SUBTYPE_DATA):
				// Data packet
				//   - If the STA is associated with the AP and this is from the DS, then transmit over the wired network
				//
				if(ibss_info != NULL){
					if(wlan_addr_eq(rx_80211_header->address_3, ibss_info->bssid)) {
						// MPDU is flagged as destined to the DS - send it for de-encapsulation and Ethernet Tx (if appropriate)
						wlan_mpdu_eth_send(mpdu,length);
					}
				}
			break;

			//---------------------------------------------------------------------
			case (MAC_FRAME_CTRL1_SUBTYPE_PROBE_REQ):
				if(ibss_info != NULL){
					if(wlan_addr_eq(rx_80211_header->address_3, bcast_addr)) {
						mpdu_ptr_u8 += sizeof(mac_header_80211);

						// Loop through tagged parameters
						while(((u32)mpdu_ptr_u8 -  (u32)mpdu)<= length){

							// What kind of tag is this?
							switch(mpdu_ptr_u8[0]){
								//-----------------------------------------------------
								case TAG_SSID_PARAMS:
									// SSID parameter set
									if((mpdu_ptr_u8[1]==0) || (memcmp(mpdu_ptr_u8+2, (u8*)default_ssid, mpdu_ptr_u8[1])==0)) {
										// Broadcast SSID or my SSID - send unicast probe response
										send_response = 1;
									}
								break;

								//-----------------------------------------------------
								case TAG_SUPPORTED_RATES:
									// Supported rates
								break;

								//-----------------------------------------------------
								case TAG_EXT_SUPPORTED_RATES:
									// Extended supported rates
								break;

								//-----------------------------------------------------
								case TAG_DS_PARAMS:
									// DS Parameter set (e.g. channel)
								break;
							}

							// Move up to the next tag
							mpdu_ptr_u8 += mpdu_ptr_u8[1]+2;
						}

						if(send_response) {
							// Create a probe response frame
							curr_tx_queue_element = queue_checkout();

							if(curr_tx_queue_element != NULL){
								curr_tx_queue_buffer = (tx_queue_buffer*)(curr_tx_queue_element->data);

								// Setup the TX header
								wlan_mac_high_setup_tx_header( &tx_header_common, rx_80211_header->address_2, ibss_info->bssid );

								// Fill in the data
								tx_length = wlan_create_probe_resp_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, ibss_info->beacon_interval, (CAPABILITIES_IBSS | CAPABILITIES_SHORT_TIMESLOT), strlen(default_ssid), (u8*)default_ssid, ibss_info->chan);

								// Setup the TX frame info
								wlan_mac_high_setup_tx_frame_info ( &tx_header_common, curr_tx_queue_element, tx_length, (TX_MPDU_FLAGS_FILL_TIMESTAMP | TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO), MANAGEMENT_QID );

								// Set the information in the TX queue buffer
								curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
								curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)(&default_unicast_mgmt_tx_params);
								curr_tx_queue_buffer->frame_info.AID         = 0;

								// Put the packet in the queue
								enqueue_after_tail(MANAGEMENT_QID, curr_tx_queue_element);

								// Poll the TX queues to possibly send the packet
								poll_tx_queues();
							}

							// Finish the function
							goto mpdu_rx_process_end;
						}
					}
				}
			break;

            //---------------------------------------------------------------------
			case (MAC_FRAME_CTRL1_SUBTYPE_BEACON):
			    // Beacon Packet
			    //   -
			    //

			    // Define the PHY timestamp offset
				#define PHY_T_OFFSET 25

			    // Update the timestamp from the beacon
				if(ibss_info != NULL){
					// If this packet was from our IBSS
					if( wlan_addr_eq( ibss_info->bssid, rx_80211_header->address_3)){



						// Move the packet pointer to after the header
						mpdu_ptr_u8 += sizeof(mac_header_80211);

						// Calculate the difference between the beacon timestamp and the packet timestamp
						//     NOTE:  We need to compensate for the time it takes to set the timestamp in the PHY
						timestamp_diff = (s64)(((beacon_probe_frame*)mpdu_ptr_u8)->timestamp) - (s64)(mpdu_info->timestamp) + PHY_T_OFFSET;

						// Set the timestamp
						if(timestamp_diff > 0){
							wlan_mac_high_set_timestamp_delta(timestamp_diff);
						}

						// We need to adjust the phase of our TBTT. To do this, we will kill the old schedule event, and restart now (which is near the TBTT)
						if(beacon_sched_id != SCHEDULE_FAILURE){
							wlan_mac_remove_schedule(SCHEDULE_FINE, beacon_sched_id);
							timestamp_diff = get_usec_timestamp() - ((beacon_probe_frame*)mpdu_ptr_u8)->timestamp;
							beacon_sched_id = wlan_mac_schedule_event_repeated(SCHEDULE_FINE, (ibss_info->beacon_interval)*1000 - timestamp_diff, 1, (void*)beacon_transmit);
						}

						if(queue_num_queued(BEACON_QID)){

							//We should destroy the beacon that is currently enqueued if
							//it exists. Note: these statements aren't typically executed.
							//It's very likely the to-be-transmitted BEACON is already down
							//in CPU_LOW's domain and needs to be cancelled there.
							curr_tx_queue_element = dequeue_from_head(BEACON_QID);
							if(curr_tx_queue_element != NULL){
								queue_checkin(curr_tx_queue_element);
								wlan_eth_dma_update();
							}
						}

						// Move the packet pointer back to the start for the rest of the function
						mpdu_ptr_u8 -= sizeof(mac_header_80211);
					}

				}

			break;


            //---------------------------------------------------------------------
			default:
				//This should be left as a verbose print. It occurs often when communicating with mobile devices since they tend to send
				//null data frames (type: DATA, subtype: 0x4) for power management reasons.
				warp_printf(PL_VERBOSE, "Received unknown frame control type/subtype %x\n",rx_80211_header->frame_control_1);
			break;
		}

		// Finish the function
		goto mpdu_rx_process_end;
	} else {
		// Process any Bad FCS packets
		goto mpdu_rx_process_end;
	}


	// Finish any processing for the RX MPDU process
	mpdu_rx_process_end:

	// Currently, asynchronous transmission of log entries is not supported
	//
	if ((rx_event_log_entry != NULL) && ((rx_event_log_entry->rate) != WLAN_MAC_RATE_1M)) {
        wn_transmit_log_entry((void *)rx_event_log_entry);
	}

    return;
}



/**
 * @brief Callback to handle new Local Traffic Generator event
 *
 * This function is called when the LTG scheduler determines a traffic generator should create a new packet. The
 * behavior of this function depends entirely on the LTG payload parameters.
 *
 * The reference implementation defines 3 LTG payload types:
 *  - LTG_PYLD_TYPE_FIXED: generate 1 fixed-length packet to single destination; callback_arg is pointer to ltg_pyld_fixed struct
 *  - LTG_PYLD_TYPE_UNIFORM_RAND: generate 1 random-length packet to signle destimation; callback_arg is pointer to ltg_pyld_uniform_rand struct
 *  - LTG_PYLD_TYPE_ALL_ASSOC_FIXED: generate 1 fixed-length packet to each associated station; callback_arg is poitner to ltg_pyld_all_assoc_fixed struct
 *
 * @param u32 id
 *  - Unique ID of the previously created LTG
 * @param void* callback_arg
 *  - Callback argument provided at LTG creation time; interpretation depends on LTG type
 * @return None
 */
void ltg_event(u32 id, void* callback_arg){

// TODO: Needs to be more like AP's LTG event

}


/**
 * @brief Reset Station Statistics
 *
 * Reset all statistics being kept for all stations
 *
 * @param  None
 * @return None
 */
void reset_station_statistics(){
	wlan_mac_high_reset_statistics(&statistics_table);
}


void reset_all_associations(){
	//FIXME: This doesn't mean anything for the IBSS implementation, but WN needs it to be here.
	//WN shouldn't require top-level code to implement this function. It should be callback that is optionally
	//set up by top-level code at boot.
}


void mpdu_dequeue(tx_queue_element* packet){
	mac_header_80211* 	header;
	tx_frame_info*		frame_info;
	ltg_packet_id*      pkt_id;
	u32 				packet_payload_size;

	header 	  			= (mac_header_80211*)((((tx_queue_buffer*)(packet->data))->frame));
	frame_info 			= (tx_frame_info*)&((((tx_queue_buffer*)(packet->data))->frame_info));
	packet_payload_size	= frame_info->length;

	switch(wlan_mac_high_pkt_type(header, packet_payload_size)){
		case PKT_TYPE_DATA_ENCAP_LTG:
			pkt_id		       = (ltg_packet_id*)((u8*)header + sizeof(mac_header_80211));
			pkt_id->unique_seq = wlan_mac_high_get_unique_seq();
		break;
	}

}

/**
 * @brief Accessor methods for global variables
 *
 * These functions will return pointers to global variables
 *
 * @param  None
 * @return None
 */
dl_list * get_station_info_list(){
	if(ibss_info != NULL){
		return &(ibss_info->associated_stations);
	} else {
		return NULL;
	}
}
dl_list * get_statistics()       { return &statistics_table;   }



