////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_sta.c
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
//          Erik Welsh (welsh [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

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
#include "wlan_mac_ipc.h"
#include "wlan_mac_misc_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_util.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_sta.h"
#include "ascii_characters.h"


// WLAN Exp includes
#include "wlan_exp_common.h"
#include "wlan_exp_node.h"
#include "wlan_exp_node_sta.h"
#include "wlan_exp_transport.h"


/*************************** Constant Definitions ****************************/

#define  WLAN_EXP_ETH                  WN_ETH_B
#define  WLAN_EXP_TYPE                 WARPNET_TYPE_80211_BASE + WARPNET_TYPE_80211_STATION


#define  WLAN_CHANNEL                  1



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
int association_state;                      // Section 10.3 of 802.11-2012
u8  uart_mode;
u8  active_scan;


// Access point information
ap_info* ap_list;
u8       num_ap_list;

u8       access_point_num_basic_rates;
u8       access_point_basic_rates[NUM_BASIC_RATES_MAX];


// Association Table variables
//   The last entry in associations[MAX_ASSOCIATIONS][] is swap space
station_info access_point;


// AP channel
u32 mac_param_chan;


// AP MAC address / Broadcast address
static u8 eeprom_mac_addr[6];
static u8 bcast_addr[6]      = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };


/*************************** Functions Prototypes ****************************/


#ifdef WLAN_USE_UART_MENU

void uart_rx(u8 rxByte);
void print_menu();
void print_station_status();

#else

void uart_rx(u8 rxByte){ };

#endif



/******************************** Functions **********************************/

int main(){

	//This function should be executed first. It will zero out memory, and if that
	//memory is used before calling this function, unexpected results may happen.
	wlan_mac_util_init_data();

    // Initialize AP list
	num_ap_list = 0;
	//free(ap_list);
	ap_list = NULL;

	xil_printf("\f----- wlan_mac_sta -----\n");
	xil_printf("Compiled %s %s\n", __DATE__, __TIME__);


	//xil_printf("_heap_start = 0x%x, %x\n", *(char*)(_heap_start),_heap_start);
	//xil_printf("_heap_end = 0x%x, %x\n", *(char*)(_heap_end),_heap_end);

    // Set Global variables
	default_unicast_rate = WLAN_MAC_RATE_18M;


	// Initialize the utility library
	wlan_lib_init();
	wlan_mac_util_set_eth_encap_mode(ENCAP_MODE_STA);
	wlan_mac_util_init( WLAN_EXP_TYPE );


	// Initialize callbacks
	wlan_mac_util_set_eth_rx_callback(       (void*)ethernet_receive);
	wlan_mac_util_set_mpdu_tx_done_callback( (void*)mpdu_transmit_done);
	wlan_mac_util_set_mpdu_rx_callback(      (void*)mpdu_rx_process);
	wlan_mac_util_set_uart_rx_callback(      (void*)uart_rx);
	wlan_mac_util_set_ipc_rx_callback(       (void*)ipc_rx);
	wlan_mac_util_set_check_queue_callback(  (void*)check_tx_queue);

    wlan_mac_ltg_set_callback(               (void*)ltg_event);


    // Initialize interrupts
	interrupt_init();


	// Initialize Association Table
	bzero(&(access_point), sizeof(station_info));

	access_point.AID = 0; //7.3.1.8 of 802.11-2007
	memset((void*)(&(access_point.addr[0])), 0xFF,6);
	access_point.seq = 0; //seq
	access_point.rx_timestamp = 0;

	// Set default SSID for AP
	access_point_ssid = malloc(strlen(default_AP_SSID)+1);
	strcpy(access_point_ssid,default_AP_SSID);


	// Set Association state for station to AP
	association_state = 1;


    // Wait for CPU Low to initialize
	while( is_cpu_low_initialized() == 0){
		xil_printf("waiting on CPU_LOW to boot\n");
	};


	// CPU Low will pass HW information to CPU High as part of the boot process
	//   - Get necessary HW information
	memcpy((void*) &(eeprom_mac_addr[0]), (void*) get_eeprom_mac_addr(), 6);


    // Set Header information
	tx_header_common.address_2 = &(eeprom_mac_addr[0]);
	tx_header_common.seq_num = 0;


    // Initialize hex display
	write_hex_display(0);


	// Set up channel
	mac_param_chan = WLAN_CHANNEL;
	set_mac_channel( mac_param_chan );


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
	if( strlen(default_AP_SSID) > 0 ) {
		active_scan = 1;
		probe_req_transmit();
	}


#ifdef USE_WARPNET_WLAN_EXP
	// Set AP processing callbacks
	node_set_process_callback( (void *)wlan_exp_node_sta_processCmd );
#endif


	while(1){
		//The design is entirely interrupt based. When no events need to be processed, the processor
		//will spin in this loop until an interrupt happens

#ifdef USE_WARPNET_WLAN_EXP
		transport_poll( WLAN_EXP_ETH );
#endif
	}
	return -1;
}




