/** @file wlan_mac_queue.c
 *  @brief Transmit Queue Framework
 *
 *  This contains code for accessing the transmit queue.
 *
 *  @copyright Copyright 2014, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 *  @bug No known bugs.
 */

#include "xil_types.h"
#include "stdlib.h"
#include "stdio.h"
#include "xparameters.h"
#include "xintc.h"
#include "string.h"

#include "wlan_mac_ipc_util.h"
#include "wlan_mac_high.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_eth_util.h"

//This list holds all of the empty, free elements
static dl_list queue_free;

//This queue_tx vector will get filled in with elements from the queue_free list
//Note: this implementation sparsely packs the queue_tx array to allow fast
//indexing at the cost of some wasted memory. The queue_tx array will be
//reallocated whenever the upper-level MAC asks to enqueue at an index
//that is larger than the current size of the array. It is assumed that this
//index will not continue to grow over the course of execution, otherwise
//this array will continue to grow and eventually be unable to be reallocated.
//Practically speaking, this means an AP needs to re-use the AIDs it issues
//stations if it wants to use the AIDs as an index into the tx queue.
static dl_list* queue_tx;
static u16 num_queue_tx;


static u32 QUEUE_NUM_DL_ENTRY;
static u32 MAX_SIZE_FOR_PACKET_BD_DL_LIST;

static void* QUEUE_BUFFER_SPACE;

static u8 dram_present;

void queue_dram_present(u8 present){
	dram_present = present;
}

int queue_init(){
	u32 i;

	if(dram_present == 1){
		MAX_SIZE_FOR_PACKET_BD_DL_LIST = 48000;
		QUEUE_NUM_DL_ENTRY = MAX_SIZE_FOR_PACKET_BD_DL_LIST / (sizeof(dl_entry));

		//Use DRAM
		//QUEUE_NUM_DL_ENTRY = 2400; //FIXME: Temporary fix to deal with the embiggened dl_entry struct
		//QUEUE_NUM_DL_ENTRY = 3000; ///Do not increase. The user scratch place in DRAM begins immediately following
						   ///the queue.
		xil_printf("Queue of %d placed in DRAM: using %d kB\n", QUEUE_NUM_DL_ENTRY, (QUEUE_NUM_DL_ENTRY*QUEUE_BUFFER_SIZE)/1024);
		QUEUE_BUFFER_SPACE = (void*)(DDR3_BASEADDR);

	} else {
		xil_printf("A working DRAM SODIMM has not been detected on this board. DRAM is required for\n");
		xil_printf("the wireless transmission queue. Halting\n");
		while(1){}
	}



	dl_list_init(&queue_free);

	bzero((void*)QUEUE_BUFFER_SPACE, QUEUE_NUM_DL_ENTRY*QUEUE_BUFFER_SIZE);

	//At boot, every dl_entry buffer descriptor is free
	//To set up the doubly linked list, we exploit the fact that we know the starting state is sequential.
	//This matrix addressing is not safe once the queue is used. The insert/remove helper functions should be used
	dl_entry* dl_entry_base;
	dl_entry_base = (dl_entry*)(QUEUE_DL_ENTRY_SPACE_BASE);

	for(i=0;i<QUEUE_NUM_DL_ENTRY;i++){
		dl_entry_base[i].data = (void*)(QUEUE_BUFFER_SPACE + (i*QUEUE_BUFFER_SIZE));
		dl_entry_insertEnd(&queue_free,&(dl_entry_base[i]));
	}

	num_queue_tx = 0;
	queue_tx = NULL;

	return QUEUE_NUM_DL_ENTRY*QUEUE_BUFFER_SIZE;

}

int queue_total_size(){
	return QUEUE_NUM_DL_ENTRY;
}

void purge_queue(u16 queue_sel){
	dl_list		   dequeue;
	u32            num_queued;

	num_queued = queue_num_queued(queue_sel);

	if( num_queued > 0 ){
		xil_printf("purging %d packets from queue for queue ID %d\n", num_queued, queue_sel);
		dequeue_from_beginning(&dequeue, queue_sel, num_queued);
		queue_checkin(&dequeue);
	}
}

