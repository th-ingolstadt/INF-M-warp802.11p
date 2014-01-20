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
#include "wlan_mac_sta.h"
#include "ascii_characters.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_dl_list.h"


// WLAN Exp includes
#include "wlan_exp_common.h"
#include "wlan_exp_node.h"
#include "wlan_exp_node_sta.h"
#include "wlan_exp_transport.h"


/*************************** Constant Definitions ****************************/

#define  WLAN_EXP_ETH                  WN_ETH_B
#define  WLAN_EXP_TYPE                 WARPNET_TYPE_80211_BASE + WARPNET_TYPE_80211_STATION

#define  WLAN_CHANNEL                  4
#define  TX_GAIN_TARGET				   45


/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/

// If you want this station to try to associate to a known AP at boot, type
//   the string here. Otherwise, let it be an empty string.
static char default_AP_SSID[] = "WARP-AP";
char*  access_point_ssid;

// Common TX header for 802.11 packets
mac_header_80211_common tx_header_common;


// Control variables
u8  default_unicast_rate;
u8  default_tx_gain_target;
int association_state;                      // Section 10.3 of 802.11-2012
u8  uart_mode;
u8  active_scan;

u8  repeated_active_scan_scheduled;
u32 active_scan_schedule_id;
u8  pause_queue;


// Access point information
ap_info* ap_list;
u8       num_ap_list;

u8       access_point_num_basic_rates;
u8       access_point_basic_rates[NUM_BASIC_RATES_MAX];


// Association Table variables
//   The last entry in associations[MAX_ASSOCIATIONS][] is swap space
station_info access_point;
statistics	 access_point_stat;

u32			 max_queue_size;
#define		 MAX_PER_FLOW_QUEUE	150


// AP channel
u32 mac_param_chan;
u32 mac_param_chan_save;


// AP MAC address / Broadcast address
static u8 eeprom_mac_addr[6];
static u8 bcast_addr[6]      = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };


/*************************** Functions Prototypes ****************************/


#ifdef WLAN_USE_UART_MENU

void uart_rx(u8 rxByte);

#else

void uart_rx(u8 rxByte){ };

#endif



/******************************** Functions **********************************/

