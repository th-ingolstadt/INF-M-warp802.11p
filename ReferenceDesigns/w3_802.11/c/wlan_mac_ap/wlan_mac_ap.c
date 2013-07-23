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

//WARP includes
#include "wlan_lib.h"
#include "wlan_mac_ap.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_util.h"


#define BEACON_INTERVAL_MS (100)
#define BEACON_INTERVAL_US (BEACON_INTERVAL_MS*1000)

#define MAX_RETRY 7

#define SSID_LEN 4
u8 SSID[SSID_LEN] = "WARP";

u16 seq_num;

#define MAX_ASSOCIATIONS 8
//The last entry in associations[MAX_ASSOCIATIONS][] is swap space
#define ASSOC_ENTRY_SIZE_BYTES 8

// AID - Addr[6] - Last Seq
u8 associations[MAX_ASSOCIATIONS+1][ASSOC_ENTRY_SIZE_BYTES];
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
	u32 ipc_msg_to_low_payload[1];
	tx_frame_info* tx_mpdu;
	u32 i;

	xil_printf("\f----- wlan_mac_ap -----\n");
	xil_printf("Compiled %s %s\n", __DATE__, __TIME__);

	wlan_lib_init();
	wlan_mac_util_init();

	for(i=0;i < NUM_TX_PKT_BUFS; i++){
		tx_mpdu = (tx_frame_info*)TX_PKT_BUF_TO_ADDR(i);
		tx_mpdu->state = TX_MPDU_STATE_EMPTY;
	}

	wlan_mac_util_set_eth_rx_callback((void*)ethernet_receive);

	//create IPC message to receive into
	ipc_msg_from_low.payload_ptr = &(ipc_msg_from_low_payload[0]);

	bcast_addr[0] = 0xFF;
	bcast_addr[1] = 0xFF;
	bcast_addr[2] = 0xFF;
	bcast_addr[3] = 0xFF;
	bcast_addr[4] = 0xFF;
	bcast_addr[5] = 0xFF;

	next_free_assoc_index = 0;
	for(i=0;i<MAX_ASSOCIATIONS;i++){
		associations[i][0] = (1+i); //7.3.1.8 of 802.11-2007
		memset((void*)(&(associations[i][1])), 0xFF,6);
		associations[i][7] = 0; //seq
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
		if(is_tx_buffer_empty()) wlan_mac_poll_eth(tx_pkt_buf);

		//Poll mailbox read msg
		if(ipc_mailbox_read_msg(&ipc_msg_from_low) == IPC_MBOX_SUCCESS){
			process_ipc_msg_from_low(&ipc_msg_from_low);
		}

	}
	return -1;
}

void ethernet_receive(u8* eth_dest, u8* eth_src, u16 tx_length){
	//Receives the pre-encapsulated Ethernet frames

	u32 i;
	u8 is_associated = 0;

	u8* mpdu_ptr_u8 = (u8*)(TX_PKT_BUF_TO_ADDR(tx_pkt_buf) + PHY_TX_PKT_BUF_MPDU_OFFSET);

	wlan_create_data_frame((void*) mpdu_ptr_u8, MAC_FRAME_CTRL2_FLAG_FROM_DS, (u8*)(&(eth_dest[0])), (u8*)(&(eeprom_mac_addr[0])), (u8*)(&(eth_src[0])), seq_num++);

	if((memcmp(&(bcast_addr[0]),&(eth_dest[0]),6)==0)){
		mpdu_transmit(tx_length,WLAN_MAC_RATE_QPSK34,0,0);
	} else {
		//Check associations
		//Is this packet meant for a station we are associated with?
		for(i=0;i<next_free_assoc_index;i++){
			if((memcmp(&(associations[i][1]),&(eth_dest[0]),6)==0)){
				is_associated = 1;
			}
		}
		if(is_associated){
			mpdu_transmit(tx_length,WLAN_MAC_RATE_QPSK34,MAX_RETRY,TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO);
		}
	}
}

void beacon_transmit(){
 	u16 tx_length;
	if(is_tx_buffer_empty()){
		tx_length = wlan_create_beacon_probe_frame((void*)(TX_PKT_BUF_TO_ADDR(tx_pkt_buf)+PHY_TX_PKT_BUF_MPDU_OFFSET), MAC_FRAME_CTRL1_SUBTYPE_BEACON, bcast_addr, eeprom_mac_addr, eeprom_mac_addr, seq_num++,BEACON_INTERVAL_MS, SSID_LEN, SSID, mac_param_chan, eeprom_mac_addr);
		mpdu_transmit(tx_length,WLAN_MAC_RATE_BPSK12,0,TX_MPDU_FLAGS_FILL_TIMESTAMP);
	}
	wlan_mac_schedule_event(BEACON_INTERVAL_US, (void*)beacon_transmit);
}

