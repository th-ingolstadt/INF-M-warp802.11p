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
#include "xtmrctr.h"
#include "xio.h"
#include "string.h"
#include "xaxicdma.h"

//WARP includes
#include "wlan_lib.h"
#include "wlan_mac_util.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_ap.h"


#define ENABLE_RATE_ADAPTATION 0

#define BEACON_INTERVAL_MS (100)
#define BEACON_INTERVAL_US (BEACON_INTERVAL_MS*1000)

#define MAX_RETRY 7

#define SSID_LEN 4
u8 SSID[SSID_LEN] = "WARP";

u16 seq_num;

XAxiCdma cdma_inst;

//The last entry in associations[MAX_ASSOCIATIONS][] is swap space
//#define ASSOC_ENTRY_SIZE_BYTES 8

// AID - Addr[6] - Last Seq
//u8 associations[MAX_ASSOCIATIONS+1][ASSOC_ENTRY_SIZE_BYTES];
station_info associations[MAX_ASSOCIATIONS+1];
u32 next_free_assoc_index;




static u32 mac_param_chan;

wlan_ipc_msg ipc_msg_from_low;
u32 ipc_msg_from_low_payload[10];

static u8 eeprom_mac_addr[6];
static u8 bcast_addr[6];

static u32 cpu_low_status;
static u32 cpu_high_status;

#define TX_BUFFER_NUM 2
u8 tx_pkt_buf;

