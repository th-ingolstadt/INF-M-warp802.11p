////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_ap.c
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
#include "wlan_mac_ap.h"
#include "ascii_characters.h"

//Time between beacon transmissions
#define BEACON_INTERVAL_MS (100)
#define BEACON_INTERVAL_US (BEACON_INTERVAL_MS*1000)

//Time between association table check
//Periodically, the association table is culled through and
//inactive stations are explicitly purged.
#define ASSOCIATION_CHECK_INTERVAL_MS (10000)
#define ASSOCIATION_CHECK_INTERVAL_US (ASSOCIATION_CHECK_INTERVAL_MS*1000)

//The amount of time since the last time a station was heard from.
//After this interval, a station can be purged from the association table
#define ASSOCIATION_TIMEOUT_S (600)
#define ASSOCIATION_TIMEOUT_US (ASSOCIATION_TIMEOUT_S*1000000)

//When the node is in the state where it temporarily allows
//associations, this interval defines how long the window for
//new associations is open
#define ASSOCIATION_ALLOW_INTERVAL_MS (30000)
#define ASSOCIATION_ALLOW_INTERVAL_US (ASSOCIATION_ALLOW_INTERVAL_MS*1000)

//Time between blinking behavior in hex displays
#define ANIMATION_RATE_US (100000)

#define MAX_RETRY 7

#define MAX_ASSOCIATIONS 8

static char default_AP_SSID[] = "WARP-AP";
char* access_point_ssid;

mac_header_80211_common tx_header_common;
u8 allow_assoc;
u8 perma_assoc_mode;

u8 default_unicast_rate;

u8 enable_animation;
u8 interactive_mode;

//The last entry in associations[MAX_ASSOCIATIONS][] is swap space

// AID - Addr[6] - Last Seq
station_info associations[MAX_ASSOCIATIONS+1];
u32 next_free_assoc_index;

static u32 mac_param_chan;

static u8 eeprom_mac_addr[6];
static u8 bcast_addr[6];

int main(){
	wlan_ipc_msg ipc_msg_to_low;
	u32 ipc_msg_to_low_payload[1];
	u32 i;


	ipc_config_rf_ifc* config_rf_ifc;

	xil_printf("\f----- wlan_mac_ap -----\n");
	xil_printf("Compiled %s %s\n", __DATE__, __TIME__);

	perma_assoc_mode = 0;
	default_unicast_rate = WLAN_MAC_RATE_18M;

	wlan_lib_init();
	wlan_mac_util_init();

	wlan_mac_util_set_eth_rx_callback((void*)ethernet_receive);
	wlan_mac_util_set_mpdu_tx_done_callback((void*)mpdu_transmit_done);
	wlan_mac_util_set_mpdu_rx_callback((void*)mpdu_rx_process);
	wlan_mac_util_set_pb_u_callback((void*)up_button);
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

	next_free_assoc_index = 0;
	bzero(&(associations[0]),sizeof(station_info)*(MAX_ASSOCIATIONS+1));
	for(i=0;i<MAX_ASSOCIATIONS;i++){
		associations[i].AID = (1+i); //7.3.1.8 of 802.11-2007
		memset((void*)(&(associations[i].addr[0])), 0xFF,6);
		associations[i].seq = 0; //seq
	}



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

	access_point_ssid = malloc(strlen(default_AP_SSID)+1);
	strcpy(access_point_ssid,default_AP_SSID);


	wlan_mac_schedule_event(SCHEDULE_COARSE, BEACON_INTERVAL_US, (void*)beacon_transmit);

	wlan_mac_schedule_event(SCHEDULE_COARSE, ASSOCIATION_CHECK_INTERVAL_US, (void*)association_timestamp_check);

	enable_animation = 1;
	wlan_mac_schedule_event(SCHEDULE_COARSE, ANIMATION_RATE_US, (void*)animate_hex);
	enable_associations();
	perma_assoc_mode = 1; //By default, associations are allowed any time.
	wlan_mac_schedule_event(SCHEDULE_COARSE, ASSOCIATION_ALLOW_INTERVAL_US, (void*)disable_associations);

	xil_printf("\nAt any time, press the Esc key in your terminal to access the AP menu\n");

	while(1){
		//The design is entirely interrupt based. When no events need to be processed, the processor
		//will spin in this loop until an interrupt happens
	}
	return -1;
}

