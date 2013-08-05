////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_util.c
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

#include "xgpio.h"

#include "wlan_lib.h"
#include "wlan_mac_util.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_eth_util.h"
#include "w3_userio.h"
#include "xparameters.h"

#define USERIO_BASEADDR XPAR_W3_USERIO_BASEADDR

static XGpio GPIO_timestamp;

function_ptr_t eth_rx_callback, mpdu_tx_callback;

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

inline void wlan_mac_poll_tx_queue(){
	packet_queue_element* tx_queue;

	//Poll high priority queue
	tx_queue = wlan_mac_queue_get_read_element(HIGH_PRI_QUEUE_SEL);
	if(tx_queue != NULL){
		mpdu_tx_callback(tx_queue);
		wlan_mac_dequeue(HIGH_PRI_QUEUE_SEL);
	} else {
		//Poll low priority queue
		tx_queue = wlan_mac_queue_get_read_element(LOW_PRI_QUEUE_SEL);
		if(tx_queue != NULL){
			mpdu_tx_callback(tx_queue);
			wlan_mac_dequeue(LOW_PRI_QUEUE_SEL);
		}
	}
}

void wlan_mac_util_process_tx_done(tx_frame_info* frame,station_info* station){

}

u8 wlan_mac_util_get_tx_rate(station_info* station){
	return station->tx_rate;
}

void write_hex_display(u8 val){
	//u8 val: 2 digit decimal value to be printed to hex displays
   userio_write_hexdisp_left(USERIO_BASEADDR, val/10);
   userio_write_hexdisp_right(USERIO_BASEADDR, val%10);
}

