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
#include "xintc.h"
#include "string.h"

#include "wlan_mac_ipc_util.h"
#include "wlan_mac_util.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_dl_list.h"

//This list holds all of the empty, free elements
static dl_list queue_free;

//This vector of lists will get filled in with elements from the free list
static dl_list queue[NUM_QUEUES];


u32 PQUEUE_LEN;
void* PQUEUE_BUFFER_SPACE_BASE;

#define USE_DRAM 1
static u8 dram_present;

void queue_dram_present(u8 present){
	dram_present = present;
}

int queue_init(){
	u32 i;

#if USE_DRAM
	if(dram_present == 1){
		//Use DRAM
		PQUEUE_LEN = 3000;
		xil_printf("Queue of %d placed in DRAM: using %d kB\n", PQUEUE_LEN, (PQUEUE_LEN*PQUEUE_MAX_FRAME_SIZE)/1024);
		PQUEUE_BUFFER_SPACE_BASE = (void*)(DDR3_BASEADDR);
	} else {
		//Use BRAM
		PQUEUE_LEN = 20;
		xil_printf("Queue of %d placed in BRAM: using %d kB\n", PQUEUE_LEN, (PQUEUE_LEN*PQUEUE_MAX_FRAME_SIZE)/1024);
		PQUEUE_BUFFER_SPACE_BASE = (void*)(PQUEUE_MEM_BASE+(PQUEUE_LEN*sizeof(packet_bd)));
	}

#else
	//Use BRAM
	PQUEUE_LEN = 20;
	xil_printf("Queue of %d placed in BRAM: using %d kB\n", PQUEUE_LEN, (PQUEUE_LEN*PQUEUE_MAX_FRAME_SIZE)/1024);
	PQUEUE_BUFFER_SPACE_BASE = (void*)(PQUEUE_MEM_BASE+(PQUEUE_LEN*sizeof(packet_bd)));
#endif

	dl_list_init(&queue_free);

	//queue_free.first = (dl_node*)PQUEUE_SPACE_BASE;
	//queue_free.last  = (dl_node*)PQUEUE_SPACE_BASE;
	//queue_free.length = 1;

	bzero((void*)PQUEUE_BUFFER_SPACE_BASE, PQUEUE_LEN*PQUEUE_MAX_FRAME_SIZE);

	//At boot, every packet_bd buffer descriptor is free
	//To set up the doubly linked list, we exploit the fact that we know the starting state is sequential.
	//This matrix addressing is not safe once the queue is used. The insert/remove helper functions should be used
	//queue_free.length = PQUEUE_LEN;
	packet_bd* packet_bd_base;
	//packet_bd_base = (packet_bd*)(queue_free.first);
	packet_bd_base = (packet_bd*)(PQUEUE_SPACE_BASE);

	//dl_node_insertEnd(&queue_free, &(packet_bd_base->node));

	//xil_printf("   [first, last, length] = [0x%08x, 0x%08x, %d]\n", queue_free.first,queue_free.last,queue_free.length);

	for(i=0;i<PQUEUE_LEN;i++){

		packet_bd_base[i].buf_ptr = (void*)(PQUEUE_BUFFER_SPACE_BASE + (i*PQUEUE_MAX_FRAME_SIZE));
		packet_bd_base[i].metadata_ptr = NULL;

		dl_node_insertEnd(&queue_free,&(packet_bd_base[i].node));

	}

	//By default, all queues are empty.
	for(i=0;i<NUM_QUEUES;i++){
		dl_list_init(&(queue[i]));
	}

	return PQUEUE_LEN*PQUEUE_MAX_FRAME_SIZE;

}

int queue_total_size(){
	return PQUEUE_LEN;
}

void purge_queue(u16 queue_sel){
	dl_list		   dequeue;
	u32            num_queued;

	num_queued = queue_num_queued(queue_sel);

	if( num_queued > 0 ){
		xil_printf("purging %d packets from queue for queue ID %d\n", num_queued, queue_sel);
		dequeue_from_beginning(&dequeue, queue_sel, 1);
		queue_checkin(&dequeue);
	}
}

void enqueue_after_end(u16 queue_sel, dl_list* list){
	packet_bd* curr_packet_bd;
	packet_bd* next_packet_bd;

	curr_packet_bd = (packet_bd*)(list->first);

	while(curr_packet_bd != NULL){
		next_packet_bd = (packet_bd*)((curr_packet_bd->node).next);
		dl_node_remove(list,&(curr_packet_bd->node));
		dl_node_insertEnd(&(queue[queue_sel]),&(curr_packet_bd->node));
		curr_packet_bd = next_packet_bd;
	}
	return;
}

void dequeue_from_beginning(dl_list* new_list, u16 queue_sel, u16 num_packet_bd){
	u32 i,num_dequeue;
	packet_bd* curr_packet_bd;

	dl_list_init(new_list);

	if(num_packet_bd <= queue[queue_sel].length){
		num_dequeue = num_packet_bd;
	} else {
		num_dequeue = queue[queue_sel].length;
	}

	for (i=0;i<num_dequeue;i++){
		curr_packet_bd = (packet_bd*)(queue[queue_sel].first);
		//Remove from free list
		dl_node_remove(&queue[queue_sel],&(curr_packet_bd->node));
		//Add to new checkout list
		dl_node_insertEnd(new_list,&(curr_packet_bd->node));
	}
	return;
}

u32 queue_num_free(){
	return queue_free.length;
}

u32 queue_num_queued(u16 queue_sel){
	return queue[queue_sel].length;
}

void queue_checkout(dl_list* new_list, u16 num_packet_bd){
	//Checks out up to num_packet_bd number of packet_bds from the free list. If num_packet_bd are not free,
	//then this function will return the number that are free and only check out that many.

	u32 i,num_checkout;
	packet_bd* curr_packet_bd;

	dl_list_init(new_list);

	if(num_packet_bd <= queue_free.length){
		num_checkout = num_packet_bd;
	} else {
		num_checkout = queue_free.length;
	}

	//Traverse the queue_free and update the pointers
	for (i=0;i<num_checkout;i++){
		curr_packet_bd = (packet_bd*)(queue_free.first);

		//Remove from free list
		dl_node_remove(&queue_free,&(curr_packet_bd->node));
		//Add to new checkout list
		dl_node_insertEnd(new_list,&(curr_packet_bd->node));
	}
	return;
}
void queue_checkin(dl_list* list){
	packet_bd* curr_packet_bd;
	packet_bd* next_packet_bd;

	curr_packet_bd = (packet_bd*)(list->first);

	while(curr_packet_bd != NULL){
		next_packet_bd = (packet_bd*)((curr_packet_bd->node).next);
		dl_node_remove(list,&(curr_packet_bd->node));
		dl_node_insertEnd(&queue_free,&(curr_packet_bd->node));
		curr_packet_bd = next_packet_bd;
	}

	return;
}