void check_tx_queue(){

	static u32 station_index = 0;
	u32 i;
	if(cpu_low_ready()){
		for(i = 0; i < (next_free_assoc_index+1); i++){
			station_index = (station_index+1)%(next_free_assoc_index+1);

			if(station_index == next_free_assoc_index){
				//Check Broadcast Queue
				if(wlan_mac_poll_tx_queue(0)){
					return;
				}
			} else {
				//Check Station Queue
				if(wlan_mac_poll_tx_queue(associations[station_index].AID)){
					return;
				}
			}
		}

	}
}

void mpdu_transmit_done(tx_frame_info* tx_mpdu){
	u32 i;
	if(tx_mpdu->AID != 0){
		for(i=0; i<next_free_assoc_index; i++){
			if( (associations[i].AID) == (tx_mpdu->AID) ) {
				//Process this TX MPDU DONE event to update any statistics used in rate adaptation
				wlan_mac_util_process_tx_done(tx_mpdu, &(associations[i]));
				break;
			}
		}
	}
}

void up_button(){
	if(allow_assoc == 0){
		//AP is currently not allowing any associations to take place
		enable_animation = 1;
		wlan_mac_schedule_event(SCHEDULE_COARSE,ANIMATION_RATE_US, (void*)animate_hex);
		enable_associations();
		wlan_mac_schedule_event(SCHEDULE_COARSE,ASSOCIATION_ALLOW_INTERVAL_US, (void*)disable_associations);
	} else if(perma_assoc_mode == 0){
		//AP is currently allowing associations, but only for the small allow window.
		//Go into permanent allow association mode.
		perma_assoc_mode = 1;
		xil_printf("Allowing associations indefinitely\n");
	} else {
		//AP is permanently allowing associations. Toggle everything off.
		perma_assoc_mode = 0;
		disable_associations();
	}
}

void uart_rx(u8 rxByte){
	wlan_ipc_msg ipc_msg_to_low;
	u32 ipc_msg_to_low_payload[1];
	ipc_config_rf_ifc* config_rf_ifc;
	u32 i;

	if(rxByte == ASCII_ESC){
		interactive_mode = 0;
		print_menu();
		return;
	}

	if(interactive_mode){
		switch(rxByte){
			case ASCII_r:
				//Reset statistics
				reset_station_statistics();
			break;
			case ASCII_d:
				//Deauthenticate all stations
				deauthenticate_stations();
			break;
		}
	} else {
		switch(rxByte){
			case ASCII_1:
				interactive_mode = 1;
				print_station_status();
			break;

			case ASCII_2:
				print_queue_status();
			break;

			case ASCII_c:

				if(mac_param_chan > 1){
					deauthenticate_stations();
					(mac_param_chan--);

					//Send a message to other processor to tell it to switch channels
					ipc_msg_to_low.msg_id = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_RF_IFC);
					ipc_msg_to_low.num_payload_words = sizeof(ipc_config_rf_ifc)/sizeof(u32);
					ipc_msg_to_low.payload_ptr = &(ipc_msg_to_low_payload[0]);
					init_ipc_config(config_rf_ifc,ipc_msg_to_low_payload,ipc_config_rf_ifc);
					config_rf_ifc->channel = mac_param_chan;
					ipc_mailbox_write_msg(&ipc_msg_to_low);
				} else {

				}
				xil_printf("(-) Channel: %d\n", mac_param_chan);


			break;
			case ASCII_C:
				if(mac_param_chan < 11){
					deauthenticate_stations();
					(mac_param_chan++);

					//Send a message to other processor to tell it to switch channels
					ipc_msg_to_low.msg_id = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_RF_IFC);
					ipc_msg_to_low.num_payload_words = sizeof(ipc_config_rf_ifc)/sizeof(u32);
					ipc_msg_to_low.payload_ptr = &(ipc_msg_to_low_payload[0]);
					init_ipc_config(config_rf_ifc,ipc_msg_to_low_payload,ipc_config_rf_ifc);
					config_rf_ifc->channel = mac_param_chan;
					ipc_mailbox_write_msg(&ipc_msg_to_low);
				} else {

				}
				xil_printf("(+) Channel: %d\n", mac_param_chan);

			break;
			case ASCII_r:
				if(default_unicast_rate > WLAN_MAC_RATE_6M){
					default_unicast_rate--;
				} else {
					default_unicast_rate = WLAN_MAC_RATE_6M;
				}

				for(i=0; i < next_free_assoc_index; i++){
					associations[i].tx_rate = default_unicast_rate;
				}

				xil_printf("(-) Default Unicast Rate: %d Mbps\n", wlan_lib_mac_rate_to_mbps(default_unicast_rate));
			break;
			case ASCII_R:
				if(default_unicast_rate < WLAN_MAC_RATE_54M){
					default_unicast_rate++;
				} else {
					default_unicast_rate = WLAN_MAC_RATE_54M;
				}

				for(i=0; i < next_free_assoc_index; i++){
					associations[i].tx_rate = default_unicast_rate;
				}
				xil_printf("(+) Default Unicast Rate: %d Mbps\n", wlan_lib_mac_rate_to_mbps(default_unicast_rate));
			break;
		}
	}
}

