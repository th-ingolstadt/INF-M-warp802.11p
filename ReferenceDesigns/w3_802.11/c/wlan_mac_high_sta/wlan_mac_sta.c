////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_sta.c
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

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
#include "wlan_mac_util.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_sta.h"
#include "ascii_characters.h"

#define MAX_RETRY 7

//If you want this station to try to associate to a known AP at boot, type
//the string here. Otherwise, let it be an empty string.
static char default_AP_SSID[] = "WARP-AP-LS";

mac_header_80211_common tx_header_common;

u8 default_unicast_rate;

//Section 10.3 of 802.11-2012
int association_state;

u8 uart_mode;
#define UART_MODE_MAIN 0
#define UART_MODE_INTERACTIVE 1
#define UART_MODE_AP_LIST 2

u8 active_scan;

ap_info* ap_list;
u8 num_ap_list;

// AID - Addr[6] - Last Seq
station_info access_point;
char* access_point_ssid;
u8 access_point_num_basic_rates;
u8 access_point_basic_rates[NUM_BASIC_RATES_MAX];

static u32 mac_param_chan;

static u8 eeprom_mac_addr[6];
static u8 bcast_addr[6];

int main(){
	wlan_ipc_msg ipc_msg_to_low;
	u32 ipc_msg_to_low_payload[1];

	ipc_config_rf_ifc* config_rf_ifc;

	xil_printf("\f----- wlan_mac_sta -----\n");
	xil_printf("Compiled %s %s\n", __DATE__, __TIME__);

	default_unicast_rate = WLAN_MAC_RATE_18M;

	wlan_lib_init();
	wlan_mac_util_init();

	wlan_mac_util_set_eth_rx_callback((void*)ethernet_receive);
	wlan_mac_util_set_mpdu_tx_done_callback((void*)mpdu_transmit_done);
	wlan_mac_util_set_mpdu_rx_callback((void*)mpdu_rx_process);
	wlan_mac_util_set_uart_rx_callback((void*)uart_rx);
	wlan_mac_util_set_ipc_rx_callback((void*)ipc_rx);
	wlan_mac_util_set_check_queue_callback((void*)check_tx_queue);

	interrupt_init();

	bcast_addr[0] = 0xFF;
	bcast_addr[1] = 0xFF;
	bcast_addr[2] = 0xFF;
	bcast_addr[3] = 0xFF;
	bcast_addr[4] = 0xFF;
	bcast_addr[5] = 0xFF;

	bzero(&(access_point),sizeof(station_info));

	access_point.AID = 0; //7.3.1.8 of 802.11-2007
	memset((void*)(&(access_point.addr[0])), 0xFF,6);
	access_point.seq = 0; //seq

	num_ap_list = 0;

	free(ap_list);
	ap_list = NULL;

	free(access_point_ssid);
	access_point_ssid = NULL;

	association_state = 1;

	while(cpu_low_initialized() == 0){
		xil_printf("waiting on CPU_LOW to boot\n");
	};
	memcpy((void*) &(eeprom_mac_addr[0]), (void*) get_eeprom_mac_addr(), 6);

	tx_header_common.address_2 = &(eeprom_mac_addr[0]);
	tx_header_common.seq_num = 0;


	write_hex_display(0);

	mac_param_chan = 1;

	//Send a message to other processor to tell it to switch channels
	ipc_msg_to_low.msg_id = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_RF_IFC);
	ipc_msg_to_low.num_payload_words = sizeof(ipc_config_rf_ifc)/sizeof(u32);
	ipc_msg_to_low.payload_ptr = &(ipc_msg_to_low_payload[0]);
	init_ipc_config(config_rf_ifc,ipc_msg_to_low_payload,ipc_config_rf_ifc);
	config_rf_ifc->channel = mac_param_chan;
	ipc_mailbox_write_msg(&ipc_msg_to_low);

	uart_mode = UART_MODE_MAIN;

	xil_printf("\nAt any time, press the Esc key in your terminal to access the AP menu\n");

	while(1){
		//The design is entirely interrupt based. When no events need to be processed, the processor
		//will spin in this loop until an interrupt happens
	}
	return -1;
}

