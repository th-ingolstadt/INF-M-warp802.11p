/** @file wlan_mac_sta.c
 *  @brief Station
 *
 *  This contains code for the 802.11 Station.
 *
 *  @copyright Copyright 2013-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

/***************************** Include Files *********************************/

// Xilinx SDK includes
#include "xparameters.h"
#include "stdio.h"
#include "stdlib.h"
#include "xtmrctr.h"
#include "xio.h"
#include "string.h"
#include "xintc.h"

// WLAN includes
#include "wlan_mac_time_util.h"
#include "wlan_mac_userio_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_entries.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "ascii_characters.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_bss_info.h"
#include "wlan_mac_scan.h"
#include "wlan_mac_sta_join.h"
#include "wlan_mac_sta.h"


// WLAN Exp includes
#include "wlan_exp.h"
#include "wlan_exp_common.h"
#include "wlan_exp_node.h"
#include "wlan_exp_node_sta.h"
#include "wlan_exp_transport.h"
#include "wlan_exp_user.h"


/*************************** Constant Definitions ****************************/

#define  WLAN_EXP_ETH                            TRANSPORT_ETH_B
#define  WLAN_EXP_NODE_TYPE                      (WLAN_EXP_TYPE_DESIGN_80211 + WLAN_EXP_TYPE_DESIGN_80211_CPU_HIGH_STA)


#define  WLAN_DEFAULT_USE_HT					  0
#define  WLAN_DEFAULT_CHANNEL                     1
#define  WLAN_DEFAULT_TX_PWR                      15
#define  WLAN_DEFAULT_TX_ANTENNA                  TX_ANTMODE_SISO_ANTA
#define  WLAN_DEFAULT_RX_ANTENNA                  RX_ANTMODE_SISO_ANTA


/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/

// If you want this station to try to associate to a known AP at boot, type
//   the string here. Otherwise, let it be an empty string.
static char                       access_point_ssid[SSID_LEN_MAX + 1] = "WARP-AP";
// static char                       access_point_ssid[SSID_LEN_MAX + 1] = "";

// Common TX header for 802.11 packets
mac_header_80211_common           tx_header_common;

// Default transmission parameters
tx_params_t                       default_unicast_mgmt_tx_params;
tx_params_t                       default_unicast_data_tx_params;
tx_params_t                       default_multicast_mgmt_tx_params;
tx_params_t                       default_multicast_data_tx_params;

// Access point information
u8                                my_aid;
bss_info*                         my_bss_info;

// List to hold Tx/Rx counts
dl_list                           counts_table;

// Tx queue variables;
static u32                        max_queue_size;
volatile u8                       pause_data_queue;

// MAC address
static u8                         wlan_mac_addr[6];

// Beacon configuration
static beacon_txrx_configure_t    gl_beacon_txrx_config;

// CPU Low configuration information
wlan_mac_low_config_t             cpu_low_config;


/*************************** Functions Prototypes ****************************/

#ifdef USE_WLAN_EXP
int  wlan_exp_process_user_cmd(u32 cmd_id, int socket_index, void * from, cmd_resp * command, cmd_resp * response, u32 max_resp_len);
#endif

void sta_set_beacon_ts_update_mode(u32 enable);


/******************************** Functions **********************************/