void enqueue_after_end(u16 queue_sel, dl_list* list){
	u32 i;

	dl_entry* next_dl_entry;
	dl_entry* curr_dl_entry;

    if((queue_sel+1) > num_queue_tx){
    	queue_tx = wlan_mac_high_realloc(queue_tx, (queue_sel+1)*sizeof(dl_list));

    	if(queue_tx == NULL){
    		xil_printf("Error in reallocating %d bytes for tx queue\n", (queue_sel+1)*sizeof(dl_list));
    	}

    	for(i = num_queue_tx; i <= queue_sel; i++){
    		dl_list_init(&(queue_tx[i]));
    	}

    	num_queue_tx = queue_sel+1;

    }

	curr_dl_entry = (dl_entry*)(list->first);

	while(curr_dl_entry != NULL){
		next_dl_entry = dl_entry_next(curr_dl_entry);
		dl_entry_remove(list,curr_dl_entry);
		dl_entry_insertEnd(&(queue_tx[queue_sel]),curr_dl_entry);
		curr_dl_entry = next_dl_entry;
	}
	return;
}

void dequeue_from_beginning(dl_list* new_list, u16 queue_sel, u16 num_packet_bd){
	u32 i,num_dequeue;
	dl_entry* curr_dl_entry;

	dl_list_init(new_list);

	if((queue_sel+1) > num_queue_tx){
		//The calling function is asking to empty from a queue_tx element that is
		//outside of the bounds that are currently allocated. This is not an error
		//condition, as it can happen when an AP checks to see if any packets are
		//ready to send to a station before any have ever been queued up for that
		//station. In this case, we simply return the newly initialized new_list
		//that says that no packets are currently in the queue_sel queue_tx.
		return;
	} else {

		if(num_packet_bd <= queue_tx[queue_sel].length){
			num_dequeue = num_packet_bd;
		} else {
			num_dequeue = queue_tx[queue_sel].length;
		}

		for (i=0;i<num_dequeue;i++){
			curr_dl_entry = (queue_tx[queue_sel].first);
			//Remove from free list
			dl_entry_remove(&queue_tx[queue_sel],curr_dl_entry);
			//Add to new checkout list
			dl_entry_insertEnd(new_list,curr_dl_entry);
		}
	}
	return;
}

u32 queue_num_free(){
	return queue_free.length;
}

u32 queue_num_queued(u16 queue_sel){
	if((queue_sel+1) > num_queue_tx){
		return 0;
	} else {
		return queue_tx[queue_sel].length;
	}
}

void queue_checkout(dl_list* new_list, u16 num_packet_bd){
	//Checks out up to num_packet_bd number of packet_bds from the free list. If num_packet_bd are not free,
	//then this function will return the number that are free and only check out that many.

	u32 i,num_checkout;
	dl_entry* curr_dl_entry;

	dl_list_init(new_list);

	if(num_packet_bd <= queue_free.length){
		num_checkout = num_packet_bd;
	} else {
		num_checkout = queue_free.length;
	}

	//Traverse the queue_free and update the pointers
	for (i=0;i<num_checkout;i++){
		curr_dl_entry = (queue_free.first);

		//Remove from free list
		dl_entry_remove(&queue_free,curr_dl_entry);
		//Add to new checkout list
		dl_entry_insertEnd(new_list,curr_dl_entry);
	}
	return;
}

void queue_checkin(dl_list* list){
	dl_entry* curr_dl_entry;
	dl_entry* next_dl_entry;

	curr_dl_entry = list->first;

	while(curr_dl_entry != NULL){
		next_dl_entry = dl_entry_next(curr_dl_entry);
		dl_entry_remove(list,curr_dl_entry);
		dl_entry_insertEnd(&queue_free,curr_dl_entry);
		curr_dl_entry = next_dl_entry;
	}

	return;
}

int wlan_mac_queue_poll(u16 queue_sel){
	int return_value = 0;

	dl_list dequeue;
	dl_entry* tx_queue_entry;

	dequeue_from_beginning(&dequeue, queue_sel,1);

	//xil_printf("wlan_mac_poll_tx_queue(%d)\n", queue_sel);

	if(dequeue.length == 1){
		return_value = 1;
		tx_queue_entry = dequeue.first;

		wlan_mac_high_mpdu_transmit(tx_queue_entry);
		queue_checkin(&dequeue);
		wlan_eth_dma_update();
	}

	return return_value;
}