void wait_for_tx_accept(){
	//This function blocks until the cpu_high_status is changed to no longer waiting for IPC ACCEPT
	while(cpu_high_status & CPU_STATUS_WAIT_FOR_IPC_ACCEPT){
		if(ipc_mailbox_read_msg(&ipc_msg_from_low) == IPC_MBOX_SUCCESS){
			process_ipc_msg_from_low(&ipc_msg_from_low);
		}
	}
}

void process_ipc_msg_from_low(wlan_ipc_msg* msg){
	u8 rx_pkt_buf;
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
					}
				break;
				case IPC_MBOX_CMD_TX_MPDU_DONE:
					//MB-Low has finished transmitting the packet buffer in msg->arg0
					tx_mpdu = (tx_frame_info*)TX_PKT_BUF_TO_ADDR(msg->arg0);

					//xil_printf("Tx from %d Status: %d, retry_count: %d\n", msg->arg0, tx_mpdu->state_verbose, tx_mpdu->retry_count);
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
				warp_printf(PL_ERROR, "An unrecoverable exception has occured in CPU_LOW, halting...\n");
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

	u32 i;

	switch(rx_80211_header->frame_control_1){

		case (MAC_FRAME_CTRL1_SUBTYPE_DATA): //Data Packet
			for(i=0;i<next_free_assoc_index;i++){
				if((memcmp(&(associations[i][1]),rx_80211_header->address_2,6)==0)){

					rx_seq = ((rx_80211_header->sequence_control)>>4)&0xFFF;

					//Check if duplicate
					if(associations[i][7] !=0  && associations[i][7] == rx_seq){
						//Data was duplicated. Here we just drop it and don't pass it up
					} else {
						//Data not duplicated
						associations[i][7] = rx_seq;
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
				if(send_response && is_tx_buffer_empty()){
					tx_length = wlan_create_beacon_probe_frame((void*)(TX_PKT_BUF_TO_ADDR(tx_pkt_buf)+PHY_TX_PKT_BUF_MPDU_OFFSET), MAC_FRAME_CTRL1_SUBTYPE_PROBE_RESP, rx_80211_header->address_2, eeprom_mac_addr, eeprom_mac_addr, seq_num++,BEACON_INTERVAL_MS, SSID_LEN, SSID, mac_param_chan, eeprom_mac_addr);
					mpdu_transmit(tx_length,WLAN_MAC_RATE_QPSK34,MAX_RETRY,(TX_MPDU_FLAGS_FILL_TIMESTAMP | TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO));
					return;
				}
			}
		break;

		case (MAC_FRAME_CTRL1_SUBTYPE_AUTH): //Authentication Packet
			if(memcmp(rx_80211_header->address_3,eeprom_mac_addr,6)==0 && is_tx_buffer_empty()){
					mpdu_ptr_u8 += sizeof(mac_header_80211);
					switch(((authentication_frame*)mpdu_ptr_u8)->auth_algorithm){
						case AUTH_ALGO_OPEN_SYSTEM:
							if(((authentication_frame*)mpdu_ptr_u8)->auth_sequence == AUTH_SEQ_REQ){//This is an auth packet from a requester
								tx_length = wlan_create_auth_frame((void*)(TX_PKT_BUF_TO_ADDR(tx_pkt_buf)+PHY_TX_PKT_BUF_MPDU_OFFSET), AUTH_ALGO_OPEN_SYSTEM, AUTH_SEQ_RESP, STATUS_SUCCESS, rx_80211_header->address_2, eeprom_mac_addr, eeprom_mac_addr, seq_num++, eeprom_mac_addr);
								mpdu_transmit(tx_length,WLAN_MAC_RATE_QPSK34,MAX_RETRY,(TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO));
								return;
							}
						break;
						default:
							tx_length = wlan_create_auth_frame((void*)(TX_PKT_BUF_TO_ADDR(tx_pkt_buf)+PHY_TX_PKT_BUF_MPDU_OFFSET), AUTH_ALGO_OPEN_SYSTEM, AUTH_SEQ_RESP, STATUS_AUTH_REJECT_CHALLENGE_FAILURE, rx_80211_header->address_2, eeprom_mac_addr, eeprom_mac_addr, seq_num++, eeprom_mac_addr);
							mpdu_transmit(tx_length,WLAN_MAC_RATE_QPSK34,MAX_RETRY,(TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO));
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
					if((memcmp(&(associations[i][1]),bcast_addr,6)==0)){
						allow_association = 1;
						if(next_free_assoc_index < (MAX_ASSOCIATIONS-2)) next_free_assoc_index++;
						break;
					} else if((memcmp(&(associations[i][1]),rx_80211_header->address_2,6)==0)){
						allow_association = 1;
						break;
					}
				}

				if(allow_association && is_tx_buffer_empty()){
					//Keep track of this association of this association
					memcpy(&(associations[i][1]),rx_80211_header->address_2,6);

					tx_length = wlan_create_association_response_frame((void*)(TX_PKT_BUF_TO_ADDR(tx_pkt_buf)+PHY_TX_PKT_BUF_MPDU_OFFSET), MAC_FRAME_CTRL1_SUBTYPE_ASSOC_RESP, rx_80211_header->address_2, eeprom_mac_addr, eeprom_mac_addr, seq_num++, STATUS_SUCCESS, 0xC000 | associations[i][0],eeprom_mac_addr);
					mpdu_transmit(tx_length,WLAN_MAC_RATE_QPSK34,MAX_RETRY,(TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO));
					xil_printf("\n\nAssociation:\n");
					print_associations();

					return;
				}

			}
		break;

		case (MAC_FRAME_CTRL1_SUBTYPE_DISASSOC): //Disassociation
				if(memcmp(rx_80211_header->address_3,eeprom_mac_addr,6)==0){
					for(i=0;i<next_free_assoc_index;i++){
						if((memcmp(&(associations[i][1]),rx_80211_header->address_2,6)==0)){
								allow_disassociation = 1;
								if(next_free_assoc_index > 0) next_free_assoc_index--;
							break;
						}
					}
					if(allow_disassociation){
						//Remove this STA from association list
						memcpy(&(associations[i][1]),bcast_addr,6);
						if(i < (next_free_assoc_index)){
							//Copy from current index to the swap space
							memcpy(&(associations[MAX_ASSOCIATIONS][0]),&(associations[i][0]),7);
							//Shift later entries back into the freed association entry
							memcpy(&(associations[i][0]),&(associations[i+1][0]), (next_free_assoc_index-i)*ASSOC_ENTRY_SIZE_BYTES);
							//xil_printf("Moving %d bytes\n",(next_free_assoc_index-i)*7);
							//Copy from swap space to current free index
							memcpy(&(associations[next_free_assoc_index][0]),&(associations[MAX_ASSOCIATIONS][0]),7);
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

void mpdu_transmit(u16 length, u8 rate, u8 retry_max, u8 flags){
	wlan_ipc_msg ipc_msg_to_low;
	tx_frame_info* tx_mpdu = (tx_frame_info*) TX_PKT_BUF_TO_ADDR(tx_pkt_buf);

	tx_mpdu->length = length;
	tx_mpdu->rate = rate;
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
		wait_for_tx_accept();
	}
	return;
}

void print_associations(){
	u64 timestamp = get_usec_timestamp();
	u32 i;

	write_hex_display(next_free_assoc_index);

	xil_printf("\n@[%x], current associations:\n",timestamp);
		xil_printf("|-ID-|-----------MAC ADDR----------|\n");
	for(i=0;i<next_free_assoc_index;i++){
		if(memcmp(&(associations[i][1]),bcast_addr,6)==0){
			xil_printf("| %02x |    |    |    |    |    |    |\n",associations[i][0]);
		} else {
			xil_printf("| %02x | %02x | %02x | %02x | %02x | %02x | %02x |\n",associations[i][0],associations[i][1],associations[i][2],associations[i][3],associations[i][4],associations[i][5],associations[i][6]);
		}
	}
		xil_printf("|----------------------------------|\n");
}

void write_hex_display(u8 val){
	//u8 val: 2 digit decimal value to be printed to hex
	//CPU_LOW has the User I/O core, so this function wraps a call to the mailbox to command the CPU_LOW to display
	//something to the hex displays

	val = val&63; //[0,99]

	wlan_ipc_msg ipc_msg_to_low;
	ipc_msg_to_low.msg_id = (IPC_MBOX_GRP_ID(IPC_MBOX_GRP_CMD)) | (IPC_MBOX_MSG_ID_TO_MSG(IPC_MBOX_CMD_WRITE_HEX));
	ipc_msg_to_low.arg0 = val;
	ipc_msg_to_low.num_payload_words = 0;
	ipc_mailbox_write_msg(&ipc_msg_to_low);
}