int ethernet_receive(packet_bd_list* tx_queue_list, u8* eth_dest, u8* eth_src, u16 tx_length){
	//Receives the pre-encapsulated Ethernet frames

	packet_bd* tx_queue = tx_queue_list->first;

	u32 i;
	u8 is_associated = 0;

	tx_header_common.address_1 = (u8*)(&(eth_dest[0]));
	tx_header_common.address_3 = (u8*)(&(eth_src[0]));

	wlan_create_data_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame, &tx_header_common, MAC_FRAME_CTRL2_FLAG_FROM_DS);
	((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.length = tx_length;

	if(wlan_addr_eq(bcast_addr, eth_dest)){
		tx_queue->metadata_ptr = NULL;
		((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.retry_max = 0;
		((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.flags = 0;
		enqueue_after_end(0, tx_queue_list);
		check_tx_queue();

	} else {
		//Check associations
		//Is this packet meant for a station we are associated with?
		for(i=0; i < next_free_assoc_index; i++) {
			if(wlan_addr_eq(associations[i].addr, eth_dest)) {
				is_associated = 1;
				break;
			}
		}
		if(is_associated) {
			tx_queue->metadata_ptr = (void*)&(associations[i]);
			((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.retry_max = MAX_RETRY;
			((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.flags = (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
			enqueue_after_end(associations[i].AID, tx_queue_list);
			check_tx_queue();
		} else {
			//Checkin this packet_bd so that it can be checked out again
			return 0;
		}
	}

	return 1;

}

void print_queue_status(){
	u32 i;
	xil_printf("\nQueue Status:\n");
	xil_printf(" FREE || BCAST|");

	for(i=0; i<next_free_assoc_index; i++){
		xil_printf("%6d|", associations[i].AID);
	}
	xil_printf("\n");

	xil_printf("%6d||%6d|",queue_num_free(),queue_num_queued(0));

	for(i=0; i<next_free_assoc_index; i++){
		xil_printf("%6d|", queue_num_queued(associations[i].AID));
	}
	xil_printf("\n");

}

void beacon_transmit() {
 	u16 tx_length;
 	packet_bd_list checkout;
 	packet_bd*	tx_queue;

 	//Checkout 1 element from the queue;
 	checkout = queue_checkout(1);

 	if(checkout.length == 1){ //There was at least 1 free queue element
 		tx_queue = checkout.first;
 		tx_header_common.address_1 = bcast_addr;
		tx_header_common.address_3 = eeprom_mac_addr;
 		tx_length = wlan_create_beacon_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame,&tx_header_common, BEACON_INTERVAL_MS, strlen(access_point_ssid), (u8*)access_point_ssid, mac_param_chan);
 		((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.length = tx_length;
 		tx_queue->metadata_ptr = NULL;
 		((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.flags = TX_MPDU_FLAGS_FILL_TIMESTAMP;
 		enqueue_after_end(0, &checkout);
 		check_tx_queue();
 	}

 	//Schedule the next beacon transmission
 	wlan_mac_schedule_event(SCHEDULE_COARSE,BEACON_INTERVAL_US, (void*)beacon_transmit);

 	return;
}

void association_timestamp_check() {

	u32 i, num_queued;
	u64 time_since_last_rx;
	packet_bd_list checkout,dequeue;
	packet_bd* tx_queue;
	u32 tx_length;

	for(i=0; i < next_free_assoc_index; i++) {

		time_since_last_rx = (get_usec_timestamp() - associations[i].rx_timestamp);
		if(time_since_last_rx > ASSOCIATION_TIMEOUT_US){
			//Send De-authentication

		 	//Checkout 1 element from the queue;
		 	checkout = queue_checkout(1);

		 	if(checkout.length == 1){ //There was at least 1 free queue element
		 		tx_queue = checkout.first;
		 		tx_header_common.address_1 = associations[i].addr;
				tx_header_common.address_3 = eeprom_mac_addr;
		 		tx_length = wlan_create_deauth_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame, &tx_header_common, DEAUTH_REASON_INACTIVITY);
		 		((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.length = tx_length;
		 		tx_queue->metadata_ptr = (void*)&(associations[i]);
		 		((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.retry_max = MAX_RETRY;
		 		((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.flags = (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
		 		enqueue_after_end(associations[i].AID, &checkout);
		 		check_tx_queue();

		 		//Purge any packets in the queue meant for this node
				num_queued = queue_num_queued(associations[i].AID);
				if(num_queued>0){
					xil_printf("purging %d packets from queue for AID %d\n",num_queued,associations[i].AID);
					dequeue = dequeue_from_beginning(associations[i].AID,1);
					queue_checkin(&dequeue);
				}

				//Remove this STA from association list
				if(next_free_assoc_index > 0) next_free_assoc_index--;
				memcpy(&(associations[i].addr[0]), bcast_addr, 6);
				if(i < next_free_assoc_index) {
					//Copy from current index to the swap space
					memcpy(&(associations[MAX_ASSOCIATIONS]), &(associations[i]), sizeof(station_info));

					//Shift later entries back into the freed association entry
					memcpy(&(associations[i]), &(associations[i+1]), (next_free_assoc_index-i)*sizeof(station_info));

					//Copy from swap space to current free index
					memcpy(&(associations[next_free_assoc_index]), &(associations[MAX_ASSOCIATIONS]), sizeof(station_info));
				}

				xil_printf("\n\nDisassociation due to inactivity:\n");
				print_associations();

			}

		}


	}

	wlan_mac_schedule_event(SCHEDULE_COARSE,ASSOCIATION_CHECK_INTERVAL_US, (void*)association_timestamp_check);
	return;
}

void mpdu_rx_process(void* pkt_buf_addr, u8 rate, u16 length) {
	void * mpdu = pkt_buf_addr + PHY_RX_PKT_BUF_MPDU_OFFSET;
	u8* mpdu_ptr_u8 = (u8*)mpdu;
	u16 tx_length;
	u8 send_response, allow_association, allow_disassociation, new_association;
	mac_header_80211* rx_80211_header;
	rx_80211_header = (mac_header_80211*)((void *)mpdu_ptr_u8);
	u16 rx_seq;
	packet_bd_list checkout;
	packet_bd*	tx_queue;

	rx_frame_info* mpdu_info = (rx_frame_info*)pkt_buf_addr;

	u32 i;
	u8 is_associated = 0;
	new_association = 0;

	for(i=0; i < next_free_assoc_index; i++) {
		if(wlan_addr_eq(associations[i].addr, (rx_80211_header->address_2))) {
			is_associated = 1;
			rx_seq = ((rx_80211_header->sequence_control)>>4)&0xFFF;
			//Check if duplicate
			associations[i].rx_timestamp = get_usec_timestamp();
			associations[i].last_rx_power = mpdu_info->rx_power;

			if( (associations[i].seq != 0)  && (associations[i].seq == rx_seq) ) {
				//Received seq num matched previously received seq num for this STA; ignore the MPDU and return
				return;

			} else {
				associations[i].seq = rx_seq;
			}

			break;
		}
	}

	switch(rx_80211_header->frame_control_1) {
		case (MAC_FRAME_CTRL1_SUBTYPE_DATA): //Data Packet
			if(is_associated){
				if((rx_80211_header->frame_control_2) & MAC_FRAME_CTRL2_FLAG_TO_DS) {
					//MPDU is flagged as destined to the DS - send it for de-encapsulation and Ethernet Tx
					wlan_mpdu_eth_send(mpdu,length);
				}
			} else {
				//TODO: Formally adopt conventions from 10.3 in 802.11-2012 for STA state transitions

				if((rx_80211_header->address_3[0] == 0x33) && (rx_80211_header->address_3[1] == 0x33)){
					//TODO: This is an IPv6 Multicast packet. It should get de-encapsulated and sent over the wire
				} else {
					//Received a data frame from a STA that claims to be associated with this AP but is not in the AP association table
					// Discard the MPDU and reply with a de-authentication frame to trigger re-association at the STA

					warp_printf(PL_WARNING, "Data from non-associated station: [%x %x %x %x %x %x], issuing de-authentication\n", rx_80211_header->address_2[0],rx_80211_header->address_2[1],rx_80211_header->address_2[2],rx_80211_header->address_2[3],rx_80211_header->address_2[4],rx_80211_header->address_2[5]);
					warp_printf(PL_WARNING, "Address 3: [%x %x %x %x %x %x]\n", rx_80211_header->address_3[0],rx_80211_header->address_3[1],rx_80211_header->address_3[2],rx_80211_header->address_3[3],rx_80211_header->address_3[4],rx_80211_header->address_3[5]);

					//Send De-authentication
					//Checkout 1 element from the queue;
					 	checkout = queue_checkout(1);

					 	if(checkout.length == 1){ //There was at least 1 free queue element
					 		tx_queue = checkout.first;
					 		tx_header_common.address_1 = rx_80211_header->address_2;
							tx_header_common.address_3 = eeprom_mac_addr;
					 		tx_length = wlan_create_deauth_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame, &tx_header_common, DEAUTH_REASON_NONASSOCIATED_STA);
					 		((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.length = tx_length;
					 		tx_queue->metadata_ptr = NULL;
					 		((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.retry_max = MAX_RETRY;
					 		((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.flags = (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
							enqueue_after_end(0, &checkout);
							check_tx_queue();
					 	}
				}
			}//END if(is_associated)

		break;

		case (MAC_FRAME_CTRL1_SUBTYPE_PROBE_REQ): //Probe Request Packet
			if(wlan_addr_eq(rx_80211_header->address_3, bcast_addr)) {
				//BSS Id: Broadcast
				mpdu_ptr_u8 += sizeof(mac_header_80211);
				while(((u32)mpdu_ptr_u8 -  (u32)mpdu)<= length){ //Loop through tagged parameters
					switch(mpdu_ptr_u8[0]){ //What kind of tag is this?
						case TAG_SSID_PARAMS: //SSID parameter set
							if((mpdu_ptr_u8[1]==0) || (memcmp(mpdu_ptr_u8+2, (u8*)access_point_ssid,mpdu_ptr_u8[1])==0)) {
								//Broadcast SSID or my SSID - send unicast probe response
								send_response = 1;
							}
						break;
						case TAG_SUPPORTED_RATES: //Supported rates
						break;
						case TAG_EXT_SUPPORTED_RATES: //Extended supported rates
						break;
						case TAG_DS_PARAMS: //DS Parameter set (e.g. channel)
						break;
					}
					mpdu_ptr_u8 += mpdu_ptr_u8[1]+2; //Move up to the next tag
				}
				if(send_response && allow_assoc) {

					//Checkout 1 element from the queue;
					checkout = queue_checkout(1);

					if(checkout.length == 1){ //There was at least 1 free queue element
						tx_queue = checkout.first;
						tx_header_common.address_1 = rx_80211_header->address_2;
						tx_header_common.address_3 = eeprom_mac_addr;
						tx_length = wlan_create_probe_resp_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame, &tx_header_common, BEACON_INTERVAL_MS, strlen(access_point_ssid), (u8*)access_point_ssid, mac_param_chan);
						((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.length = tx_length;
						tx_queue->metadata_ptr = NULL;
						((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.retry_max = MAX_RETRY;
						((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.flags = (TX_MPDU_FLAGS_FILL_TIMESTAMP | TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
						enqueue_after_end(0, &checkout);
						check_tx_queue();
					}

					return;
				}
			}
		break;

		case (MAC_FRAME_CTRL1_SUBTYPE_AUTH): //Authentication Packet
			if(wlan_addr_eq(rx_80211_header->address_3, eeprom_mac_addr)) {
					mpdu_ptr_u8 += sizeof(mac_header_80211);
					switch(((authentication_frame*)mpdu_ptr_u8)->auth_algorithm){
						case AUTH_ALGO_OPEN_SYSTEM:
							if(((authentication_frame*)mpdu_ptr_u8)->auth_sequence == AUTH_SEQ_REQ){//This is an auth packet from a requester

								//Checkout 1 element from the queue;
								checkout = queue_checkout(1);

								if(checkout.length == 1){ //There was at least 1 free queue element
									tx_queue = checkout.first;
									tx_header_common.address_1 = rx_80211_header->address_2;
									tx_header_common.address_3 = eeprom_mac_addr;
									tx_length = wlan_create_auth_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame, &tx_header_common, AUTH_ALGO_OPEN_SYSTEM, AUTH_SEQ_RESP, STATUS_SUCCESS);
									((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.length = tx_length;
									tx_queue->metadata_ptr = NULL;
									((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.retry_max = MAX_RETRY;
									((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.flags = (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
									enqueue_after_end(0, &checkout);
									check_tx_queue();
								}

								return;
							}
						break;
						default:

							//Checkout 1 element from the queue;
							checkout = queue_checkout(1);

							if(checkout.length == 1){ //There was at least 1 free queue element
								tx_queue = checkout.first;
								tx_header_common.address_1 = rx_80211_header->address_2;
								tx_header_common.address_3 = eeprom_mac_addr;
								tx_length = wlan_create_auth_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame, &tx_header_common, AUTH_ALGO_OPEN_SYSTEM, AUTH_SEQ_RESP, STATUS_AUTH_REJECT_CHALLENGE_FAILURE);
								((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.length = tx_length;
								tx_queue->metadata_ptr = NULL;
								((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.retry_max = MAX_RETRY;
								((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.flags = (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
								enqueue_after_end(0, &checkout);
								check_tx_queue();
							}

							warp_printf(PL_WARNING,"Unsupported authentication algorithm (0x%x)\n", ((authentication_frame*)mpdu_ptr_u8)->auth_algorithm);
							return;
						break;
					}
				}
		break;

		case (MAC_FRAME_CTRL1_SUBTYPE_REASSOC_REQ): //Re-association Request
		case (MAC_FRAME_CTRL1_SUBTYPE_ASSOC_REQ): //Association Request
			if(wlan_addr_eq(rx_80211_header->address_3, eeprom_mac_addr)) {
				for(i=0; i <= next_free_assoc_index; i++) {
					if(wlan_addr_eq((associations[i].addr), bcast_addr)) {
						allow_association = 1;
						new_association = 1;

						if(next_free_assoc_index < (MAX_ASSOCIATIONS-2)) next_free_assoc_index++;
						break;

					} else if(wlan_addr_eq((associations[i].addr), rx_80211_header->address_2)) {
						allow_association = 1;
						new_association = 0;
						break;
					}
				}

				if(allow_association) {
					//Keep track of this association of this association
					memcpy(&(associations[i].addr[0]), rx_80211_header->address_2, 6);
					associations[i].tx_rate = default_unicast_rate; //Default tx_rate for this station. Rate adaptation may change this value.
					associations[i].num_tx_total = 0;
					associations[i].num_tx_success = 0;

					//associations[i].tx_rate = WLAN_MAC_RATE_16QAM34; //Default tx_rate for this station. Rate adaptation may change this value.

					//Checkout 1 element from the queue;
					checkout = queue_checkout(1);

					if(checkout.length == 1){ //There was at least 1 free queue element
						tx_queue = checkout.first;
						tx_header_common.address_1 = rx_80211_header->address_2;
						tx_header_common.address_3 = eeprom_mac_addr;
						tx_length = wlan_create_association_response_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame, &tx_header_common, STATUS_SUCCESS, associations[i].AID);
						((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.length = tx_length;
						tx_queue->metadata_ptr = (void*)&(associations[i]);
						((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.retry_max = MAX_RETRY;
						((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.flags = (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
						enqueue_after_end(associations[i].AID, &checkout);
						check_tx_queue();
					}

					if(new_association == 1) {
						xil_printf("\n\nNew Association - ID %d\n", associations[i].AID);

						//Print the updated association table to the UART (slow, but useful for observing association success)
						print_associations();
					}

					return;
				}

			}
		break;

		case (MAC_FRAME_CTRL1_SUBTYPE_DISASSOC): //Disassociation
				if(wlan_addr_eq(rx_80211_header->address_3, eeprom_mac_addr)) {
					for(i=0;i<next_free_assoc_index;i++){
						if(wlan_addr_eq(associations[i].addr, rx_80211_header->address_2)) {
								allow_disassociation = 1;
								if(next_free_assoc_index > 0) next_free_assoc_index--;
							break;
						}
					}

					if(allow_disassociation) {
						//Remove this STA from association list
						memcpy(&(associations[i].addr[0]), bcast_addr, 6);
						if(i < next_free_assoc_index) {
							//Copy from current index to the swap space
							memcpy(&(associations[MAX_ASSOCIATIONS]), &(associations[i]), sizeof(station_info));

							//Shift later entries back into the freed association entry
							memcpy(&(associations[i]), &(associations[i+1]), (next_free_assoc_index-i)*sizeof(station_info));

							//Copy from swap space to current free index
							memcpy(&(associations[next_free_assoc_index]), &(associations[MAX_ASSOCIATIONS]), sizeof(station_info));
						}


						xil_printf("\n\nDisassociation:\n");
						print_associations();

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

void print_associations(){
	u64 timestamp = get_usec_timestamp();
	u32 i;

	write_hex_display(next_free_assoc_index);
	xil_printf("\n   Current Associations\n (MAC time = %d usec)\n",timestamp);
			xil_printf("|-ID-|----- MAC ADDR ----|\n");
	for(i=0; i < next_free_assoc_index; i++){
		if(wlan_addr_eq(associations[i].addr, bcast_addr)) {
			xil_printf("| %02x |                   |\n", associations[i].AID);
		} else {
			xil_printf("| %02x | %02x:%02x:%02x:%02x:%02x:%02x |\n", associations[i].AID,
					associations[i].addr[0],associations[i].addr[1],associations[i].addr[2],associations[i].addr[3],associations[i].addr[4],associations[i].addr[5]);
		}
	}
			xil_printf("|------------------------|\n");

	return;
}

void enable_associations(){
	xil_printf("Allowing new associations\n");

	wlan_ipc_msg ipc_msg_to_low;
	u32 ipc_msg_to_low_payload[1];
	ipc_config_phy_rx* config_phy_rx;
	//Send a message to other processor to tell it to switch channels
	ipc_msg_to_low.msg_id = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_PHY_RX);
	ipc_msg_to_low.num_payload_words = sizeof(ipc_config_phy_rx)/sizeof(u32);
	ipc_msg_to_low.payload_ptr = &(ipc_msg_to_low_payload[0]);
	init_ipc_config(config_phy_rx,ipc_msg_to_low_payload,ipc_config_phy_rx);
	config_phy_rx->enable_dsss = 1;
	ipc_mailbox_write_msg(&ipc_msg_to_low);
	allow_assoc = 1;

}

void disable_associations(){

	wlan_ipc_msg ipc_msg_to_low;
	u32 ipc_msg_to_low_payload[1];
	ipc_config_phy_rx* config_phy_rx;
	//Send a message to other processor to tell it to switch channels

	if(perma_assoc_mode == 0){
		xil_printf("Not allowing new associations\n");

		ipc_msg_to_low.msg_id = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_PHY_RX);
		ipc_msg_to_low.num_payload_words = sizeof(ipc_config_phy_rx)/sizeof(u32);
		ipc_msg_to_low.payload_ptr = &(ipc_msg_to_low_payload[0]);
		init_ipc_config(config_phy_rx,ipc_msg_to_low_payload,ipc_config_phy_rx);
		config_phy_rx->enable_dsss = 0;
		ipc_mailbox_write_msg(&ipc_msg_to_low);
		allow_assoc = 0;
		enable_animation = 0;
		write_hex_display(next_free_assoc_index);
		write_hex_display_dots(0);
	}
}

void animate_hex(){
	static u8 i = 0;
	if(enable_animation){
		//write_hex_display(next_free_assoc_index,i%2);
		write_hex_display_dots(i%2);
		i++;
		wlan_mac_schedule_event(SCHEDULE_COARSE, ANIMATION_RATE_US, (void*)animate_hex);
	}
}


void print_menu(){
	xil_printf("\f");
	xil_printf("********************** AP Menu **********************\n");
	xil_printf("[1] - Interactive AP Status\n");
	xil_printf("[2] - Print Queue Status\n");
	xil_printf("\n");
	xil_printf("[c/C] - change channel (note: changing channel will\n");
	xil_printf("        purge any associations, forcing stations to\n");
	xil_printf("        join the network again)\n");
	xil_printf("[r/R] - change default unicast rate\n");
	xil_printf("*****************************************************\n");
}

void print_station_status(){
	u32 i;
	u64 timestamp;

	if(interactive_mode){
		timestamp = get_usec_timestamp();
		xil_printf("\f");

		for(i=0; i < next_free_assoc_index; i++){
			xil_printf("---------------------------------------------------\n");
			xil_printf(" AID: %02x -- MAC Addr: %02x:%02x:%02x:%02x:%02x:%02x\n", associations[i].AID,
					associations[i].addr[0],associations[i].addr[1],associations[i].addr[2],associations[i].addr[3],associations[i].addr[4],associations[i].addr[5]);
			xil_printf("     - Last heard from %d ms ago\n",((u32)(timestamp - (associations[i].rx_timestamp)))/1000);
			xil_printf("     - Last Rx Power: %d dBm\n",associations[i].last_rx_power);
			xil_printf("     - # of queued MPDUs: %d\n", queue_num_queued(associations[i].AID));
			xil_printf("     - # Tx MPDUs: %d (%d successful)\n", associations[i].num_tx_total, associations[i].num_tx_success);

		}
		    xil_printf("---------------------------------------------------\n");
		    xil_printf("\n");
		    xil_printf("[r] - reset statistics\n");
		    xil_printf("[d] - deauthenticate all stations\n");


		//Update display
		wlan_mac_schedule_event(SCHEDULE_COARSE, 1000000, (void*)print_station_status);
	}
}

void reset_station_statistics(){
	u32 i;
	for(i=0; i < next_free_assoc_index; i++){
		associations[i].num_tx_total = 0;
		associations[i].num_tx_success = 0;
	}
}

void deauthenticate_stations(){
	u32 i;
	packet_bd_list checkout,dequeue;
	u32 num_queued;
	packet_bd* tx_queue;
	u32 tx_length;

	for(i=0; i < next_free_assoc_index; i++){
		//Send De-authentication

	 	//Checkout 1 element from the queue;
	 	checkout = queue_checkout(1);

	 	if(checkout.length == 1){ //There was at least 1 free queue element
	 		tx_queue = checkout.first;
	 		tx_header_common.address_1 = associations[i].addr;
			tx_header_common.address_3 = eeprom_mac_addr;
	 		tx_length = wlan_create_deauth_frame((void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame, &tx_header_common, DEAUTH_REASON_INACTIVITY);
	 		((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.length = tx_length;
	 		tx_queue->metadata_ptr = (void*)&(associations[i]);
	 		((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.retry_max = MAX_RETRY;
	 		((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.flags = (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
	 		enqueue_after_end(associations[i].AID, &checkout);
	 		check_tx_queue();

	 		//Purge any packets in the queue meant for this node
	 		num_queued = queue_num_queued(associations[i].AID);
	 		if(num_queued>0){
	 			xil_printf("purging %d packets from queue for AID %d\n",num_queued,associations[i].AID);
	 			dequeue = dequeue_from_beginning(associations[i].AID,1);
	 			queue_checkin(&dequeue);
	 		}

			//Remove this STA from association list
			if(next_free_assoc_index > 0) next_free_assoc_index--;
			memcpy(&(associations[i].addr[0]), bcast_addr, 6);
			if(i < next_free_assoc_index) {
				//Copy from current index to the swap space
				memcpy(&(associations[MAX_ASSOCIATIONS]), &(associations[i]), sizeof(station_info));

				//Shift later entries back into the freed association entry
				memcpy(&(associations[i]), &(associations[i+1]), (next_free_assoc_index-i)*sizeof(station_info));

				//Copy from swap space to current free index
				memcpy(&(associations[next_free_assoc_index]), &(associations[MAX_ASSOCIATIONS]), sizeof(station_info));
			}

		}
	}
	write_hex_display(next_free_assoc_index);
}