int main(){

	//This function should be executed first. It will zero out memory, and if that
	//memory is used before calling this function, unexpected results may happen.
	wlan_mac_high_heap_init();

    // Initialize AP list
	num_ap_list = 0;
	//free(ap_list);
	ap_list = NULL;
	repeated_active_scan_scheduled = 0;

	max_queue_size = MAX_PER_FLOW_QUEUE;

	//Unpause the queue
	pause_queue = 0;

	xil_printf("\f----- wlan_mac_sta -----\n");
	xil_printf("Compiled %s %s\n", __DATE__, __TIME__);


	//xil_printf("_heap_start = 0x%x, %x\n", *(char*)(_heap_start),_heap_start);
	//xil_printf("_heap_end = 0x%x, %x\n", *(char*)(_heap_end),_heap_end);

    // Set Global variables
	default_unicast_rate = WLAN_MAC_RATE_18M;
	default_tx_gain_target = TX_GAIN_TARGET;

	// Initialize the utility library
    wlan_mac_high_init();
#ifdef USE_WARPNET_WLAN_EXP
	//node_info_set_max_assn( 1 );
	wlan_mac_exp_configure(WLAN_EXP_TYPE, WLAN_EXP_ETH);
#endif


	// Initialize callbacks
	wlan_mac_util_set_eth_rx_callback(       (void*)ethernet_receive);
	wlan_mac_high_set_mpdu_tx_done_callback( (void*)mpdu_transmit_done);
	wlan_mac_high_set_mpdu_rx_callback(      (void*)mpdu_rx_process);
	wlan_mac_high_set_uart_rx_callback(      (void*)uart_rx);
	wlan_mac_high_set_mpdu_accept_callback(  (void*)check_tx_queue);
	wlan_mac_ltg_sched_set_callback(         (void*)ltg_event);

	wlan_mac_util_set_eth_encap_mode(ENCAP_MODE_STA);


    // Initialize interrupts
	wlan_mac_high_interrupt_init();


	// Initialize Association Table
	bzero(&(access_point), sizeof(station_info));

	access_point.AID = 0; //7.3.1.8 of 802.11-2007
	memset((void*)(&(access_point.addr[0])), 0xFF,6);

	access_point.stats = &access_point_stat;

	access_point.rx.last_seq = 0; //seq
	access_point.rx.last_timestamp = 0;

	// Set default SSID for AP
	access_point_ssid = wlan_mac_high_malloc(strlen(default_AP_SSID)+1);
	strcpy(access_point_ssid,default_AP_SSID);


	// Set Association state for station to AP
	association_state = 1;

    // Wait for CPU Low to initialize
	while( wlan_mac_high_is_cpu_low_initialized() == 0){
		xil_printf("waiting on CPU_LOW to boot\n");
	};


	// CPU Low will pass HW information to CPU High as part of the boot process
	//   - Get necessary HW information
	memcpy((void*) &(eeprom_mac_addr[0]), (void*) wlan_mac_high_get_eeprom_mac_addr(), 6);


    // Set Header information
	tx_header_common.address_2 = &(eeprom_mac_addr[0]);
	tx_header_common.seq_num = 0;


    // Initialize hex display
	wlan_mac_high_write_hex_display(0);


	// Set up channel
	mac_param_chan = WLAN_CHANNEL;
	mac_param_chan_save = mac_param_chan;
	wlan_mac_high_set_channel( mac_param_chan );


	// Print Station information to the terminal
    xil_printf("WLAN MAC Station boot complete: \n");
    xil_printf("  Default SSID : %s \n", access_point_ssid);
    xil_printf("  Channel      : %d \n", mac_param_chan);
	xil_printf("  MAC Addr     : %x-%x-%x-%x-%x-%x\n\n",eeprom_mac_addr[0],eeprom_mac_addr[1],eeprom_mac_addr[2],eeprom_mac_addr[3],eeprom_mac_addr[4],eeprom_mac_addr[5]);


#ifdef WLAN_USE_UART_MENU
	uart_mode = UART_MODE_MAIN;

	xil_printf("\nAt any time, press the Esc key in your terminal to access the AP menu\n");
#endif


	// If there is a default SSID, initiate a probe request
	if( strlen(access_point_ssid) > 0 ) start_active_scan();

#ifdef USE_WARPNET_WLAN_EXP
	// Set AP processing callbacks
	node_set_process_callback( (void *)wlan_exp_node_sta_processCmd );
#endif


	wlan_mac_high_interrupt_start();


	while(1){
		//The design is entirely interrupt based. When no events need to be processed, the processor
		//will spin in this loop until an interrupt happens

#ifdef USE_WARPNET_WLAN_EXP
		wlan_mac_high_interrupt_stop();
		transport_poll( WLAN_EXP_ETH );
		wlan_mac_high_interrupt_start();
#endif
	}
	return -1;
}




void check_tx_queue(){

	u8 i;

	if(pause_queue == 0){
		static u32 queue_index = 0;
		if( wlan_mac_high_is_cpu_low_ready() ){
			for(i=0;i<2;i++){
				//Alternate between checking the unassociated queue and the associated queue
				queue_index = (queue_index+1)%2;
				if(wlan_mac_queue_poll(queue_index)){
					return;
				}
			}
		}
	}

}



void mpdu_transmit_done(tx_frame_info* tx_mpdu){
	tx_entry* tx_event_log_entry;

	void * mpdu = (void*)tx_mpdu + PHY_TX_PKT_BUF_MPDU_OFFSET;
	u8* mpdu_ptr_u8 = (u8*)mpdu;
	mac_header_80211* tx_80211_header;
	tx_80211_header = (mac_header_80211*)((void *)mpdu_ptr_u8);



	tx_event_log_entry = get_next_empty_tx_entry();

	if(tx_event_log_entry != NULL){
		wlan_mac_high_cdma_start_transfer((&((tx_entry*)tx_event_log_entry)->mac_hdr), tx_80211_header, sizeof(mac_header_80211));
		tx_event_log_entry->result                   = tx_mpdu->state_verbose;
		tx_event_log_entry->gain_target              = tx_mpdu->gain_target;
		tx_event_log_entry->length                   = tx_mpdu->length;
		tx_event_log_entry->rate                     = tx_mpdu->rate;
		tx_event_log_entry->gain_target				 = tx_mpdu->gain_target;
		tx_event_log_entry->chan_num				 = mac_param_chan;
		tx_event_log_entry->pkt_type				 = wlan_mac_high_pkt_type(mpdu,tx_mpdu->length);
		tx_event_log_entry->retry_count              = tx_mpdu->retry_count;
		tx_event_log_entry->timestamp_create         = tx_mpdu->timestamp_create;
		tx_event_log_entry->delay_accept             = tx_mpdu->delay_accept;
		tx_event_log_entry->delay_done               = tx_mpdu->delay_done;
		tx_event_log_entry->ant_mode				 = 0; //TODO;
	}

	wlan_mac_high_process_tx_done(tx_mpdu, &(access_point));
}




