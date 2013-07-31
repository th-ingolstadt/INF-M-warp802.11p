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

#define BEACON_INTERVAL_MS (100)
#define BEACON_INTERVAL_US (BEACON_INTERVAL_MS*1000)

#define ASSOCIATION_CHECK_INTERVAL_MS (10000)
#define ASSOCIATION_CHECK_INTERVAL_US (ASSOCIATION_CHECK_INTERVAL_MS*1000)

#define MAX_RETRY 7

#define SSID_LEN 7
u8 SSID[SSID_LEN] = "WARP-AP";

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



	//Wait for CPU_LOW to report that it has fully initialized and is ready for traffic
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

	mac_param_chan = 9;

	//Send a message to other processor to tell it to switch channels
	ipc_msg_to_low.msg_id = IPC_MBOX_GRP_ID(IPC_MBOX_GRP_PARAM) | IPC_MBOX_MSG_ID(IPC_MBOX_PARAM_SET_CHANNEL);
	ipc_msg_to_low.num_payload_words = 1;
	ipc_msg_to_low.payload_ptr = &(ipc_msg_to_low_payload[0]);
	ipc_msg_to_low_payload[0] = mac_param_chan;
	ipc_mailbox_write_msg(&ipc_msg_to_low);

	wlan_mac_schedule_event(BEACON_INTERVAL_US, (void*)beacon_transmit);
	wlan_mac_schedule_event(ASSOCIATION_CHECK_INTERVAL_US, (void*)association_timestamp_check);

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

	u32 i;
	u8 is_associated = 0;

	wlan_create_data_frame((void*) tx_queue->frame, MAC_FRAME_CTRL2_FLAG_FROM_DS, (u8*)(&(eth_dest[0])), (u8*)(&(eeprom_mac_addr[0])), (u8*)(&(eth_src[0])), seq_num++);
	tx_queue->frame_info.length = tx_length;

	if(wlan_addr_eq(bcast_addr, eth_dest)){
		//TODO: Remember to set current retry to 0 when popping from queue
		tx_queue->station_info_ptr = NULL;
		tx_queue->frame_info.retry_max = 0;
		tx_queue->frame_info.flags = 0;
		wlan_mac_enqueue(LOW_PRI_QUEUE_SEL);

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
			tx_queue->station_info_ptr = &(associations[i]);
			tx_queue->frame_info.retry_max = MAX_RETRY;
			tx_queue->frame_info.flags = (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
			wlan_mac_enqueue(LOW_PRI_QUEUE_SEL);
		}
	}

	return;
}

void beacon_transmit() {
 	u16 tx_length;
 	packet_queue_element* tx_queue;
 	tx_queue = wlan_mac_queue_get_write_element(LOW_PRI_QUEUE_SEL);

 	if(tx_queue != NULL) {
		tx_length = wlan_create_beacon_probe_frame((void*)(tx_queue->frame), MAC_FRAME_CTRL1_SUBTYPE_BEACON, bcast_addr, eeprom_mac_addr, eeprom_mac_addr, seq_num++,BEACON_INTERVAL_MS, SSID_LEN, SSID, mac_param_chan, eeprom_mac_addr);
		tx_queue->station_info_ptr = NULL;
		tx_queue->frame_info.length = tx_length;
		tx_queue->frame_info.flags = TX_MPDU_FLAGS_FILL_TIMESTAMP;
		wlan_mac_enqueue(LOW_PRI_QUEUE_SEL);
	}

 	//Schedule the next beacon transmission
 	wlan_mac_schedule_event(BEACON_INTERVAL_US, (void*)beacon_transmit);

 	return;
}

