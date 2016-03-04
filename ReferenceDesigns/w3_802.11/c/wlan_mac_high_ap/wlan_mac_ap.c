/** @file wlan_mac_ap.c
 *  @brief Access Point
 *
 *  This contains code for the 802.11 Access Point.
 *
 *  @copyright Copyright 2013-2015, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

//Xilinx SDK includes
#include "stdio.h"
#include "stdlib.h"
#include "xtmrctr.h"
#include "xio.h"
#include "string.h"
#include "xintc.h"
#include "xparameters.h"

//802.11 ref design includes
#include "wlan_mac_addr_filter.h"
#include "wlan_mac_time_util.h"
#include "wlan_mac_userio_util.h"
#include "wlan_mac_pkt_buf_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_entries.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_dl_list.h"
#include "ascii_characters.h"
#include "wlan_mac_bss_info.h"
#include "wlan_mac_mgmt_tags.h"
#include "wlan_mac_ap.h"

// Experiments framework includes
#include "wlan_exp.h"
#include "wlan_exp_common.h"
#include "wlan_exp_node.h"
#include "wlan_exp_node_ap.h"
#include "wlan_exp_transport.h"
#include "wlan_exp_user.h"


/*************************** Constant Definitions ****************************/
#define  WLAN_EXP_ETH                            TRANSPORT_ETH_B
#define  WLAN_EXP_NODE_TYPE                     (WLAN_EXP_TYPE_DESIGN_80211 + WLAN_EXP_TYPE_DESIGN_80211_CPU_HIGH_AP)

#define  WLAN_DEFAULT_CHANNEL      11
#define  WLAN_DEFAULT_TX_PWR       15
#define  WLAN_DEFAULT_TX_PHY_MODE  PHY_MODE_NONHT
#define  WLAN_DEFAULT_TX_ANTENNA   TX_ANTMODE_SISO_ANTA
#define  WLAN_DEFAULT_RX_ANTENNA   RX_ANTMODE_SISO_ANTA

#define  WLAN_DEFAULT_BEACON_INTERVAL_TU         100

/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/

// SSID variables
static char default_AP_SSID[] = "WARP-AP";

// Common TX header for 802.11 packets
static mac_header_80211_common    tx_header_common;

// Default Transmission Parameters
tx_params_t default_unicast_mgmt_tx_params;
tx_params_t default_unicast_data_tx_params;
tx_params_t default_multicast_mgmt_tx_params;
tx_params_t default_multicast_data_tx_params;

// "my_bss_info" is a pointer to the bss_info that describes this AP.
// Inside this structure is a dl_list of station_info. This is a list
// of all stations that are currently associated with this AP. In the
// 802.11-2012 Section 10.3 parlance, these stations are currently
// in State 4 (Authenticated, Associated). As part of the association
// process, we want to track stations as they transition through the
// 10.3 states. To do this, we'll create a totally separate dl_list of
// station_info that represent stations in State 2
// (Authenticated, Unassociated). Only members of this list will be allowed
// to elevate to State 4 in the my_bss_info.
bss_info*	                      my_bss_info;

dl_list		                      counts_table;
dl_list		                      station_info_state_2;

// Tx queue variables;
static u32                        max_queue_size;

// AP channel
volatile u32                      mac_param_chan;

// MAC address
static u8 	                      wlan_mac_addr[6];

// Traffic Indication Map State
// These global structs must be protected against externing. Any
// modifications of these structs should be done via an explicit
// setter that also updates the beacon template.
static volatile ps_conf           		gl_power_save_configuration;
static volatile	u32				  		num_dozed_stations;
static volatile mgmt_tag_template_t*	mgmt_tag_tim_template;
static volatile u32				  		mgmt_tag_tim_update_schedule_id;

static	beacon_txrx_configure_t	  gl_beacon_txrx_config; //TODO: Need to create a setter for this struct that also pushes it via IPC to CPU_LOW
															//Also, bss_info as well as beacon template packet contents should be updated as well
															//for beacon_interval changes



/*************************** Functions Prototypes ****************************/

#ifdef USE_WLAN_EXP
int  wlan_exp_process_user_cmd(u32 cmd_id, int socket_index, void * from, cmd_resp * command, cmd_resp * response, u32 max_resp_len);
#endif


/******************************** Functions **********************************/

int main(){
	ps_conf           initial_power_save_configuration;
	bss_config_t	  bss_config;

	u8 disallow_filter[6] = {0x00,0x00,0x00,0x00,0x00,0x00};
	u8 disallow_mask[6]   = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

	xil_printf("\f");
	xil_printf("----- Mango 802.11 Reference Design -----\n");
	xil_printf("----- v1.5   ----------------------------\n");
	xil_printf("----- wlan_mac_ap -----------------------\n");

	xil_printf("Compiled %s %s\n\n", __DATE__, __TIME__);

	//heap_init() must be executed before any use of malloc. This explicit init
	// handles the case of soft-reset of the MicroBlaze leaving stale values in the heap RAM
	wlan_mac_high_heap_init();

	//Initialize the MAC framework
	wlan_mac_high_init();

	// AP does not currently advertise a BSS
	configure_bss(NULL);

	//Define the default PHY and MAC params for all transmissions

	//New associations adopt these unicast params; the per-node params can be
	// overridden via wlan_exp calls or by custom C code
	default_unicast_data_tx_params.phy.power          = WLAN_DEFAULT_TX_PWR;
	default_unicast_data_tx_params.phy.mcs            = WLAN_MAC_MCS_18M;
	default_unicast_data_tx_params.phy.phy_mode       = WLAN_DEFAULT_TX_PHY_MODE;
	default_unicast_data_tx_params.phy.antenna_mode   = WLAN_DEFAULT_TX_ANTENNA;

	default_unicast_mgmt_tx_params.phy.power          = WLAN_DEFAULT_TX_PWR;
	default_unicast_mgmt_tx_params.phy.mcs            = WLAN_MAC_MCS_6M;
	default_unicast_mgmt_tx_params.phy.phy_mode       = WLAN_DEFAULT_TX_PHY_MODE;
	default_unicast_mgmt_tx_params.phy.antenna_mode   = WLAN_DEFAULT_TX_ANTENNA;

	//All multicast traffic (incl. broadcast) uses these default Tx params
	default_multicast_data_tx_params.phy.power        = WLAN_DEFAULT_TX_PWR;
	default_multicast_data_tx_params.phy.mcs          = WLAN_MAC_MCS_6M;
	default_multicast_data_tx_params.phy.phy_mode     = WLAN_DEFAULT_TX_PHY_MODE;
	default_multicast_data_tx_params.phy.antenna_mode = WLAN_DEFAULT_TX_ANTENNA;

	default_multicast_mgmt_tx_params.phy.power        = WLAN_DEFAULT_TX_PWR;
	default_multicast_mgmt_tx_params.phy.mcs          = WLAN_MAC_MCS_6M;
	default_multicast_mgmt_tx_params.phy.phy_mode     = WLAN_DEFAULT_TX_PHY_MODE;
	default_multicast_mgmt_tx_params.phy.antenna_mode = WLAN_DEFAULT_TX_ANTENNA;

	// Setup the counts lists
	dl_list_init(&counts_table);

	// Set the maximum associations
	wlan_mac_high_set_max_associations(MAX_NUM_ASSOC);

	// Calculate the maximum length of any Tx queue
	//   (queue_total_size()- eth_get_num_rx_bd()) is the number of queue entries available after dedicating some to the ETH DMA
	//   MAX_PER_FLOW_QUEUE is the absolute max length of any queue; long queues (a.k.a. buffer bloat) are bad
	max_queue_size = min((queue_total_size()- eth_get_num_rx_bd()) / (1), MAX_TX_QUEUE_LEN);

	// Initialize callbacks
	wlan_mac_util_set_eth_rx_callback(          (void*)ethernet_receive);
	wlan_mac_high_set_mpdu_tx_done_callback(    (void*)mpdu_transmit_done);
	wlan_mac_high_set_beacon_tx_done_callback(  (void*)beacon_transmit_done);
	wlan_mac_high_set_mpdu_rx_callback(         (void*)mpdu_rx_process);
	wlan_mac_high_set_pb_u_callback(            (void*)up_button);

	wlan_mac_high_set_uart_rx_callback(         (void*)uart_rx);
	wlan_mac_high_set_poll_tx_queues_callback(  (void*)poll_tx_queues);
	wlan_mac_high_set_mpdu_dequeue_callback(    (void*)mpdu_dequeue);
    wlan_mac_ltg_sched_set_callback(            (void*)ltg_event);
    queue_set_state_change_callback(			(void*)queue_state_change);

    // Configure the wireless-wired encapsulation mode (AP and STA behaviors are different)
    wlan_mac_util_set_eth_encap_mode(ENCAP_MODE_AP);

    // Ask CPU Low for its status
    // The response to this request will be handled asynchronously
    wlan_mac_high_request_low_state();


    // Wait for CPU Low to initialize
	while( wlan_mac_high_is_cpu_low_initialized() == 0 ){
		xil_printf("waiting on CPU_LOW to boot\n");
	}

#ifdef USE_WLAN_EXP
    u32                  node_type;
    wlan_mac_hw_info_t * hw_info;

    // NOTE:  To use the WLAN Experiments Framework, it must be initialized after
    //        CPU low has populated the hw_info structure in the MAC High framework.

    // Reset all callbacks
    wlan_exp_reset_all_callbacks();

    // Set WLAN Exp callbacks
    wlan_exp_set_init_callback(                    (void *) wlan_exp_node_ap_init);
    wlan_exp_set_process_node_cmd_callback(        (void *) wlan_exp_process_node_cmd);
    wlan_exp_set_reset_station_counts_callback(    (void *) reset_station_counts);
    wlan_exp_set_purge_all_data_tx_queue_callback( (void *) purge_all_data_tx_queue);
    //   - wlan_exp_set_tx_cmd_add_association_callback() should not be used by the AP
    wlan_exp_set_process_user_cmd_callback(        (void *) wlan_exp_process_user_cmd);

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


	// The node's MAC address is stored in the EEPROM, accessible only to CPU Low
	// CPU Low provides this to CPU High after it boots
	memcpy((void*) &(wlan_mac_addr[0]), (void*) get_mac_hw_addr_wlan(), 6);

    // Set Header information
	tx_header_common.address_2 = &(wlan_mac_addr[0]);

    // Initialize hex display
	ap_update_hex_display(0);

	// Configure default radio and PHY params via messages to CPU Low
	mac_param_chan = WLAN_DEFAULT_CHANNEL;
	wlan_mac_high_set_channel( mac_param_chan );
	wlan_mac_high_set_rx_ant_mode(WLAN_DEFAULT_RX_ANTENNA);
	wlan_mac_high_set_tx_ctrl_pow(WLAN_DEFAULT_TX_PWR);

	// Configure CPU Low's filter for passing Rx packets up to CPU High
	//  Default is "promiscuous" mode - pass all data and management packets with good or bad checksums
	//   This allows logging of all data/management receptions, even if they're not intended for this node
	wlan_mac_high_set_rx_filter_mode( (RX_FILTER_FCS_ALL | RX_FILTER_HDR_ALL) );

	// Initialize interrupts
	wlan_mac_high_interrupt_init();

    // Setup default scheduled events:
	//  Periodic beacon transmissions

	initial_power_save_configuration.enable = 1;
	initial_power_save_configuration.dtim_period = 1;
	initial_power_save_configuration.dtim_count = 0;
	initial_power_save_configuration.dtim_mcast_allow_window = (WLAN_DEFAULT_BEACON_INTERVAL_TU * BSS_MICROSECONDS_IN_A_TU) / 4;

	memcpy(bss_config.bssid, wlan_mac_addr, 6);
	strcpy(bss_config.ssid, default_AP_SSID);
	bss_config.chan = WLAN_DEFAULT_CHANNEL;
	bss_config.ht_capable = 1;
	bss_config.beacon_interval = WLAN_DEFAULT_BEACON_INTERVAL_TU;
	bss_config.update_mask = (BSS_FIELD_MASK_BSSID  		 |
							  BSS_FIELD_MASK_CHAN   		 |
							  BSS_FIELD_MASK_SSID			 |
							  BSS_FIELD_MASK_BEACON_INTERVAL |
							  BSS_FIELD_MASK_HT_CAPABLE);
	configure_bss(&bss_config);

	// Initialize TIM management tag that will be postpended to a beacon
	mgmt_tag_tim_update_schedule_id = SCHEDULE_ID_RESERVED_MAX;
	mgmt_tag_tim_template = NULL;

	// Set the global power save configuration
	//   Note: this should be called after wlan_mac_high_configure_beacon_transmit
	set_power_save_configuration(initial_power_save_configuration);

	//  Periodic check for timed-out associations
	wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, ASSOCIATION_CHECK_INTERVAL_US, SCHEDULE_REPEAT_FOREVER, (void*)association_timestamp_check);

	// Set Periodic blinking of hex display (period of 500 with min of 2 and max of 400)
	set_hex_pwm_period(500);
	set_hex_pwm_min_max(2, 400);
	enable_hex_pwm();

	// Reset the event log
	event_log_reset();

	// Set address filter
	if((wlan_mac_high_get_user_io_state()&GPIO_MASK_DS_3)){
		xil_printf("Disallowing Associations\n");
		wlan_mac_addr_filter_reset();
		wlan_mac_addr_filter_add(disallow_mask, disallow_filter);
	}

    // Print AP information to the terminal
    xil_printf("------------------------\n");
    xil_printf("WLAN MAC AP boot complete: \n");
    xil_printf("  Serial Number : W3-a-%05d\n", hw_info->serial_number);

