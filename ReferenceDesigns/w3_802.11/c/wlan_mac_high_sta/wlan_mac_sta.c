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

mac_header_80211_common tx_header_common;

u8 default_unicast_rate;

u8 interactive_mode;
u8 active_scan;

ap_info* ap_list;
u8 num_ap_list;

// AID - Addr[6] - Last Seq
station_info access_point;

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

	while(cpu_low_initialized() == 0){
		xil_printf("waiting on CPU_LOW to boot\n");
	};
	memcpy((void*) &(eeprom_mac_addr[0]), (void*) get_eeprom_mac_addr(), 6);

	tx_header_common.address_2 = &(eeprom_mac_addr[0]);
	tx_header_common.seq_num = 0;


	write_hex_display(0);

	mac_param_chan = 4;

	//Send a message to other processor to tell it to switch channels
	ipc_msg_to_low.msg_id = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_RF_IFC);
	ipc_msg_to_low.num_payload_words = sizeof(ipc_config_rf_ifc)/sizeof(u32);
	ipc_msg_to_low.payload_ptr = &(ipc_msg_to_low_payload[0]);
	init_ipc_config(config_rf_ifc,ipc_msg_to_low_payload,ipc_config_rf_ifc);
	config_rf_ifc->channel = mac_param_chan;
	ipc_mailbox_write_msg(&ipc_msg_to_low);

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

	if(rxByte == ASCII_ESC){
		interactive_mode = 0;
		print_menu();
		return;
	}

	if(interactive_mode){
		switch(rxByte){

		}
	} else {
		switch(rxByte){
			case ASCII_1:
				interactive_mode = 1;
				//TODO: print_station_status();
			break;

			case ASCII_a:
				//Send bcast probe requests across all channels
				if(active_scan ==0){
					num_ap_list = 0;
					free(ap_list);
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
	}
}

void probe_req_transmit(){

	static u8 curr_channel_index = 0;
	wlan_ipc_msg ipc_msg_to_low;
	u32 ipc_msg_to_low_payload[1];
	ipc_config_rf_ifc* config_rf_ifc;
	u16 tx_length;
	packet_bd_list checkout;
	packet_bd*	tx_queue;

	mac_param_chan = curr_channel_index + 1; //+1 is to shift [0,10] index to [1,11] channel number

	//xil_printf("mac_param_chan = %d\n", mac_param_chan);

	//Send a message to other processor to tell it to switch channels
	ipc_msg_to_low.msg_id = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_RF_IFC);
	ipc_msg_to_low.num_payload_words = sizeof(ipc_config_rf_ifc)/sizeof(u32);
	ipc_msg_to_low.payload_ptr = &(ipc_msg_to_low_payload[0]);
	init_ipc_config(config_rf_ifc,ipc_msg_to_low_payload,ipc_config_rf_ifc);
	config_rf_ifc->channel = mac_param_chan;
	ipc_mailbox_write_msg(&ipc_msg_to_low);

	//Send probe request

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

	curr_channel_index = (curr_channel_index+1)%11;
	if(curr_channel_index > 0){
		active_scan = 1;
		wlan_mac_schedule_event(SCHEDULE_COARSE, 100000, (void*)probe_req_transmit);
	} else {
		active_scan = 0;
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

		case (MAC_FRAME_CTRL1_SUBTYPE_PROBE_REQ): //Probe Request Packet

		break;

		case (MAC_FRAME_CTRL1_SUBTYPE_BEACON): //Beacon Packet
		case (MAC_FRAME_CTRL1_SUBTYPE_PROBE_RESP): //Probe Response Packet
			//xil_printf("Probe Response\n");
			for (i=0;i<num_ap_list;i++){
				if(wlan_addr_eq(ap_list[i].bssid, rx_80211_header->address_3)){
					curr_ap_info = &(ap_list[i]);
					break;
				}
			}

			if(curr_ap_info == NULL){
				ap_list = realloc(ap_list, sizeof(ap_info)*num_ap_list);
				if(ap_list != NULL){
					num_ap_list++;
					//xil_printf("num_ap_list = %d, ap_list = 0x%x\n", num_ap_list, ap_list);
					curr_ap_info = &(ap_list[num_ap_list-1]);
				} else {
					return;
				}

			}

			curr_ap_info->rx_power = mpdu_info->rx_power;

			//Copy BSSID into ap_info struct
			memcpy(curr_ap_info->bssid, rx_80211_header->address_3,6);

			mpdu_ptr_u8 += (sizeof(mac_header_80211) + sizeof(beacon_probe_frame));
			while(((u32)mpdu_ptr_u8 -  (u32)mpdu)<= length){ //Loop through tagged parameters
				switch(mpdu_ptr_u8[0]){ //What kind of tag is this?
					case TAG_SSID_PARAMS: //SSID parameter set
						ssid = (char*)(&(mpdu_ptr_u8[2]));
						memcpy(curr_ap_info->ssid, ssid ,mpdu_ptr_u8[1]);
						//Terminate the string
						(curr_ap_info->ssid)[mpdu_ptr_u8[1]] = 0;
					break;
					case TAG_SUPPORTED_RATES: //Supported rates
					break;
					case TAG_EXT_SUPPORTED_RATES: //Extended supported rates
					break;
					case TAG_DS_PARAMS: //DS Parameter set (e.g. channel)
						curr_ap_info->chan = mpdu_ptr_u8[2];
					break;
				}
				mpdu_ptr_u8 += mpdu_ptr_u8[1]+2; //Move up to the next tag
			}

			//strcpy(curr_ap_info->ssid,"Test");

			print_ap_list();


		break;

		case (MAC_FRAME_CTRL1_SUBTYPE_AUTH): //Authentication Packet

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
	u32 i;

	xil_printf("\f");
	xil_printf("************************ AP List *************************\n");

	for(i=0; i<num_ap_list; i++){
		xil_printf("[%d] SSID:     %s\n", i, ap_list[i].ssid);
		xil_printf("     BSSID:    %x-%x-%x-%x-%x-%x\n", ap_list[i].bssid[0],ap_list[i].bssid[1],ap_list[i].bssid[2],ap_list[i].bssid[3],ap_list[i].bssid[4],ap_list[i].bssid[5]);
		xil_printf("     Channel:  %d\n",ap_list[i].chan);
		xil_printf("     Rx Power: %d dBm\n",ap_list[i].rx_power);
	}


	xil_printf("**********************************************************\n");
}


void print_menu(){
	xil_printf("\f");
	xil_printf("********************** Station Menu **********************\n");
	xil_printf("[1] - Interactive Station Status\n");
	xil_printf("[2] - Print Queue Status\n");
	xil_printf("\n");
	xil_printf("[a] - 	active scan and display nearby APs\n");
	xil_printf("[r/R] - change default unicast rate\n");
	xil_printf("*********************************************************\n");
}