void check_tx_queue(){

	u8 i;
	static u32 queue_index = 0;
	if( is_cpu_low_ready() ){
		for(i=0;i<2;i++){
			//Alternate between checking the unassociated queue and the associated queue
			queue_index = (queue_index+1)%2;
			if(wlan_mac_poll_tx_queue(queue_index)){
				return;
			}
		}
	}
}



void mpdu_transmit_done(tx_frame_info* tx_mpdu){
	tx_event* tx_event_log_entry;

	void * mpdu = (void*)tx_mpdu + PHY_RX_PKT_BUF_MPDU_OFFSET;
	u8* mpdu_ptr_u8 = (u8*)mpdu;
	mac_header_80211* tx_80211_header;
	tx_80211_header = (mac_header_80211*)((void *)mpdu_ptr_u8);



	tx_event_log_entry = get_curr_tx_log();

	if(tx_event_log_entry != NULL){
		tx_event_log_entry->state = tx_mpdu->state;
		tx_event_log_entry->AID = 1;
		tx_event_log_entry->power = 0; //TODO: I'm on the fence as to whether this should be power or Tx Gains
		tx_event_log_entry->length = tx_mpdu->length;
		tx_event_log_entry->rate = tx_mpdu->rate;
		tx_event_log_entry->mac_type = tx_80211_header->frame_control_1;
		tx_event_log_entry->seq = ((tx_80211_header->sequence_control)>>4)&0xFFF;
		tx_event_log_entry->retry_count = tx_mpdu->retry_count;

		increment_log();
	}

	wlan_mac_util_process_tx_done(tx_mpdu, &(access_point));
}