void attempt_association(){
	//It is assumed that the global "access_point" has a valid BSSID (MAC Address).
	//This function should only be called after selecting an access point through active scan

	static u8      curr_try = 0;
	u16            tx_length;
	dl_list checkout;
	packet_bd*	   tx_queue;

	switch(association_state){

		case 1:
			//Initial start state, unauthenticated, unassociated
			//Checkout 1 element from the queue;
			curr_try = 0;
		break;

		case 2:
			//Authenticated, not associated
			curr_try = 0;
			//Checkout 1 element from the queue;
			queue_checkout(&checkout,1);
			if(checkout.length == 1){ //There was at least 1 free queue element
				tx_queue = (packet_bd*)(checkout.first);

				wlan_mac_high_setup_tx_header( &tx_header_common, access_point.addr, access_point.addr );

				tx_length = wlan_create_association_req_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame, &tx_header_common, (u8)strlen(access_point_ssid), (u8*)access_point_ssid, access_point_num_basic_rates, access_point_basic_rates);

		 		wlan_mac_high_setup_tx_queue ( tx_queue, NULL, tx_length, MAX_RETRY, TX_GAIN_TARGET,
		 				         (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO) );

				enqueue_after_end(0, &checkout);
				check_tx_queue();
			}
			if( curr_try < (ASSOCIATION_NUM_TRYS - 1) ){
				wlan_mac_schedule_event(SCHEDULE_COARSE, ASSOCIATION_TIMEOUT_US, (void*)attempt_association);
				curr_try++;
			} else {
				curr_try = 0;
				if( strlen(access_point_ssid) > 0 ) start_active_scan();
			}

		break;

		case 3:
			//Authenticated and associated (Pending RSN Authentication)
			//Not-applicable for current 802.11 Reference Design
			curr_try = 0;
		break;

		case 4:
			//Authenticated and associated
			curr_try = 0;

		break;
	}

	return;
}




void attempt_authentication(){
	//It is assumed that the global "access_point" has a valid BSSID (MAC Address).
	//This function should only be called after selecting an access point through active scan

	static u8      curr_try = 0;
	u16            tx_length;
	dl_list checkout;
	packet_bd*	   tx_queue;

	switch(association_state){

		case 1:
			//Initial start state, unauthenticated, unassociated
			//Checkout 1 element from the queue;
			queue_checkout(&checkout,1);
			if(checkout.length == 1){ //There was at least 1 free queue element
				tx_queue = (packet_bd*)(checkout.first);

				wlan_mac_high_setup_tx_header( &tx_header_common, access_point.addr, access_point.addr );

				tx_length = wlan_create_auth_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame, &tx_header_common, AUTH_ALGO_OPEN_SYSTEM, AUTH_SEQ_REQ, STATUS_SUCCESS);

		 		wlan_mac_high_setup_tx_queue ( tx_queue, NULL, tx_length, MAX_RETRY, TX_GAIN_TARGET,
		 				         (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO) );

				enqueue_after_end(0, &checkout);
				check_tx_queue();
			}
			if( curr_try < (AUTHENTICATION_NUM_TRYS - 1) ){
				wlan_mac_schedule_event(SCHEDULE_COARSE, AUTHENTICATION_TIMEOUT_US, (void*)attempt_authentication);
				curr_try++;
			} else {
				curr_try = 0;
				if( strlen(access_point_ssid) > 0 ) start_active_scan();
			}


		break;

		case 2:
			//Authenticated, not associated
			curr_try = 0;
		break;

		case 3:
			//Authenticated and associated (Pending RSN Authentication)
			//Not-applicable for current 802.11 Reference Design
			curr_try = 0;
		break;

		case 4:
			//Authenticated and associated
			curr_try = 0;

		break;

	}

	return;
}


