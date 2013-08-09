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

//This list holds all of the empty, free elements
static pqueue_list queue_free;

//This vector of lists will get filled in with elements from the free list
static pqueue_list queue[NUM_QUEUES];


u32 PQUEUE_LEN;
void* PQUEUE_BUFFER_SPACE_BASE;

#define USE_DRAM 1

void queue_init(){
	u32 i;

#if USE_DRAM
	if(memory_test()==0){
		//Use DRAM
		PQUEUE_LEN = 1000;
		xil_printf("Queue of %d placed in DRAM: using %d kB\n", PQUEUE_LEN, (PQUEUE_LEN*PQUEUE_MAX_FRAME_SIZE)/1024);
		PQUEUE_BUFFER_SPACE_BASE = (void*)(DDR3_BASEADDR);
	} else {
		//Use BRAM
		PQUEUE_LEN = 20;
		xil_printf("Queue of %d placed in BRAM: using %d kB\n", PQUEUE_LEN, (PQUEUE_LEN*PQUEUE_MAX_FRAME_SIZE)/1024);
		PQUEUE_BUFFER_SPACE_BASE = (void*)(PQUEUE_MEM_BASE+(PQUEUE_LEN*sizeof(pqueue)));
	}
#else
	//Use BRAM
	PQUEUE_LEN = 20;
	xil_printf("Queue of %d placed in BRAM: using %d kB\n", PQUEUE_LEN, (PQUEUE_LEN*PQUEUE_MAX_FRAME_SIZE)/1024);
	PQUEUE_BUFFER_SPACE_BASE = (void*)(PQUEUE_MEM_BASE+(PQUEUE_LEN*sizeof(pqueue)));
#endif


	queue_free = pqueue_list_init();

	queue_free.first = (pqueue*)pqueue_SPACE_BASE;

	bzero((void*)PQUEUE_BUFFER_SPACE_BASE, PQUEUE_LEN*PQUEUE_MAX_FRAME_SIZE);

	//At boot, every pqueue buffer descriptor is free
	//To set up the doubly linked list, we exploit the fact that we know the starting state is sequential.
	//This matrix addressing is not safe once the queue is used. The insert/remove helper functions should be used
	queue_free.length = PQUEUE_LEN;
	for(i=0;i<PQUEUE_LEN;i++){
		queue_free.first[i].pktbuf_ptr = (tx_packet_buffer*)(PQUEUE_BUFFER_SPACE_BASE + (i*PQUEUE_MAX_FRAME_SIZE));
		queue_free.first[i].station_info_ptr = NULL;

		//xil_printf("pqueue %d: pktbuf_ptr = 0x%08x\n",i, queue_free.first[i].pktbuf_ptr);

		if(i==(PQUEUE_LEN-1)){
			queue_free.first[i].next = NULL;
			queue_free.last = &(queue_free.first[i]);
		} else {
			queue_free.first[i].next = &(queue_free.first[i+1]);
		}


		if(i==0){
			queue_free.first[i].prev = NULL;
		} else {
			queue_free.first[i].prev = &(queue_free.first[i-1]);
		}

	}

	//By default, all queues are empty.
	for(i=0;i<NUM_QUEUES;i++){
		queue[i] = pqueue_list_init();
		queue[i].length = 0;
		queue[i].first = NULL;
		queue[i].last = NULL;
	}


}

int queue_total_size(){
	return PQUEUE_LEN;
}

void enqueue_after_end(u16 queue_sel, pqueue_list* list){
	pqueue* curr_pqueue;
	pqueue* next_pqueue;

	curr_pqueue = list->first;

	while(curr_pqueue != NULL){
		next_pqueue = curr_pqueue->next;
		pqueue_remove(list,curr_pqueue);
		pqueue_insertEnd(&(queue[queue_sel]),curr_pqueue);
		curr_pqueue = next_pqueue;
	}
	return;
}

pqueue_list dequeue_from_beginning(u16 queue_sel, u16 num_pqueue){
	u32 i,num_dequeue;
	pqueue_list new_list = pqueue_list_init();
	pqueue* curr_pqueue;

	if(num_pqueue <= queue[queue_sel].length){
		num_dequeue = num_pqueue;
	} else {
		num_dequeue = queue[queue_sel].length;
	}

	for (i=0;i<num_dequeue;i++){
		curr_pqueue = queue[queue_sel].first;
		//Remove from free list
		pqueue_remove(&queue[queue_sel],curr_pqueue);
		//Add to new checkout list
		pqueue_insertEnd(&new_list,curr_pqueue);
	}
	return new_list;
}