void attempt_association(){
	//It is assumed that the global "access_point" has a valid BSSID (MAC Address).
	//This function should only be called after selecting an access point through active scan

	static u8      curr_try = 0;
	u16            tx_length;
	packet_bd_list checkout;
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
			checkout = queue_checkout(1);
			if(checkout.length == 1){ //There was at least 1 free queue element
				tx_queue = checkout.first;

		 		setup_tx_header( &tx_header_common, access_point.addr, access_point.addr );

				tx_length = wlan_create_association_req_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame, &tx_header_common, (u8)strlen(access_point_ssid), (u8*)access_point_ssid, access_point_num_basic_rates, access_point_basic_rates);

		 		setup_tx_queue ( tx_queue, NULL, tx_length, MAX_RETRY,
		 				         (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO) );

				enqueue_after_end(0, &checkout);
				check_tx_queue();
			}
			if( curr_try < (ASSOCIATION_NUM_TRYS - 1) ){
				wlan_mac_schedule_event(SCHEDULE_COARSE, ASSOCIATION_TIMEOUT_US, (void*)attempt_association);
				curr_try++;
			} else {
				curr_try = 0;
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
	packet_bd_list checkout;
	packet_bd*	   tx_queue;

	switch(association_state){

		case 1:
			//Initial start state, unauthenticated, unassociated
			//Checkout 1 element from the queue;
			checkout = queue_checkout(1);
			if(checkout.length == 1){ //There was at least 1 free queue element
				tx_queue = checkout.first;

		 		setup_tx_header( &tx_header_common, access_point.addr, access_point.addr );

				tx_length = wlan_create_auth_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame, &tx_header_common, AUTH_ALGO_OPEN_SYSTEM, AUTH_SEQ_REQ, STATUS_SUCCESS);

		 		setup_tx_queue ( tx_queue, NULL, tx_length, MAX_RETRY,
		 				         (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO) );

				enqueue_after_end(0, &checkout);
				check_tx_queue();
			}
			if( curr_try < (AUTHENTICATION_NUM_TRYS - 1) ){
				wlan_mac_schedule_event(SCHEDULE_COARSE, AUTHENTICATION_TIMEOUT_US, (void*)attempt_authentication);
				curr_try++;
			} else {
				curr_try = 0;
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





void probe_req_transmit(){
	u32 i;

	static u8 curr_channel_index = 0;
	u16 tx_length;
	packet_bd_list checkout;
	packet_bd*	tx_queue;

	mac_param_chan = curr_channel_index + 1; //+1 is to shift [0,10] index to [1,11] channel number

	//xil_printf("+++ probe_req_transmit mac_param_chan = %d\n", mac_param_chan);

	//Send a message to other processor to tell it to switch channels
	set_mac_channel( mac_param_chan );

	//Send probe request

	//xil_printf("Probe Req SSID: %s, Len: %d\n",access_point_ssid, strlen(access_point_ssid));

	for(i = 0; i<NUM_PROBE_REQ; i++){
	//Checkout 1 element from the queue;
	checkout = queue_checkout(1);
		if(checkout.length == 1){ //There was at least 1 free queue element
			tx_queue = checkout.first;

			setup_tx_header( &tx_header_common, bcast_addr, bcast_addr );

			tx_length = wlan_create_probe_req_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame,&tx_header_common, strlen(access_point_ssid), (u8*)access_point_ssid, mac_param_chan);

	 		setup_tx_queue ( tx_queue, NULL, tx_length, 0, 0 );

			enqueue_after_end(0, &checkout);
			check_tx_queue();
		}
	}

	curr_channel_index = (curr_channel_index+1)%11;

	if(curr_channel_index > 0){
		wlan_mac_schedule_event(SCHEDULE_COARSE, 100000, (void*)probe_req_transmit);
	} else {
		wlan_mac_schedule_event(SCHEDULE_COARSE, 100000, (void*)print_ap_list);
	}
}





int ethernet_receive(packet_bd_list* tx_queue_list, u8* eth_dest, u8* eth_src, u16 tx_length){

	//Receives the pre-encapsulated Ethernet frames
	packet_bd* tx_queue = tx_queue_list->first;

	//setup_tx_header( &tx_header_common, (u8*)(&(eth_dest[0])), (u8*)(&(eth_src[0])) );
	setup_tx_header( &tx_header_common, (u8*)access_point.addr,(u8*)(&(eth_dest[0])));

	wlan_create_data_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame, &tx_header_common, MAC_FRAME_CTRL2_FLAG_TO_DS);

	if(wlan_addr_eq(bcast_addr, eth_dest)){
		setup_tx_queue ( tx_queue, NULL, tx_length, 0, 0 );

		enqueue_after_end(0, tx_queue_list);
		check_tx_queue();

	} else {

		if(access_point.AID != 0){
			setup_tx_queue ( tx_queue, (void*)&(access_point), tx_length, MAX_RETRY,
							 (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO) );

			enqueue_after_end(1, tx_queue_list);
			check_tx_queue();
		}

	}

	return 1;


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

	rx_event* rx_event_log_entry;

	rx_frame_info* mpdu_info = (rx_frame_info*)pkt_buf_addr;

	u8 is_associated = 0;

	rx_event_log_entry = get_curr_rx_log();


	if(rx_event_log_entry != NULL){
			rx_event_log_entry->state = mpdu_info->state;
			rx_event_log_entry->AID = 0;
			rx_event_log_entry->power = mpdu_info->rx_power;
			rx_event_log_entry->length = mpdu_info->length;
			rx_event_log_entry->rate = mpdu_info->rate;
			rx_event_log_entry->mac_type = rx_80211_header->frame_control_1;
			rx_event_log_entry->seq = ((rx_80211_header->sequence_control)>>4)&0xFFF;
			rx_event_log_entry->flags = 0; //TODO: fill in with retry flag, etc

			increment_log();
	}



	if(wlan_addr_eq(access_point.addr, (rx_80211_header->address_2))) {
		is_associated = 1;

		if(rx_event_log_entry != NULL) rx_event_log_entry->AID = 1;

		rx_seq = ((rx_80211_header->sequence_control)>>4)&0xFFF;
		//Check if duplicate
		access_point.rx_timestamp = get_usec_timestamp();
		access_point.last_rx_power = mpdu_info->rx_power;

		if( (access_point.seq != 0)  && (access_point.seq == rx_seq) ) {
			//Received seq num matched previously received seq num for this STA; ignore the MPDU and return
			return;

		} else {
			access_point.seq = rx_seq;
		}
	}


	switch(rx_80211_header->frame_control_1) {
	case (MAC_FRAME_CTRL1_SUBTYPE_DATA): //Data Packet
		if(is_associated){
			if((rx_80211_header->frame_control_2) & MAC_FRAME_CTRL2_FLAG_FROM_DS) {
				//MPDU is flagged as destined to the DS - send it for de-encapsulation and Ethernet Tx (if appropriate)

				(access_point.num_rx_success)++;
				(access_point.num_rx_bytes) += mpdu_info->length;

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
					write_hex_display(access_point.AID);
					access_point.tx_rate = default_unicast_rate;
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
				access_point.AID = 0;
				write_hex_display(access_point.AID);
				memset((void*)(&(access_point.addr[0])), 0xFF,6);
				access_point.seq = 0; //seq
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
						ap_list = malloc(sizeof(ap_info)*(num_ap_list+1));
					} else {
						ap_list = realloc(ap_list, sizeof(ap_info)*(num_ap_list+1));
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

										if(valid_tagged_rate(mpdu_ptr_u8[2+i])){

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

											if(valid_tagged_rate(mpdu_ptr_u8[2+i])){
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
}




void ltg_event(u32 id){
	packet_bd_list checkout;
	packet_bd* tx_queue;
	u32 tx_length;
	u8* mpdu_ptr_u8;
	llc_header* llc_hdr;

	if(id == 0 && (access_point.AID > 0)){
		//Send a Data packet to AP
		//Checkout 1 element from the queue;
		checkout = queue_checkout(1);

		if(checkout.length == 1){ //There was at least 1 free queue element
			tx_queue = checkout.first;

	 		setup_tx_header( &tx_header_common, access_point.addr, access_point.addr );

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

			tx_length = 1200; //TODO: The rest of the payload is just... whatever. This will tell the PHY to send a longer packet
							  //and pretend the payload is something interesting

	 		setup_tx_queue ( tx_queue, (void*)&(access_point), tx_length, MAX_RETRY,
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
	active_scan = 0;

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
			tagged_rate_to_readable_rate(ap_list[i].basic_rates[j], str);
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
					set_mac_channel( mac_param_chan );

					xil_printf("\nAttempting to join %s\n", ap_list[ap_sel].ssid);
					memcpy(access_point.addr, ap_list[ap_sel].bssid, 6);

					access_point_ssid = realloc(access_point_ssid, strlen(ap_list[ap_sel].ssid)+1);
					//xil_printf("allocated %d bytes in 0x%08x\n", strlen(ap_list[ap_sel].ssid), access_point_ssid);
					strcpy(access_point_ssid,ap_list[ap_sel].ssid);

					access_point_num_basic_rates = ap_list[ap_sel].num_basic_rates;
					memcpy(access_point_basic_rates, ap_list[ap_sel].basic_rates,access_point_num_basic_rates);

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
	access_point.num_tx_total = 0;
	access_point.num_tx_success = 0;
	access_point.num_rx_success = 0;
	access_point.num_rx_bytes = 0;
}
