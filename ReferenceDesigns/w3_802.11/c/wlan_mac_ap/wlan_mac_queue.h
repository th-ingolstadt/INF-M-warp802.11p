////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_queue.h
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

#ifndef WLAN_MAC_QUEUE_H_
#define WLAN_MAC_QUEUE_H_

#define NUM_QUEUES 9 //1 for each association + 1 for non-associated packets

typedef struct pqueue_bd pqueue_bd;
struct pqueue_bd{
	station_info* station_info_ptr;
	pqueue_bd* next;
	pqueue_bd* prev;
	tx_packet_buffer* pktbuf_ptr;
};

typedef struct {
	pqueue_bd* first;
	pqueue_bd* last;
	u16 length;
} pqueue_ring;

#define PQUEUE_MAX_FRAME_SIZE	0x800 	//2KB
#define PQUEUE_LEN				20 		//Total Queue Size (bytes) = PQUEUE_LEN*(PQUEUE_MAX_FRAME_SIZE + sizeof(pqueue_bd))

//Bottom 48kB of data BRAM is used for PQUEUE
#define PQUEUE_MEM_BASE				(XPAR_MB_HIGH_DATA_BRAM_CTRL_S_AXI_BASEADDR)

//First section of pqueue memory space is the pqueue buffer descriptors
#define PQUEUE_BD_SPACE_BASE		PQUEUE_MEM_BASE

//Second section of pqueue memory space is the raw buffer space for payloads
#define PQUEUE_BUFFER_SPACE_BASE	(PQUEUE_MEM_BASE+(PQUEUE_LEN*sizeof(pqueue_bd)))



void queue_init();

void enqueue_after_end(u16 queue_sel, pqueue_ring* ring);
pqueue_ring dequeue_from_beginning(u16 queue_sel, u16 num_pqueue);


//Functions for checking in and out pqueues from the free ring
pqueue_ring queue_checkout(u16 num_pqueue);
void queue_checkin(pqueue_ring* ring);

//Doubly linked list helper functions
void pqueue_insertAfter(pqueue_ring* ring, pqueue_bd* bd, pqueue_bd* bd_new);
void pqueue_insertBefore(pqueue_ring* ring, pqueue_bd* bd, pqueue_bd* bd_new);
void pqueue_insertBeginning(pqueue_ring* ring, pqueue_bd* bd_new);
void pqueue_insertEnd(pqueue_ring* ring, pqueue_bd* bd_new);
void pqueue_remove(pqueue_ring* ring, pqueue_bd* bd);

pqueue_ring pqueue_ring_init();
void pqueue_print(pqueue_ring* ring);


#endif /* WLAN_MAC_QUEUE_H_ */
