////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_util.c
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

#include "xaxiethernet.h"
#include "xllfifo.h"
#include "xgpio.h"

#include "wlan_lib.h"
#include "wlan_mac_util.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_queue.h"

XAxiEthernet ETH_A_MAC_Instance;
XAxiEthernet_Config *ETH_A_MAC_CFG_ptr;
XLlFifo ETH_A_FIFO_Instance;
XLlFifo* FIFO_INST;

static XGpio GPIO_timestamp;

static function_ptr_t eth_rx_callback, mpdu_tx_callback;

//Scheduler
#define SCHEDULER_NUM_EVENTS 3
static u8 scheduler_in_use[SCHEDULER_NUM_EVENTS];
static function_ptr_t scheduler_callbacks[SCHEDULER_NUM_EVENTS];
static u64 scheduler_timestamps[SCHEDULER_NUM_EVENTS];

void wlan_mac_util_init(){
	wlan_eth_init();
	wlan_mac_queue_init();
	gpio_timestamp_initialize();
}

void wlan_mac_util_set_eth_rx_callback(void(*callback)()){
	eth_rx_callback = (function_ptr_t)callback;
}

void wlan_mac_util_set_mpdu_tx_callback(void(*callback)()){
	mpdu_tx_callback = (function_ptr_t)callback;
}

void gpio_timestamp_initialize(){
	XGpio_Initialize(&GPIO_timestamp, TIMESTAMP_GPIO_DEVICE_ID);
	XGpio_SetDataDirection(&GPIO_timestamp, TIMESTAMP_GPIO_LSB_CHAN, 0xFFFFFFFF);
	XGpio_SetDataDirection(&GPIO_timestamp, TIMESTAMP_GPIO_MSB_CHAN, 0xFFFFFFFF);
}

inline u64 get_usec_timestamp(){
	u32 timestamp_high_u32;
	u32 timestamp_low_u32;
	u64 timestamp_u64;
	timestamp_high_u32 = XGpio_DiscreteRead(&GPIO_timestamp,TIMESTAMP_GPIO_MSB_CHAN);
	timestamp_low_u32 = XGpio_DiscreteRead(&GPIO_timestamp,TIMESTAMP_GPIO_LSB_CHAN);
	timestamp_u64 = (((u64)timestamp_high_u32)<<32) + ((u64)timestamp_low_u32);
	return timestamp_u64;
}

void wlan_eth_init(){
		u32 status;

		ETH_A_MAC_CFG_ptr = XAxiEthernet_LookupConfig(ETH_A_MAC_DEVICE_ID);
		status = XAxiEthernet_CfgInitialize(&ETH_A_MAC_Instance, ETH_A_MAC_CFG_ptr, ETH_A_MAC_CFG_ptr->BaseAddress);
		XLlFifo_Initialize(&ETH_A_FIFO_Instance, ETH_A_MAC_CFG_ptr->AxiDevBaseAddress);
		if (status != XST_SUCCESS)
			xil_printf("*** EMAC init error\n");

		status  = XAxiEthernet_ClearOptions(&ETH_A_MAC_Instance, XAE_LENTYPE_ERR_OPTION | XAE_FLOW_CONTROL_OPTION | XAE_FCS_STRIP_OPTION);// | XTE_MULTICAST_OPTION);
		status |= XAxiEthernet_SetOptions(&ETH_A_MAC_Instance, XAE_PROMISC_OPTION | XAE_MULTICAST_OPTION | XAE_BROADCAST_OPTION | XAE_RECEIVER_ENABLE_OPTION | XAE_TRANSMITTER_ENABLE_OPTION | XAE_JUMBO_OPTION);
		if (status != XST_SUCCESS)
			xil_printf("*** Error setting EMAC options\n, code %d", status);

		XAxiEthernet_SetOperatingSpeed(&ETH_A_MAC_Instance, 1000);
		//usleep(1 * 10000);

		XAxiEthernet_Start(&ETH_A_MAC_Instance);

		FIFO_INST = &ETH_A_FIFO_Instance;
}

