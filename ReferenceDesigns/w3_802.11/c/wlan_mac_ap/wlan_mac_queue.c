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

//This ring holds all of the empty, free elements
static pqueue_ring queue_free;

//This vector of rings will get filled in with elements from the free ring
static pqueue_ring queue[NUM_QUEUES];

void queue_init(){
	u32 i;

	queue_free = pqueue_ring_init();

	queue_free.first = (pqueue_bd*)PQUEUE_BD_SPACE_BASE;

	bzero((void*)PQUEUE_BUFFER_SPACE_BASE, PQUEUE_LEN*PQUEUE_MAX_FRAME_SIZE);

	//At boot, every pqueue buffer descriptor is free
	//To set up the doubly linked list, we exploit the fact that we know the starting state is sequential.
	//This matrix addressing is not safe once the queue is used. The insert/remove helper functions should be used
	queue_free.length = PQUEUE_LEN;
	for(i=0;i<PQUEUE_LEN;i++){
		queue_free.first[i].frame_ptr = (u8*)PQUEUE_BUFFER_SPACE_BASE + (i*PQUEUE_MAX_FRAME_SIZE);
		queue_free.first[i].station_info_ptr = NULL;

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
		queue[i] = pqueue_ring_init();
		queue[i].length = 0;
		queue[i].first = NULL;
		queue[i].last = NULL;
	}

	/*
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//Simulate what would happen if we don't want all of the pqueues we checkout and want to check some back in
	//This case will occur when dealing with Ethernet frames, where not every BD will contain a frame we intend to
	//send (e.g. non-ARP and non-IP packets)
	pqueue_ring checkout,checkin,dequeue;
	pqueue_bd* curr_pqueue;
	pqueue_bd* next_pqueue;
	u32 checkout_len;

	checkin = pqueue_ring_init();

	checkout = queue_checkout(8);
	curr_pqueue = checkout.first;
	checkout_len = checkout.length;

	//This for loop is only used as a way for me to pull out a few pqueues to check back in
	//in practice, the while loop method (commented out) should be used.
	for(i=0;i<checkout_len;i++){
	//while(curr_pqueue != NULL){
		next_pqueue = curr_pqueue->next;

		if((i==0) || (i==3) || (i==5)){
			pqueue_remove(&checkout,curr_pqueue);
			pqueue_insertEnd(&checkin,curr_pqueue);
		}

		curr_pqueue = next_pqueue;
	}
	//xil_printf("\ncheckin\n");
	//pqueue_print(&checkin);
	//xil_printf("\ncheckout\n");
	//pqueue_print(&checkout);
	queue_checkin(&checkin);

	//xil_printf("\ncheckin\n");
	//pqueue_print(&checkin);
	//xil_printf("\nfree\n");
	//pqueue_print(&queue_free);

	enqueue_after_end(0, &checkout);
	//xil_printf("\ncheckout\n");
	//pqueue_print(&checkout);


	dequeue = dequeue_from_beginning(0,1);
	xil_printf("\nqueue[0]\n");
	pqueue_print(&(queue[0]));

	xil_printf("\ndequeue\n");
	pqueue_print(&(dequeue));


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	 */
}

void enqueue_after_end(u16 queue_sel, pqueue_ring* ring){
	pqueue_bd* curr_pqueue;
	pqueue_bd* next_pqueue;

	curr_pqueue = ring->first;

	while(curr_pqueue != NULL){
		next_pqueue = curr_pqueue->next;
		pqueue_remove(ring,curr_pqueue);
		pqueue_insertEnd(&(queue[queue_sel]),curr_pqueue);
		curr_pqueue = next_pqueue;
	}
	return;
}

pqueue_ring dequeue_from_beginning(u16 queue_sel, u16 num_pqueue){
	u32 i,num_dequeue;
	pqueue_ring new_ring = pqueue_ring_init();
	pqueue_bd* curr_pqueue;

	if(num_pqueue <= queue[queue_sel].length){
		num_dequeue = num_pqueue;
	} else {
		num_dequeue = queue[queue_sel].length;
	}

	for (i=0;i<num_dequeue;i++){
		curr_pqueue = queue[queue_sel].first;
		//Remove from free ring
		pqueue_remove(&queue[queue_sel],curr_pqueue);
		//Add to new checkout ring
		pqueue_insertEnd(&new_ring,curr_pqueue);
	}
	return new_ring;
}

pqueue_ring queue_checkout(u16 num_pqueue){
	//Checks out up to num_pqueue number of pqueues from the free ring. If num_pqueue are not free,
	//then this function will return the number that are free and only check out that many.

	u32 i,num_checkout;
	pqueue_ring new_ring = pqueue_ring_init();
	pqueue_bd* curr_pqueue;

	if(num_pqueue <= queue_free.length){
		num_checkout = num_pqueue;
	} else {
		num_checkout = queue_free.length;
	}

	//Traverse the queue_free and update the pointers
	for (i=0;i<num_checkout;i++){
		curr_pqueue = queue_free.first;
		//Remove from free ring
		pqueue_remove(&queue_free,curr_pqueue);
		//Add to new checkout ring
		pqueue_insertEnd(&new_ring,curr_pqueue);
	}
	return new_ring;
}
void queue_checkin(pqueue_ring* ring){
	pqueue_bd* curr_pqueue;
	pqueue_bd* next_pqueue;

	curr_pqueue = ring->first;

	while(curr_pqueue != NULL){
		next_pqueue = curr_pqueue->next;
		pqueue_remove(ring,curr_pqueue);
		pqueue_insertEnd(&queue_free,curr_pqueue);
		curr_pqueue = next_pqueue;
	}

	return;
}