#ifdef WLAN_USE_UART_MENU
	xil_printf("\nPress the Esc key in your terminal to access the UART menu\n");
#endif

	// Finally enable all interrupts to start handling wireless and wired traffic
	wlan_mac_high_interrupt_restore_state(INTERRUPTS_ENABLED);


	while(1) {
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

void set_power_save_configuration(ps_conf power_save_configuration){
	interrupt_state_t curr_interrupt_state;
	gl_power_save_configuration = power_save_configuration;

	//TODO:
	//	initial_power_save_configuration.dtim_period = 1;
	//  initial_power_save_configuration.dtim_count = 0;
	//  are currently the only supported parameters.
	//
	//	Supporting other parameters will require modifications
	//  to beacon_transmit_done() to decrement the current
	//  count after each beacon Tx.
	gl_power_save_configuration.dtim_period = 1;
	gl_power_save_configuration.dtim_count = 0;

	curr_interrupt_state = wlan_mac_high_interrupt_stop();
	update_tim_tag_all(SCHEDULE_ID_RESERVED_MAX);
	wlan_mac_high_interrupt_restore_state(curr_interrupt_state);

}

void queue_state_change(u32 QID, u8 queue_len){
	//queue_len will take on a value of 0 or 1
	//and represents the state of the queue after the state
	//changes.
	u8 aid;

	if(mgmt_tag_tim_update_schedule_id != SCHEDULE_ID_RESERVED_MAX){
		//We already have a pending full TIM state re-write scheduled, so we won't bother
		//with a per-queue change.
		return;
	}

	if(mgmt_tag_tim_template == NULL){
		//The TIM tag is not present in the current beacon template.
		//We have no choice but to do a full TIM tag update and write.
		if((gl_power_save_configuration.enable != 0) && (num_dozed_stations != 0)){
			update_tim_tag_all(SCHEDULE_ID_RESERVED_MAX);
		}
	} else {
		//There is a TIM tag that is already present. We can update
		//only the relevant byte that applies to this queue state
		//change
		if(QID == MCAST_QID) {
			update_tim_tag_aid(0, queue_len);
		} else {
			aid = AID_TO_QID(QID);
			update_tim_tag_aid(aid, queue_len);
		}
	}
}

inline void update_tim_tag_aid(u8 aid, u8 bit_val_in){

	// The intention of this function is to modify as little of an existing TIM
	// tag in the beacon template packet buffer as possible to reduce the amount
	// of time that CPU could be waiting on the packet buffer to be unlocked.
	//
	//Note: AID = 0 is invalid. And input of 0 to this function
	//indicates that the multicast bit in the tim_control byte should
	//be modified.

	u32 				existing_mgmt_tag_length 	= 0;	// Size of the management tag that is currently in the packet buffer
	u8                  tim_control					= 0;
	u16 				tim_byte_idx           		= 0;
	u8					tim_bit_idx            		= 0;
	u8 					bit_val 					= (bit_val_in & 1);
	tx_frame_info*  	tx_frame_info_ptr 			= (tx_frame_info*)TX_PKT_BUF_TO_ADDR(TX_PKT_BUF_BEACON);

	if(my_bss_info == NULL) return;

	//First, we should determine whether a call to update_time_tag_all is scheduled
	//for some time in the future. If it is, we can just return immediately and let that
	//execution clean up any pending TIM state changes.
	if(mgmt_tag_tim_update_schedule_id != SCHEDULE_ID_RESERVED_MAX){
		return;
	}

	if(mgmt_tag_tim_template == NULL){
		//There currently isn't any TIM tag in the packet buffer, which means
		//that we need to restore the full state and not toggle a single
		//bit
		update_tim_tag_all(SCHEDULE_ID_RESERVED_MAX);
		return;
	}

	//There exists a TIM tag in the beacon. We should determine its length
	existing_mgmt_tag_length 	= mgmt_tag_tim_template->header.tag_length;
	tim_byte_idx 				= aid / 8;

	if((tim_byte_idx + 4) > existing_mgmt_tag_length){
		//The current byte we intend to modify is larger than the existing tag. In this case,
		//we fall back on the update_tim_tag_all function since we cannnot simply modify a single
		//bit.
		update_tim_tag_all(SCHEDULE_ID_RESERVED_MAX);
		return;
	}

	if((tx_frame_info_ptr->tx_pkt_buf_state != READY) || (lock_tx_pkt_buf(TX_PKT_BUF_BEACON) != PKT_BUF_MUTEX_SUCCESS)){
		//Note: The order of operations in the above if() clause is important. If the tx_pkt_buf_state is not ready.
		//then we should not even attempt to lock the beacon template packet buffer. This behavior is ensured if
		//the tx_pkt_buf_state is checked on on the left-hand side of the || because the || operation is a sequence
		//point in C.

		//CPU_LOW currently has the beacon packet buffer locked, which means that it is actively
		//transmitting the beacon and it is not safe to modify the contents of the packet. We will
		//use the scheduler to call update_tim_tag_all() sometime later when it is likely that the
		//packet buffer is no longer locked.
		mgmt_tag_tim_update_schedule_id = wlan_mac_schedule_event_repeated(SCHEDULE_FINE,
																		   (my_bss_info->beacon_interval * BSS_MICROSECONDS_IN_A_TU) / 4,
																		   1,
																		   (void*)update_tim_tag_all);
		return;
	}

	// If we have made it this far into the function, then we know that the TIM tag currently
	// exists in the beacon template packet buffer, the mgmt_tag_tim_template is non-NULL, and
	// finally that the TIM tag is long enough that we can simply modify bit(s) that correspond
	// to the AID argument of the function.

	if(aid == 0){
		if(bit_val){
			tim_control |= 0x01; //Raise the multicast bit in the TIM control field
			mgmt_tag_tim_template->data[2] = tim_control; 			//TIM Control (top 7 bits are offset for partial map)
			mgmt_tag_tim_template->data[3] |= tim_control&1; 		//Per 10.2.1.3 in 802.11-2012: AID 0 is treated as
																	//the multicast buffer state
		} else {
			tim_control = 0;
			mgmt_tag_tim_template->data[2] = tim_control; 			//TIM Control (top 7 bits are offset for partial map)
			mgmt_tag_tim_template->data[3] &= ~(tim_control&1); 	//Per 10.2.1.3 in 802.11-2012: AID 0 is treated as
																	//the multicast buffer state
		}
	} else {
		if(bit_val){
			mgmt_tag_tim_template->data[3+tim_byte_idx] |= 1<<tim_bit_idx;
		} else {
			mgmt_tag_tim_template->data[3+tim_byte_idx] &= ~(1<<tim_bit_idx);
		}
	}

	if(unlock_tx_pkt_buf(TX_PKT_BUF_BEACON) != PKT_BUF_MUTEX_SUCCESS){
		xil_printf("Error: Unable to unlock Beacon packet buffer during update_tim_tag_all\n");
	}
	return;

}

void update_tim_tag_all(u32 sched_id){

	tx_frame_info*  	tx_frame_info_ptr 			= (tx_frame_info*)TX_PKT_BUF_TO_ADDR(TX_PKT_BUF_BEACON);
	u32 				existing_mgmt_tag_length 	= 0;	// Size of the management tag that is currently in the packet buffer
	u32					next_mgmt_tag_length		= 0;	// Size that we will update the management tag to be.
															// Note: a third size we have to track is present in the global mgmt_tag_tim
	dl_entry*			curr_station_entry;
	station_info*		curr_station;
	u8                  tim_control;
	u32 				i;
	u16 				tim_byte_idx           		= 0;
	u16 				tim_next_byte_idx      		= 0;
	u8					tim_bit_idx            		= 0;

	if(my_bss_info == NULL) return;
	if(sched_id == SCHEDULE_ID_RESERVED_MAX){
		//This function was called manually (not via the scheduler)

		//First, we should determine whether a call to update_time_tag_all is scheduled
		//for some time in the future. If it is, we can just return immediately and let that
		//execution clean up any pending TIM state changes.
		if(mgmt_tag_tim_update_schedule_id != SCHEDULE_ID_RESERVED_MAX){
			return;
		}
	}

	mgmt_tag_tim_update_schedule_id = SCHEDULE_ID_RESERVED_MAX;

	if((tx_frame_info_ptr->tx_pkt_buf_state != READY) || (lock_tx_pkt_buf(TX_PKT_BUF_BEACON) != PKT_BUF_MUTEX_SUCCESS)){
		//Note: The order of operations in the above if() clause is important. If the tx_pkt_buf_state is not ready.
		//then we should not even attempt to lock the beacon template packet buffer. This behavior is ensured if
		//the tx_pkt_buf_state is checked on on the left-hand side of the || because the || operation is a sequence
		//point in C.

		//CPU_LOW currently has the beacon packet buffer locked, which means that it is actively
		//transmitting the beacon and it is not safe to modify the contents of the packet. We will
		//use the scheduler to call update_tim_tag_all() sometime later when it is likely that the
		//packet buffer is no longer locked.
		mgmt_tag_tim_update_schedule_id = wlan_mac_schedule_event_repeated(SCHEDULE_FINE,
																		   (my_bss_info->beacon_interval * BSS_MICROSECONDS_IN_A_TU) / 4,
																		   1,
																		   (void*)update_tim_tag_all);
		return;
	}

	if(mgmt_tag_tim_template != NULL){
		//There exists a TIM tag in the beacon. We should determine its length
		existing_mgmt_tag_length = mgmt_tag_tim_template->header.tag_length;
	}

	//----------------------------------
	// 1. If we are not going to include the TIM tag in the next beacon transmission, we want to
	// exit this context quickly and not bother updating state that ultimately will not be used.
	if( (gl_power_save_configuration.enable == 0) || (num_dozed_stations == 0) ){
		if(mgmt_tag_tim_template != NULL){
			//We need to remove the existing tag
			mgmt_tag_tim_template = NULL;
			//We can leave the tag in place and just reduce the length
			//accordingly.
			tx_frame_info_ptr->length -= (existing_mgmt_tag_length+sizeof(mgmt_tag_header));
		}
	} else {
		//----------------------------------
		// 2. If we are going to include the TIM tag, we will refresh the full state of the
		// TIM tag based upon queue occupancy

		//We'll start updating the TIM tag from the last associated station.
		//Since we know that the WLAN MAC High Framework keeps the dl_list of
		//associated stations in increasing AID order, we can use this final
		//station's AID to define the size of the TIM tag.
		curr_station_entry = my_bss_info->associated_stations.last;

		if(curr_station_entry != NULL){
			curr_station = (station_info*)(curr_station_entry->data);
			next_mgmt_tag_length = 4 + ((curr_station->AID) / 8);
		} else {
			//Note: this clause should never execute since num_dozed_stations must be 0
			//if no one is associated.
			next_mgmt_tag_length = 4;
		}

		if(mgmt_tag_tim_template == NULL){
			//We need to add the tag to the end of the beacon template
			//and update the length field of the tx_frame_info.
			mgmt_tag_tim_template = (mgmt_tag_template_t*)((void*)(tx_frame_info_ptr)
													+PHY_TX_PKT_BUF_MPDU_OFFSET
													+tx_frame_info_ptr->length
													-WLAN_PHY_FCS_NBYTES);
			mgmt_tag_tim_template->header.tag_element_id 	= MGMT_TAG_TIM;
			tx_frame_info_ptr->length += sizeof(mgmt_tag_header);
		}

		mgmt_tag_tim_template->header.tag_length 		= next_mgmt_tag_length;

		tim_control = 0; //The top 7 bits are an offset for the partial map

		if(queue_num_queued(MCAST_QID)>0){
			tim_control |= 0x01; //Raise the multicast bit in the TIM control field
		}
		curr_station_entry = my_bss_info->associated_stations.first;
		while(curr_station_entry != NULL){
			curr_station = (station_info*)(curr_station_entry->data);

			if(queue_num_queued(AID_TO_QID(curr_station->AID))){
				tim_next_byte_idx = (curr_station->AID) / 8;

				if(tim_next_byte_idx > tim_byte_idx){
					//We've moved on to a new octet. We should zero everything after the previous octet
					//up to and including the new octet.
					for(i = tim_byte_idx+1; i <= tim_next_byte_idx; i++){
						mgmt_tag_tim_template->data[3+i] = 0;
					}
				}

				tim_bit_idx  = (curr_station->AID) % 8;
				tim_byte_idx = tim_next_byte_idx;

				//Raise the bit for this station in the TIM partial bitmap
				mgmt_tag_tim_template->data[3+tim_byte_idx] |= 1<<tim_bit_idx;
			}

			curr_station_entry = dl_entry_next(curr_station_entry);
		}

		mgmt_tag_tim_template->data[0] = gl_power_save_configuration.dtim_count;
		mgmt_tag_tim_template->data[1] = gl_power_save_configuration.dtim_period;
		mgmt_tag_tim_template->data[2] = tim_control; 		//TIM Control (top 7 bits are offset for partial map)
		mgmt_tag_tim_template->data[3] |= tim_control&1; 	//Per 10.2.1.3 in 802.11-2012: AID 0 is treated as
															//the multicast buffer state
		tx_frame_info_ptr->length += (next_mgmt_tag_length - existing_mgmt_tag_length);
	}
	if(unlock_tx_pkt_buf(TX_PKT_BUF_BEACON) != PKT_BUF_MUTEX_SUCCESS){
		xil_printf("Error: Unable to unlock Beacon packet buffer during update_tim_tag_all\n");
	}
	return;

}

void beacon_transmit_done( tx_frame_info* tx_mpdu, wlan_mac_low_tx_details_t* tx_low_details ){
	u32 first_tx_time_delta;

	gl_power_save_configuration.dtim_timestamp = get_system_time_usec() + gl_power_save_configuration.dtim_mcast_allow_window;

	poll_tx_queues(); //We just entered a window where we are allowing multicast packets to dequeue. We should
					  //poll the Tx queues and see if there are any ready to be dequeued.

	first_tx_time_delta = (u32)(tx_low_details->tx_start_timestamp_mpdu - (tx_mpdu->timestamp_create + tx_mpdu->delay_accept));

	if( (first_tx_time_delta < 9) ){
		//This captures a subtle effect in the DCF hardware. A random backoff is calculated on the
		//first transmission of an MPDU in case a CCA_BUSY causes a deferral. If there is no deferral,
		//this slot count is not used. We can sanitize this value here by seeing if the packet transmitted
		//immediately (i.e. a time from start to accept that is less than a slot). In this case, we know
		//there was no backoff needed for this transmission. We signify this event with a num_slots value
		//of -1.
		tx_low_details->num_slots = -1;
	}
	// Log the TX low
	wlan_exp_log_create_tx_low_entry(tx_mpdu, tx_low_details, 0);
}

/**
 * @brief Poll Tx queues to select next available packet to transmit
 *
 * This function is called whenever the upper MAC is ready to send a new packet
 * to the lower MAC for transmission. The next packet to transmit is selected
 * from one of the currently-enabled Tx queues.
 *
 * The reference implementation uses a simple queue prioritization scheme:
 *   - Two queue groups are defined: Management (MGMT_QGRP) and Data (DATA_QGRP)
 *     - The Management group contains one queue for all management traffic
 *     - The Data group contains one queue for multicast data plus one queue per associated STA
 *   - The code alternates its polling between queue groups
 *   - In each group queues are polled via round robin
 *
 *  This scheme gives priority to management transmissions to help avoid timeouts during
 *  association handshakes and treats each associated STA with equal priority.
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
	interrupt_state_t curr_interrupt_state;
	u32 i,k;

	if(my_bss_info == NULL) return;

	curr_interrupt_state = wlan_mac_high_interrupt_stop();

	#define NUM_QUEUE_GROUPS 2
	typedef enum {MGMT_QGRP, DATA_QGRP} queue_group_t;

	// Remember the next group to poll between calls to this function
	//   This implements the ping-pong poll between the MGMT_QGRP and DATA_QGRP groups
	static queue_group_t next_queue_group = MGMT_QGRP;
	queue_group_t curr_queue_group;

	// Remember the last queue polled between calls to this function
	//   This implements the round-robin poll of queues in the DATA_QGRP group
	static dl_entry* next_station_info_entry = NULL;
	dl_entry* curr_station_info_entry;

	station_info* curr_station_info;

	if( wlan_mac_high_is_dequeue_allowed() ){
		for(k = 0; k < NUM_QUEUE_GROUPS; k++){
			curr_queue_group = next_queue_group;

			switch(curr_queue_group){
				case MGMT_QGRP:
					next_queue_group = DATA_QGRP;
					if(dequeue_transmit_checkin(MANAGEMENT_QID)){
						goto poll_cleanup;
						return;
					}
				break;

				case DATA_QGRP:
					next_queue_group = MGMT_QGRP;
					curr_station_info_entry = next_station_info_entry;

					for(i = 0; i < (my_bss_info->associated_stations.length + 1); i++) {
						// Loop through all associated stations' queues and the broadcast queue
						if(curr_station_info_entry == NULL){
							// Check the broadcast queue
							next_station_info_entry = my_bss_info->associated_stations.first;

							if( (num_dozed_stations == 0)
									|| (get_system_time_usec() < gl_power_save_configuration.dtim_timestamp)
									|| (gl_power_save_configuration.enable == 0) ){
								if(dequeue_transmit_checkin(MCAST_QID)){
									// Found a not-empty queue, transmitted a packet
									goto poll_cleanup;
									return;
								}
							}

							curr_station_info_entry = next_station_info_entry;

						} else {
							curr_station_info = (station_info*)(curr_station_info_entry->data);
							if( wlan_mac_high_is_valid_association(&my_bss_info->associated_stations, curr_station_info) ){
								if(curr_station_info_entry == my_bss_info->associated_stations.last){
									// We've reached the end of the table, so we wrap around to the beginning
									next_station_info_entry = NULL;
								} else {
									next_station_info_entry = dl_entry_next(curr_station_info_entry);
								}

								if(((curr_station_info->flags & STATION_INFO_FLAG_DOZE) == 0) || (gl_power_save_configuration.enable == 0)){
									if(dequeue_transmit_checkin(AID_TO_QID(curr_station_info->AID))){
										// Found a not-empty queue, transmitted a packet
										goto poll_cleanup;
										return;
									}
								}

								curr_station_info_entry = next_station_info_entry;

							} else {
								// This curr_station_info is invalid. Perhaps it was removed from
								// the association table before poll_tx_queues was called. We will
								// start the round robin checking back at broadcast.
								next_station_info_entry = NULL;
								goto poll_cleanup;
								return;
							} // END if(is_valid_association)
						}
					} // END for loop over association table
				break;
			} // END switch(queue group)
		} // END loop over queue groups
	} // END CPU low is ready

	poll_cleanup:

	wlan_mac_high_interrupt_restore_state(curr_interrupt_state);

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
	dl_entry*	  curr_station_info_entry;
	station_info* curr_station_info;

	if(my_bss_info == NULL) return;

	int			  iter = my_bss_info->associated_stations.length;

	// Purge all data transmit queues
	purge_queue(MCAST_QID);                                    		// Broadcast Queue
	curr_station_info_entry = my_bss_info->associated_stations.first;

	while( (curr_station_info_entry != NULL) && (iter-- > 0)){
		curr_station_info = (station_info*)(curr_station_info_entry->data);
		purge_queue(AID_TO_QID(curr_station_info->AID));       		// Each unicast queue
		curr_station_info_entry = dl_entry_next(curr_station_info_entry);
	}
}



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
 */
void mpdu_transmit_done(tx_frame_info* tx_mpdu, wlan_mac_low_tx_details_t* tx_low_details, u16 num_tx_low_details) {
	u32                    i;
	station_info*          station 				   = NULL;
	dl_entry*	           entry				   = NULL;
	u32					   first_tx_time_delta;


	if(my_bss_info != NULL) entry = wlan_mac_high_find_station_info_AID(&(my_bss_info->associated_stations), tx_mpdu->AID);
	if(entry != NULL) station = (station_info*)(entry->data);

	// Additional variables (Future Use)
	// void*                  mpdu                    = (u8*)tx_mpdu + PHY_TX_PKT_BUF_MPDU_OFFSET;
	// u8*                    mpdu_ptr_u8             = (u8*)mpdu;
	// mac_header_80211*      tx_80211_header         = (mac_header_80211*)((void *)mpdu_ptr_u8);

	// Log all of the TX Low transmissions

	//xil_printf("------------\n"); //DEBUG
	for(i = 0; i < num_tx_low_details; i++) {

		if( i == 0 ){
			if( (tx_low_details[i].tx_details_type == TX_DETAILS_RTS_ONLY) || (tx_low_details[i].tx_details_type == TX_DETAILS_RTS_MPDU)){
				first_tx_time_delta = (u32)(tx_low_details[i].tx_start_timestamp_ctrl - (tx_mpdu->timestamp_create + tx_mpdu->delay_accept));
			} else {
				first_tx_time_delta = (u32)(tx_low_details[i].tx_start_timestamp_mpdu - (tx_mpdu->timestamp_create + tx_mpdu->delay_accept));
			}

			if( (first_tx_time_delta < 9) ){
				//This captures a subtle effect in the DCF hardware. A random backoff is calculated on the
				//first transmission of an MPDU in case a CCA_BUSY causes a deferral. If there is no deferral,
				//this slot count is not used. We can sanitize this value here by seeing if the packet transmitted
				//immediately (i.e. a time from start to accept that is less than a slot). In this case, we know
				//there was no backoff needed for this transmission. We signify this event with a num_slots value
				//of -1.
				tx_low_details[i].num_slots = -1;
			}
		}

		// Log the TX low
		wlan_exp_log_create_tx_low_entry(tx_mpdu, &tx_low_details[i], i);
	}

	// Log the TX MPDU
	wlan_exp_log_create_tx_high_entry(tx_mpdu, mac_param_chan);

	// Update the counts for the node to which the packet was just transmitted
	if(tx_mpdu->AID != 0) {
		wlan_mac_high_update_tx_counts(tx_mpdu, station);
	}


	// Send log entry to wlan_exp controller immediately (not currently supported)
	//
	// if (tx_high_event_log_entry != NULL) {
    //     wn_transmit_log_entry((void *)tx_high_event_log_entry);
	// }
}



/**
 * @brief Callback to handle push of up button
 *
 * Reference implementation does nothing.
 *
 * @param None
 * @return None
 */
void up_button(){
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

	u32               payload_length;
	u32               min_ltg_payload_length;
	dl_entry*	      station_info_entry           = NULL;
	station_info*     station                      = NULL;
	u8*               addr_da;
	u8                is_multicast;
	u8                queue_sel;
	tx_queue_element* curr_tx_queue_element        = NULL;
	tx_queue_buffer*  curr_tx_queue_buffer         = NULL;
	u8                continue_loop;

	if(my_bss_info != NULL){
		switch(((ltg_pyld_hdr*)callback_arg)->type){
			case LTG_PYLD_TYPE_FIXED:
				payload_length = ((ltg_pyld_fixed*)callback_arg)->length;
				addr_da = ((ltg_pyld_fixed*)callback_arg)->addr_da;

				is_multicast = wlan_addr_mcast(addr_da);
				if(is_multicast){
					queue_sel = MCAST_QID;
				} else {
					station_info_entry = wlan_mac_high_find_station_info_ADDR(&my_bss_info->associated_stations, addr_da);
					if(station_info_entry != NULL){
						station = (station_info*)(station_info_entry->data);
						queue_sel = AID_TO_QID(station->AID);
					} else {
						return;
					}
				}
			break;

			case LTG_PYLD_TYPE_UNIFORM_RAND:
				payload_length = (rand()%(((ltg_pyld_uniform_rand*)(callback_arg))->max_length - ((ltg_pyld_uniform_rand*)(callback_arg))->min_length))+((ltg_pyld_uniform_rand*)(callback_arg))->min_length;
				addr_da = ((ltg_pyld_fixed*)callback_arg)->addr_da;

				is_multicast = wlan_addr_mcast(addr_da);
				if(is_multicast){
					queue_sel = MCAST_QID;
				} else {
					station_info_entry = wlan_mac_high_find_station_info_ADDR(&my_bss_info->associated_stations, addr_da);
					if(station_info_entry != NULL){
						station = (station_info*)(station_info_entry->data);
						queue_sel = AID_TO_QID(station->AID);
					} else {
						return;
					}
				}
			break;

			case LTG_PYLD_TYPE_ALL_ASSOC_FIXED:
				if(my_bss_info->associated_stations.length > 0){
					station_info_entry = my_bss_info->associated_stations.first;
					station = (station_info*)station_info_entry->data;
					addr_da = station->addr;
					queue_sel = AID_TO_QID(station->AID);
					is_multicast = 0;
					payload_length = ((ltg_pyld_all_assoc_fixed*)callback_arg)->length;
				} else {
					return;
				}
			break;

			default:
				xil_printf("ERROR ltg_event: Unknown LTG Payload Type! (%d)\n", ((ltg_pyld_hdr*)callback_arg)->type);
				return;
			break;
		}

		do{
			continue_loop = 0;

			if(queue_num_queued(queue_sel) < max_queue_size){
				// Checkout 1 element from the queue;
				curr_tx_queue_element = queue_checkout();
				if(curr_tx_queue_element != NULL){
					// Create LTG packet
					curr_tx_queue_buffer = ((tx_queue_buffer*)(curr_tx_queue_element->data));

					// Setup the MAC header
					wlan_mac_high_setup_tx_header( &tx_header_common, addr_da, wlan_mac_addr );

					min_ltg_payload_length = wlan_create_ltg_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, MAC_FRAME_CTRL2_FLAG_FROM_DS, id);
					payload_length = max(payload_length+sizeof(mac_header_80211)+WLAN_PHY_FCS_NBYTES, min_ltg_payload_length);

					// Finally prepare the 802.11 header
					if (is_multicast) {
						wlan_mac_high_setup_tx_frame_info ( &tx_header_common,
															curr_tx_queue_element,
															payload_length,
															(TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_FILL_UNIQ_SEQ),
															queue_sel );
					} else {
						wlan_mac_high_setup_tx_frame_info ( &tx_header_common,
															curr_tx_queue_element,
															payload_length,
															(TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO | TX_MPDU_FLAGS_FILL_UNIQ_SEQ),
															queue_sel );
					}

					// Update the queue entry metadata to reflect the new new queue entry contents
					if (is_multicast || (station == NULL)) {
						curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
						curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)&default_multicast_data_tx_params;
						curr_tx_queue_buffer->frame_info.AID     	 = 0;
					} else {
					    curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_STATION_INFO;
					    curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)station;
						curr_tx_queue_buffer->frame_info.AID         = station->AID;
					}

					// Submit the new packet to the appropriate queue
					enqueue_after_tail(queue_sel, curr_tx_queue_element);

				} else {
					// There aren't any free queue elements right now.
					// As such, there probably isn't any point to continuing this callback.
					// We'll return and try again once it is called the next time.
					return;
				}
			}

			if(((ltg_pyld_hdr*)callback_arg)->type == LTG_PYLD_TYPE_ALL_ASSOC_FIXED){
				station_info_entry = dl_entry_next(station_info_entry);
				if(station_info_entry != NULL){
					station = (station_info*)station_info_entry->data;
					addr_da = station->addr;
					queue_sel = AID_TO_QID(station->AID);
					is_multicast = 0;
					continue_loop = 1;
				} else {
					continue_loop = 0;
				}
			} else {
				continue_loop = 0;
			}
		} while(continue_loop == 1);
	}
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
	station_info* station;
	dl_entry*     entry;

	if(my_bss_info == NULL) return 0;

	// Determine how to send the packet
	if( wlan_addr_mcast(eth_dest) ) {

		// Send the multicast packet
		if(queue_num_queued(MCAST_QID) < max_queue_size){

			// Send the pre-encapsulated Ethernet frame over the wireless interface
			//     NOTE:  The queue element has already been provided, so we do not need to check if it is NULL
			tx_queue_buffer* curr_tx_queue_buffer = (tx_queue_buffer*)(curr_tx_queue_element->data);

			// Setup the TX header
			wlan_mac_high_setup_tx_header( &tx_header_common, (u8*)(&(eth_dest[0])), (u8*)(&(eth_src[0])) );

			// Fill in the data
			wlan_create_data_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, MAC_FRAME_CTRL2_FLAG_FROM_DS);

			// Setup the TX frame info
			wlan_mac_high_setup_tx_frame_info ( &tx_header_common,
												curr_tx_queue_element,
												tx_length,
												0,
												MCAST_QID);

			// Set the information in the TX queue buffer
			curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
			curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)&(default_multicast_data_tx_params);
			curr_tx_queue_buffer->frame_info.AID         = 0;

			// Put the packet in the queue
			enqueue_after_tail(MCAST_QID, curr_tx_queue_element);

		} else {
			// Packet was not successfully enqueued
			return 0;
		}

	} else {
		// Check associations
		//     Is this packet meant for a station we are associated with?
		entry = wlan_mac_high_find_station_info_ADDR(&my_bss_info->associated_stations, eth_dest);

		if( entry != NULL ) {
			station = (station_info*)(entry->data);

			// Send the unicast packet
			if(queue_num_queued(AID_TO_QID(station->AID)) < max_queue_size){

				// Send the pre-encapsulated Ethernet frame over the wireless interface
				//     NOTE:  The queue element has already been provided, so we do not need to check if it is NULL
				tx_queue_buffer* curr_tx_queue_buffer = (tx_queue_buffer*)(curr_tx_queue_element->data);

				// Setup the TX header
				wlan_mac_high_setup_tx_header( &tx_header_common, (u8*)(&(eth_dest[0])), (u8*)(&(eth_src[0])) );

				// Fill in the data
				wlan_create_data_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, MAC_FRAME_CTRL2_FLAG_FROM_DS);

				// Setup the TX frame info
				wlan_mac_high_setup_tx_frame_info ( &tx_header_common,
													curr_tx_queue_element,
													tx_length,
													(TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO),
													AID_TO_QID(station->AID));

				// Set the information in the TX queue buffer
				curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_STATION_INFO;
				curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)station;
				curr_tx_queue_buffer->frame_info.AID         = station->AID;

				// Put the packet in the queue
				enqueue_after_tail(AID_TO_QID(station->AID), curr_tx_queue_element);

			} else {
				// Packet was not successfully enqueued
				return 0;
			}
		} else {
			// Packet was not successfully enqueued
			return 0;
		}
	}

	// Packet successfully enqueued
	return 1;
}