u32 queue_num_free(){
	return queue_free.length;
}

u32 queue_num_queued(u16 queue_sel){
	return queue[queue_sel].length;
}

pqueue_list queue_checkout(u16 num_pqueue){
	//Checks out up to num_pqueue number of pqueues from the free list. If num_pqueue are not free,
	//then this function will return the number that are free and only check out that many.

	u32 i,num_checkout;
	pqueue_list new_list = pqueue_list_init();
	pqueue* curr_pqueue;

	if(num_pqueue <= queue_free.length){
		num_checkout = num_pqueue;
	} else {
		num_checkout = queue_free.length;
	}

	//Traverse the queue_free and update the pointers
	for (i=0;i<num_checkout;i++){
		curr_pqueue = queue_free.first;
		//Remove from free list
		pqueue_remove(&queue_free,curr_pqueue);
		//Add to new checkout list
		pqueue_insertEnd(&new_list,curr_pqueue);
	}
	return new_list;
}
void queue_checkin(pqueue_list* list){
	pqueue* curr_pqueue;
	pqueue* next_pqueue;

	curr_pqueue = list->first;

	while(curr_pqueue != NULL){
		next_pqueue = curr_pqueue->next;
		pqueue_remove(list,curr_pqueue);
		pqueue_insertEnd(&queue_free,curr_pqueue);
		curr_pqueue = next_pqueue;
	}

	return;
}

void pqueue_insertAfter(pqueue_list* list, pqueue* bd, pqueue* bd_new){
	bd_new->prev = bd;
	bd_new->next = bd->next;
	if(bd->next == NULL){
		list->last = bd_new;
	} else {
		bd->next->prev = bd_new;
	}
	bd->next = bd_new;
	(list->length)++;
	return;
}

void pqueue_insertBefore(pqueue_list* list, pqueue* bd, pqueue* bd_new){
	bd_new->prev = bd->prev;
	bd_new->next = bd;
	if(bd->prev == NULL){
		list->first = bd_new;
	} else {
		bd->prev->next = bd_new;
	}
	bd->prev = bd_new;
	(list->length)++;
	return;
}

void pqueue_insertBeginning(pqueue_list* list, pqueue* bd_new){
	if(list->first == NULL){
		list->first = bd_new;
		list->last = bd_new;
		bd_new->prev = NULL;
		bd_new->next = NULL;
		(list->length)++;
	} else {
		pqueue_insertBefore(list, list->first, bd_new);
	}
	return;
}

void pqueue_insertEnd(pqueue_list* list, pqueue* bd_new){
	if(list->last == NULL){
		pqueue_insertBeginning(list,bd_new);
	} else {
		pqueue_insertAfter(list,list->last, bd_new);
	}
	return;
}

void pqueue_remove(pqueue_list* list, pqueue* bd){
	if(bd->prev == NULL){
		list->first = bd->next;
	} else {
		bd->prev->next = bd->next;
	}

	if(bd->next == NULL){
		list->last = bd->prev;
	} else {
		bd->next->prev = bd->prev;
	}
	(list->length)--;
}

pqueue_list pqueue_list_init(){
	pqueue_list list;
	list.first = NULL;
	list.last = NULL;
	list.length = 0;
	return list;
}


void pqueue_print(pqueue_list* list){
	pqueue* curr_bd = list->first;

	xil_printf("******** pqueue_print ********\n");
	xil_printf("list->first:     0x%08x\n", list->first);
	xil_printf("list->last:      0x%08x\n", list->last);
	xil_printf("list->length:    %d\n\n", list->length);

	while(curr_bd != NULL){
		xil_printf("0x%08x\n", curr_bd);
		xil_printf("  |  prev:      0x%08x\n", curr_bd->prev);
		xil_printf("  |  next:      0x%08x\n", curr_bd->next);
		xil_printf("  |       pktbuf_ptr: 0x%08x\n", curr_bd->pktbuf_ptr);
		curr_bd = curr_bd->next;
	}
}