void start_active_scan(){
	//Purge any knowledge of existing APs
	stop_active_scan();
	xil_printf("Starting active scan\n");
	num_ap_list = 0;
	wlan_mac_high_free(ap_list);
	ap_list = NULL;
	association_state = 1;
	active_scan = 1;
	repeated_active_scan_scheduled = 1;
	active_scan_schedule_id = wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, ACTIVE_SCAN_UPDATE_RATE, SCHEDULE_REPEAT_FOREVER, (void*)probe_req_transmit);
	probe_req_transmit();
}

void stop_active_scan(){
	xil_printf("Stopping active scan\n");
	if(repeated_active_scan_scheduled) wlan_mac_remove_schedule(SCHEDULE_COARSE, active_scan_schedule_id);
	active_scan = 0;
	repeated_active_scan_scheduled = 0;
}


void probe_req_transmit(){
	u32 i;

	static u8 curr_channel_index = 0;
	u16 tx_length;
	dl_list checkout;
	packet_bd*	tx_queue;

	mac_param_chan = curr_channel_index + 1; //+1 is to shift [0,10] index to [1,11] channel number

	//xil_printf("+++ probe_req_transmit mac_param_chan = %d\n", mac_param_chan);

	//Send a message to other processor to tell it to switch channels
	wlan_mac_high_set_channel( mac_param_chan );

	//Send probe request

	//xil_printf("Probe Req SSID: %s, Len: %d\n",access_point_ssid, strlen(access_point_ssid));

	for(i = 0; i<NUM_PROBE_REQ; i++){
	//Checkout 1 element from the queue;
	queue_checkout(&checkout,1);
		if(checkout.length == 1){ //There was at least 1 free queue element
			tx_queue = (packet_bd*)(checkout.first);

			wlan_mac_high_setup_tx_header( &tx_header_common, bcast_addr, bcast_addr );

			tx_length = wlan_create_probe_req_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame,&tx_header_common, strlen(access_point_ssid), (u8*)access_point_ssid, mac_param_chan);

	 		wlan_mac_high_setup_tx_queue ( tx_queue, NULL, tx_length, 0, TX_GAIN_TARGET, 0 );

			enqueue_after_end(0, &checkout);
			check_tx_queue();
		}
	}

	curr_channel_index = (curr_channel_index+1)%11;

	if(curr_channel_index > 0){
		wlan_mac_schedule_event(SCHEDULE_COARSE, ACTIVE_SCAN_DWELL, (void*)probe_req_transmit);
	} else {
		wlan_mac_schedule_event(SCHEDULE_COARSE, ACTIVE_SCAN_DWELL, (void*)print_ap_list);
	}
}





int ethernet_receive(dl_list* tx_queue_list, u8* eth_dest, u8* eth_src, u16 tx_length){

	if(access_point.AID > 0){
		//Receives the pre-encapsulated Ethernet frames
		packet_bd* tx_queue = (packet_bd*)(tx_queue_list->first);

		wlan_mac_high_setup_tx_header( &tx_header_common, (u8*)access_point.addr,(u8*)(&(eth_dest[0])));

		wlan_create_data_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame, &tx_header_common, MAC_FRAME_CTRL2_FLAG_TO_DS);

		if(wlan_addr_eq(bcast_addr, eth_dest)){
			if(queue_num_queued(0) < max_queue_size){
				wlan_mac_high_setup_tx_queue ( tx_queue, NULL, tx_length, 0, TX_GAIN_TARGET, 0 );

				enqueue_after_end(0, tx_queue_list);
				check_tx_queue();
			} else {
				return 0;
			}

		} else {

			if(access_point.AID != 0){

				if(queue_num_queued(1) < max_queue_size){

					wlan_mac_high_setup_tx_queue ( tx_queue, (void*)&(access_point), tx_length, MAX_RETRY, TX_GAIN_TARGET,
									 (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO) );

					enqueue_after_end(1, tx_queue_list);
					check_tx_queue();
				} else {
					return 0;
				}
			}

		}

		return 1;
	} else {
		//STA is not currently associated, so we won't send any eth frames
		return 0;
	}
}




