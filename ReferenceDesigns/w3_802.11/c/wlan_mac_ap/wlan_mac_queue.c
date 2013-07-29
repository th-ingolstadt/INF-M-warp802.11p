////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_queue.c
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

#include "xil_types.h"
#include "stdlib.h"
#include "stdio.h"
#include "xparameters.h"
#include "string.h"

#include "wlan_lib.h"
#include "wlan_mac_util.h"
#include "wlan_mac_queue.h"


static packet_queue_element* low_pri_tx_queue;
static u16 low_pri_queue_read_index;
static u16 low_pri_queue_write_index;

static packet_queue_element* high_pri_tx_queue;
static u16 high_pri_queue_read_index;
static u16 high_pri_queue_write_index;

void wlan_mac_queue_init(){

	//xil_printf("LOW_PRI_QUEUE_BASEADDR 	= 0x%08x\n",LOW_PRI_QUEUE_BASEADDR);
	//xil_printf("HIGH_PRI_QUEUE_BASEADDR = 0x%08x\n",HIGH_PRI_QUEUE_BASEADDR);

	low_pri_tx_queue = (packet_queue_element*)LOW_PRI_QUEUE_BASEADDR;
	bzero(&(low_pri_tx_queue[0]),sizeof(packet_queue_element)*LOW_PRI_TX_QUEUE_LENGTH);
	low_pri_queue_read_index = 0;
	low_pri_queue_write_index = 0;

	high_pri_tx_queue = (packet_queue_element*)HIGH_PRI_QUEUE_BASEADDR;
	bzero(&(high_pri_tx_queue[0]),sizeof(packet_queue_element)*HIGH_PRI_TX_QUEUE_LENGTH);
	high_pri_queue_read_index = 0;
	high_pri_queue_write_index = 0;
}

u16 wlan_mac_queue_get_size(u8 queue_sel){
	u16 return_value;

	switch(queue_sel){
		case HIGH_PRI_QUEUE_SEL:
			if(high_pri_queue_write_index >= high_pri_queue_read_index){
				return_value = (high_pri_queue_write_index-high_pri_queue_read_index);
			} else {
				return_value = (high_pri_queue_write_index + HIGH_PRI_TX_QUEUE_LENGTH - high_pri_queue_read_index);
			}


		break;
		case LOW_PRI_QUEUE_SEL:
			if(low_pri_queue_write_index >= low_pri_queue_read_index){
				return_value = (low_pri_queue_write_index-low_pri_queue_read_index);
			} else {
				return_value = (low_pri_queue_write_index + LOW_PRI_TX_QUEUE_LENGTH - low_pri_queue_read_index);
			}


		break;
	}



	//xil_printf("Queue size = %d\n", return_value);
	return return_value;
}

packet_queue_element* wlan_mac_queue_get_write_element(u8 queue_sel){
	//Returns ptr to next valid write address if queue is not full
	//Returns NULL if queue is full
	void* return_value;

	switch(queue_sel){
		case HIGH_PRI_QUEUE_SEL:
			if(wlan_mac_queue_get_size(HIGH_PRI_QUEUE_SEL) < (HIGH_PRI_TX_QUEUE_LENGTH-1)){
				return_value = &(high_pri_tx_queue[high_pri_queue_write_index]);
			} else {
				return_value =  NULL;
			}
		break;
		case LOW_PRI_QUEUE_SEL:
			if(wlan_mac_queue_get_size(LOW_PRI_QUEUE_SEL) < (LOW_PRI_TX_QUEUE_LENGTH-1)){
				return_value = &(low_pri_tx_queue[low_pri_queue_write_index]);
			} else {
				return_value =  NULL;
			}
			//if(return_value!= NULL) xil_printf("Write to 0x%08x\n",return_value);

		break;
	}


	//xil_printf("Write to %d\n",queue_write_index);
	return return_value;
}

packet_queue_element* wlan_mac_queue_get_read_element(u8 queue_sel){
	//Returns ptr to next valid read address if queue is not empty
	//Returns NULL if queue is empty
	void * return_value;

	switch(queue_sel){
		case HIGH_PRI_QUEUE_SEL:
			if(wlan_mac_queue_get_size(HIGH_PRI_QUEUE_SEL)>0){
				return_value = &(high_pri_tx_queue[high_pri_queue_read_index]);
			} else {
				return_value =  NULL;
			}
		break;
		case LOW_PRI_QUEUE_SEL:
			if(wlan_mac_queue_get_size(LOW_PRI_QUEUE_SEL)>0){
				return_value = &(low_pri_tx_queue[low_pri_queue_read_index]);
			} else {
				return_value =  NULL;
			}
			//if(return_value!= NULL) xil_printf("Read from 0x%08x\n",return_value);

		break;
	}


	//xil_printf("Read from %d\n",queue_read_index);
	return return_value;
}

void wlan_mac_queue_push(u8 queue_sel){
	//This command should only be called if wlan_mac_queue_get_write_addr returns non-NULL value;
	switch(queue_sel){
		case HIGH_PRI_QUEUE_SEL:
			high_pri_queue_write_index = (high_pri_queue_write_index+1)%HIGH_PRI_TX_QUEUE_LENGTH;
		break;
		case LOW_PRI_QUEUE_SEL:
			//xil_printf("Pushed %d\n",low_pri_queue_write_index);
			low_pri_queue_write_index = (low_pri_queue_write_index+1)%LOW_PRI_TX_QUEUE_LENGTH;
			//write_hex_display(wlan_mac_queue_get_size(queue_sel));
		break;
	}

	//xil_printf("PUSH\n");
}

void wlan_mac_queue_pop(u8 queue_sel){
	switch(queue_sel){
		case HIGH_PRI_QUEUE_SEL:
			high_pri_queue_read_index = (high_pri_queue_read_index+1)%HIGH_PRI_TX_QUEUE_LENGTH;
		break;
		case LOW_PRI_QUEUE_SEL:
			//xil_printf("Popped %d\n",low_pri_queue_read_index);
			low_pri_queue_read_index = (low_pri_queue_read_index+1)%LOW_PRI_TX_QUEUE_LENGTH;
			//write_hex_display(wlan_mac_queue_get_size(queue_sel));
		break;
	}
	//xil_printf("POP\n");
}