//TODO: Create function to update beacon live fields (e.g. TIM bitmap)
// We may need to formalize a beacon ping/pong handshake to avoid any races
// created by CPU_HIGH modifying the beacon payload while the PHY is actively
// reading bytes out of the packet buffer to create a waveform.


/**
 * @brief Check the time since the AP heard from each station
 *
 * This function will check the timestamp of the last reception from each station and send a
 * de-authentication packet to any stations that have timed out.
 *
 * @param  None
 * @return None
 */
void association_timestamp_check() {

	u32                 aid;
	u64                 time_since_last_activity;
	station_info*       curr_station_info;
	dl_entry*           curr_station_info_entry;
	dl_entry*           next_station_info_entry;

	if(my_bss_info == NULL) return;

	next_station_info_entry = my_bss_info->associated_stations.first;

	while(next_station_info_entry != NULL) {
		curr_station_info_entry = next_station_info_entry;
		next_station_info_entry = dl_entry_next(curr_station_info_entry);

		curr_station_info        = (station_info*)(curr_station_info_entry->data);
		time_since_last_activity = (get_system_time_usec() - curr_station_info->latest_activity_timestamp);

		// De-authenticate the station if we have timed out and we have not disabled this check for the station
		if((time_since_last_activity > ASSOCIATION_TIMEOUT_US) && ((curr_station_info->flags & STATION_INFO_FLAG_DISABLE_ASSOC_CHECK) == 0)){

			aid = deauthenticate_station( curr_station_info );

			if (aid != 0) {
				xil_printf("\n\nDisassociation due to inactivity:\n");
			}
		}
	}

	next_station_info_entry = station_info_state_2.first;

	while(next_station_info_entry != NULL) {
		curr_station_info_entry = next_station_info_entry;
		next_station_info_entry = dl_entry_next(curr_station_info_entry);

		curr_station_info        = (station_info*)(curr_station_info_entry->data);
		time_since_last_activity = (get_system_time_usec() - curr_station_info->latest_activity_timestamp);

		// De-authenticate the station if we have timed out and we have not disabled this check for the station
		if((time_since_last_activity > ASSOCIATION_TIMEOUT_US) && ((curr_station_info->flags & STATION_INFO_FLAG_DISABLE_ASSOC_CHECK) == 0)){
			aid = deauthenticate_station( curr_station_info );
			if (aid != 0) {
				xil_printf("\n\nDeauthentication due to inactivity:\n");
			}
		}
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
 * @return None
 */
void mpdu_rx_process(void* pkt_buf_addr) {

	rx_frame_info*      frame_info               = (rx_frame_info*)pkt_buf_addr;
	void*               mpdu                     = (u8*)pkt_buf_addr + PHY_RX_PKT_BUF_MPDU_OFFSET;
	u8*                 mpdu_ptr_u8              = (u8*)mpdu;
	mac_header_80211*   rx_80211_header          = (mac_header_80211*)((void *)mpdu_ptr_u8);

	u8                  send_response            = 0;
	u16                 tx_length;
	u16                 rx_seq;
	tx_queue_element*   curr_tx_queue_element;
	tx_queue_buffer*    curr_tx_queue_buffer;
	rx_common_entry*    rx_event_log_entry;

	dl_entry*	        associated_station_entry = NULL;
	station_info*       associated_station       = NULL;
	counts_txrx*        station_counts           = NULL;

	u8                  unicast_to_me;
	u8                  to_multicast;
	u8                  eth_send;
	u8                  allow_auth               = 0;

	u8					pre_llc_offset			 = 0;

	u8 					mcs	     = frame_info->phy_details.mcs;
	u8 					phy_mode = frame_info->phy_details.phy_mode;
	u16 				length   = frame_info->phy_details.length;

	// Set the additional info field to NULL
	frame_info->additional_info = (u32)NULL;

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
	if( frame_info->state == RX_MPDU_STATE_FCS_GOOD){

		// Update the association information
		if(my_bss_info != NULL){
			associated_station_entry = wlan_mac_high_find_station_info_ADDR(&my_bss_info->associated_stations, (rx_80211_header->address_2));
		}

		if( associated_station_entry != NULL ){
			associated_station = (station_info*)(associated_station_entry->data);

			// Update PS state
			if((rx_80211_header->frame_control_2) & MAC_FRAME_CTRL2_FLAG_POWER_MGMT){
				if( (associated_station->flags & STATION_INFO_FLAG_DOZE) == 0 ){
					//Station was not previously dozing
					num_dozed_stations++;
					if(num_dozed_stations == 1){
						//This is the first station in our BSS that has entered a doze state.
						//We need include the TIM management tag in the next beacon.
						update_tim_tag_all(SCHEDULE_ID_RESERVED_MAX);
					}
				}
				associated_station->flags |= STATION_INFO_FLAG_DOZE;
			} else {
				if( associated_station->flags & STATION_INFO_FLAG_DOZE ){
					//Station was previously dozing
					num_dozed_stations--;
					if(num_dozed_stations == 0){
						//All stations are now awake. We can drop the TIM management
						//tag in the next beacons.
						update_tim_tag_all(SCHEDULE_ID_RESERVED_MAX);
					}
				}
				associated_station->flags = (associated_station->flags) & ~STATION_INFO_FLAG_DOZE;
			}

			// Update station information
			frame_info->additional_info                    = (u32)associated_station;

			associated_station->latest_activity_timestamp = get_system_time_usec();
			associated_station->rx.last_power             = frame_info->rx_power;
			associated_station->rx.last_mcs               = mcs;
			//FIXME: need last phy_mode too? how does this interact with HT capabilities?

			station_counts = associated_station->counts;

			// Sequence number is 12 MSB of seq_control field
			rx_seq         = ((rx_80211_header->sequence_control) >> 4) & 0xFFF;

			// Check if this was a duplicate reception
			//   - Received seq num matched previously received seq num for this STA
			if( (associated_station->rx.last_seq == rx_seq) ) {
				if(rx_event_log_entry != NULL){
					rx_event_log_entry->flags |= RX_ENTRY_FLAGS_IS_DUPLICATE;
				}

				// Finish the function
				goto mpdu_rx_process_end;
			} else {
				associated_station->rx.last_seq = rx_seq;
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
				case MAC_FRAME_CTRL1_SUBTYPE_QOSDATA:
					pre_llc_offset = sizeof(qos_control);
				case (MAC_FRAME_CTRL1_SUBTYPE_DATA):
					// Data packet
					//   - Determine if this packet is from an associated station
					//   - Depending on the type and destination, transmit the packet wirelessly or over the wired network
					//

					if(associated_station != NULL){

						// MPDU is flagged as destined to the DS
						if((rx_80211_header->frame_control_2) & MAC_FRAME_CTRL2_FLAG_TO_DS) {
							eth_send = 1;

							// Check if this is a multicast packet
							if(wlan_addr_mcast(rx_80211_header->address_3)){
								//TODO: Needs fix for QoS case to handle u16 offset of QoS Control
								// Send the data packet over the wireless
								curr_tx_queue_element = queue_checkout();

								if(curr_tx_queue_element != NULL){
									curr_tx_queue_buffer = (tx_queue_buffer*)(curr_tx_queue_element->data);

									// Setup the TX header
									wlan_mac_high_setup_tx_header( &tx_header_common, rx_80211_header->address_3, rx_80211_header->address_2);

									// Fill in the data
									mpdu_ptr_u8  = curr_tx_queue_buffer->frame;
									tx_length    = wlan_create_data_frame((void*)mpdu_ptr_u8, &tx_header_common, MAC_FRAME_CTRL2_FLAG_FROM_DS);
									mpdu_ptr_u8 += sizeof(mac_header_80211);
									wlan_mac_high_cdma_start_transfer(mpdu_ptr_u8, (void*)rx_80211_header + sizeof(mac_header_80211), length - sizeof(mac_header_80211));


									// Setup the TX frame info
									wlan_mac_high_setup_tx_frame_info ( &tx_header_common,
																		curr_tx_queue_element,
																		length, 0,
																		MCAST_QID );

									// Set the information in the TX queue buffer
									curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
									curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)(&default_multicast_data_tx_params);
									curr_tx_queue_buffer->frame_info.AID         = 0;

									// Make sure the DMA transfer is complete
									wlan_mac_high_cdma_finish_transfer();

									// Put the packet in the queue
									enqueue_after_tail(MCAST_QID, curr_tx_queue_element);

								}
							} else {
								// Packet is not a multi-cast packet.  Check if it is destined for one of our stations
								if(my_bss_info != NULL){
									associated_station_entry = wlan_mac_high_find_station_info_ADDR(&my_bss_info->associated_stations, rx_80211_header->address_3);
								}
								if(associated_station_entry != NULL){
									//TODO: Needs fix for QoS case to handle u16 offset of QoS Control
									associated_station = (station_info*)(associated_station_entry->data);

									// Send the data packet over the wireless to our station
									curr_tx_queue_element = queue_checkout();

									if(curr_tx_queue_element != NULL){
										curr_tx_queue_buffer = (tx_queue_buffer*)(curr_tx_queue_element->data);

										// Setup the TX header
										wlan_mac_high_setup_tx_header( &tx_header_common, rx_80211_header->address_3, rx_80211_header->address_2);

										// Fill in the data
										mpdu_ptr_u8  = curr_tx_queue_buffer->frame;
										tx_length    = wlan_create_data_frame((void*)mpdu_ptr_u8, &tx_header_common, MAC_FRAME_CTRL2_FLAG_FROM_DS);
										mpdu_ptr_u8 += sizeof(mac_header_80211);
										wlan_mac_high_cdma_start_transfer(mpdu_ptr_u8, (void*)rx_80211_header + sizeof(mac_header_80211), length - sizeof(mac_header_80211));

										// Setup the TX frame info
										wlan_mac_high_setup_tx_frame_info ( &tx_header_common,
																			curr_tx_queue_element,
																			length,
																			(TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO),
																			AID_TO_QID(associated_station->AID) );


										// Set the information in the TX queue buffer
										curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_STATION_INFO;
										curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)(associated_station);
										curr_tx_queue_buffer->frame_info.AID         = associated_station->AID;

										// Make sure the DMA transfer is complete
										wlan_mac_high_cdma_finish_transfer();

										// Put the packet in the queue
										enqueue_after_tail(AID_TO_QID(associated_station->AID),  curr_tx_queue_element);

										// Given we sent the packet wirelessly to our stations, if we do not allow Ethernet transmissions
										//   of wireless transmissions, then do not send over Ethernet
										#ifndef ALLOW_ETH_TX_OF_WIRELESS_TX
										eth_send = 0;
										#endif
									}
								}
							}

							// Encapsulate the packet and send over the wired network
							if(eth_send){
								wlan_mpdu_eth_send(mpdu, length, pre_llc_offset);
							}
						}
					} else {
						// Packet was not from an associated station
						//   - Print a WARNING and send a de-authentication to trigger a re-association
						//
						//
						if(unicast_to_me){

							// Received a data frame from a STA that claims to be associated with this AP but is not in the AP association table
							//   Discard the MPDU and reply with a de-authentication frame to trigger re-association at the STA
							wlan_printf(PL_WARNING, "Data from non-associated station: [%x %x %x %x %x %x], issuing de-authentication\n", rx_80211_header->address_2[0],rx_80211_header->address_2[1],rx_80211_header->address_2[2],rx_80211_header->address_2[3],rx_80211_header->address_2[4],rx_80211_header->address_2[5]);
							wlan_printf(PL_WARNING, "Address 3: [%x %x %x %x %x %x]\n", rx_80211_header->address_3[0],rx_80211_header->address_3[1],rx_80211_header->address_3[2],rx_80211_header->address_3[3],rx_80211_header->address_3[4],rx_80211_header->address_3[5]);

							// Send de-authentication packet to the station
							curr_tx_queue_element = queue_checkout();

							if(curr_tx_queue_element != NULL){
								curr_tx_queue_buffer = (tx_queue_buffer*)(curr_tx_queue_element->data);

								// Setup the TX header
								wlan_mac_high_setup_tx_header( &tx_header_common, rx_80211_header->address_2, wlan_mac_addr );

								// Fill in the data
								tx_length = wlan_create_deauth_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, DEAUTH_REASON_NONASSOCIATED_STA);

								// Setup the TX frame info
								wlan_mac_high_setup_tx_frame_info ( &tx_header_common,
																	curr_tx_queue_element,
																	tx_length,
																	(TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO),
																	MANAGEMENT_QID );

								// Set the information in the TX queue buffer
								curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
								curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)(&default_unicast_mgmt_tx_params);
								curr_tx_queue_buffer->frame_info.AID         = 0;

								// Put the packet in the queue
								enqueue_after_tail(MANAGEMENT_QID, curr_tx_queue_element);
							}
						}
					} // END if(associated_station != NULL)
				break;


				//---------------------------------------------------------------------
				case (MAC_FRAME_CTRL1_SUBTYPE_PROBE_REQ):
					// Probe Request Packet
					//   - Check that this packet is to the broadcast address
					//   - Look at the tagged parameters
					//   - Depending on the parameters, send a probe response
					//
					if(wlan_addr_eq(rx_80211_header->address_3, bcast_addr)) {
						mpdu_ptr_u8 += sizeof(mac_header_80211);

						// Loop through tagged parameters
						while(((u32)mpdu_ptr_u8 -  (u32)mpdu)<= (length - WLAN_PHY_FCS_NBYTES)){

							// What kind of tag is this?
							switch(mpdu_ptr_u8[0]){
								//-----------------------------------------------------
								case TAG_SSID_PARAMS:
									// SSID parameter set
									if((my_bss_info != NULL) && ((mpdu_ptr_u8[1]==0) || (memcmp(mpdu_ptr_u8+2, (u8*)my_bss_info->ssid, mpdu_ptr_u8[1])==0))) {
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

						if((my_bss_info != NULL) && send_response) {
							// Create a probe response frame
							curr_tx_queue_element = queue_checkout();

							if(curr_tx_queue_element != NULL){
								curr_tx_queue_buffer = (tx_queue_buffer*)(curr_tx_queue_element->data);

								// Setup the TX header
								wlan_mac_high_setup_tx_header( &tx_header_common, rx_80211_header->address_2, wlan_mac_addr );

								// Fill in the data
								tx_length = wlan_create_probe_resp_frame((void*)(curr_tx_queue_buffer->frame),
																		 &tx_header_common,
																		 my_bss_info);

								// Setup the TX frame info
								wlan_mac_high_setup_tx_frame_info ( &tx_header_common,
																	curr_tx_queue_element,
																	tx_length,
																	(TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO | TX_MPDU_FLAGS_FILL_TIMESTAMP),
																	MANAGEMENT_QID );

								// Set the information in the TX queue buffer
								curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
								curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)(&default_unicast_mgmt_tx_params);
								curr_tx_queue_buffer->frame_info.AID         = 0;

								// Put the packet in the queue
								enqueue_after_tail(MANAGEMENT_QID, curr_tx_queue_element);
							}

							// Finish the function
							goto mpdu_rx_process_end;
						}
					}
				break;


				//---------------------------------------------------------------------
				case (MAC_FRAME_CTRL1_SUBTYPE_AUTH):
					// Authentication Packet
					//   - Check if authentication is allowed
					//   - Potentially send authentication response
					//
					if(wlan_addr_eq(rx_80211_header->address_3, wlan_mac_addr) && wlan_mac_addr_filter_is_allowed(rx_80211_header->address_2)) {
						mpdu_ptr_u8 += sizeof(mac_header_80211);
						switch(((authentication_frame*)mpdu_ptr_u8)->auth_algorithm ){
							case AUTH_ALGO_OPEN_SYSTEM:
								allow_auth = 1;
							break;
							default:
								allow_auth = 0;
							break;
						}
					}

					// Only send response if the packet was from a requester
					//
					if((my_bss_info != NULL) && ((authentication_frame*)mpdu_ptr_u8)->auth_sequence == AUTH_SEQ_REQ){

						if(allow_auth){

							if(wlan_mac_high_find_station_info_ADDR (&(my_bss_info->associated_stations), rx_80211_header->address_2) == NULL){
								xil_printf("Authenticated, Unassociated Stations:\n");
								//This station wasn't already authenticated/associated (state 4), so we'll manually add it to the state 2 list.
								wlan_mac_high_add_association(&station_info_state_2, &counts_table, rx_80211_header->address_2, ADD_ASSOCIATION_ANY_AID);
							}

							// Create a successful authentication response frame
							curr_tx_queue_element = queue_checkout();

							if(curr_tx_queue_element != NULL){
								curr_tx_queue_buffer = (tx_queue_buffer*)(curr_tx_queue_element->data);

								// Setup the TX header
								wlan_mac_high_setup_tx_header( &tx_header_common, rx_80211_header->address_2, wlan_mac_addr );

								// Fill in the data
								tx_length = wlan_create_auth_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, AUTH_ALGO_OPEN_SYSTEM, AUTH_SEQ_RESP, STATUS_SUCCESS);

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

							}

							// Finish the function
							goto mpdu_rx_process_end;

						} else {
							// Create a unsuccessful authentication response frame
							curr_tx_queue_element = queue_checkout();

							if(curr_tx_queue_element != NULL){
								curr_tx_queue_buffer = (tx_queue_buffer*)(curr_tx_queue_element->data);

								// Setup the TX header
								wlan_mac_high_setup_tx_header( &tx_header_common, rx_80211_header->address_2, wlan_mac_addr );

								// Fill in the data
								tx_length = wlan_create_auth_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, AUTH_ALGO_OPEN_SYSTEM, AUTH_SEQ_RESP, STATUS_AUTH_REJECT_UNSPECIFIED);

								// Setup the TX frame info
								wlan_mac_high_setup_tx_frame_info ( &tx_header_common,
																	curr_tx_queue_element,
																	tx_length,
																	(TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO),
																	MANAGEMENT_QID );

								// Set the information in the TX queue buffer
								curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
								curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)(&default_unicast_mgmt_tx_params);
								curr_tx_queue_buffer->frame_info.AID         = 0;

								// Put the packet in the queue
								enqueue_after_tail(MANAGEMENT_QID, curr_tx_queue_element);

							}
						}

						// Finish the function
						goto mpdu_rx_process_end;
					}
				break;


				//---------------------------------------------------------------------
				case (MAC_FRAME_CTRL1_SUBTYPE_ASSOC_REQ):
				case (MAC_FRAME_CTRL1_SUBTYPE_REASSOC_REQ):
					// Association Request / Re-association Request
					//   - Check if the packet is for me
					//

					if((my_bss_info != NULL) && wlan_addr_eq(rx_80211_header->address_3, my_bss_info->bssid)) {

						// Check if we have authenticated this TA
						if (wlan_mac_high_find_station_info_ADDR(&station_info_state_2, rx_80211_header->address_2) != NULL){

							xil_printf("Authenticated, Unassociated Stations:\n");
							wlan_mac_high_remove_association(&station_info_state_2, &counts_table, rx_80211_header->address_2);

							// NOTE:  This function handles both the case that the station is already in the association
							//   table and the case that the association needs to be added to the association table
							//

							xil_printf("Authenticated, Associated Stations:\n");
							associated_station = wlan_mac_high_add_association(&my_bss_info->associated_stations, &counts_table, rx_80211_header->address_2, ADD_ASSOCIATION_ANY_AID);
							ap_update_hex_display(my_bss_info->associated_stations.length);
						}

						if(associated_station != NULL) {

							// Log the association state change
							add_station_info_to_log(associated_station, STATION_INFO_ENTRY_NO_CHANGE, WLAN_EXP_STREAM_ASSOC_CHANGE);

							// Create a successful association response frame
							curr_tx_queue_element = queue_checkout();

							if(curr_tx_queue_element != NULL){
								curr_tx_queue_buffer = (tx_queue_buffer*)(curr_tx_queue_element->data);

								// Setup the TX header
								wlan_mac_high_setup_tx_header( &tx_header_common, rx_80211_header->address_2, my_bss_info->bssid );

								// Fill in the data
								tx_length = wlan_create_association_response_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, STATUS_SUCCESS, associated_station->AID);

								// Setup the TX frame info
								wlan_mac_high_setup_tx_frame_info ( &tx_header_common,
																	curr_tx_queue_element,
																	tx_length,
																	(TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO ),
																	AID_TO_QID(associated_station->AID) );

								// Set the information in the TX queue buffer
								curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_STATION_INFO;
								curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)associated_station;
								curr_tx_queue_buffer->frame_info.AID         = associated_station->AID;

								// Put the packet in the queue
								enqueue_after_tail(AID_TO_QID(associated_station->AID), curr_tx_queue_element);

							}

							// Finish the function
							goto mpdu_rx_process_end;

						} else {
							// Create an unsuccessful association response frame
							curr_tx_queue_element = queue_checkout();

							if(curr_tx_queue_element != NULL){
								curr_tx_queue_buffer = (tx_queue_buffer*)(curr_tx_queue_element->data);

								// Setup the TX header
								wlan_mac_high_setup_tx_header( &tx_header_common, rx_80211_header->address_2, my_bss_info->bssid );

								// Fill in the data
								tx_length = wlan_create_association_response_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, STATUS_REJECT_TOO_MANY_ASSOCIATIONS, 0);

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

							}

							// Finish the function
							goto mpdu_rx_process_end;
						}
					}
				break;


				//---------------------------------------------------------------------
				case (MAC_FRAME_CTRL1_SUBTYPE_DISASSOC):
					// Disassociation
					//   - Log the assocation state change
					//   - Remove the assocation and update the display
					//
					if(associated_station != NULL){
						if ((associated_station->flags & STATION_INFO_DO_NOT_REMOVE) != STATION_INFO_DO_NOT_REMOVE) {
							// Log association state change
							add_station_info_to_log(associated_station, STATION_INFO_ENTRY_ZERO_AID, WLAN_EXP_STREAM_ASSOC_CHANGE);
						}
					}
					if(my_bss_info != NULL){
						xil_printf("Authenticated, Associated Stations:\n");
						wlan_mac_high_remove_association(&my_bss_info->associated_stations, &counts_table, rx_80211_header->address_2);

						ap_update_hex_display(my_bss_info->associated_stations.length);
					}
				break;

				case (MAC_FRAME_CTRL1_SUBTYPE_NULLDATA):
				break;

				//---------------------------------------------------------------------
				default:
					//This should be left as a verbose print. It occurs often when communicating with mobile devices since they tend to send
					//null data frames (type: DATA, subtype: 0x4) for power management reasons.
					wlan_printf(PL_VERBOSE, "Received unknown frame control type/subtype %x\n",rx_80211_header->frame_control_1);

				break;
			}
		}

		goto mpdu_rx_process_end;

	} else {
		// Process any Bad FCS packets
		goto mpdu_rx_process_end;
	}

	// Finish any processing for the RX MPDU process
	mpdu_rx_process_end:

	// Currently, asynchronous transmission of log entries is not supported
	//
	// if (rx_event_log_entry != NULL) {
    //     wn_transmit_log_entry((void *)rx_event_log_entry);
	// }

	return;
}