void mpdu_rx_process(void* pkt_buf_addr, u8 rate, u16 length) {
	u32 i;
	void * mpdu = pkt_buf_addr + PHY_RX_PKT_BUF_MPDU_OFFSET;
	u8* mpdu_ptr_u8 = (u8*)mpdu;

	ap_info* curr_ap_info = NULL;
	char* ssid;

	mac_header_80211* rx_80211_header;
	rx_80211_header = (mac_header_80211*)((void *)mpdu_ptr_u8);
	u16 rx_seq;

	rx_common_entry* rx_event_log_entry;

	rx_frame_info* mpdu_info = (rx_frame_info*)pkt_buf_addr;

	u8 is_associated = 0;


	//*************
	// Event logging
	//*************
	if(rate != WLAN_MAC_RATE_1M){
		rx_event_log_entry = (rx_common_entry*)get_next_empty_rx_ofdm_entry();
	} else {
		rx_event_log_entry = (rx_common_entry*)get_next_empty_rx_dsss_entry();
	}
	if(rx_event_log_entry != NULL){
		wlan_mac_high_cdma_start_transfer(&(rx_event_log_entry->mac_hdr), rx_80211_header, sizeof(mac_header_80211));
		rx_event_log_entry->fcs_status = (mpdu_info->state == RX_MPDU_STATE_FCS_GOOD) ? RX_ENTRY_FCS_GOOD : RX_ENTRY_FCS_BAD;
		rx_event_log_entry->timestamp  =  mpdu_info->timestamp;
		rx_event_log_entry->power      = mpdu_info->rx_power;
		rx_event_log_entry->rf_gain    = mpdu_info->rf_gain;
		rx_event_log_entry->bb_gain    = mpdu_info->bb_gain;
		rx_event_log_entry->length     = mpdu_info->length;
		rx_event_log_entry->rate       = mpdu_info->rate;
		rx_event_log_entry->pkt_type   = wlan_mac_high_pkt_type(mpdu,length);
		rx_event_log_entry->chan_num   = mac_param_chan;
		rx_event_log_entry->ant_mode   = mpdu_info->ant_mode;
#ifdef WLAN_MAC_ENTRIES_LOG_CHAN_EST
		if(rate != WLAN_MAC_RATE_1M) wlan_mac_high_cdma_start_transfer(((rx_ofdm_entry*)rx_event_log_entry)->channel_est, mpdu_info->channel_est, sizeof(mpdu_info->channel_est));
#endif
	}

	if(mpdu_info->state == RX_MPDU_STATE_FCS_GOOD){
		if(wlan_addr_eq(access_point.addr, (rx_80211_header->address_2))) {
			is_associated = 1;

			rx_seq = ((rx_80211_header->sequence_control)>>4)&0xFFF;
			//Check if duplicate
			access_point.rx.last_timestamp = get_usec_timestamp();
			access_point.rx.last_power = mpdu_info->rx_power;
			access_point.rx.last_rate = mpdu_info->rate;

			//xil_printf("%d ? %d\n", access_point.rx.last_seq, rx_seq);

			if( (access_point.rx.last_seq != 0)  && (access_point.rx.last_seq == rx_seq) ) {
				//Received seq num matched previously received seq num for this STA; ignore the MPDU and return
				return;

			} else {
				access_point.rx.last_seq = rx_seq;
			}
		}


		switch(rx_80211_header->frame_control_1) {
		case (MAC_FRAME_CTRL1_SUBTYPE_DATA): //Data Packet
			if(is_associated){
				if((rx_80211_header->frame_control_2) & MAC_FRAME_CTRL2_FLAG_FROM_DS) {
					//MPDU is flagged as destined to the DS - send it for de-encapsulation and Ethernet Tx (if appropriate)

					if(wlan_addr_eq(bcast_addr, rx_80211_header->address_1) == 0){
						//We only count the receive statistics of unicast wireless packets
					    (access_point.stats->num_rx_success)++;
						(access_point.stats->num_rx_bytes) += mpdu_info->length;
					}

					wlan_mpdu_eth_send(mpdu,length);
				}
			}
			break;

			case (MAC_FRAME_CTRL1_SUBTYPE_ASSOC_RESP): //Association response
				if(association_state == 2){
					mpdu_ptr_u8 += sizeof(mac_header_80211);

					if(((association_response_frame*)mpdu_ptr_u8)->status_code == STATUS_SUCCESS){
						association_state = 4;
						access_point.AID = (((association_response_frame*)mpdu_ptr_u8)->association_id)&~0xC000;
						wlan_mac_high_write_hex_display(access_point.AID);
						access_point.tx.rate = default_unicast_rate;
						xil_printf("Association succeeded\n");
					} else {
						association_state = -1;
						xil_printf("Association failed, reason code %d\n", ((association_response_frame*)mpdu_ptr_u8)->status_code);
					}
				}

			break;

			case (MAC_FRAME_CTRL1_SUBTYPE_AUTH): //Authentication
					if(association_state == 1 && wlan_addr_eq(rx_80211_header->address_3, access_point.addr) && wlan_addr_eq(rx_80211_header->address_1, eeprom_mac_addr)) {
						mpdu_ptr_u8 += sizeof(mac_header_80211);
						switch(((authentication_frame*)mpdu_ptr_u8)->auth_algorithm){
							case AUTH_ALGO_OPEN_SYSTEM:
								if(((authentication_frame*)mpdu_ptr_u8)->auth_sequence == AUTH_SEQ_RESP){//This is an auth response
									if(((authentication_frame*)mpdu_ptr_u8)->status_code == STATUS_SUCCESS){
										//AP is letting us authenticate
										association_state = 2;
										attempt_association();
									}
									return;
								}
							break;
						}
					}

			break;

			case (MAC_FRAME_CTRL1_SUBTYPE_DEAUTH): //Deauthentication
					if(wlan_addr_eq(rx_80211_header->address_1, eeprom_mac_addr)){
						access_point.AID = 0;
						wlan_mac_high_write_hex_display(access_point.AID);
						//memset((void*)(&(access_point.addr[0])), 0xFF,6);
						access_point.rx.last_seq = 0; //seq
						if( strlen(access_point_ssid) > 0 ) start_active_scan();
					}
			break;

			case (MAC_FRAME_CTRL1_SUBTYPE_BEACON): //Beacon Packet
			case (MAC_FRAME_CTRL1_SUBTYPE_PROBE_RESP): //Probe Response Packet

					if(active_scan){

					for (i=0;i<num_ap_list;i++){

						if(wlan_addr_eq(ap_list[i].bssid, rx_80211_header->address_3)){
							curr_ap_info = &(ap_list[i]);
							//xil_printf("     Matched at 0x%08x\n", curr_ap_info);
							break;
						}
					}

					if(curr_ap_info == NULL){

						if(ap_list == NULL){
							ap_list = wlan_mac_high_malloc(sizeof(ap_info)*(num_ap_list+1));
						} else {
							ap_list = wlan_mac_high_realloc(ap_list, sizeof(ap_info)*(num_ap_list+1));
						}

						if(ap_list != NULL){
							num_ap_list++;
							curr_ap_info = &(ap_list[num_ap_list-1]);
						} else {
							xil_printf("Reallocation of ap_list failed\n");
							return;
						}

					}

					curr_ap_info->rx_power = mpdu_info->rx_power;
					curr_ap_info->num_basic_rates = 0;

					//Copy BSSID into ap_info struct
					memcpy(curr_ap_info->bssid, rx_80211_header->address_3,6);

					mpdu_ptr_u8 += sizeof(mac_header_80211);
					if((((beacon_probe_frame*)mpdu_ptr_u8)->capabilities)&CAPABILITIES_PRIVACY){
						curr_ap_info->private = 1;
					} else {
						curr_ap_info->private = 0;
					}

					mpdu_ptr_u8 += sizeof(beacon_probe_frame);
					//xil_printf("\n");
					while(((u32)mpdu_ptr_u8 -  (u32)mpdu)<= length){ //Loop through tagged parameters
						switch(mpdu_ptr_u8[0]){ //What kind of tag is this?
							case TAG_SSID_PARAMS: //SSID parameter set
								ssid = (char*)(&(mpdu_ptr_u8[2]));


								memcpy(curr_ap_info->ssid, ssid ,min(mpdu_ptr_u8[1],SSID_LEN_MAX-1));
								//Terminate the string
								(curr_ap_info->ssid)[min(mpdu_ptr_u8[1],SSID_LEN_MAX-1)] = 0;

							break;
							case TAG_SUPPORTED_RATES: //Supported rates
								for(i=0;i < mpdu_ptr_u8[1]; i++){
									if(mpdu_ptr_u8[2+i]&RATE_BASIC){
										//This is a basic rate. It is required by the AP in order to associate.
										if((curr_ap_info->num_basic_rates) < NUM_BASIC_RATES_MAX){

											if(wlan_mac_high_valid_tagged_rate(mpdu_ptr_u8[2+i])){

												(curr_ap_info->basic_rates)[(curr_ap_info->num_basic_rates)] = mpdu_ptr_u8[2+i];
												(curr_ap_info->num_basic_rates)++;
											} else {

											}
										} else {
										}
									}
								}
							break;
							case TAG_EXT_SUPPORTED_RATES: //Extended supported rates
								for(i=0;i < mpdu_ptr_u8[1]; i++){
										if(mpdu_ptr_u8[2+i]&RATE_BASIC){
											//This is a basic rate. It is required by the AP in order to associate.
											if((curr_ap_info->num_basic_rates) < NUM_BASIC_RATES_MAX){

												if(wlan_mac_high_valid_tagged_rate(mpdu_ptr_u8[2+i])){
												//	xil_printf("Basic rate #%d: 0x%x\n", (curr_ap_info->num_basic_rates), mpdu_ptr_u8[2+i]);

													(curr_ap_info->basic_rates)[(curr_ap_info->num_basic_rates)] = mpdu_ptr_u8[2+i];
													(curr_ap_info->num_basic_rates)++;
												} else {
													//xil_printf("Invalid tagged rate. ignoring.");
												}
											} else {
												//xil_printf("Error: too many rates were flagged as basic. ignoring.");
											}
										}
									}

							break;
							case TAG_DS_PARAMS: //DS Parameter set (e.g. channel)
								curr_ap_info->chan = mpdu_ptr_u8[2];
							break;
						}
						mpdu_ptr_u8 += mpdu_ptr_u8[1]+2; //Move up to the next tag
					}

				}

			break;

			default:
				//This should be left as a verbose print. It occurs often when communicating with mobile devices since they tend to send
				//null data frames (type: DATA, subtype: 0x4) for power management reasons.
				warp_printf(PL_VERBOSE, "Received unknown frame control type/subtype %x\n",rx_80211_header->frame_control_1);
			break;
		}
		return;
	} else {
		//Bad FCS
	}
}