void check_tx_queue(){

	u8 i;
	static u32 queue_index = 0;
	if(cpu_low_ready()){
		for(i=0;i<2;i++){
			//Alternate between checking the bcast queue and the unicast queue
			queue_index = (queue_index+1)%2;
			if(wlan_mac_poll_tx_queue(queue_index)){
				return;
			}
		}
	}
}

void mpdu_transmit_done(tx_frame_info* tx_mpdu){
	//TODO: call wlan_mac_util_process_tx_done
	/*	u32 i;
	if(tx_mpdu->AID != 0){
		for(i=0; i<next_free_assoc_index; i++){
			if( (associations[i].AID) == (tx_mpdu->AID) ) {
				//Process this TX MPDU DONE event to update any statistics used in rate adaptation
				wlan_mac_util_process_tx_done(tx_mpdu, &(associations[i]));
				break;
			}
		}
	}*/
}

void uart_rx(u8 rxByte){
	#define MAX_NUM_AP_CHARS 4
	static char numerical_entry[MAX_NUM_AP_CHARS+1];
	static u8 curr_decade = 0;
	u16 ap_sel;
	wlan_ipc_msg ipc_msg_to_low;
	u32 ipc_msg_to_low_payload[1];
	ipc_config_rf_ifc* config_rf_ifc;

	if(rxByte == ASCII_ESC){
		uart_mode = UART_MODE_MAIN;
		print_menu();
		return;
	}

	switch(uart_mode){
		case UART_MODE_MAIN:
			switch(rxByte){
				case ASCII_1:
					uart_mode = UART_MODE_INTERACTIVE;
					//TODO: print_station_status();
				break;

				case ASCII_a:
					//Send bcast probe requests across all channels
					if(active_scan ==0){
						num_ap_list = 0;
						//xil_printf("- Free 0x%08x\n",ap_list);
						free(ap_list);
						ap_list = NULL;
						active_scan = 1;
						//xil_printf("+++ starting active scan\n");
						probe_req_transmit();
					}
				break;

				case ASCII_r:
					if(default_unicast_rate > WLAN_MAC_RATE_6M){
						default_unicast_rate--;
					} else {
						default_unicast_rate = WLAN_MAC_RATE_6M;
					}


					access_point.tx_rate = default_unicast_rate;


					xil_printf("(-) Default Unicast Rate: %d Mbps\n", wlan_lib_mac_rate_to_mbps(default_unicast_rate));
				break;
				case ASCII_R:
					if(default_unicast_rate < WLAN_MAC_RATE_54M){
						default_unicast_rate++;
					} else {
						default_unicast_rate = WLAN_MAC_RATE_54M;
					}

					access_point.tx_rate = default_unicast_rate;

					xil_printf("(+) Default Unicast Rate: %d Mbps\n", wlan_lib_mac_rate_to_mbps(default_unicast_rate));
				break;
			}
		break;
		case UART_MODE_INTERACTIVE:
			switch(rxByte){

			}
		break;
		case UART_MODE_AP_LIST:
			switch(rxByte){
				case ASCII_CR:

					numerical_entry[curr_decade] = 0;
					curr_decade = 0;

					ap_sel = str2num(numerical_entry);

					if( (ap_sel >= 0) && (ap_sel <= (num_ap_list-1))){

						if( ap_list[ap_sel].private == 0) {
							uart_mode = UART_MODE_MAIN;
							mac_param_chan = ap_list[ap_sel].chan;

							//Send a message to other processor to tell it to switch channels
							ipc_msg_to_low.msg_id = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_RF_IFC);
							ipc_msg_to_low.num_payload_words = sizeof(ipc_config_rf_ifc)/sizeof(u32);
							ipc_msg_to_low.payload_ptr = &(ipc_msg_to_low_payload[0]);
							init_ipc_config(config_rf_ifc,ipc_msg_to_low_payload,ipc_config_rf_ifc);
							config_rf_ifc->channel = mac_param_chan;
							ipc_mailbox_write_msg(&ipc_msg_to_low);


							xil_printf("\nAttempting to join %s\n", ap_list[ap_sel].ssid);
							memcpy(access_point.addr, ap_list[ap_sel].bssid, 6);

							access_point_ssid = realloc(access_point_ssid, strlen(ap_list[ap_sel].ssid));
							//xil_printf("allocated %d bytes in 0x%08x\n", strlen(ap_list[ap_sel].ssid), access_point_ssid);
							strcpy(access_point_ssid,ap_list[ap_sel].ssid);

							access_point_num_basic_rates = ap_list[ap_sel].num_basic_rates;
							memcpy(access_point_basic_rates, ap_list[ap_sel].basic_rates,access_point_num_basic_rates);

							association_state = 1;
							attempt_authentication();

						} else {
							xil_printf("\nInvalid selection, please choose an AP that is not private: ");
						}


					} else {

						xil_printf("\nInvalid selection, please choose a number between [0,%d]: ", num_ap_list-1);

					}



				break;
				case ASCII_DEL:
					if(curr_decade > 0){
						curr_decade--;
						xil_printf("\b \b");
					}

				break;
				default:
					if( (rxByte <= ASCII_9) && (rxByte >= ASCII_0) ){
						//the user entered a character

						if(curr_decade < MAX_NUM_AP_CHARS){
							xil_printf("%c", rxByte);
							numerical_entry[curr_decade] = rxByte;
							curr_decade++;
						}



					}

				break;

			}
		break;

	}


}