/**
 * @brief Reset Station Counts
 *
 * Reset all counts being kept for all stations
 *
 * @param  None
 * @return None
 */
void reset_station_counts(){
	wlan_mac_high_reset_counts(&counts_table);
}


/**
 * @brief Deauthenticate given station in the Association Table
 *
 * Deauthenticate the station in the associations table
 *
 * @param  station_info * station
 *   - Station to be deauthenticated
 * @return u32 aid
 *   - AID of the station that was deauthenticated; AID of 0 is reserved to indicate failure
 */
u32  deauthenticate_station( station_info* station ) {

	tx_queue_element*		curr_tx_queue_element;
	tx_queue_buffer* 		curr_tx_queue_buffer;
	u32            tx_length;
	u32            aid;

	if((my_bss_info == NULL) || (station == NULL)){
		return 0;
	}

	// Get the AID
	aid = station->AID;

	// Send De-authentication packet
	curr_tx_queue_element = queue_checkout();

	if(curr_tx_queue_element != NULL){
		curr_tx_queue_buffer = (tx_queue_buffer*)(curr_tx_queue_element->data);

		// Setup the TX header
		wlan_mac_high_setup_tx_header( &tx_header_common, station->addr, wlan_mac_addr );

		// Fill in the data
		tx_length = wlan_create_deauth_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, DEAUTH_REASON_INACTIVITY);

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

 		// Purge any packets in the queue meant for this node
		purge_queue(AID_TO_QID(aid));
	}

	// Log the association state change
	add_station_info_to_log(station, STATION_INFO_ENTRY_ZERO_AID, WLAN_EXP_STREAM_ASSOC_CHANGE);

	// Remove this STA from association list
	xil_printf("Authenticated, Associated Stations:\n");
	wlan_mac_high_remove_association(&my_bss_info->associated_stations, &counts_table, station->addr);

	ap_update_hex_display(my_bss_info->associated_stations.length);

	return aid;
}