int main() {
	// This list of channels will be used by the active scan state machine. The STA will scan
	//  each channel looking for a network with the default SSID
	//

	volatile join_parameters_t*             join_parameters;

	// Print initial message to UART
	xil_printf("\f");
	xil_printf("----- Mango 802.11 Reference Design -----\n");
	xil_printf("----- v1.5   ----------------------------\n");
	xil_printf("----- wlan_mac_sta ----------------------\n");
	xil_printf("Compiled %s %s\n\n", __DATE__, __TIME__);

	// Heap Initialization
	//    The heap must be initialized before any use of malloc. This explicit
	//    init handles the case of soft-reset of the MicroBlaze leaving stale
	//    values in the heap RAM
	wlan_mac_high_heap_init();

	// Initialize the maximum TX queue size
	max_queue_size = MAX_TX_QUEUE_LEN;

	// Unpause the queue
	pause_data_queue = 0;

	// Initialize AID / Beacons configuration (not associated with an AP)
	my_aid = 0;

	gl_beacon_txrx_config.ts_update_mode = ALWAYS_UPDATE;
	bzero(gl_beacon_txrx_config.bssid_match, BSSID_LEN);
	gl_beacon_txrx_config.beacon_tx_mode = NO_BEACON_TX;
	gl_beacon_txrx_config.beacon_interval_tu = 0;

	// New associations adopt these unicast params; the per-node params can be
	//   overridden via wlan_exp calls or by custom C code
	default_unicast_data_tx_params.phy.power          = WLAN_DEFAULT_TX_PWR;
	default_unicast_data_tx_params.phy.mcs            = 3;
#if WLAN_DEFAULT_USE_HT
	default_unicast_data_tx_params.phy.phy_mode       = PHY_MODE_HTMF;
#else
	default_unicast_data_tx_params.phy.phy_mode       = PHY_MODE_NONHT;
#endif
	default_unicast_data_tx_params.phy.antenna_mode   = WLAN_DEFAULT_TX_ANTENNA;

	default_unicast_mgmt_tx_params.phy.power          = WLAN_DEFAULT_TX_PWR;
	default_unicast_mgmt_tx_params.phy.mcs            = 0;
	default_unicast_mgmt_tx_params.phy.phy_mode       = PHY_MODE_NONHT;
	default_unicast_mgmt_tx_params.phy.antenna_mode   = WLAN_DEFAULT_TX_ANTENNA;

	// All multicast traffic (incl. broadcast) uses these default Tx params
	default_multicast_data_tx_params.phy.power        = WLAN_DEFAULT_TX_PWR;
	default_multicast_data_tx_params.phy.mcs          = 0;
	default_multicast_data_tx_params.phy.phy_mode     = PHY_MODE_NONHT;
	default_multicast_data_tx_params.phy.antenna_mode = WLAN_DEFAULT_TX_ANTENNA;

	default_multicast_mgmt_tx_params.phy.power        = WLAN_DEFAULT_TX_PWR;
	default_multicast_mgmt_tx_params.phy.mcs          = 0;
	default_multicast_mgmt_tx_params.phy.phy_mode     = PHY_MODE_NONHT;
	default_multicast_mgmt_tx_params.phy.antenna_mode = WLAN_DEFAULT_TX_ANTENNA;

	// Initialize the utility library
	wlan_mac_high_init();

	// STA is not currently a member of a BSS
	configure_bss(NULL);

	// Initialize the join state machine
	wlan_mac_sta_join_init();

	// Initialize callbacks
	wlan_mac_util_set_eth_rx_callback(           (void *) ethernet_receive);
	wlan_mac_high_set_mpdu_tx_done_callback(     (void *) mpdu_transmit_done);
	wlan_mac_high_set_mpdu_dequeue_callback(     (void *) mpdu_dequeue);
	wlan_mac_high_set_mpdu_rx_callback(          (void *) mpdu_rx_process);
	wlan_mac_high_set_uart_rx_callback(          (void *) uart_rx);
	wlan_mac_high_set_poll_tx_queues_callback(   (void *) poll_tx_queues);
	wlan_mac_ltg_sched_set_callback(             (void *) ltg_event);
	wlan_mac_high_set_pb_u_callback(             (void *) up_button);
	wlan_mac_scan_set_tx_probe_request_callback( (void *) send_probe_req);
	wlan_mac_scan_set_state_change_callback(	 (void *) process_scan_state_change);

	// Set the Ethernet ecapsulation mode
	wlan_mac_util_set_eth_encap_mode(ENCAP_MODE_STA);

	// Initialize the association and counts tables
	dl_list_init(&counts_table);

	// Set the maximum associations
	wlan_mac_high_set_max_associations(MAX_NUM_ASSOC);

	// Ask CPU Low for its status
	//     The response to this request will be handled asynchronously
	wlan_mac_high_request_low_state();

	// Wait for CPU Low to initialize
	while( wlan_mac_high_is_cpu_low_initialized() == 0){
		xil_printf("waiting on CPU_LOW to boot\n");
	};

#ifdef USE_WLAN_EXP
    u32                  node_type;
    wlan_mac_hw_info_t * hw_info;

    // NOTE:  To use the WLAN Experiments Framework, it must be initialized after
    //        CPU low has populated the hw_info structure in the MAC High framework.

    // Reset all callbacks
    wlan_exp_reset_all_callbacks();

    // Set WLAN Exp callbacks
    wlan_exp_set_init_callback(                     (void *) wlan_exp_node_sta_init);
    wlan_exp_set_process_node_cmd_callback(         (void *) wlan_exp_process_node_cmd);
    wlan_exp_set_purge_all_data_tx_queue_callback(  (void *) purge_all_data_tx_queue);
    //   - wlan_exp_set_tx_cmd_add_association_callback() should not be used by the STA
    wlan_exp_set_process_user_cmd_callback(         (void *) wlan_exp_process_user_cmd);
    wlan_exp_set_beacon_ts_update_mode_callback(    (void *) sta_set_beacon_ts_update_mode);
    wlan_exp_set_process_config_bss_callback(       (void *) configure_bss);
    //   - wlan_exp_set_beacon_tx_param_update_callback() should not be used by the STA

    // Get the hardware info that has been collected from CPU low
    hw_info = get_mac_hw_info();

    // Set the node type
    node_type = WLAN_EXP_NODE_TYPE + hw_info->wlan_exp_type;

    // Configure the wlan_exp framework
    wlan_exp_init(node_type, WLAN_EXP_ETH);

    // Initialize WLAN Exp
    wlan_exp_node_init(node_type, hw_info->serial_number, hw_info->fpga_dna,
                       WLAN_EXP_ETH, hw_info->hw_addr_wlan_exp, hw_info->hw_addr_wlan);
#endif

	// CPU Low will pass HW information to CPU High as part of the boot process
	//   - Get necessary HW information
	memcpy((void*) &(wlan_mac_addr[0]), (void*) get_mac_hw_addr_wlan(), BSSID_LEN);

    // Set Header information
	tx_header_common.address_2 = &(wlan_mac_addr[0]);

    // Initialize hex display
	sta_update_hex_display(my_aid);

	// Set CPU Low configuration (radio / PHY parameters)
	//     - rx_filter_mode:
	//         - Default is "promiscuous" mode - pass all data and management packets
	//           with good or bad checksums.  This allows logging of all data/management
	//           receptions, even if they're not intended for this node
	//
	cpu_low_config.channel        = WLAN_DEFAULT_CHANNEL;
	cpu_low_config.rx_ant_mode    = WLAN_DEFAULT_RX_ANTENNA;
	cpu_low_config.rx_filter_mode = (RX_FILTER_FCS_ALL | RX_FILTER_HDR_ALL);
	cpu_low_config.tx_ctrl_pow    = WLAN_DEFAULT_TX_PWR;

	// Send configuration to CPU Low
	wlan_mac_high_update_low_config(&cpu_low_config);

	// Initialize interrupts
	wlan_mac_high_interrupt_init();

	// Schedule all events
	//     None at this time

	// Reset the event log
	event_log_reset();

	// Print Station information to the terminal
    xil_printf("------------------------\n");
    xil_printf("WLAN MAC Station boot complete: \n");
    xil_printf("  Serial Number : W3-a-%05d\n", hw_info->serial_number);
    xil_printf("  Default SSID  : %s \n", access_point_ssid);
	xil_printf("  MAC Addr      : %02x:%02x:%02x:%02x:%02x:%02x\n\n", wlan_mac_addr[0], wlan_mac_addr[1], wlan_mac_addr[2], wlan_mac_addr[3], wlan_mac_addr[4], wlan_mac_addr[5]);

#ifdef WLAN_USE_UART_MENU
	xil_printf("\nPress the Esc key in your terminal to access the UART menu\n");
#endif

	// Start the interrupts
	wlan_mac_high_interrupt_restore_state(INTERRUPTS_ENABLED);

	// If there is a default SSID and the DIP switch allows it, initiate a probe request
	if ((strlen(access_point_ssid) > 0) && ((wlan_mac_high_get_user_io_state()&GPIO_MASK_DS_3) == 0)) {
		// Get current join parameters
		join_parameters = wlan_mac_sta_get_join_parameters();

		// Set join parameters
		//     - Zero out BSSID / Channel so the node performs a scan before joining
		join_parameters->channel = 0;
		bzero((void*)join_parameters->bssid, BSSID_LEN);

		wlan_mac_high_free(join_parameters->ssid);
		join_parameters->ssid = strndup(access_point_ssid, SSID_LEN_MAX);

		// Join the default SSID
		wlan_mac_sta_join();
	}

	while(1){
#ifdef USE_WLAN_EXP
		// The wlan_exp Ethernet handling is not interrupt based. Periodic polls of the wlan_exp
		//     transport are required to service new commands. All other node activity (wired/wireless Tx/Rx,
		//     scheduled events, user interaction, etc) are handled via interrupt service routines
		transport_poll(WLAN_EXP_ETH);
#endif
	}

	// Unreachable, but non-void return keeps the compiler happy
	return -1;
}



/*****************************************************************************/
/**
 * @brief Send probe requet
 *
 * This function is part of the scan infrastructure and will be called whenever
 * the node needs to send a probe request.
 *
 * @param   None
 * @return  None
 *
 *****************************************************************************/