void pqueue_insertAfter(pqueue_ring* ring, pqueue_bd* bd, pqueue_bd* bd_new){
	bd_new->prev = bd;
	bd_new->next = bd->next;
	if(bd->next == NULL){
		ring->last = bd_new;
	} else {
		bd->next->prev = bd_new;
	}
	bd->next = bd_new;
	(ring->length)++;
	return;
}

void pqueue_insertBefore(pqueue_ring* ring, pqueue_bd* bd, pqueue_bd* bd_new){
	bd_new->prev = bd->prev;
	bd_new->next = bd;
	if(bd->prev == NULL){
		ring->first = bd_new;
	} else {
		bd->prev->next = bd_new;
	}
	bd->prev = bd_new;
	(ring->length)++;
	return;
}

void pqueue_insertBeginning(pqueue_ring* ring, pqueue_bd* bd_new){
	if(ring->first == NULL){
		ring->first = bd_new;
		ring->last = bd_new;
		bd_new->prev = NULL;
		bd_new->next = NULL;
		(ring->length)++;
	} else {
		pqueue_insertBefore(ring, ring->first, bd_new);
	}
	return;
}

void pqueue_insertEnd(pqueue_ring* ring, pqueue_bd* bd_new){
	if(ring->last == NULL){
		pqueue_insertBeginning(ring,bd_new);
	} else {
		pqueue_insertAfter(ring,ring->last, bd_new);
	}
	return;
}

void pqueue_remove(pqueue_ring* ring, pqueue_bd* bd){
	if(bd->prev == NULL){
		ring->first = bd->next;
	} else {
		bd->prev->next = bd->next;
	}

	if(bd->next == NULL){
		ring->last = bd->prev;
	} else {
		bd->next->prev = bd->prev;
	}
	(ring->length)--;
}

pqueue_ring pqueue_ring_init(){
	pqueue_ring ring;
	ring.first = NULL;
	ring.last = NULL;
	ring.length = 0;
	return ring;
}


void pqueue_print(pqueue_ring* ring){
	pqueue_bd* curr_bd = ring->first;

	xil_printf("******** pqueue_print ********\n");
	xil_printf("ring->first:     0x%08x\n", ring->first);
	xil_printf("ring->last:      0x%08x\n", ring->last);
	xil_printf("ring->length:    %d\n\n", ring->length);

	while(curr_bd != NULL){
		xil_printf("0x%08x\n", curr_bd);
		xil_printf("  |  prev:      0x%08x\n", curr_bd->prev);
		xil_printf("  |  next:      0x%08x\n", curr_bd->next);
		xil_printf("  |       frame_ptr: 0x%08x\n", curr_bd->frame_ptr);
		curr_bd = curr_bd->next;
	}
}




























////////////////////////// OLD QUEUE /////////////////////////////

static packet_queue_element* low_pri_tx_queue;
static u16 low_pri_queue_read_index;
static u16 low_pri_queue_write_index;

static packet_queue_element* high_pri_tx_queue;
static u16 high_pri_queue_read_index;
static u16 high_pri_queue_write_index;

u16 wlan_mac_queue_get_size(u8 queue_sel){
	u16 return_value;

	//We adopt the convention that (read_index == write_index) indicates an empty buffer. Not a full buffer.
	//The TX_QUEUE_LENGTH definitions actually contain one more element than will actually be filled in order to
	//avoid full/empty ambiguity.

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

		break;
	}
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

	return return_value;
}

void wlan_mac_enqueue(u8 queue_sel){
	//This command should only be called if wlan_mac_queue_get_write_addr returns non-NULL value;
	switch(queue_sel){
		case HIGH_PRI_QUEUE_SEL:
			high_pri_queue_write_index = (high_pri_queue_write_index+1)%HIGH_PRI_TX_QUEUE_LENGTH;
		break;
		case LOW_PRI_QUEUE_SEL:
			low_pri_queue_write_index = (low_pri_queue_write_index+1)%LOW_PRI_TX_QUEUE_LENGTH;
		break;
	}

}

void wlan_mac_dequeue(u8 queue_sel){
	switch(queue_sel){
		case HIGH_PRI_QUEUE_SEL:
			high_pri_queue_read_index = (high_pri_queue_read_index+1)%HIGH_PRI_TX_QUEUE_LENGTH;
		break;
		case LOW_PRI_QUEUE_SEL:
			low_pri_queue_read_index = (low_pri_queue_read_index+1)%LOW_PRI_TX_QUEUE_LENGTH;
		break;
	}
}