void ltg_event(u32 id, void* callback_arg){
	dl_list checkout;
	packet_bd* tx_queue;
	u32 tx_length;
	u8* mpdu_ptr_u8;
	llc_header* llc_hdr;
	u32 payload_length = 0;

	switch(((ltg_pyld_hdr*)callback_arg)->type){
		case LTG_PYLD_TYPE_FIXED:
			payload_length = ((ltg_pyld_fixed*)callback_arg)->length;
		break;
		case LTG_PYLD_TYPE_UNIFORM_RAND:
			payload_length = (rand()%(((ltg_pyld_uniform_rand*)(callback_arg))->max_length - ((ltg_pyld_uniform_rand*)(callback_arg))->min_length))+((ltg_pyld_uniform_rand*)(callback_arg))->min_length;
		break;
		default:
		break;
	}

	if(id == 0 && (access_point.AID > 0)){
		//Send a Data packet to AP
		//Checkout 1 element from the queue;
		queue_checkout(&checkout,1);

		if(checkout.length == 1){ //There was at least 1 free queue element
			tx_queue = (packet_bd*)(checkout.first);

			wlan_mac_high_setup_tx_header( &tx_header_common, access_point.addr, access_point.addr );

			mpdu_ptr_u8 = (u8*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame;
			tx_length = wlan_create_data_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame, &tx_header_common, MAC_FRAME_CTRL2_FLAG_TO_DS);

			mpdu_ptr_u8 += sizeof(mac_header_80211);
			llc_hdr = (llc_header*)(mpdu_ptr_u8);

			//Prepare the MPDU LLC header
			llc_hdr->dsap = LLC_SNAP;
			llc_hdr->ssap = LLC_SNAP;
			llc_hdr->control_field = LLC_CNTRL_UNNUMBERED;
			bzero((void *)(llc_hdr->org_code), 3); //Org Code 0x000000: Encapsulated Ethernet
			llc_hdr->type = LLC_TYPE_CUSTOM;

			tx_length += sizeof(llc_header);
			tx_length += payload_length;

	 		wlan_mac_high_setup_tx_queue ( tx_queue, (void*)&(access_point), tx_length, MAX_RETRY, TX_GAIN_TARGET,
	 				         (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO) );

			enqueue_after_end(1, &checkout);
			check_tx_queue();
		}
	}

}