void wlan_mac_send_eth(void* mpdu, u16 length){
	//De-encapsulate packet and send over Ethernet
	mac_header_80211* rx80211_hdr;
	llc_header* llc_hdr;
	ethernet_header* eth_hdr;

	u8* mpdu_ptr_u8;
	u32 eth_fifo_vacancy_bytes;


	mpdu_ptr_u8 = (u8*) mpdu;

	rx80211_hdr = (mac_header_80211*)((void *)mpdu_ptr_u8);
	llc_hdr = (llc_header*)((void *)mpdu_ptr_u8 + sizeof(mac_header_80211));
	eth_hdr = (ethernet_header*)((void *)mpdu_ptr_u8 + sizeof(mac_header_80211) + sizeof(llc_header) - sizeof(ethernet_header));

	length = length - sizeof(mac_header_80211) - sizeof(llc_header) + sizeof(ethernet_header);

	memmove(&(eth_hdr->address_destination[0]),&(rx80211_hdr->address_3[0]),6);
	memmove(&(eth_hdr->address_source[0]),&(rx80211_hdr->address_2[0]),6);

	switch(llc_hdr->type){
		case LLC_TYPE_ARP:
			eth_hdr->type = ETH_TYPE_ARP;
		break;

		case LLC_TYPE_IP:
			eth_hdr->type = ETH_TYPE_IP;
		break;
		default:
			return;
		break;
	}
	eth_fifo_vacancy_bytes = (XLlFifo_TxVacancy((XLlFifo *)FIFO_INST) << 2);
	if(  eth_fifo_vacancy_bytes > (length)){
		XLlFifo_Write((XLlFifo *)FIFO_INST, (u8*)eth_hdr, length);
		XLlFifo_TxSetLen((XLlFifo *)FIFO_INST, length);
	} else {
		warp_printf(PL_ERROR,"TX Eth FIFO is full, vacancy is %d bytes\n", eth_fifo_vacancy_bytes);
	}


}

void wlan_mac_schedule_event(u32 delay, void(*callback)()){
	u32 k;

	u64 timestamp = get_usec_timestamp();

	for (k = 0; k<SCHEDULER_NUM_EVENTS; k++){
		if(scheduler_in_use[k] == 0){ //Found an empty schedule element
			scheduler_in_use[k] = 1; //We are using this schedule element
			scheduler_callbacks[k] = (function_ptr_t)callback;
			scheduler_timestamps[k] = timestamp+(u64)delay;
			return;
		}
	}
	warp_printf(PL_ERROR,"ERROR: %d schedules already filled\n",SCHEDULER_NUM_EVENTS);
}

inline void poll_schedule(){
	u32 k;
	u64 timestamp = get_usec_timestamp();

	for(k = 0; k<SCHEDULER_NUM_EVENTS; k++){
		if(scheduler_in_use[k] == 1){
			if(timestamp > scheduler_timestamps[k]){
				scheduler_in_use[k] = 0; //Free up schedule element before calling callback in case that function wants to reschedule
				scheduler_callbacks[k]();
			}
		}
	}
}