void attempt_association(){
	//It is assumed that the global "access_point" has a valid BSSID (MAC Address).
	//This function should only be called after selecting an access point through active scan

	#define TIMEOUT_US 100000
	#define NUM_TRYS 5

	static u8 curr_try = 0;
	u16 tx_length;
	packet_bd_list checkout;
	packet_bd*	tx_queue;

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
				tx_header_common.address_1 = access_point.addr;
				tx_header_common.address_3 = access_point.addr;


				tx_length = wlan_create_association_req_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame, &tx_header_common, (u8)strlen(access_point_ssid), (u8*)access_point_ssid, access_point_num_basic_rates, access_point_basic_rates);
				((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.length = tx_length;
				tx_queue->metadata_ptr = NULL;
				((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.retry_max = MAX_RETRY;
				((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.flags = (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
				enqueue_after_end(1, &checkout);
				check_tx_queue();
			}
			if(curr_try < (NUM_TRYS-1)){
				wlan_mac_schedule_event(SCHEDULE_COARSE, TIMEOUT_US, (void*)attempt_association);
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

	#define TIMEOUT_US 100000
	#define NUM_TRYS 5

	static u8 curr_try = 0;
	u16 tx_length;
	packet_bd_list checkout;
	packet_bd*	tx_queue;

	switch(association_state){

		case 1:
			//Initial start state, unauthenticated, unassociated
			//Checkout 1 element from the queue;
			checkout = queue_checkout(1);
			if(checkout.length == 1){ //There was at least 1 free queue element
				tx_queue = checkout.first;
				tx_header_common.address_1 = access_point.addr;
				tx_header_common.address_3 = bcast_addr;
				tx_length = wlan_create_auth_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame, &tx_header_common, AUTH_ALGO_OPEN_SYSTEM, AUTH_SEQ_REQ, STATUS_SUCCESS);
				((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.length = tx_length;
				tx_queue->metadata_ptr = NULL;
				((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.retry_max = MAX_RETRY;
				((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.flags = (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
				enqueue_after_end(1, &checkout);
				check_tx_queue();
			}
			if(curr_try < (NUM_TRYS-1)){
				wlan_mac_schedule_event(SCHEDULE_COARSE, TIMEOUT_US, (void*)attempt_authentication);
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
	#define NUM_PROBE_REQ 5
	u32 i;

	static u8 curr_channel_index = 0;
	wlan_ipc_msg ipc_msg_to_low;
	u32 ipc_msg_to_low_payload[1];
	ipc_config_rf_ifc* config_rf_ifc;
	u16 tx_length;
	packet_bd_list checkout;
	packet_bd*	tx_queue;

	mac_param_chan = curr_channel_index + 1; //+1 is to shift [0,10] index to [1,11] channel number

	//xil_printf("+++ probe_req_transmit mac_param_chan = %d\n", mac_param_chan);

	//Send a message to other processor to tell it to switch channels
	ipc_msg_to_low.msg_id = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_RF_IFC);
	ipc_msg_to_low.num_payload_words = sizeof(ipc_config_rf_ifc)/sizeof(u32);
	ipc_msg_to_low.payload_ptr = &(ipc_msg_to_low_payload[0]);
	init_ipc_config(config_rf_ifc,ipc_msg_to_low_payload,ipc_config_rf_ifc);
	config_rf_ifc->channel = mac_param_chan;
	ipc_mailbox_write_msg(&ipc_msg_to_low);

	//Send probe request

	for(i = 0; i<NUM_PROBE_REQ; i++){
	//Checkout 1 element from the queue;
	checkout = queue_checkout(1);
		if(checkout.length == 1){ //There was at least 1 free queue element
			tx_queue = checkout.first;
			tx_header_common.address_1 = bcast_addr;
			tx_header_common.address_3 = bcast_addr;
			tx_length = wlan_create_probe_req_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame,&tx_header_common, 0, 0, mac_param_chan);
			((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.length = tx_length;
			tx_queue->metadata_ptr = NULL;
			((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.flags = 0;
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

	u32 i;
	u8 is_associated = 0;

	tx_header_common.address_1 = (u8*)(&(eth_dest[0]));
	tx_header_common.address_3 = (u8*)(&(eth_src[0]));

	wlan_create_data_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame, &tx_header_common, MAC_FRAME_CTRL2_FLAG_TO_DS);
	((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.length = tx_length;

	if(wlan_addr_eq(bcast_addr, eth_dest)){
		tx_queue->metadata_ptr = NULL;
		((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.retry_max = 0;
		((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.flags = 0;
		enqueue_after_end(0, tx_queue_list);
		check_tx_queue();

	} else {
		//TODO: understand portal encapsulation for STA... do you overwrite source MAC?

		if(is_associated) {
			tx_queue->metadata_ptr = (void*)&(access_point);
			((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.retry_max = MAX_RETRY;
			((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.flags = (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
			enqueue_after_end(1, tx_queue_list); //Queue 1 is unicast
			check_tx_queue();
		} else {
			//Checkin this packet_bd so that it can be checked out again
			return 0;
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

	void * new_alloc_ptr;

	mac_header_80211* rx_80211_header;
	rx_80211_header = (mac_header_80211*)((void *)mpdu_ptr_u8);
	u16 rx_seq;

	rx_frame_info* mpdu_info = (rx_frame_info*)pkt_buf_addr;

	u8 is_associated = 0;


	if(wlan_addr_eq(access_point.addr, (rx_80211_header->address_2))) {
		is_associated = 1;
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

		break;

		case (MAC_FRAME_CTRL1_SUBTYPE_ASSOC_RESP): //Association response
			if(association_state == 2){
				mpdu_ptr_u8 += sizeof(mac_header_80211);

				if(((association_response_frame*)mpdu_ptr_u8)->status_code == STATUS_SUCCESS){
					association_state = 4;
					//TODO: update AID field;
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

		case (MAC_FRAME_CTRL1_SUBTYPE_BEACON): //Beacon Packet
		case (MAC_FRAME_CTRL1_SUBTYPE_PROBE_RESP): //Probe Response Packet

				if(active_scan){
				//xil_printf("Probe Response\n");
				for (i=0;i<num_ap_list;i++){

					//xil_printf("%d -- num_ap_list = %d -- match: %d: %02x-%02x-%02x-%02x-%02x-%02x == %02x-%02x-%02x-%02x-%02x-%02x \n",i, num_ap_list, wlan_addr_eq(ap_list[i].bssid, rx_80211_header->address_3), ( ap_list[i].bssid)[0],(ap_list[i].bssid)[1],(ap_list[i].bssid)[2],(ap_list[i].bssid)[3],(ap_list[i].bssid)[4],(ap_list[i].bssid)[5], ( rx_80211_header->address_3)[0],( rx_80211_header->address_3)[1],( rx_80211_header->address_3)[2],( rx_80211_header->address_3)[3],( rx_80211_header->address_3)[4],( rx_80211_header->address_3)[5]);

					//if(num_ap_list>0){
					//	xil_printf("ap_list = 0x%08x\n", ap_list);
					//}


					//DEBUG DELETE ME
			/*		char temp[33];
					u8* mpdu_ptr_u8_tmp = (u8*)mpdu_ptr_u8;
					xil_printf("%02x-%02x-%02x-%02x-%02x-%02x \n",( rx_80211_header->address_3)[0],( rx_80211_header->address_3)[1],( rx_80211_header->address_3)[2],( rx_80211_header->address_3)[3],( rx_80211_header->address_3)[4],( rx_80211_header->address_3)[5]);
					mpdu_ptr_u8_tmp += (sizeof(mac_header_80211) + sizeof(beacon_probe_frame));
					while(((u32)mpdu_ptr_u8_tmp -  (u32)mpdu)<= length){ //Loop through tagged parameters
						switch(mpdu_ptr_u8_tmp[0]){ //What kind of tag is this?
							case TAG_SSID_PARAMS: //SSID parameter set
								ssid = (char*)(&(mpdu_ptr_u8_tmp[2]));
								memcpy(temp, ssid ,mpdu_ptr_u8_tmp[1]);
								//Terminate the string
								(temp)[mpdu_ptr_u8_tmp[1]] = 0;
								xil_printf("Len: %d, SSID: %s\n",mpdu_ptr_u8_tmp[1],temp);
							break;
						}
						mpdu_ptr_u8_tmp += mpdu_ptr_u8_tmp[1]+2; //Move up to the next tag
					}*/
					//DEBUG DELETE ME

					if(wlan_addr_eq(ap_list[i].bssid, rx_80211_header->address_3)){
						curr_ap_info = &(ap_list[i]);
						//xil_printf("     Matched at 0x%08x\n", curr_ap_info);
						break;
					}
				}

				if(curr_ap_info == NULL){
					//xil_printf("num_ap_list = %d\n", num_ap_list);
	//				xil_printf("num_ap_list = %, adding new entry: %x-%x-%x-%x-%x-%x \n", num_ap_list, ( rx_80211_header->address_3)[0],( rx_80211_header->address_3)[1],( rx_80211_header->address_3)[2],( rx_80211_header->address_3)[3],( rx_80211_header->address_3)[4],( rx_80211_header->address_3)[5]);
					if(ap_list == NULL){
						ap_list = malloc(sizeof(ap_info)*(num_ap_list+1));
	//					xil_printf("+ Malloc'd 0x%08x with %d bytes\n",ap_list, sizeof(ap_info)*(num_ap_list+1));
					} else {
						ap_list = realloc(ap_list, sizeof(ap_info)*(num_ap_list+1));
						//new_alloc_ptr = malloc(sizeof(ap_info)*(num_ap_list+1));
						//memcpy(ap_list,new_alloc_ptr,sizeof(ap_info)*(num_ap_list+1));
						//free(ap_list);
						//ap_list = new_alloc_ptr;
	//					xil_printf("+ Realloc'd 0x%08x with %d bytes\n",ap_list, sizeof(ap_info)*(num_ap_list+1));
					}

	//				xil_printf("malloc_usable_size = %d\n", malloc_usable_size(ap_list));

					//xil_printf("%d existing entries, new entry: %x-%x-%x-%x-%x-%x \n", num_ap_list, ( rx_80211_header->address_3)[0],( rx_80211_header->address_3)[1],( rx_80211_header->address_3)[2],( rx_80211_header->address_3)[3],( rx_80211_header->address_3)[4],( rx_80211_header->address_3)[5]);
					//xil_printf("realloc ap_list = 0x%08x\n", ap_list);
					if(ap_list != NULL){
						num_ap_list++;
						curr_ap_info = &(ap_list[num_ap_list-1]);
						//xil_printf("num_ap_list = %d, ap_list = 0x%x, curr_ap_info = 0x%08x\n", num_ap_list, ap_list, curr_ap_info);
					} else {
						xil_printf("Reallocation of ap_list failed\n");
						return;
					}

				}

	//			xil_printf("curr_ap_info = 0x%08x\n",curr_ap_info);

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
	//						xil_printf("SSID Len = %d\n", mpdu_ptr_u8[1]);

							memcpy(curr_ap_info->ssid, ssid ,min(mpdu_ptr_u8[1],SSID_LEN_MAX-1));
							//Terminate the string
							(curr_ap_info->ssid)[min(mpdu_ptr_u8[1],SSID_LEN_MAX-1)] = 0;
	//						xil_printf("SSID = %s\n", (curr_ap_info->ssid));
						break;
						case TAG_SUPPORTED_RATES: //Supported rates
							for(i=0;i < mpdu_ptr_u8[1]; i++){
								if(mpdu_ptr_u8[2+i]&RATE_BASIC){
									//This is a basic rate. It is required by the AP in order to associate.
									if((curr_ap_info->num_basic_rates) < NUM_BASIC_RATES_MAX){

										if(valid_tagged_rate(mpdu_ptr_u8[2+i])){
										//	xil_printf("Basic rate #%d: 0x%x\n", (curr_ap_info->num_basic_rates), mpdu_ptr_u8[2+i]);

											(curr_ap_info->basic_rates)[(curr_ap_info->num_basic_rates)] = mpdu_ptr_u8[2+i];
											(curr_ap_info->num_basic_rates)++;
										} else {
											xil_printf("Invalid tagged rate. ignoring.");
										}
									} else {
										xil_printf("Error: too many rates were flagged as basic. ignoring.");
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
												xil_printf("Invalid tagged rate. ignoring.");
											}
										} else {
											xil_printf("Error: too many rates were flagged as basic. ignoring.");
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

				//strcpy(curr_ap_info->ssid,"Test");

				//print_ap_list();
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

void print_ap_list(){
	u32 i,j;
	char str[4];

	uart_mode = UART_MODE_AP_LIST;
	active_scan = 0;

	xil_printf("\f");
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

	xil_printf("\n(*) Private Network (not supported)\n");
	xil_printf("\n To join a network, type the number next to the SSID that\n");
	xil_printf("you want to join and press enter. Otherwise, press Esc to return\n");
	xil_printf("AP Selection: ");

}


void print_menu(){
	xil_printf("\f");
	xil_printf("********************** Station Menu **********************\n");
	xil_printf("[1] - Interactive Station Status\n");
	xil_printf("[2] - Print Queue Status\n");
	xil_printf("\n");
	xil_printf("[a] - 	active scan and display nearby APs\n");
	xil_printf("[r/R] - change default unicast rate\n");

}

int str2num(char* str){
	//For now this only works with non-negative values
	int return_value = 0;
	u8 decade_index;
	int multiplier;
	u8 string_length = strlen(str);
	u32 i;

	for(decade_index = 0; decade_index < string_length; decade_index++){
		multiplier = 1;
		for(i = 0; i < (string_length - 1 - decade_index) ; i++){
			multiplier = multiplier*10;
		}
		return_value += multiplier*(u8)(str[decade_index] - 48);
	}

	return return_value;
}