void print_ap_list(){
	u32 i,j;
	char str[4];
	u16 ap_sel;

	uart_mode = UART_MODE_AP_LIST;
	//active_scan = 0;
	pause_queue = 0;

	//Revert to the previous channel that we were on prior to the active scan
	mac_param_chan = mac_param_chan_save;
	wlan_mac_high_set_channel( mac_param_chan );

//	xil_printf("\f");

	xil_printf("************************ AP List *************************\n");

	for(i=0; i<num_ap_list; i++){
		xil_printf("[%d] SSID:     %s ", i, ap_list[i].ssid);
		if(ap_list[i].private == 1){
			xil_printf("(*)\n");
		} else {
			xil_printf("\n");
		}

		xil_printf("    BSSID:         %02x-%02x-%02x-%02x-%02x-%02x\n", ap_list[i].bssid[0],ap_list[i].bssid[1],ap_list[i].bssid[2],ap_list[i].bssid[3],ap_list[i].bssid[4],ap_list[i].bssid[5]);
		xil_printf("    Channel:       %d\n",ap_list[i].chan);
		xil_printf("    Rx Power:      %d dBm\n",ap_list[i].rx_power);
		xil_printf("    Basic Rates:   ");
		for(j = 0; j < (ap_list[i].num_basic_rates); j++ ){
			wlan_mac_high_tagged_rate_to_readable_rate(ap_list[i].basic_rates[j], str);
			xil_printf("%s, ",str);
		}
		xil_printf("\b\b \n");

	}


	if(strlen(access_point_ssid) == 0){
		xil_printf("\n(*) Private Network (not supported)\n");
		xil_printf("\n To join a network, type the number next to the SSID that\n");
		xil_printf("you want to join and press enter. Otherwise, press Esc to return\n");
		xil_printf("AP Selection: ");
	} else {
		for(i=0; i<num_ap_list; i++){
			if(strcmp(access_point_ssid,ap_list[i].ssid) == 0){
				ap_sel = i;
				if( ap_list[ap_sel].private == 0) {
					mac_param_chan = ap_list[ap_sel].chan;

					//Send a message to other processor to tell it to switch channels
					wlan_mac_high_set_channel( mac_param_chan );

					xil_printf("\nAttempting to join %s\n", ap_list[ap_sel].ssid);
					memcpy(access_point.addr, ap_list[ap_sel].bssid, 6);

					access_point_ssid = wlan_mac_high_realloc(access_point_ssid, strlen(ap_list[ap_sel].ssid)+1);
					strcpy(access_point_ssid,ap_list[ap_sel].ssid);

					access_point_num_basic_rates = ap_list[ap_sel].num_basic_rates;
					memcpy(access_point_basic_rates, ap_list[ap_sel].basic_rates,access_point_num_basic_rates);

					stop_active_scan();
					association_state = 1;
					attempt_authentication();
					return;
				} else {
					xil_printf("AP with SSID %s is private\n", access_point_ssid);
					return;
				}
			}
		}
		xil_printf("Failed to find AP with SSID of %s\n", access_point_ssid);
	}

}