inline void wlan_mac_poll_eth(){
	u32 size;
	void * eth_start_ptr;
	ethernet_header* eth_hdr;
	llc_header* llc_hdr;
	u16 tx_length;

	u8 eth_dest[6];
	u8 eth_src[6];

	u8* mpdu_ptr_u8;
	packet_queue_element* tx_queue;


	if(XLlFifo_IsRxEmpty((XLlFifo *)FIFO_INST)){
			return;
	} else {
		if(XLlFifo_RxOccupancy((XLlFifo *)FIFO_INST)) {
			tx_queue = wlan_mac_queue_get_write_element(LOW_PRI_QUEUE_SEL);

			if(tx_queue != NULL){
				mpdu_ptr_u8 = tx_queue->frame;
				size = XLlFifo_RxGetLen((XLlFifo *)FIFO_INST);
				eth_start_ptr = (void*)mpdu_ptr_u8+sizeof(mac_header_80211)+sizeof(llc_header)-sizeof(ethernet_header);
				XLlFifo_Read((XLlFifo *)FIFO_INST, (void*) eth_start_ptr, size);
				tx_length = size-sizeof(ethernet_header)+sizeof(llc_header)+sizeof(mac_header_80211);
				eth_hdr = (ethernet_header*)eth_start_ptr;
				llc_hdr = (llc_header*)((void*)mpdu_ptr_u8+sizeof(mac_header_80211));

				memcpy((&(eth_src[0])),(&(eth_hdr->address_source[0])),6);
				memcpy((&(eth_dest[0])),(&(eth_hdr->address_destination[0])),6);

				llc_hdr->dsap = LLC_SNAP;
				llc_hdr->ssap = LLC_SNAP;
				llc_hdr->control_field = LLC_CNTRL_UNNUMBERED;
				bzero((void *)(&(llc_hdr->org_code[0])),3); //Org Code: Encapsulated Ethernet

				switch(eth_hdr->type){
					case ETH_TYPE_ARP:
						llc_hdr->type = LLC_TYPE_ARP;
						eth_rx_callback(tx_queue,&(eth_dest[0]),&(eth_src[0]),tx_length);
					break;
					case ETH_TYPE_IP:
						llc_hdr->type = LLC_TYPE_IP;
						eth_rx_callback(tx_queue,&(eth_dest[0]),&(eth_src[0]),tx_length);
					break;
				}
			}
		}
	}
	return;
}

inline void wlan_mac_poll_tx_queue(){
	packet_queue_element* tx_queue;

	//Poll high priority queue
	tx_queue = wlan_mac_queue_get_read_element(HIGH_PRI_QUEUE_SEL);
	if(tx_queue != NULL){
		mpdu_tx_callback(tx_queue);
		wlan_mac_queue_pop(HIGH_PRI_QUEUE_SEL);
	} else {
		//Poll low priority queue
		tx_queue = wlan_mac_queue_get_read_element(LOW_PRI_QUEUE_SEL);
		if(tx_queue != NULL){
			mpdu_tx_callback(tx_queue);
			wlan_mac_queue_pop(LOW_PRI_QUEUE_SEL);
		}
	}
}

void wlan_mac_util_process_tx_done(tx_frame_info* frame,station_info* station){

	//xil_printf("TX DONE-- AID: %d, state_verbose = %d, retry_count = %d\n", frame->AID, frame->state_verbose, frame->retry_count);

	if(frame->retry_count > 0){
		//There was at least 1 missed ACK during this transmission
		station->total_missed_acks += frame->retry_count;
		station->consecutive_good_acks = 0;
	}
	if(frame->state_verbose == TX_MPDU_STATE_VERBOSE_SUCCESS){
		(station->consecutive_good_acks)++;
	}

}

u8 wlan_mac_util_get_tx_rate(station_info* station){

	xil_printf("good_acks: %d, total_missed: %d\n",station->consecutive_good_acks, station->total_missed_acks);

	if(station->consecutive_good_acks >= MIN_CONSECUTIVE_GOOD_ACKS){
		if(station->tx_rate < RATE_ADAPT_MAX_RATE){
			(station->tx_rate)++;
			xil_printf("STA AID %d: rate increased to %d\n",station->AID,station->tx_rate);
		}
		station->consecutive_good_acks = 0;
		station->total_missed_acks = 0;
	} else {
		if(station->total_missed_acks >= MIN_TOTAL_MISSED_ACKS){
			if(station->tx_rate > RATE_ADAPT_MIN_RATE){
				(station->tx_rate)--;
				xil_printf("STA AID %d: rate decreased to %d\n",station->AID,station->tx_rate);
			}
			station->consecutive_good_acks = 0;
			station->total_missed_acks = 0;
		}
	}
	return station->tx_rate;
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