void association_timestamp_check() {

	u32 i;
	u64 time_since_last_rx;
	packet_queue_element* tx_queue;
	u32 tx_length;

	for(i=0; i < next_free_assoc_index; i++) {

		time_since_last_rx = (get_usec_timestamp() - associations[i].rx_timestamp);
		if(time_since_last_rx > ASSOCIATION_TIMEOUT_US){
			//xil_printf("AID: %d, last heard from %d usec ago\n", associations[i].AID, (u32)time_since_last_rx);
			//Send De-authentication
			tx_queue = wlan_mac_queue_get_write_element(LOW_PRI_QUEUE_SEL);

			if(tx_queue != NULL){
				tx_length = wlan_create_deauth_frame((void*)(tx_queue->frame), DEAUTH_REASON_INACTIVITY, associations[i].addr, eeprom_mac_addr, eeprom_mac_addr, seq_num++, eeprom_mac_addr);
				tx_queue->station_info_ptr = NULL;
				tx_queue->frame_info.length = tx_length;
				tx_queue->frame_info.retry_max = MAX_RETRY;
				tx_queue->frame_info.flags = (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
				wlan_mac_enqueue(LOW_PRI_QUEUE_SEL);

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



	wlan_mac_schedule_event(ASSOCIATION_CHECK_INTERVAL_US, (void*)association_timestamp_check);
	return;
}

void process_ipc_msg_from_low(wlan_ipc_msg* msg) {
	u8 rx_pkt_buf;
	u16 i;
	rx_frame_info* rx_mpdu;
	tx_frame_info* tx_mpdu;

	switch(IPC_MBOX_MSG_ID_TO_GRP(msg->msg_id)) {
		case IPC_MBOX_GRP_CMD:

			switch(IPC_MBOX_MSG_ID_TO_MSG(msg->msg_id)) {
				case IPC_MBOX_CMD_RX_MPDU_READY:
					//This message indicates CPU Low has received an MPDU addressed to this node or to the broadcast address
					rx_pkt_buf = msg->arg0;

					//First attempt to lock the indicated Rx pkt buf (CPU Low must unlock it before sending this msg)
					if(lock_pkt_buf_rx(rx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
						warp_printf(PL_ERROR,"Error: unable to lock pkt_buf %d\n",rx_pkt_buf);
					} else {
						rx_mpdu = (rx_frame_info*)RX_PKT_BUF_TO_ADDR(rx_pkt_buf);

						//xil_printf("MB-HIGH: processing buffer %d, mpdu state = %d, length = %d, rate = %d\n",rx_pkt_buf,rx_mpdu->state, rx_mpdu->length,rx_mpdu->rate);
						mpdu_rx_process((void*)(RX_PKT_BUF_TO_ADDR(rx_pkt_buf)), rx_mpdu->rate, rx_mpdu->length);

						//Free up the rx_pkt_buf
						rx_mpdu->state = RX_MPDU_STATE_EMPTY;

						if(unlock_pkt_buf_rx(rx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
							warp_printf(PL_ERROR, "Error: unable to unlock pkt_buf %d\n",rx_pkt_buf);
						}
					}
				break;

				case IPC_MBOX_CMD_TX_MPDU_ACCEPT:
					//This message indicates CPU Low has begun the Tx process for the previously submitted MPDU
					// CPU High is now free to begin processing its next Tx frame and submit it to CPU Low
					// CPU Low will not accept a new frame until the previous one is complete

					if(tx_pkt_buf != (msg->arg0)) {
						warp_printf(PL_ERROR, "Received CPU_LOW acceptance of buffer %d, but was expecting buffer %d\n", tx_pkt_buf, msg->arg0);
					}

					tx_pkt_buf = (tx_pkt_buf + 1) % TX_BUFFER_NUM;

					cpu_high_status &= (~CPU_STATUS_WAIT_FOR_IPC_ACCEPT);

					if(lock_pkt_buf_tx(tx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS) {
						warp_printf(PL_ERROR,"Error: unable to lock tx pkt_buf %d\n",tx_pkt_buf);
					} else {
						tx_mpdu = (tx_frame_info*)TX_PKT_BUF_TO_ADDR(tx_pkt_buf);
						tx_mpdu->state = TX_MPDU_STATE_TX_PENDING;

						//Poll the Tx queue to retrieve the next frame for transmission
						wlan_mac_poll_tx_queue();
					}
				break;

				case IPC_MBOX_CMD_TX_MPDU_DONE:
					//This message indicates CPU Low has finished the Tx process for the previously submitted-accepted frame
					// CPU High should do any necessary post-processing, then recycle the packet buffer

					tx_mpdu = (tx_frame_info*)TX_PKT_BUF_TO_ADDR(msg->arg0);

					if(tx_mpdu->AID != 0){
						for(i=0; i<next_free_assoc_index; i++){
							if( (associations[i].AID) == (tx_mpdu->AID) ) {
								//Process this TX MPDU DONE event to update any statistics used in rate adaptation
								wlan_mac_util_process_tx_done(tx_mpdu, &(associations[i]));
								break;
							}
						}
					}

				break;

				default:
					warp_printf(PL_ERROR, "Unknown IPC message type %d\n",IPC_MBOX_MSG_ID_TO_MSG(msg->msg_id));
				break;
			}
		break; //END case MSG_GROUP == IPC_MBOX_GRP_CMD

		case IPC_MBOX_GRP_MAC_ADDR:
			//CPU Low updated the node's MAC address (typically stored in the WARP v3 EEPROM, accessible only to CPU Low)
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
	} //END switch(MSG_GROUP)

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
	packet_queue_element* tx_queue;

	u32 i;
	u8 is_associated = 0;
	new_association = 0;

	for(i=0; i < next_free_assoc_index; i++) {
		if(wlan_addr_eq(associations[i].addr, (rx_80211_header->address_2))) {
			is_associated = 1;
			rx_seq = ((rx_80211_header->sequence_control)>>4)&0xFFF;
			//Check if duplicate
			associations[i].rx_timestamp = get_usec_timestamp();
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
					wlan_mac_send_eth(mpdu,length);
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
					tx_queue = wlan_mac_queue_get_write_element(HIGH_PRI_QUEUE_SEL);

					if(tx_queue != NULL){
						tx_length = wlan_create_deauth_frame((void*)(tx_queue->frame), DEAUTH_REASON_NONASSOCIATED_STA, rx_80211_header->address_2, eeprom_mac_addr, eeprom_mac_addr, seq_num++, eeprom_mac_addr);
						tx_queue->station_info_ptr = NULL;
						tx_queue->frame_info.length = tx_length;
						tx_queue->frame_info.retry_max = MAX_RETRY;
						tx_queue->frame_info.flags = (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
						wlan_mac_enqueue(HIGH_PRI_QUEUE_SEL);
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
							if((mpdu_ptr_u8[1]==0) || (memcmp(mpdu_ptr_u8+2, (u8*)SSID,mpdu_ptr_u8[1])==0)) {
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
				if(send_response) {
					tx_queue = wlan_mac_queue_get_write_element(HIGH_PRI_QUEUE_SEL);
					if(tx_queue != NULL){
						tx_length = wlan_create_beacon_probe_frame((void*)(tx_queue->frame), MAC_FRAME_CTRL1_SUBTYPE_PROBE_RESP, rx_80211_header->address_2, eeprom_mac_addr, eeprom_mac_addr, seq_num++,BEACON_INTERVAL_MS, SSID_LEN, SSID, mac_param_chan, eeprom_mac_addr);
						tx_queue->station_info_ptr = NULL;
						tx_queue->frame_info.length = tx_length;
						tx_queue->frame_info.retry_max = MAX_RETRY;
						tx_queue->frame_info.flags = (TX_MPDU_FLAGS_FILL_TIMESTAMP | TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
						wlan_mac_enqueue(HIGH_PRI_QUEUE_SEL);
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
								tx_queue = wlan_mac_queue_get_write_element(HIGH_PRI_QUEUE_SEL);
								if(tx_queue != NULL){
									tx_length = wlan_create_auth_frame((void*)(tx_queue->frame), AUTH_ALGO_OPEN_SYSTEM, AUTH_SEQ_RESP, STATUS_SUCCESS, rx_80211_header->address_2, eeprom_mac_addr, eeprom_mac_addr, seq_num++, eeprom_mac_addr);
									tx_queue->station_info_ptr = NULL;
									tx_queue->frame_info.length = tx_length;
									tx_queue->frame_info.retry_max = MAX_RETRY;
									tx_queue->frame_info.flags = (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
									wlan_mac_enqueue(HIGH_PRI_QUEUE_SEL);
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
								wlan_mac_enqueue(HIGH_PRI_QUEUE_SEL);
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
					associations[i].tx_rate = WLAN_MAC_RATE_QPSK34; //Default tx_rate for this station. Rate adaptation may change this value.

					tx_queue = wlan_mac_queue_get_write_element(HIGH_PRI_QUEUE_SEL);
					if(tx_queue != NULL){
						tx_length = wlan_create_association_response_frame((void*)(tx_queue->frame), MAC_FRAME_CTRL1_SUBTYPE_ASSOC_RESP, rx_80211_header->address_2, eeprom_mac_addr, eeprom_mac_addr, seq_num++, STATUS_SUCCESS, 0xC000 | associations[i].AID,eeprom_mac_addr);
						tx_queue->station_info_ptr = NULL;
						tx_queue->frame_info.length = tx_length;
						tx_queue->frame_info.retry_max = MAX_RETRY;
						tx_queue->frame_info.flags = (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
						wlan_mac_enqueue(HIGH_PRI_QUEUE_SEL);
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

int is_tx_buffer_empty(){
	tx_frame_info* tx_mpdu = (tx_frame_info*) TX_PKT_BUF_TO_ADDR(tx_pkt_buf);
	if(tx_mpdu->state == TX_MPDU_STATE_TX_PENDING && ((cpu_high_status & CPU_STATUS_WAIT_FOR_IPC_ACCEPT) == 0)){
		return 1;
	} else {
		return 0;
	}
}

void mpdu_transmit(packet_queue_element* tx_queue) {
	wlan_ipc_msg ipc_msg_to_low;
	tx_frame_info* tx_mpdu = (tx_frame_info*) TX_PKT_BUF_TO_ADDR(tx_pkt_buf);
	station_info* station = tx_queue->station_info_ptr;

	if(is_tx_buffer_empty()){

		//For now, this is just a one-shot DMA transfer that effectively blocks
		while(XAxiCdma_IsBusy(&cdma_inst)) {}
		XAxiCdma_SimpleTransfer(&cdma_inst, (u32)&(tx_queue->frame_info), (u32)TX_PKT_BUF_TO_ADDR(tx_pkt_buf), tx_queue->frame_info.length + sizeof(tx_frame_info) + PHY_TX_PKT_BUF_PHY_HDR_SIZE, NULL, NULL);
		while(XAxiCdma_IsBusy(&cdma_inst)) {}

		//TODO:
		//Consider breaking up the transfer from TX queue to the outgoing tx_pkt_buf into two DMA transfers.
		//	1)	The first will cover the frame_info metadata + PHY_TX_PKT_BUF_PHY_HDR_SIZE + an 802.11 header's worth of bytes + 8 (another u64 for the timestamp in beacons)
		//		CPU_HIGH will block on this transfer.
		//	2)	The second will cover the rest of the frame. CPU_HIGH will not block on this transfer and will immediately notify CPU_LOW that a frame
		//		is ready to be transmitted.

		if(station == NULL){
			//Broadcast transmissions have no station information, so we default to a nominal rate
			tx_mpdu->AID = 0;
			tx_mpdu->rate = WLAN_MAC_RATE_BPSK12;
		} else {
			//Request the rate to use for this station
			tx_mpdu->AID = station->AID;
			tx_mpdu->rate = wlan_mac_util_get_tx_rate(station);
		}

		tx_mpdu->state = TX_MPDU_STATE_READY;
		tx_mpdu->retry_count = 0;

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