int main(){
	wlan_ipc_msg ipc_msg_to_low;
	u32 status;
	u32 ipc_msg_to_low_payload[1];
	tx_frame_info* tx_mpdu;
	u32 i;

	xil_printf("\f----- wlan_mac_ap -----\n");
	xil_printf("Compiled %s %s\n", __DATE__, __TIME__);

	wlan_lib_init();
	wlan_mac_util_init();

	//Initialize the central DMA (CDMA) driver
	XAxiCdma_Config *cdma_cfg_ptr;
	cdma_cfg_ptr = XAxiCdma_LookupConfig(XPAR_AXI_CDMA_0_DEVICE_ID);
	status = XAxiCdma_CfgInitialize(&cdma_inst, cdma_cfg_ptr, cdma_cfg_ptr->BaseAddress);
	if (status != XST_SUCCESS) {
		warp_printf(PL_ERROR,"Error initializing CDMA: %d\n", status);
	}
	XAxiCdma_IntrDisable(&cdma_inst, XAXICDMA_XR_IRQ_ALL_MASK);



	for(i=0;i < NUM_TX_PKT_BUFS; i++){
		tx_mpdu = (tx_frame_info*)TX_PKT_BUF_TO_ADDR(i);
		tx_mpdu->state = TX_MPDU_STATE_EMPTY;
	}

	wlan_mac_util_set_eth_rx_callback((void*)ethernet_receive);
	wlan_mac_util_set_mpdu_tx_callback((void*)mpdu_transmit);

	//create IPC message to receive into
	ipc_msg_from_low.payload_ptr = &(ipc_msg_from_low_payload[0]);

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



	//Wait for mb_low to report that it has fully initialized and is ready for traffic
	do{
		//Poll mailbox read msg
		if(ipc_mailbox_read_msg(&ipc_msg_from_low) == IPC_MBOX_SUCCESS){
			process_ipc_msg_from_low(&ipc_msg_from_low);
		}
	} while ((cpu_low_status & CPU_STATUS_INITIALIZED) == 0);

	write_hex_display(0);

	tx_pkt_buf = 0;
	if(lock_pkt_buf_tx(tx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
		warp_printf(PL_ERROR,"Error: unable to lock pkt_buf %d\n",tx_pkt_buf);
	}

	tx_mpdu = (tx_frame_info*)TX_PKT_BUF_TO_ADDR(tx_pkt_buf);
	tx_mpdu->state = TX_MPDU_STATE_TX_PENDING;

	cpu_high_status |= CPU_STATUS_INITIALIZED;

	mac_param_chan = 4;

	//Send a message to other processor to tell it to switch channels
	ipc_msg_to_low.msg_id = IPC_MBOX_GRP_ID(IPC_MBOX_GRP_PARAM) | IPC_MBOX_MSG_ID(IPC_MBOX_PARAM_SET_CHANNEL);
	ipc_msg_to_low.num_payload_words = 1;
	ipc_msg_to_low.payload_ptr = &(ipc_msg_to_low_payload[0]);
	ipc_msg_to_low_payload[0] = mac_param_chan;
	ipc_mailbox_write_msg(&ipc_msg_to_low);

	wlan_mac_schedule_event(BEACON_INTERVAL_US, (void*)beacon_transmit);

	while(1){
		//Poll Scheduler
		poll_schedule();

		//Poll Ethernet
		//if(is_tx_buffer_empty()) wlan_mac_poll_eth(tx_pkt_buf);
		wlan_mac_poll_eth();

		//Poll Wireless Transmit Queue
		if((cpu_high_status & CPU_STATUS_WAIT_FOR_IPC_ACCEPT) == 0) wlan_mac_poll_tx_queue();

		//Poll mailbox read msg
		if(ipc_mailbox_read_msg(&ipc_msg_from_low) == IPC_MBOX_SUCCESS){
			process_ipc_msg_from_low(&ipc_msg_from_low);
		}

	}
	return -1;
}

void ethernet_receive(packet_queue_element* tx_queue, u8* eth_dest, u8* eth_src, u16 tx_length){
	//Receives the pre-encapsulated Ethernet frames

//void mpdu_transmit_OLD(station_info* station, u16 length, u8 retry_max, u8 flags)
	u32 i;
	u8 is_associated = 0;
	//volatile tx_frame_info frame_info = tx_queue->frame_info;

	wlan_create_data_frame((void*) tx_queue->frame, MAC_FRAME_CTRL2_FLAG_FROM_DS, (u8*)(&(eth_dest[0])), (u8*)(&(eeprom_mac_addr[0])), (u8*)(&(eth_src[0])), seq_num++);
	tx_queue->frame_info.length = tx_length;

	if((memcmp(&(bcast_addr[0]),&(eth_dest[0]),6)==0)){
		//mpdu_transmit_OLD(NULL,tx_length,0,0);
		//TODO: Remember to sent current retry to 0 when popping from queue
		tx_queue->station_info_ptr = NULL;
		tx_queue->frame_info.retry_max = 0;
		tx_queue->frame_info.flags = 0;
		wlan_mac_queue_push(LOW_PRI_QUEUE_SEL);

	} else {
		//Check associations
		//Is this packet meant for a station we are associated with?
		for(i=0;i<next_free_assoc_index;i++){
			if((memcmp(&(associations[i].addr[0]),&(eth_dest[0]),6)==0)){
				is_associated = 1;
				break;
			}
		}
		if(is_associated){
			tx_queue->station_info_ptr = &(associations[i]);
			tx_queue->frame_info.retry_max = MAX_RETRY;
			tx_queue->frame_info.flags = TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO;
			wlan_mac_queue_push(LOW_PRI_QUEUE_SEL);

		//	mpdu_transmit_OLD(&(associations[i]),tx_length,MAX_RETRY,TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
		}
	}
}

void beacon_transmit(){
 	u16 tx_length;
 	packet_queue_element* tx_queue;
 	tx_queue = wlan_mac_queue_get_write_element(LOW_PRI_QUEUE_SEL);
 	if(tx_queue != NULL){
		tx_length = wlan_create_beacon_probe_frame((void*)(tx_queue->frame), MAC_FRAME_CTRL1_SUBTYPE_BEACON, bcast_addr, eeprom_mac_addr, eeprom_mac_addr, seq_num++,BEACON_INTERVAL_MS, SSID_LEN, SSID, mac_param_chan, eeprom_mac_addr);
		tx_queue->station_info_ptr = NULL;
		tx_queue->frame_info.length = tx_length;
		tx_queue->frame_info.flags = TX_MPDU_FLAGS_FILL_TIMESTAMP;
		wlan_mac_queue_push(LOW_PRI_QUEUE_SEL);
		//mpdu_transmit_OLD(NULL,tx_length,0,TX_MPDU_FLAGS_FILL_TIMESTAMP);
	}


	wlan_mac_schedule_event(BEACON_INTERVAL_US, (void*)beacon_transmit);
}

//void wait_for_tx_accept(){
	//This function blocks until the cpu_high_status is changed to no longer waiting for IPC ACCEPT
//	while(cpu_high_status & CPU_STATUS_WAIT_FOR_IPC_ACCEPT){
//		if(ipc_mailbox_read_msg(&ipc_msg_from_low) == IPC_MBOX_SUCCESS){
//			process_ipc_msg_from_low(&ipc_msg_from_low);
//		}
//	}
//}

void process_ipc_msg_from_low(wlan_ipc_msg* msg){
	u8 rx_pkt_buf;
	u16 i;
	rx_frame_info* rx_mpdu;
	tx_frame_info* tx_mpdu;

	switch(IPC_MBOX_MSG_ID_TO_GRP(msg->msg_id)){
		case IPC_MBOX_GRP_CMD:

			switch(IPC_MBOX_MSG_ID_TO_MSG(msg->msg_id)){
				case IPC_MBOX_CMD_RX_MPDU_READY:
					//Message is an indication that an Rx Pkt Buf needs processing
					rx_pkt_buf = msg->arg0;

					if(lock_pkt_buf_rx(rx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
						warp_printf(PL_ERROR,"Error: unable to lock pkt_buf %d\n",rx_pkt_buf);
					} else {
						rx_mpdu = (rx_frame_info*)RX_PKT_BUF_TO_ADDR(rx_pkt_buf);

						//xil_printf("MB-HIGH: processing buffer %d, mpdu state = %d, length = %d, rate = %d\n",rx_pkt_buf,rx_mpdu->state, rx_mpdu->length,rx_mpdu->rate);
						mpdu_process((void*)(RX_PKT_BUF_TO_ADDR(rx_pkt_buf)), rx_mpdu->rate, rx_mpdu->length);

						//Free up the rx_pkt_buf
						rx_mpdu->state = RX_MPDU_STATE_EMPTY;

						if(unlock_pkt_buf_rx(rx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
							warp_printf(PL_ERROR,"Error: unable to unlock pkt_buf %d\n",rx_pkt_buf);
						}
					}
				break;
				case IPC_MBOX_CMD_TX_MPDU_ACCEPT:
					cpu_high_status &= (~CPU_STATUS_WAIT_FOR_IPC_ACCEPT);

					if(tx_pkt_buf != msg->arg0){
						warp_printf(PL_ERROR,"Received CPU_LOW acceptance of buffer %d, but was expecting buffer %d\n", tx_pkt_buf, msg->arg0);
					}

					tx_pkt_buf = (tx_pkt_buf + 1) % TX_BUFFER_NUM;
					if(lock_pkt_buf_tx(tx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
						warp_printf(PL_ERROR,"Error: unable to lock tx pkt_buf %d\n",tx_pkt_buf);
					} else {
						tx_mpdu = (tx_frame_info*)TX_PKT_BUF_TO_ADDR(tx_pkt_buf);
						tx_mpdu->state = TX_MPDU_STATE_TX_PENDING;
						wlan_mac_poll_tx_queue();
					}
				break;
				case IPC_MBOX_CMD_TX_MPDU_DONE:
					//CPU_LOW has finished transmitting the packet buffer in msg->arg0
					tx_mpdu = (tx_frame_info*)TX_PKT_BUF_TO_ADDR(msg->arg0);

					if(tx_mpdu->AID != 0){
						for(i=0; i<next_free_assoc_index; i++){
							if(associations[i].AID == tx_mpdu->AID){
								//Process this TX MPDU DONE event to update any statistics used in rate adaptation
								wlan_mac_util_process_tx_done(tx_mpdu,&(associations[i]));
								break;
							}
						}
					}

				break;
				default:
					warp_printf(PL_ERROR, "Unknown IPC message type %d\n",IPC_MBOX_MSG_ID_TO_MSG(msg->msg_id));
				break;
			}
		break;
		case IPC_MBOX_GRP_MAC_ADDR:
			memcpy((void*) &(eeprom_mac_addr[0]), (void*) &(ipc_msg_from_low_payload[0]), 6);

		break;
		case IPC_MBOX_GRP_CPU_STATUS:
			cpu_low_status = ipc_msg_from_low_payload[0];
			if(cpu_low_status & CPU_STATUS_EXCEPTION){
				warp_printf(PL_ERROR, "An unrecoverable exception has occurred in CPU_LOW, halting...\n");
				warp_printf(PL_ERROR, "Reason code: %d\n", ipc_msg_from_low_payload[1]);
				while(1){}
			}
		break;
		default:
			warp_printf(PL_ERROR,"ERROR: Unknown IPC message group %d\n", IPC_MBOX_MSG_ID_TO_GRP(msg->msg_id));
		break;

	}
}

void mpdu_process(void* pkt_buf_addr, u8 rate, u16 length){
	void * mpdu = pkt_buf_addr + PHY_RX_PKT_BUF_MPDU_OFFSET;
	u8* mpdu_ptr_u8 = (u8*)mpdu;
	u16 tx_length;
	u8 send_response, allow_association, allow_disassociation;
	mac_header_80211* rx_80211_header;
	rx_80211_header = (mac_header_80211*)((void *)mpdu_ptr_u8);
	u16 rx_seq;
	packet_queue_element* tx_queue;

	u32 i;

	switch(rx_80211_header->frame_control_1){

		case (MAC_FRAME_CTRL1_SUBTYPE_DATA): //Data Packet
			for(i=0;i<next_free_assoc_index;i++){
				if((memcmp(&(associations[i].addr[0]),rx_80211_header->address_2,6)==0)){

					rx_seq = ((rx_80211_header->sequence_control)>>4)&0xFFF;

					//Check if duplicate
					if(associations[i].seq !=0  && associations[i].seq == rx_seq){
						//Data was duplicated. Here we just drop it and don't pass it up
					} else {
						//Data not duplicated
						associations[i].seq = rx_seq;
						if((rx_80211_header->frame_control_2) & MAC_FRAME_CTRL2_FLAG_TO_DS) wlan_mac_send_eth(mpdu,length);
					}
					return;
				}
			}

		break;

		case (MAC_FRAME_CTRL1_SUBTYPE_PROBE_REQ): //Probe Request Packet
			if(memcmp(rx_80211_header->address_3,bcast_addr,6)==0){//BSS Id: Broadcast
				mpdu_ptr_u8 += sizeof(mac_header_80211);
				while(((u32)mpdu_ptr_u8 -  (u32)mpdu)<= length){ //Loop through tagged parameters
					switch(mpdu_ptr_u8[0]){ //What kind of tag is this?
						case TAG_SSID_PARAMS: //SSID parameter set
							if((mpdu_ptr_u8[1]==0) || (memcmp(mpdu_ptr_u8+2, (u8*)SSID,mpdu_ptr_u8[1])==0)){ //Broadcast SSID or my SSID
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
				if(send_response){
					tx_queue = wlan_mac_queue_get_write_element(HIGH_PRI_QUEUE_SEL);
					if(tx_queue != NULL){
						tx_length = wlan_create_beacon_probe_frame((void*)(tx_queue->frame), MAC_FRAME_CTRL1_SUBTYPE_PROBE_RESP, rx_80211_header->address_2, eeprom_mac_addr, eeprom_mac_addr, seq_num++,BEACON_INTERVAL_MS, SSID_LEN, SSID, mac_param_chan, eeprom_mac_addr);
						tx_queue->station_info_ptr = NULL;
						tx_queue->frame_info.length = tx_length;
						tx_queue->frame_info.retry_max = MAX_RETRY;
						tx_queue->frame_info.flags = (TX_MPDU_FLAGS_FILL_TIMESTAMP | TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
						wlan_mac_queue_push(HIGH_PRI_QUEUE_SEL);
					}
					return;
				}
			}
		break;

		case (MAC_FRAME_CTRL1_SUBTYPE_AUTH): //Authentication Packet
			if(memcmp(rx_80211_header->address_3,eeprom_mac_addr,6)==0){
					mpdu_ptr_u8 += sizeof(mac_header_80211);
					switch(((authentication_frame*)mpdu_ptr_u8)->auth_algorithm){
						case AUTH_ALGO_OPEN_SYSTEM:
							if(((authentication_frame*)mpdu_ptr_u8)->auth_sequence == AUTH_SEQ_REQ){//This is an auth packet from a requester
								tx_queue = wlan_mac_queue_get_write_element(HIGH_PRI_QUEUE_SEL);
								if(tx_queue != NULL){
									tx_length = wlan_create_auth_frame((void*)(tx_queue->frame), AUTH_ALGO_OPEN_SYSTEM, AUTH_SEQ_RESP, STATUS_SUCCESS, rx_80211_header->address_2, eeprom_mac_addr, eeprom_mac_addr, seq_num++, eeprom_mac_addr);
									tx_queue->station_info_ptr = NULL;
									tx_queue->frame_info.length = tx_length;
									tx_queue->frame_info.retry_max = MAX_RETRY;
									tx_queue->frame_info.flags = (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
									wlan_mac_queue_push(HIGH_PRI_QUEUE_SEL);
								}
								return;
							}
						break;
						default:
							tx_queue = wlan_mac_queue_get_write_element(HIGH_PRI_QUEUE_SEL);
							if(tx_queue != NULL){
								tx_length = wlan_create_auth_frame((void*)(tx_queue->frame), AUTH_ALGO_OPEN_SYSTEM, AUTH_SEQ_RESP, STATUS_AUTH_REJECT_CHALLENGE_FAILURE, rx_80211_header->address_2, eeprom_mac_addr, eeprom_mac_addr, seq_num++, eeprom_mac_addr);
								tx_queue->station_info_ptr = NULL;
								tx_queue->frame_info.length = tx_length;
								tx_queue->frame_info.retry_max = MAX_RETRY;
								tx_queue->frame_info.flags = (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
								wlan_mac_queue_push(HIGH_PRI_QUEUE_SEL);
							}
							warp_printf(PL_WARNING,"Unsupported authentication algorithm (0x%x)\n", ((authentication_frame*)mpdu_ptr_u8)->auth_algorithm);
							return;
						break;
					}
				}
		break;

		case (MAC_FRAME_CTRL1_SUBTYPE_REASSOC_REQ): //Re-association Request
		case (MAC_FRAME_CTRL1_SUBTYPE_ASSOC_REQ): //Association Request
			if(memcmp(rx_80211_header->address_3,eeprom_mac_addr,6)==0){
				for(i=0;i<=next_free_assoc_index;i++){
					if((memcmp(&(associations[i].addr[0]),bcast_addr,6)==0)){
						allow_association = 1;
						if(next_free_assoc_index < (MAX_ASSOCIATIONS-2)) next_free_assoc_index++;
						break;
					} else if((memcmp(&(associations[i].addr[0]),rx_80211_header->address_2,6)==0)){
						allow_association = 1;
						break;
					}
				}

				if(allow_association){
					//Keep track of this association of this association
					memcpy(&(associations[i].addr[0]),rx_80211_header->address_2,6);
					associations[i].tx_rate = WLAN_MAC_RATE_QPSK34; //Default tx_rate for this station. Rate adaptation may change this value.

					//tx_length = wlan_create_association_response_frame((void*)(TX_PKT_BUF_TO_ADDR(tx_pkt_buf)+PHY_TX_PKT_BUF_MPDU_OFFSET), MAC_FRAME_CTRL1_SUBTYPE_ASSOC_RESP, rx_80211_header->address_2, eeprom_mac_addr, eeprom_mac_addr, seq_num++, STATUS_SUCCESS, 0xC000 | associations[i].AID,eeprom_mac_addr);
					//mpdu_transmit_OLD(NULL,tx_length,MAX_RETRY,(TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO));
					tx_queue = wlan_mac_queue_get_write_element(HIGH_PRI_QUEUE_SEL);
					if(tx_queue != NULL){
						tx_length = wlan_create_association_response_frame((void*)(tx_queue->frame), MAC_FRAME_CTRL1_SUBTYPE_ASSOC_RESP, rx_80211_header->address_2, eeprom_mac_addr, eeprom_mac_addr, seq_num++, STATUS_SUCCESS, 0xC000 | associations[i].AID,eeprom_mac_addr);
						tx_queue->station_info_ptr = NULL;
						tx_queue->frame_info.length = tx_length;
						tx_queue->frame_info.retry_max = MAX_RETRY;
						tx_queue->frame_info.flags = (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
						wlan_mac_queue_push(HIGH_PRI_QUEUE_SEL);
					}
					xil_printf("\n\nAssociation:\n");
					print_associations();

					return;
				}

			}
		break;

		case (MAC_FRAME_CTRL1_SUBTYPE_DISASSOC): //Disassociation
				if(memcmp(rx_80211_header->address_3,eeprom_mac_addr,6)==0){
					for(i=0;i<next_free_assoc_index;i++){
						if((memcmp(&(associations[i].addr[0]),rx_80211_header->address_2,6)==0)){
								allow_disassociation = 1;
								if(next_free_assoc_index > 0) next_free_assoc_index--;
							break;
						}
					}
					if(allow_disassociation){
						//Remove this STA from association list
						memcpy(&(associations[i].addr[0]),bcast_addr,6);
						if(i < (next_free_assoc_index)){
							//Copy from current index to the swap space
							memcpy(&(associations[MAX_ASSOCIATIONS]),&(associations[i]),sizeof(station_info));
							//Shift later entries back into the freed association entry
							memcpy(&(associations[i]),&(associations[i+1]), (next_free_assoc_index-i)*sizeof(station_info));
							//xil_printf("Moving %d bytes\n",(next_free_assoc_index-i)*7);
							//Copy from swap space to current free index
							memcpy(&(associations[next_free_assoc_index]),&(associations[MAX_ASSOCIATIONS]),sizeof(station_info));
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
}

int is_tx_buffer_empty(){
	tx_frame_info* tx_mpdu = (tx_frame_info*) TX_PKT_BUF_TO_ADDR(tx_pkt_buf);
	if(tx_mpdu->state == TX_MPDU_STATE_TX_PENDING && ((cpu_high_status & CPU_STATUS_WAIT_FOR_IPC_ACCEPT) == 0)){
		return 1;
	} else {
		return 0;
	}
}

/*void mpdu_transmit_OLD(station_info* station, u16 length, u8 retry_max, u8 flags){
	wlan_ipc_msg ipc_msg_to_low;
	tx_frame_info* tx_mpdu = (tx_frame_info*) TX_PKT_BUF_TO_ADDR(tx_pkt_buf);

	if(station == NULL){
		//Broadcast transmissions have no station information, so we default
		//to a nominal rate
		tx_mpdu->AID = 0;
		tx_mpdu->rate = WLAN_MAC_RATE_BPSK12;
	} else {
		tx_mpdu->AID = station->AID;
		#if ENABLE_RATE_ADAPTATION
			tx_mpdu->rate = wlan_mac_util_get_tx_rate(station);
		#else
			tx_mpdu->rate = WLAN_MAC_RATE_QPSK34;
		#endif
	}
	tx_mpdu->length = length;



	tx_mpdu->state = TX_MPDU_STATE_READY;
	tx_mpdu->retry_count = 0;
	tx_mpdu->retry_max	 = retry_max;
	tx_mpdu->flags = flags;

	ipc_msg_to_low.msg_id = (IPC_MBOX_GRP_ID(IPC_MBOX_GRP_CMD)) | (IPC_MBOX_MSG_ID_TO_MSG(IPC_MBOX_CMD_TX_MPDU_READY));
	ipc_msg_to_low.arg0 = tx_pkt_buf;
	ipc_msg_to_low.num_payload_words = 0;

	if(unlock_pkt_buf_tx(tx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
		warp_printf(PL_ERROR,"Error: unable to unlock tx pkt_buf %d\n",tx_pkt_buf);
	} else {
		cpu_high_status |= CPU_STATUS_WAIT_FOR_IPC_ACCEPT;
		ipc_mailbox_write_msg(&ipc_msg_to_low);
		//TODO: wait_for_tx_accept();
	}
	return;
}*/

void mpdu_transmit(packet_queue_element* tx_queue){
	wlan_ipc_msg ipc_msg_to_low;
	tx_frame_info* tx_mpdu = (tx_frame_info*) TX_PKT_BUF_TO_ADDR(tx_pkt_buf);
	station_info* station = tx_queue->station_info_ptr;
	u32 start_addr;
	int transfer_len;
	int safe_transfer_len;

	if(is_tx_buffer_empty()){
		//xil_printf("\nmpdu_transmit:\n");
		//xil_printf("len = %d, flags = 0x%x, first byte: 0x%x", tx_queue->frame_info.length, tx_queue->frame_info.flags, tx_queue->frame[0]);



		//memcpy((void*)TX_PKT_BUF_TO_ADDR(tx_pkt_buf), (void*)&(tx_queue->frame_info), tx_queue->frame_info.length + sizeof(tx_frame_info) + PHY_TX_PKT_BUF_PHY_HDR_SIZE);
		//

		//For now, this is just a one-shot DMA transfer that effectively blocks
		while(XAxiCdma_IsBusy(&cdma_inst)) {}
		XAxiCdma_SimpleTransfer(&cdma_inst, (u32)&(tx_queue->frame_info), (u32)TX_PKT_BUF_TO_ADDR(tx_pkt_buf), tx_queue->frame_info.length + sizeof(tx_frame_info) + PHY_TX_PKT_BUF_PHY_HDR_SIZE, NULL, NULL);
		while(XAxiCdma_IsBusy(&cdma_inst)) {}

		//TODO:
		//We break up the transfer from TX queue to the outgoing tx_pkt_buf into two DMA transfers.
		//	1)	The first will cover the frame_info metadata + PHY_TX_PKT_BUF_PHY_HDR_SIZE + an 802.11 header's worth of bytes + 8 (another u64 for the timestamp in beacons)
		//		CPU_HIGH will block on this transfer.
		//	2)	The second will cover the rest of the frame. CPU_HIGH will not block on this transfer and will immediately notify CPU_LOW that a frame
		//		is ready to be transmitted.

		//start_addr = (u32)&(tx_queue->frame_info);
		//transfer_len = tx_queue->frame_info.length + sizeof(tx_frame_info) + PHY_TX_PKT_BUF_PHY_HDR_SIZE;
		//safe_transfer_len = sizeof(tx_frame_info) + PHY_TX_PKT_BUF_PHY_HDR_SIZE + sizeof(mac_header_80211) + 8;

		//while(XAxiCdma_IsBusy(&cdma_inst)) {}
		//XAxiCdma_SimpleTransfer(&cdma_inst, start_addr, (u32)TX_PKT_BUF_TO_ADDR(tx_pkt_buf), min(transfer_len,safe_transfer_len), NULL, NULL);

		//transfer_len -= safe_transfer_len;

		//while(XAxiCdma_IsBusy(&cdma_inst)) {}

		//if(transfer_len > 0) XAxiCdma_SimpleTransfer(&cdma_inst, start_addr+transfer_len, (u32)TX_PKT_BUF_TO_ADDR(tx_pkt_buf)+transfer_len, transfer_len, NULL, NULL);

		if(station == NULL){
			//Broadcast transmissions have no station information, so we default
			//to a nominal rate
			tx_mpdu->AID = 0;
			tx_mpdu->rate = WLAN_MAC_RATE_BPSK12;
		} else {
			tx_mpdu->AID = station->AID;
			#if ENABLE_RATE_ADAPTATION
				tx_mpdu->rate = wlan_mac_util_get_tx_rate(station);
			#else
				tx_mpdu->rate = WLAN_MAC_RATE_QPSK34;
			#endif
		}

		tx_mpdu->state = TX_MPDU_STATE_READY;
		tx_mpdu->retry_count = 0;


		//xil_printf("\n");
		//xil_printf("tx_mpdu->length = %d\n",tx_mpdu->length);
		//xil_printf("tx_mpdu->rate =   %d\n",tx_mpdu->rate);
		//xil_printf("tx_mpdu->flags =   0x%02x\n",tx_mpdu->flags);
		//xil_printf("tx_mpdu->retry_max =   %d\n",tx_mpdu->retry_max);
		//xil_printf("tx_queue->frame[0] =   0x%x\n",tx_queue->frame[0]);


		ipc_msg_to_low.msg_id = (IPC_MBOX_GRP_ID(IPC_MBOX_GRP_CMD)) | (IPC_MBOX_MSG_ID_TO_MSG(IPC_MBOX_CMD_TX_MPDU_READY));
		ipc_msg_to_low.arg0 = tx_pkt_buf;
		ipc_msg_to_low.num_payload_words = 0;

		if(unlock_pkt_buf_tx(tx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
			warp_printf(PL_ERROR,"Error: unable to unlock tx pkt_buf %d\n",tx_pkt_buf);
		} else {
			cpu_high_status |= CPU_STATUS_WAIT_FOR_IPC_ACCEPT;
			ipc_mailbox_write_msg(&ipc_msg_to_low);
		}
	} else {
		warp_printf(PL_ERROR, "Bad state in mpdu_transmit. Attempting to transmit but tx_buffer %d is not empty\n",tx_pkt_buf);
	}
	return;
}


void print_associations(){
	u64 timestamp = get_usec_timestamp();
	u32 i;

	//write_hex_display(next_free_assoc_index);
	xil_printf("\n@[%x], current associations:\n",timestamp);
		xil_printf("|-ID-|-----------MAC ADDR----------|\n");
	for(i=0;i<next_free_assoc_index;i++){
		if(memcmp(&(associations[i].addr[0]),bcast_addr,6)==0){
			xil_printf("| %02x |    |    |    |    |    |    |\n",associations[i].AID);
		} else {
			xil_printf("| %02x | %02x | %02x | %02x | %02x | %02x | %02x |\n",associations[i].AID,associations[i].addr[0],associations[i].addr[1],associations[i].addr[2],associations[i].addr[3],associations[i].addr[4],associations[i].addr[5]);
		}
	}
		xil_printf("|----------------------------------|\n");
}