/**
 * @brief Deauthenticate all stations in the Association Table
 *
 * Loop through all associations in the table and deauthenticate the stations
 *
 * @param  None
 * @return None
 */
void deauthenticate_all_stations(){
	interrupt_state_t   curr_interrupt_state;
	station_info*	curr_station_info;
	dl_entry*		next_station_info_entry;
	dl_entry*		curr_station_info_entry;
	int 			iter;

	if(my_bss_info == NULL) return;

	iter = my_bss_info->associated_stations.length;
	curr_interrupt_state = wlan_mac_high_interrupt_stop();

	next_station_info_entry = my_bss_info->associated_stations.first;

	// Deauthenticate all stations and remove from the association table
	//
	// NOTE:  Cannot use a for loop for this iteration b/c we could remove
	//   elements from the list.
	while( (next_station_info_entry != NULL) && (iter-- > 0)){
		curr_station_info_entry = next_station_info_entry;
		next_station_info_entry = dl_entry_next(curr_station_info_entry);
		curr_station_info = (station_info*)(curr_station_info_entry->data);
		deauthenticate_station(curr_station_info);
	}

	wlan_mac_high_interrupt_restore_state(curr_interrupt_state);

}

u32	configure_bss(bss_config_t* bss_config){
	u32					return_status 				= 0;
	u8					zero_addr[6] 				= {0, 0, 0, 0, 0, 0};
	u8					update_beacon_template 		= 0;
	u8					send_beacon_config_to_low 	= 0;
	u8					send_channel_switch_to_low	= 0;

	bss_info*			local_bss_info;
	interrupt_state_t   curr_interrupt_state;
	station_info* 		curr_station_info;
	dl_entry* 			next_station_info_entry;
	dl_entry* 			curr_station_info_entry;
	int					iter;

	//---------------------------------------------------------
	// 1. Check for any invalid inputs or combination of inputs
	//		We will first verify that every requested update to the BSS
	//		configuration before modifying anything. This will prevent
	//		a partial update of BSS configuration with valid parameters
	//		before discovering an invalid parameter.

	if(bss_config != NULL){
		if( bss_config->update_mask & BSS_FIELD_MASK_BSSID ){
			if( wlan_addr_eq(bss_config->bssid,zero_addr) == 0 ){
				if( wlan_addr_eq(bss_config->bssid,wlan_mac_addr) == 0 ){
					// In the AP implementation, the BSSID provided must be
					// the hardware MAC address of the node
					return_status |= BSS_CONFIG_FAILURE_BSSID_INVALID;
				}
				if( ((bss_config->update_mask & BSS_FIELD_MASK_SSID) == 0) ||
					((bss_config->update_mask & BSS_FIELD_MASK_CHAN) == 0) ||
					((bss_config->update_mask & BSS_FIELD_MASK_BEACON_INTERVAL) == 0) ){
					return_status |= BSS_CONFIG_FAILURE_BSSID_INSUFFICIENT_ARGUMENTS;
				}
			}
		}
		if( bss_config->update_mask & BSS_FIELD_MASK_CHAN ){
			if( wlan_verify_channel(bss_config->chan) != XST_SUCCESS ){
				return_status |= BSS_CONFIG_FAILURE_CHANNEL_INVALID;
			}
		}
		if( bss_config->update_mask & BSS_FIELD_MASK_BEACON_INTERVAL ){
			if( (bss_config->beacon_interval != BEACON_INTERVAL_NO_BEACON_TX) &&
				(bss_config->beacon_interval <  10)){
				return_status |= BSS_CONFIG_FAILURE_BEACON_INTERVAL_INVALID;
			}
		}
		if(bss_config->update_mask & BSS_FIELD_MASK_HT_CAPABLE){
			if( (bss_config->ht_capable > 1) ){
				return_status |= BSS_CONFIG_FAILURE_HT_CAPABLE_INVALID;
			}
		}
	}

	if(return_status == 0){
		//---------------------------------------------------------
		// 2. Apply BSS configuration changes
		//		Now that we have verified that the provided bss_config_t struct
		//		is valid, we can apply those changes.

		// We should disable interrupts around these modifications to prevent
		// state changing out from underneath this context while the new BSS
		// configuration parameters are only partially updated.
		curr_interrupt_state = wlan_mac_high_interrupt_stop();
		if( (bss_config == NULL)||(bss_config->update_mask & BSS_FIELD_MASK_BSSID) ){
			// We are adopting a new BSSID. This could mean either
			//    1) We are shutting the BSS down
			// or 2) We are shutting the BSS down and then starting
			//		 a new BSS.
			// In either case, we must first remove any station_info structs
			// that are members of the current my_bss_info and return to
			// a NULL my_bss_info state.

			if(my_bss_info != NULL){
				next_station_info_entry = my_bss_info->associated_stations.first;
				iter = my_bss_info->associated_stations.length;

				while( (next_station_info_entry != NULL) && (iter-- > 0) ){
					curr_station_info_entry = next_station_info_entry;
					next_station_info_entry = dl_entry_next(curr_station_info_entry);
					curr_station_info = (station_info*)(curr_station_info_entry->data);
		            purge_queue(AID_TO_QID(curr_station_info->AID));
					wlan_mac_high_remove_association( &my_bss_info->associated_stations, &counts_table, curr_station_info->addr );
					ap_update_hex_display(my_bss_info->associated_stations.length);
				}

				//Inform the MAC High Framework that we no longer will keep this BSS Info. This will
				//Allow it to be overwritten in the future to make space for new BSS Infos.
				my_bss_info->flags &= ~BSS_FLAGS_KEEP;

				my_bss_info = NULL;

				//Disable beacons immediately.
				gl_beacon_txrx_config.beacon_tx_mode = NO_BEACON_TX;
				bzero(gl_beacon_txrx_config.bssid_match, 6);
				wlan_mac_high_config_txrx_beacon(&gl_beacon_txrx_config);
			}

			// We have used bss_config == NULL as a shortcut for removing the BSS state of this node. This operation
			// was executed just above this comment. Rather that continuing to check non-NULLness of bss_config
			// throughout the rest of this function, we can just re-enable interrupts and return early.

			if(bss_config == NULL){
				wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
				return return_status;
			}

			// my_bss_info is guaranteed to be NULL at this point in the code

			if(wlan_addr_eq(bss_config->bssid,zero_addr) == 0 ){
				//We should create a new bss_info or overwrite an existing one with matching BSSID.
				//Note: the wildcard SSID and 0-valued channel arguments are temporary. Because of
				//the error checking at the top of this function, we can rely on the fact that
				//bss_config contains a valid SSID as well as channel. These fields will be updated
				//next.
				local_bss_info = wlan_mac_high_create_bss_info(bss_config->bssid, "", 0);
				if(local_bss_info != NULL){
					local_bss_info->flags |= BSS_FLAGS_KEEP;
					local_bss_info->capabilities = (CAPABILITIES_SHORT_TIMESLOT | CAPABILITIES_ESS);
					my_bss_info = local_bss_info;
				}
			}
		}
		//---------------------------------------------------------
		// 3. Clean up
		//		Now that my_bss_info has been updated, we can communicate
		//		those changes to CPU_LOW so that we tune to the correct channel,
		//		send beacons at the correct interval, and update the beacon
		//		template packet buffer.
		if(my_bss_info != NULL){
			if(bss_config->update_mask & BSS_FIELD_MASK_CHAN){
				my_bss_info->chan = bss_config->chan;
				send_channel_switch_to_low = 1;
				update_beacon_template = 1;
			}
			if(bss_config->update_mask & BSS_FIELD_MASK_SSID){
				strcpy(my_bss_info->ssid, bss_config->ssid);
				update_beacon_template = 1;
			}
			if(bss_config->update_mask & BSS_FIELD_MASK_BEACON_INTERVAL){
				my_bss_info->beacon_interval = bss_config->beacon_interval;
				update_beacon_template = 1;
				send_beacon_config_to_low = 1;
			}
			if(bss_config->update_mask & BSS_FIELD_MASK_HT_CAPABLE){
				//TODO:
				// 1) Update Beacon Template Capabilities
				// 2) Update existing MCS selections for defaults and
				//	  associated stations?
			}
			if( update_beacon_template ){
				//We need to update the beacon template. In the event that CPU_LOW currently has it locked,
				//we will just block for now until it unlocks. A TODO is for this is to schedule a future
				//update of the beacon template.
				wlan_mac_high_setup_tx_header( &tx_header_common, (u8 *)bcast_addr, my_bss_info->bssid );
				while( wlan_mac_high_configure_beacon_tx_template( &tx_header_common, my_bss_info, &default_multicast_mgmt_tx_params, TX_MPDU_FLAGS_FILL_TIMESTAMP | TX_MPDU_FLAGS_REQ_BO ) != 0 ){}
			}
			if( send_channel_switch_to_low ){
				wlan_mac_high_set_channel(my_bss_info->chan);
			}
			if( send_beacon_config_to_low ){
				memcpy(gl_beacon_txrx_config.bssid_match, my_bss_info->bssid, 6);
				if(my_bss_info->beacon_interval == BEACON_INTERVAL_NO_BEACON_TX){
					gl_beacon_txrx_config.beacon_tx_mode = NO_BEACON_TX;
				} else {
					gl_beacon_txrx_config.beacon_tx_mode = AP_BEACON_TX;
				}
				gl_beacon_txrx_config.beacon_interval_tu = my_bss_info->beacon_interval;
				gl_beacon_txrx_config.beacon_template_pkt_buf = TX_PKT_BUF_BEACON;
				wlan_mac_high_config_txrx_beacon(&gl_beacon_txrx_config);
			}

			xil_printf("BSS Details: \n");
			xil_printf("  BSSID           : %02x-%02x-%02x-%02x-%02x-%02x\n",my_bss_info->bssid[0],my_bss_info->bssid[1],my_bss_info->bssid[2],my_bss_info->bssid[3],my_bss_info->bssid[4],my_bss_info->bssid[5]);
			xil_printf("   SSID           : %s\n", my_bss_info->ssid);
			xil_printf("   Channel        : %d\n", my_bss_info->chan);
			xil_printf("   Beacon Interval: %d TU (%d us)\n",my_bss_info->beacon_interval, my_bss_info->beacon_interval*1024);

		}
		wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
	}



	return return_status;
}