void send_probe_req(){
	u16                 			tx_length;
	tx_queue_element*				curr_tx_queue_element;
	tx_queue_buffer* 				curr_tx_queue_buffer;
	volatile scan_parameters_t* 	scan_parameters = wlan_mac_scan_get_parameters();

	// Send probe request
	curr_tx_queue_element = queue_checkout();

	if(curr_tx_queue_element != NULL){
		curr_tx_queue_buffer = (tx_queue_buffer*)(curr_tx_queue_element->data);

		// Setup the TX header
		wlan_mac_high_setup_tx_header( &tx_header_common, (u8 *)bcast_addr, (u8 *)bcast_addr );

		// Fill in the data
		tx_length = wlan_create_probe_req_frame((void*)(curr_tx_queue_buffer->frame),&tx_header_common, scan_parameters->ssid);

		// Setup the TX frame info
		wlan_mac_high_setup_tx_frame_info ( &tx_header_common, curr_tx_queue_element, tx_length, 0, MANAGEMENT_QID );

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


/*****************************************************************************/
/**
 * @brief Handle state change in the network scanner
 *
 * This function is part of the scan infrastructure and will be called whenever
 * the scanner is started, stopped, paused, or resumed. This allows the STA
 * to revert the channel to a known-good state when the scanner has stopped and
 * also serves as notification to the project that it should stop dequeueing data
 * frames since it might be on a different channel than its intended recipient.
 *
 * @param   None
 * @return  None
 *
 *****************************************************************************/
void process_scan_state_change(scan_state_t scan_state){

	// ------------------------------------------------------------------------
	// Note on scanning:
	//
	//   Currently, scanning should only be done with my_bss_info = NULL, ie the
	// node is not currently in a BSS.  This is to avoid any corner cases.  The
	// STA needs to do the following things to make scanning safe when my_bss_info
	// is not NULL:
	//
	//     - Send a NULL data packet indicating that the station is entering a
	//       DOZE state when the scan is started or resumed.
	//     - Send a NULL data packet indicating that the station is entering an
	//       AWAKE state when the scan is paused or stopped.
	//
	// Note: The STA doesn't need full power savings functionality to enact this
	// behavior (e.g. listening for DTIMS at particular intervals, watching the TIM
	// bitmap for our AID, etc).  The decision to doze or wake isn't a function of
	// what the AP is up to, it's a function of whether the STA decides to start
	// scanning.
	// ------------------------------------------------------------------------

	switch(scan_state){
		case SCAN_IDLE:
		case SCAN_PAUSED:
			pause_data_queue = 0;
			if(my_bss_info != NULL){
				wlan_mac_high_set_radio_channel(
						wlan_mac_high_bss_channel_spec_to_radio_chan(my_bss_info->chan_spec));
			}
		break;
		case SCAN_RUNNING:
			pause_data_queue = 1;
		break;
	}
}



/*****************************************************************************/
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
 *****************************************************************************/
void poll_tx_queues(){
	u8 i;

	#define MAX_NUM_QUEUE 2

	// Are we pausing transmissions?
	if (pause_data_queue == 0) {

		static u32 queue_index = 0;

		// Is CPU low ready for a transmission?
		if (wlan_mac_high_is_dequeue_allowed()) {

			// Alternate between checking the management queue and the data queue
			for (i = 0; i < MAX_NUM_QUEUE; i++) {
				queue_index = (queue_index + 1) % MAX_NUM_QUEUE;

				switch(queue_index){
					case 0:  if(dequeue_transmit_checkin(MANAGEMENT_QID)) { return; }  break;
					case 1:  if(dequeue_transmit_checkin(UNICAST_QID))    { return; }  break;
				}
			}
		}
	} else {
		// We are only currently allowed to send management frames. Typically this is caused by an ongoing
		// active scan
		if (wlan_mac_high_is_dequeue_allowed()) {
			dequeue_transmit_checkin(MANAGEMENT_QID);
		}
	}
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
	purge_queue(MCAST_QID);           // Broadcast Queue
	purge_queue(UNICAST_QID);         // Unicast Queue
}




/*****************************************************************************/
/**
 * @brief Callback to handle a packet after it was transmitted by the lower-level MAC
 *
 * This function is called when CPU Low indicates it has completed the Tx process for a packet previously
 * submitted by CPU High.
 *
 * CPU High has two responsibilities post-Tx:
 *  - Cleanup any resources dedicated to the packet
 *  - Update any counts and log info to reflect the Tx result
 *
 * @param tx_frame_info* tx_mpdu
 *  - Pointer to the MPDU which was just transmitted
 * @param wlan_mac_low_tx_details* tx_low_details
 *  - Pointer to the array of data recorded by the lower-level MAC about each re-transmission of the MPDU
 * @param u16 num_tx_low_details
 *  - number of elements in array pointed to by previous argument
 * @return None
 *****************************************************************************/
void mpdu_transmit_done(tx_frame_info* tx_mpdu, wlan_mac_low_tx_details_t* tx_low_details, u16 num_tx_low_details) {
	u32                   	 i;
	station_info_t*          station_info 				   = NULL;


	if(my_bss_info != NULL) station_info = (station_info_t*)(my_bss_info->associated_stations.first->data);

	// Additional variables (Future Use)
	// void*                  mpdu                    = (u8*)tx_mpdu + PHY_TX_PKT_BUF_MPDU_OFFSET;
	// u8*                    mpdu_ptr_u8             = (u8*)mpdu;
	// mac_header_80211*      tx_80211_header         = (mac_header_80211*)((void *)mpdu_ptr_u8);

	// Log all of the TX Low transmissions
	for (i = 0; i < num_tx_low_details; i++) {
		// Log the TX low
		wlan_exp_log_create_tx_low_entry(tx_mpdu, &tx_low_details[i], i);
	}

	// Log the TX MPDU
	wlan_exp_log_create_tx_high_entry(tx_mpdu);

	// Update the counts for the node to which the packet was just transmitted
	if (tx_mpdu->AID != 0) {
		wlan_mac_high_update_tx_counts(tx_mpdu, station_info);
	}

	// Send log entry to wlan_exp controller immediately (not currently supported)
	//
	// if (tx_high_event_log_entry != NULL) {
    //     wn_transmit_log_entry((void *)tx_high_event_log_entry);
	// }
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
 * @param tx_queue_element* curr_tx_queue_element
 *  - A single queue element containing the packet to transmit
 * @param u8* eth_dest
 *  - 6-byte destination address from original Ethernet packet
 * @param u8* eth_src
 *  - 6-byte source address from original Ethernet packet
 * @param u16 tx_length
 *  - Length (in bytes) of the packet payload
 * @return 1 for successful enqueuing of the packet, 0 otherwise
 *****************************************************************************/
int ethernet_receive(tx_queue_element* curr_tx_queue_element, u8* eth_dest, u8* eth_src, u16 tx_length){
	tx_queue_buffer* 	curr_tx_queue_buffer;
	station_info_t* 	ap_station_info;


	// Check associations
	//     Is there an AP to send the packet to?
	if(my_bss_info != NULL){
		ap_station_info = (station_info_t*)((my_bss_info->associated_stations.first)->data);

		// Send the packet to the AP
		if(queue_num_queued(UNICAST_QID) < max_queue_size){

			// Send the pre-encapsulated Ethernet frame over the wireless interface
			//     NOTE:  The queue element has already been provided, so we do not need to check if it is NULL
			curr_tx_queue_buffer = (tx_queue_buffer*)(curr_tx_queue_element->data);

			// Setup the TX header
			wlan_mac_high_setup_tx_header( &tx_header_common, ap_station_info->addr,(u8*)(&(eth_dest[0])));

			// Fill in the data
			wlan_create_data_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, MAC_FRAME_CTRL2_FLAG_TO_DS);

			// Setup the TX frame info
			wlan_mac_high_setup_tx_frame_info ( &tx_header_common, curr_tx_queue_element, tx_length, (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO), UNICAST_QID );

			// Set the information in the TX queue buffer
			curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_STATION_INFO;
			curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)ap_station_info;
			curr_tx_queue_buffer->frame_info.AID         = 0;

			// Put the packet in the queue
			enqueue_after_tail(UNICAST_QID, curr_tx_queue_element);



		} else {
			// Packet was not successfully enqueued
			return 0;
		}

		// Packet was successfully enqueued
		return 1;
	} else {

		// STA is not currently associated, so we won't send any eth frames
		return 0;
	}
}



/*****************************************************************************/
/**
 * @brief Process received MPDUs
 *
 * This callback function will process all the received MPDUs.
 *
 * This function must implement the state machine that will allow a station to join the AP.
 *
 * @param  void * pkt_buf_addr
 *     - Packet buffer address;  Contains the contents of the MPDU as well as other packet information from CPU low
 * @return None
 *****************************************************************************/
void mpdu_rx_process(void* pkt_buf_addr) {

	rx_frame_info*      frame_info                = (rx_frame_info*)pkt_buf_addr;
	void*               mpdu                     = (u8*)pkt_buf_addr + PHY_RX_PKT_BUF_MPDU_OFFSET;
	u8*                 mpdu_ptr_u8              = (u8*)mpdu;
	mac_header_80211*   rx_80211_header          = (mac_header_80211*)((void *)mpdu_ptr_u8);

	u16                 rx_seq;
	rx_common_entry*    rx_event_log_entry       = NULL;

	dl_entry*	        associated_station_entry;
	station_info_t*     ap_station_info          = NULL;
	counts_txrx*        station_counts           = NULL;

	u8                  unicast_to_me;
	u8                  to_multicast;
	u8                  is_associated            = 0;
	dl_entry*			bss_info_entry;
	bss_info*			curr_bss_info;
	u8					pre_llc_offset			 = 0;

	u8 					mcs	     = frame_info->phy_details.mcs;
	u16 				length   = frame_info->phy_details.length;


	// Log the reception
	rx_event_log_entry = wlan_exp_log_create_rx_entry(frame_info);

	// If this function was passed a CTRL frame (e.g., CTS, ACK), then we should just quit.
	// The only reason this occured was so that it could be logged in the line above.
	if((rx_80211_header->frame_control_1 & 0xF) == MAC_FRAME_CTRL1_TYPE_CTRL){
		goto mpdu_rx_process_end;
	}

	// Determine destination of packet
	unicast_to_me = wlan_addr_eq(rx_80211_header->address_1, wlan_mac_addr);
	to_multicast  = wlan_addr_mcast(rx_80211_header->address_1);

    // If the packet is good (ie good FCS) and it is destined for me, then process it
	if( (frame_info->state == RX_MPDU_STATE_FCS_GOOD)){

		// Update the association information
		if(my_bss_info != NULL){
			associated_station_entry = wlan_mac_high_find_station_info_ADDR(&(my_bss_info->associated_stations), (rx_80211_header->address_2));
		} else {
			associated_station_entry = NULL;
		}

		if(associated_station_entry != NULL) {
			ap_station_info = (station_info_t*)(associated_station_entry->data);

			// Update station information
			ap_station_info->latest_activity_timestamp = get_system_time_usec();
			ap_station_info->rx.last_power             = frame_info->rx_power;
			ap_station_info->rx.last_mcs               = mcs;

			is_associated  = 1;
			rx_seq         = ((rx_80211_header->sequence_control)>>4)&0xFFF;

			station_counts = ap_station_info->counts;

			// Check if this was a duplicate reception
			//   - Received seq num matched previously received seq num for this STA
			if( ((rx_80211_header->frame_control_2) & MAC_FRAME_CTRL2_FLAG_RETRY) && (ap_station_info->rx.last_seq == rx_seq) ) {
				if(rx_event_log_entry != NULL){
					rx_event_log_entry->flags |= RX_ENTRY_FLAGS_IS_DUPLICATE;
				}

				// Finish the function
				goto mpdu_rx_process_end;
			} else {
				ap_station_info->rx.last_seq = rx_seq;
			}
		} else {
			station_counts = wlan_mac_high_add_counts(&counts_table, NULL, rx_80211_header->address_2);
		}

        // Update receive counts
		if(station_counts != NULL){
			station_counts->latest_txrx_timestamp = get_system_time_usec();
			if((rx_80211_header->frame_control_1 & 0xF) == MAC_FRAME_CTRL1_TYPE_DATA){
				((station_counts)->data.rx_num_packets)++;
				((station_counts)->data.rx_num_bytes) += (length - WLAN_PHY_FCS_NBYTES - sizeof(mac_header_80211));
			} else if((rx_80211_header->frame_control_1 & 0xF) == MAC_FRAME_CTRL1_TYPE_MGMT) {
				((station_counts)->mgmt.rx_num_packets)++;
				((station_counts)->mgmt.rx_num_bytes) += (length - WLAN_PHY_FCS_NBYTES - sizeof(mac_header_80211));
			}
		}
		if(unicast_to_me || to_multicast){
			// Process the packet
			switch(rx_80211_header->frame_control_1) {
				//---------------------------------------------------------------------
				case (MAC_FRAME_CTRL1_SUBTYPE_BEACON):
				case (MAC_FRAME_CTRL1_SUBTYPE_PROBE_RESP):
					//TODO: Log a MAC time change
				break;

				//---------------------------------------------------------------------
				case MAC_FRAME_CTRL1_SUBTYPE_QOSDATA:
					pre_llc_offset = sizeof(qos_control);
				case (MAC_FRAME_CTRL1_SUBTYPE_DATA):
					// Data packet
					//   - If the STA is associated with the AP and this is from the DS, then transmit over the wired network
					//
					if(is_associated){
						if((rx_80211_header->frame_control_2) & MAC_FRAME_CTRL2_FLAG_FROM_DS) {
							// MPDU is flagged as destined to the DS - send it for de-encapsulation and Ethernet Tx (if appropriate)
							wlan_mpdu_eth_send(mpdu,length, pre_llc_offset);
						}
					}
				break;


				//---------------------------------------------------------------------
				case (MAC_FRAME_CTRL1_SUBTYPE_ASSOC_RESP):
					// Association response
					//   - If we are in the correct association state and the response was success, then associate with the AP
					//
					mpdu_ptr_u8 += sizeof(mac_header_80211);

					if(wlan_addr_eq(rx_80211_header->address_1, wlan_mac_addr) && ((association_response_frame*)mpdu_ptr_u8)->status_code == STATUS_SUCCESS){
						// AP is authenticating us. Update BSS_info.
						bss_info_entry = wlan_mac_high_find_bss_info_BSSID(rx_80211_header->address_3);

						if(bss_info_entry != NULL){
							curr_bss_info = (bss_info*)(bss_info_entry->data);
							if(curr_bss_info->state == BSS_STATE_AUTHENTICATED){
								curr_bss_info->state = BSS_STATE_ASSOCIATED;
								curr_bss_info->last_join_attempt_result = SUCCESSFUL;
								wlan_mac_sta_join_bss_attempt_poll((((association_response_frame*)mpdu_ptr_u8)->association_id)&~0xC000);
							}
						}
					} else {
						// AP is rejecting association request
						//     - Check that the response was from a known BSS
						bss_info_entry = wlan_mac_high_find_bss_info_BSSID(rx_80211_header->address_3);

						if (bss_info_entry != NULL) {
							// Set the last_join_attempt_result to "DENIED"
							//     - In wlan_mac_sta_join_bss_attempt_poll(), this will stop the current join
							//       and take appropriate action.
							curr_bss_info = (bss_info*)(bss_info_entry->data);
							curr_bss_info->last_join_attempt_result = DENIED;

							xil_printf("Join process association failed for BSS %s\n", ((bss_info*)(bss_info_entry->data))->ssid);
						}

						xil_printf("Association failed, reason code %d\n", ((association_response_frame*)mpdu_ptr_u8)->status_code);
					}
				break;


				//---------------------------------------------------------------------
				case (MAC_FRAME_CTRL1_SUBTYPE_AUTH):
					// Authentication Response

					if( wlan_addr_eq(rx_80211_header->address_1, wlan_mac_addr)) {

						// Move the packet pointer to after the header
						mpdu_ptr_u8 += sizeof(mac_header_80211);

						// Check the authentication algorithm
						switch(((authentication_frame*)mpdu_ptr_u8)->auth_algorithm){

							case AUTH_ALGO_OPEN_SYSTEM:
								// Check that this was a successful authentication response
								if(((authentication_frame*)mpdu_ptr_u8)->auth_sequence == AUTH_SEQ_RESP){

									if(((authentication_frame*)mpdu_ptr_u8)->status_code == STATUS_SUCCESS){
										// AP is authenticating us. Update BSS_info.
										bss_info_entry = wlan_mac_high_find_bss_info_BSSID(rx_80211_header->address_3);

										if(bss_info_entry != NULL){
											curr_bss_info = (bss_info*)(bss_info_entry->data);
											if(curr_bss_info->state == BSS_STATE_UNAUTHENTICATED){
												curr_bss_info->state = BSS_STATE_AUTHENTICATED;
												curr_bss_info->last_join_attempt_result = SUCCESSFUL;
												wlan_mac_sta_join_bss_attempt_poll(0);
											}
										}
									}

									// Finish the function
									goto mpdu_rx_process_end;
								}
							break;

							default:
								// STA cannot support authentication request
								//     - Check that the response was from a known BSS
								bss_info_entry = wlan_mac_high_find_bss_info_BSSID(rx_80211_header->address_3);

								if (bss_info_entry != NULL) {
									// Set the last_join_attempt_result to "DENIED"
									//     - In wlan_mac_sta_join_bss_attempt_poll(), this will stop the current join
									//       and take appropriate action.
									curr_bss_info = (bss_info*)(bss_info_entry->data);
									curr_bss_info->last_join_attempt_result = DENIED;

									xil_printf("Join process authentication failed for BSS %s\n", ((bss_info*)(bss_info_entry->data))->ssid);
								}

								xil_printf("Authentication failed.  AP uses authentication algorithm %d which is not support by the 802.11 reference design.\n", ((authentication_frame*)mpdu_ptr_u8)->auth_algorithm);
							break;
						}
					}
				break;


				//---------------------------------------------------------------------
				case (MAC_FRAME_CTRL1_SUBTYPE_DEAUTH):
					// De-authentication
					//   - If we are being de-authenticated, then log and update the association state
					//   - Start and active scan to find the AP if an SSID is defined
					//
					if(my_bss_info != NULL){
						if(wlan_addr_eq(rx_80211_header->address_1, wlan_mac_addr) && (wlan_mac_high_find_station_info_ADDR(&(my_bss_info->associated_stations), rx_80211_header->address_2) != NULL)){

							//
							// TODO:  (Optional) Log association state change
							//

							// Stop any on-going join
							if (wlan_mac_is_joining()) { wlan_mac_sta_join_return_to_idle(); }

							// Purge all packets for the AP
							purge_queue(UNICAST_QID);

							// Update the hex display to show that we are no longer associated
							sta_update_hex_display(0);

							// Set BSS state
							my_bss_info->state = BSS_STATE_UNAUTHENTICATED;

							// Remove the association
							curr_bss_info = my_bss_info;
							configure_bss(NULL);

							//
							// Note:  This is the place in the code where it would be easy to add new
							//     "just de-authenticated" behaviors, such as an auto-re-join protocol.
							//     The comments below show a simple "re-join the same AP" protocol using
							//     the curr_bss_info variable assigned above.
							//
							// Try to re-join the AP
							// volatile join_parameters_t*  join_parameters = wlan_mac_sta_get_join_parameters();
							// bzero((void*)join_parameters->bssid, 6);
							// wlan_mac_high_free(join_parameters->ssid);
							// join_parameters->ssid = strndup(curr_bss_info->ssid, SSID_LEN_MAX);
							// wlan_mac_sta_join();
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
#ifdef USE_WLAN_EXP
	if ((rx_event_log_entry != NULL) && (((rx_event_log_entry->mcs) != 0) && ((rx_event_log_entry->phy_mode) != 0))) {
        wlan_exp_transmit_log_entry((void *)rx_event_log_entry);
	}
#endif

    return;
}



/*****************************************************************************/
/**
 * Callback to process a packet that is being dequeued
 *
 * @param  packet            - Pointer to the TX queue element that is being dequeued
 *
 *****************************************************************************/
void mpdu_dequeue(tx_queue_element* packet){
	mac_header_80211* 	header;
	tx_frame_info*		frame_info;
	u32 				packet_payload_size;

	header 	  			= (mac_header_80211*)((((tx_queue_buffer*)(packet->data))->frame));
	frame_info 			= (tx_frame_info*)&((((tx_queue_buffer*)(packet->data))->frame_info));
	packet_payload_size	= frame_info->length;

	switch(wlan_mac_high_pkt_type(header, packet_payload_size)){
		case PKT_TYPE_DATA_ENCAP_ETH:
			// Overwrite addr1 of this packet with the currently associated AP. This will allow previously
			// enqueued packets to seamlessly hand off if this STA joins a new AP
			if(my_bss_info != NULL){
				memcpy(header->address_1, my_bss_info->bssid, 6);
			} else {
				xil_printf("Dequeue error: no associated AP\n");
			}
		break;
	}
}



/*****************************************************************************/
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
 *****************************************************************************/
void ltg_event(u32 id, void* callback_arg){

	u32                 payload_length;
	u32                 min_ltg_payload_length;
	u8*                 addr_da;
	station_info_t*     ap_station_info;
	tx_queue_element* curr_tx_queue_element        = NULL;
	tx_queue_buffer*  curr_tx_queue_buffer         = NULL;

	if(my_bss_info != NULL){
		switch(((ltg_pyld_hdr*)callback_arg)->type){
			case LTG_PYLD_TYPE_FIXED:
				addr_da = ((ltg_pyld_fixed*)callback_arg)->addr_da;
				payload_length = ((ltg_pyld_fixed*)callback_arg)->length;
			break;
			case LTG_PYLD_TYPE_UNIFORM_RAND:
				addr_da = ((ltg_pyld_uniform_rand*)callback_arg)->addr_da;
				payload_length = (rand()%(((ltg_pyld_uniform_rand*)(callback_arg))->max_length - ((ltg_pyld_uniform_rand*)(callback_arg))->min_length))+((ltg_pyld_uniform_rand*)(callback_arg))->min_length;
			break;
			default:
				xil_printf("ERROR ltg_event: Unknown LTG Payload Type! (%d)\n", ((ltg_pyld_hdr*)callback_arg)->type);
				return;
			break;
		}

		ap_station_info = (station_info_t*)((my_bss_info->associated_stations.first)->data);

		// Send a Data packet to AP
		if(queue_num_queued(UNICAST_QID) < max_queue_size){
			// Checkout 1 element from the queue;
			curr_tx_queue_element = queue_checkout();
			if(curr_tx_queue_element != NULL){
				// Create LTG packet
				curr_tx_queue_buffer = ((tx_queue_buffer*)(curr_tx_queue_element->data));

				// Setup the MAC header
				wlan_mac_high_setup_tx_header( &tx_header_common, ap_station_info->addr, addr_da );

				min_ltg_payload_length = wlan_create_ltg_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, MAC_FRAME_CTRL2_FLAG_TO_DS, id);
				payload_length = max(payload_length+sizeof(mac_header_80211)+WLAN_PHY_FCS_NBYTES, min_ltg_payload_length);

				// Finally prepare the 802.11 header
				wlan_mac_high_setup_tx_frame_info ( &tx_header_common, curr_tx_queue_element, payload_length, (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO | TX_MPDU_FLAGS_FILL_UNIQ_SEQ), UNICAST_QID);

				// Update the queue entry metadata to reflect the new new queue entry contents
				curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_STATION_INFO;
				curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)ap_station_info;
				curr_tx_queue_buffer->frame_info.AID         = 0;

				// Submit the new packet to the appropriate queue
				enqueue_after_tail(UNICAST_QID, curr_tx_queue_element);
			}
		}
	}
}



/*****************************************************************************/
/**
 * @brief Reset Station Counts
 *
 * Reset all counts being kept for all stations
 *
 * @param  None
 * @return None
 *****************************************************************************/
void reset_station_counts(){
	wlan_mac_high_reset_counts(&counts_table);
}



/*****************************************************************************/
/**
 * @brief Disassociate the STA from the associated AP
 *
 * This function will disassociate the STA from the AP if it is associated.  Otherwise,
 * it will do nothing.  It also logs any association state change
 *
 * @param  None
 * @return int
 *  -  0 if successful
 *  - -1 if unsuccessful
 *
 *  @note This function uses global variables:  association_state, association_table
 *      and counts_table
 *****************************************************************************/
int  sta_disassociate( void ) {
	int                 status = 0;
	station_info_t*     ap_station_info          = NULL;
	dl_entry*           ap_station_info_entry;
	tx_queue_element*   curr_tx_queue_element;
	tx_queue_buffer*    curr_tx_queue_buffer;
	u32                 tx_length;

	// If the STA is currently associated, remove the association; otherwise do nothing
	if(my_bss_info != NULL){

		// Get the AP station info
		ap_station_info_entry = my_bss_info->associated_stations.first;
		ap_station_info       = (station_info_t*)ap_station_info_entry->data;

		//
		// TODO:  (Optional) Log association state change
		//

		// ---------------------------------------------------------------
		// Send de-authentication message to tell AP that the STA is leaving

		// Jump to BSS channel before sending.  No need to change back.
		cpu_low_config.channel = wlan_mac_high_bss_channel_spec_to_radio_chan(my_bss_info->chan_spec);
		wlan_mac_high_set_radio_channel(cpu_low_config.channel);

		// Send disassociation packet
		curr_tx_queue_element = queue_checkout();

		if(curr_tx_queue_element != NULL){
			curr_tx_queue_buffer = (tx_queue_buffer*)(curr_tx_queue_element->data);

			// Setup the TX header
			wlan_mac_high_setup_tx_header(&tx_header_common, ap_station_info->addr, wlan_mac_addr );

			// Fill in the data
			tx_length = wlan_create_disassoc_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, DISASSOC_REASON_STA_IS_LEAVING);

			// Setup the TX frame info
			wlan_mac_high_setup_tx_frame_info ( &tx_header_common,
												curr_tx_queue_element,
												tx_length,
												(TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO ),
												MANAGEMENT_QID );

			// Set the information in the TX queue buffer
			curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
			curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)(&default_unicast_mgmt_tx_params);
			curr_tx_queue_buffer->frame_info.AID         = 0;

			// Put the packet in the queue
			enqueue_after_tail(MANAGEMENT_QID, curr_tx_queue_element);

			// Purge any data for the AP
			purge_queue(UNICAST_QID);
		}

		// Set BSS to NULL
		configure_bss(NULL);
	}

	return status;
}



/*****************************************************************************/
/**
 *
 *****************************************************************************/
u32	configure_bss(bss_config_t* bss_config){
	u32                 return_status               = 0;
	u8                  send_channel_switch_to_low  = 0;
	u8                  send_beacon_config_to_low   = 0;

	bss_info*           local_bss_info;
	interrupt_state_t   curr_interrupt_state;
	station_info_t*     curr_station_info;
	dl_entry*           curr_station_info_entry;
	station_info_t*     ap_station_info		        = NULL;

	//---------------------------------------------------------
	// 1. Check for any invalid inputs or combination of inputs
	//      First verify the requested update to the BSS configuration before
	//      modifying anything. This will prevent a partial update of BSS
	//      configuration with valid parameters before discovering an invalid
	//      parameter.

	if (bss_config != NULL) {
		if (bss_config->update_mask & BSS_FIELD_MASK_BSSID) {
			if (wlan_addr_eq(bss_config->bssid, zero_addr) == 0) {
				if ((my_bss_info != NULL) && wlan_addr_eq(bss_config->bssid, my_bss_info->bssid)) {
					// The caller of this function claimed that it was updating the BSSID,
					// but the new BSSID matches the one already specified in my_bss_info.
					// Complete the rest of this function as if that bit in the update mask
					// were not set
					bss_config->update_mask &= ~BSS_FIELD_MASK_BSSID;
				} else {
					// Changing the BSSID, perform necessary argument checks
					if ((bss_config->bssid[0] & MAC_ADDR_MSB_MASK_LOCAL) == 1) {
						// In the STA implementation, the BSSID provided must not
						// be locally generated.
						return_status |= BSS_CONFIG_FAILURE_BSSID_INVALID;
					}
					if (((bss_config->update_mask & BSS_FIELD_MASK_SSID) == 0) ||
						((bss_config->update_mask & BSS_FIELD_MASK_CHAN) == 0)) {
						return_status |= BSS_CONFIG_FAILURE_BSSID_INSUFFICIENT_ARGUMENTS;
					}
				}
			}
		}
		if (bss_config->update_mask & BSS_FIELD_MASK_CHAN) {
			if (wlan_verify_channel(
					wlan_mac_high_bss_channel_spec_to_radio_chan(my_bss_info->chan_spec)) != XST_SUCCESS) {
				return_status |= BSS_CONFIG_FAILURE_CHANNEL_INVALID;
			}
		}
		if (bss_config->update_mask & BSS_FIELD_MASK_BEACON_INTERVAL) {
			// There is no error condition for setting the beacon interval
			// at the STA since a STA is incapable of sending beacons
		}
		if (bss_config->update_mask & BSS_FIELD_MASK_HT_CAPABLE) {
			if (bss_config->ht_capable > 1) {
				return_status |= BSS_CONFIG_FAILURE_HT_CAPABLE_INVALID;
			}
		}
	}

	if (return_status == 0) {
		//---------------------------------------------------------
		// 2. Apply BSS configuration changes
		//      Now that the provided bss_config_t struct is valid, apply the changes.

		// Disable interrupts around these modifications to prevent state
		// changing out from underneath this context while the new BSS
		// configuration parameters are only partially updated.
		curr_interrupt_state = wlan_mac_high_interrupt_stop();

		if ((bss_config == NULL) || (bss_config->update_mask & BSS_FIELD_MASK_BSSID)) {
			// Adopting a new BSSID. This could mean either
			//    1) Shutting the BSS down
			// or 2) Shutting the BSS down and then starting a new BSS.
			//
			// In either case, first remove any station_info structs
			// that are members of the current my_bss_info and return to
			// a NULL my_bss_info state.
			//
			// This will not result in any OTA transmissions to the stations.

			if (my_bss_info != NULL) {
				curr_station_info_entry = my_bss_info->associated_stations.first;
				curr_station_info = (station_info_t*)(curr_station_info_entry->data);

				// Purge any data for the AP
				purge_queue(UNICAST_QID);

				// Remove the association
				wlan_mac_high_remove_station_info( &my_bss_info->associated_stations, &counts_table, curr_station_info->addr );

				// Update the hex display to show STA is not currently associated
				sta_update_hex_display(0);

				// Inform the MAC High Framework to no longer will keep this BSS Info. This will
				// allow it to be overwritten in the future to make space for new BSS Infos.
				my_bss_info->flags &= ~BSS_FLAGS_KEEP;
				my_bss_info->state  = BSS_STATE_UNAUTHENTICATED;

				// Set "my_bss_info" to NULL
				//     - All functions must be able to handle my_bss_info = NULL
				my_bss_info = NULL;

				// Disable beacon processing immediately
				bzero(gl_beacon_txrx_config.bssid_match, BSSID_LEN);
				wlan_mac_high_config_txrx_beacon(&gl_beacon_txrx_config);
			}

			// Pause the data queue, if un-paused
			//     Since interrupts are disabled, this does not need to be done before the purge_queue()
			if (pause_data_queue == 0) {
				pause_data_queue = 1;
			}

			// (bss_config == NULL) is one way to remove the BSS state of the node. This operation
			// was executed just above.  Rather that continuing to check non-NULLness of bss_config
			// throughout the rest of this function, just re-enable interrupts and return early.

			if (bss_config == NULL) {
				wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
				return return_status;
			}

			// my_bss_info is guaranteed to be NULL at this point in the code
			// bss_config is guaranteed to be non-NULL at this point in the code

			// Update BSS
			//     - BSSID must not be zero_addr (reserved address)
			if (wlan_addr_eq(bss_config->bssid, zero_addr) == 0) {
				// Stop the join state machine if it is running
				if (wlan_mac_is_joining()) {
					wlan_mac_sta_join_return_to_idle();
				}

				// Stop the scan state machine if it is running
				if (wlan_mac_scan_is_scanning()) {
					wlan_mac_scan_stop();
				}

				// Create a new bss_info or overwrite an existing one with matching BSSID.
				//     Note:  The wildcard SSID and 0-valued channel arguments are temporary. Because
				//         of the error checking at the top of this function, the bss_config will
				//         contain a valid SSID as well as channel. These fields will be updated
				//         in step 3).
				local_bss_info = wlan_mac_high_create_bss_info(bss_config->bssid, "", 0);

				if (local_bss_info != NULL) {
					local_bss_info->flags |= BSS_FLAGS_KEEP;
					local_bss_info->capabilities = (CAPABILITIES_SHORT_TIMESLOT | CAPABILITIES_ESS);
					my_bss_info = local_bss_info;

					// Add AP to association table
					ap_station_info = wlan_mac_high_add_station_info(&(my_bss_info->associated_stations), &counts_table, my_bss_info->bssid, 0);

					if(ap_station_info != NULL) {

						//Start off with a copy of the default unicast data Tx parameters.
						ap_station_info->tx = default_unicast_data_tx_params;

						if(my_bss_info->flags & BSS_FLAGS_HT_CAPABLE){
							ap_station_info->flags |= STATION_INFO_FLAG_HT_CAPABLE;
						} else {
							ap_station_info->flags &= ~STATION_INFO_FLAG_HT_CAPABLE;
						}

						if((ap_station_info->flags & STATION_INFO_FLAG_HT_CAPABLE) == 0){
							//If this station is not capable of HT phy_mode, then we'll adjust its tx_params.
							//Note: If the default tx_params does not support HT, then we will not explicitly
							//set the phy_mode to HT just because the STA is capable of it
							ap_station_info->tx.phy.phy_mode = PHY_MODE_NONHT;
						}

						//
						// TODO:  (Optional) Log association state change
						//
					}
				}
			}
		}

		//---------------------------------------------------------
		// 3. Clean up
		//      Now that my_bss_info has been updated, CPU_HIGH can communicate
		//      the changes to CPU_LOW so that the node is tuned to the correct channel,
		//      send beacons at the correct interval, and update the beacon
		//      template packet buffer.
		if (my_bss_info != NULL) {

			if (bss_config->update_mask & BSS_FIELD_MASK_CHAN) {
				my_bss_info->chan_spec = bss_config->chan_spec;

				// Update local CPU_LOW parameters
				cpu_low_config.channel = wlan_mac_high_bss_channel_spec_to_radio_chan(my_bss_info->chan_spec);

				send_channel_switch_to_low = 1;
			}
			if (bss_config->update_mask & BSS_FIELD_MASK_SSID) {
				strncpy(my_bss_info->ssid, bss_config->ssid, SSID_LEN_MAX);
			}
			if (bss_config->update_mask & BSS_FIELD_MASK_BEACON_INTERVAL) {
				my_bss_info->beacon_interval = bss_config->beacon_interval;
				send_beacon_config_to_low = 1;
			}
			if (bss_config->update_mask & BSS_FIELD_MASK_HT_CAPABLE) {
				//TODO:
				//     1) Update Beacon Template capabilities
				//     2) Update existing MCS selections for defaults and
				//        associated stations?
				if (bss_config->ht_capable) {
					my_bss_info->flags |= BSS_FLAGS_HT_CAPABLE;
				} else {
					my_bss_info->flags &= ~BSS_FLAGS_HT_CAPABLE;
				}
			}

			// Update the channel
			if (send_channel_switch_to_low) {
				wlan_mac_high_set_radio_channel(
						wlan_mac_high_bss_channel_spec_to_radio_chan(my_bss_info->chan_spec));
			}

			// Update Beacon configuration
			if (send_beacon_config_to_low) {
				memcpy(gl_beacon_txrx_config.bssid_match, my_bss_info->bssid, BSSID_LEN);

				gl_beacon_txrx_config.beacon_interval_tu = my_bss_info->beacon_interval;        // CPU_LOW does not need this parameter for the STA project
				gl_beacon_txrx_config.beacon_template_pkt_buf = TX_PKT_BUF_BEACON;              // CPU_LOW does not need this parameter for the STA project

				wlan_mac_high_config_txrx_beacon(&gl_beacon_txrx_config);
			}

			// Unpause the queue, if paused
			if (pause_data_queue) {
				pause_data_queue = 0;
			}

			// Update the hex diplay with the current AID
			sta_update_hex_display(my_aid);

			// Print new BSS information
			xil_printf("BSS Details: \n");
			xil_printf("  BSSID           : %02x-%02x-%02x-%02x-%02x-%02x\n",my_bss_info->bssid[0],my_bss_info->bssid[1],my_bss_info->bssid[2],my_bss_info->bssid[3],my_bss_info->bssid[4],my_bss_info->bssid[5]);
			xil_printf("   SSID           : %s\n", my_bss_info->ssid);
			xil_printf("   Channel        : %d\n", wlan_mac_high_bss_channel_spec_to_radio_chan(my_bss_info->chan_spec));
			xil_printf("   Beacon Interval: %d TU (%d us)\n",my_bss_info->beacon_interval, my_bss_info->beacon_interval*1024);
		}

		// Restore interrupts after all BSS changes
		wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
	}

	return return_status;
}



/*****************************************************************************/
/**
 * @brief Callback to handle beacon MAC time update mode
 *
 * @param  u32 enable        - Enable / Disable MAC time update from beacons
 * @return None
 *
 *****************************************************************************/
void sta_set_beacon_ts_update_mode(u32 enable){
    if (enable) {
        gl_beacon_txrx_config.ts_update_mode = ALWAYS_UPDATE;
    } else {
        gl_beacon_txrx_config.ts_update_mode = NEVER_UPDATE;
    }

    // Push beacon configuration to CPU_LOW
    wlan_mac_high_config_txrx_beacon(&gl_beacon_txrx_config);
}



/*****************************************************************************/
/**
 * @brief Callback to handle push of up button
 *
 * Reference implementation does nothing.
 *
 * @param None
 * @return None
 *****************************************************************************/
void up_button(){
	return;
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
dl_list * get_station_info_list(){
	if(my_bss_info != NULL){
		return &(my_bss_info->associated_stations);
	} else {
		return NULL;
	}
}
dl_list * get_counts()           { return &counts_table;        }
u8      * get_wlan_mac_addr()    { return (u8 *)&wlan_mac_addr; }



/*****************************************************************************/
/**
 * @brief STA specific hex display update command
 *
 * This function update the hex display for the STA.  In general, this function
 * is a wrapper for standard hex display commands found in wlan_mac_misc_util.c.
 * However, this wrapper was implemented so that it would be easy to do other
 * actions when the STA needed to update the hex display.
 *
 * @param   val              - Value to be displayed (between 0 and 99)
 * @return  None
 *****************************************************************************/
void sta_update_hex_display(u8 val) {

    // Use standard hex display write
    write_hex_display(val);
}



#ifdef USE_WLAN_EXP

// ****************************************************************************
// Define MAC Specific User Commands
//
// NOTE:  All User Command IDs (CMDID_*) must be a 24 bit unique number
//

//-----------------------------------------------
// MAC Specific User Commands
//
// #define CMDID_USER_<COMMAND_NAME>                       0x100000


//-----------------------------------------------
// MAC Specific User Command Parameters
//
// #define CMD_PARAM_USER_<PARAMETER_NAME>                 0x00000000



/*****************************************************************************/
/**
 * Process User Commands
 *
 * This function is part of the WLAN Exp framework and will process the framework-
 * level user commands.  This function intentionally does not implement any user
 * commands and it is left to the user to implement any needed functionality.   By
 * default, any commands not processed in this function will print an error to the
 * UART.
 *
 * @param   socket_index     - Index of the socket on which to send message
 * @param   from             - Pointer to socket address structure (struct sockaddr *) where command is from
 * @param   command          - Pointer to Command
 * @param   response         - Pointer to Response
 * @param   max_resp_len     - Maximum number of u32 words allowed in response
 *
 * @return  int              - Status of the command:
 *                                 NO_RESP_SENT - No response has been sent
 *                                 RESP_SENT    - A response has been sent
 *
 * @note    See on-line documentation for more information:
 *          https://warpproject.org/trac/wiki/802.11/wlan_exp/Extending
 *
 *****************************************************************************/
int wlan_exp_process_user_cmd(u32 cmd_id, int socket_index, void * from, cmd_resp * command, cmd_resp * response, u32 max_resp_len) {

    //
    // IMPORTANT ENDIAN NOTES:
    //     - command
    //         - header - Already endian swapped by the framework (safe to access directly)
    //         - args   - Must be endian swapped as necessary by code (framework does not know the contents of the command)
    //     - response
    //         - header - Will be endian swapped by the framework (safe to write directly)
    //         - args   - Must be endian swapped as necessary by code (framework does not know the contents of the response)
    //

    // Standard variables
    //
    // Used for accessing command arguments and constructing the command response header/payload
    //
    // NOTE:  Some of the standard variables below have been commented out.  This was to remove
    //     compiler warnings for "unused variables" since the default implementation is empty.  As
    //     you add commands, you should un-comment the standard variables.
    //
    u32                 resp_sent      = NO_RESP_SENT;

#if 0
    cmd_resp_hdr      * cmd_hdr        = command->header;
    cmd_resp_hdr      * resp_hdr       = response->header;

    u32               * cmd_args_32    = command->args;
    u32               * resp_args_32   = response->args;

    u32                 resp_index     = 0;
#endif

    switch(cmd_id){

//-----------------------------------------------------------------------------
// MAC Specific User Commands
//-----------------------------------------------------------------------------

        // Template framework for a Command
        //
        // NOTE:  The WLAN Exp framework assumes that the Over-the-Wire format of the data is
        //     big endian.  However, the node processes data using little endian.  Therefore,
        //     any data received from the host must be properly endian-swapped and similarly,
        //     any data sent to the host must be properly endian-swapped.  The built-in Xilinx
        //     functions:  Xil_Ntohl() (Network to Host) and Xil_Htonl() (Host to Network) are
        //     used for this.
        //
#if 0
        //---------------------------------------------------------------------
        case CMDID_USER_<COMMAND_NAME>: {
            // Command Description
            //
            // Message format:
            //     cmd_args_32[0:N]    Document command arguments from the host
            //
            // Response format:
            //     resp_args_32[0:M]   Document response arguments from the node
            //
            // NOTE:  Variables are declared above.
            // NOTE:  Please take care of the endianness of the arguments (see comment above)
            //

            // Variables for template command
            int                 status;
            u32                 arg_0;
            interrupt_state_t   curr_interrupt_state;

            // Initialize variables
            status      = CMD_PARAM_SUCCESS;
            arg_0       = Xil_Ntohl(cmd_args_32[0]);              // Swap endianness of command argument

            // Do something with argument(s)
            xil_printf("Command argument 0: 0x%08x\n", arg_0);

            // If necessary, disable interrupts before processing the command
            //  Interrupts must be disabled if the command implementation relies on any state that might
            //  change during an interrupt service routine. See the user guide for more details
            //  https://warpproject.org/trac/wiki/802.11/wlan_exp/Extending
            curr_interrupt_state = wlan_mac_high_interrupt_stop();

            // Process command arguments and generate any response payload
            //     NOTE:  If interrupts were disabled above, take care to avoid any long-running code in this
            //         block (i.e. avoid xil_printf()). When interrupts are disabled, CPU High is unable to
            //         respond to CPU Low (ie CPU High will not send / receive packets) and execute scheduled
            //         tasks, such as LTGs.

            // Re-enable interrupts before returning (only do this if wlan_mac_high_interrupt_stop() is called above)
            wlan_mac_high_interrupt_restore_state(curr_interrupt_state);

            // Send response
            //   NOTE:  It is good practice to send a status as the first argument of the response.
            //       This way it is easy to determine if the other data in the response is valid.
            //       Predefined status values are:  CMD_PARAM_SUCCESS, CMD_PARAM_ERROR
            //
            resp_args_32[resp_index++] = Xil_Htonl(status);       // Swap endianness of response arguments

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;
#endif


        //---------------------------------------------------------------------
        default: {
            wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown STA user command: 0x%x\n", cmd_id);
        }
        break;
    }

    return resp_sent;
}


#endif