void reset_station_statistics(){
	access_point.stats->num_tx_total = 0;
	access_point.stats->num_tx_success = 0;
	access_point.stats->num_retry = 0;
	access_point.stats->num_rx_success = 0;
	access_point.stats->num_rx_bytes = 0;
}


dl_list * get_statistics(){
	return NULL;
}


/*****************************************************************************/
/**
* Get AP List
*
* This function will populate the buffer with:
*   buffer[0]      = Number of stations
*   buffer[1 .. N] = memcpy of the station information structure
* where N is less than max_words
*
* @param    stations      - Station info pointer
*           num_stations  - Number of stations to copy
*           buffer        - u32 pointer to the buffer to transfer the data
*           max_words     - The maximum number of words in the buffer
*
* @return	Number of words copied in to the buffer
*
* @note     None.
*
******************************************************************************/
int get_ap_list( ap_info * ap_list, u32 num_ap, u32 * buffer, u32 max_words ) {

	unsigned int size;
	unsigned int index;

	index     = 0;

	// Set number of Association entries
	buffer[ index++ ] = num_ap;

	// Get total size (in bytes) of data to be transmitted
	size   = num_ap * sizeof( ap_info );
	// Get total size of data (in words) to be transmitted
	index += size / sizeof( u32 );
    if ( (size > 0 ) && (index < max_words) ) {
        memcpy( &buffer[1], ap_list, size );
    }

// #ifdef _DEBUG_
	#ifdef USE_WARPNET_WLAN_EXP
    wlan_exp_print_ap_list( ap_list, num_ap );
	#endif
// #endif

	return index;
}