void mpdu_dequeue(tx_queue_element* packet){
	mac_header_80211* 	header;
	tx_frame_info*		frame_info;
	u32 				packet_payload_size;
	dl_entry*			curr_station_entry;
	station_info*		curr_station;

	header 	  			= (mac_header_80211*)((((tx_queue_buffer*)(packet->data))->frame));
	frame_info 			= (tx_frame_info*)&((((tx_queue_buffer*)(packet->data))->frame_info));
	packet_payload_size	= frame_info->length;


	switch(wlan_mac_high_pkt_type(header, packet_payload_size)){
		case PKT_TYPE_DATA_ENCAP_ETH:
			if(my_bss_info != NULL){
				curr_station_entry = wlan_mac_high_find_station_info_AID(&(my_bss_info->associated_stations), frame_info->AID);
				if(curr_station_entry != NULL){
					curr_station = (station_info*)(curr_station_entry->data);
					if(queue_num_queued(AID_TO_QID(curr_station->AID)) > 1){
						//If the is more data (in addition to this packet) queued for this station, we can let it know
						//in the frame_control_2 field.
						header->frame_control_2 |= MAC_FRAME_CTRL2_FLAG_MORE_DATA;
					} else {
						header->frame_control_2 = (header->frame_control_2) & ~MAC_FRAME_CTRL2_FLAG_MORE_DATA;
					}
				}
			}
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
	if(my_bss_info != NULL){
		return &(my_bss_info->associated_stations);
	} else {
		return NULL;
	}
}
dl_list * get_counts()           { return &counts_table;   }
u8      * get_wlan_mac_addr()    { return (u8 *)&wlan_mac_addr;      }



/**
 * @brief AP specific hex display update command
 *
 * This function update the hex display for the AP.  In general, this function
 * is a wrapper for standard hex display commands found in wlan_mac_misc_util.c.
 * However, this wrapper was implemented so that it would be easy to do other
 * actions when the AP needed to update the hex display.
 *
 * @param   val              - Value to be displayed (between 0 and 99)
 * @return  None
 */
void ap_update_hex_display(u8 val) {

    // Use standard hex display write with PWMs enabled
    write_hex_display_with_pwm(val);
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
            wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown AP user command: 0x%x\n", cmd_id);
        }
        break;
    }

    return resp_sent;
}


#endif

