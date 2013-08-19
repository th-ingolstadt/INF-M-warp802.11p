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

#define NUM_QUEUES 10

typedef struct pqueue pqueue;
struct pqueue{
	station_info* station_info_ptr;
	pqueue* next;
	pqueue* prev;
	tx_packet_buffer* pktbuf_ptr;
};

typedef struct {
	pqueue* first;
	pqueue* last;
	u16 length;
} pqueue_list;

#define PQUEUE_MAX_FRAME_SIZE	0x800 	//2KB

//If using BRAM for pqueues
//#define PQUEUE_LEN				20 		//Total Queue Size (bytes) = PQUEUE_LEN*(PQUEUE_MAX_FRAME_SIZE + sizeof(pqueue))

//If using DRAM for pqueues//
//#define PQUEUE_LEN				1000 		//Total Queue Size (bytes) = PQUEUE_LEN*(PQUEUE_MAX_FRAME_SIZE + sizeof(pqueue))

//Bottom 48kB of data BRAM is used for PQUEUE
#define PQUEUE_MEM_BASE				(XPAR_MB_HIGH_DATA_BRAM_CTRL_S_AXI_BASEADDR)

//First section of pqueue memory space is the pqueue buffer descriptors
#define PQUEUE_SPACE_BASE		PQUEUE_MEM_BASE

//Second section of pqueue memory space is the raw buffer space for payloads
//Use BRAM for queue
//#define PQUEUE_BUFFER_SPACE_BASE	(PQUEUE_MEM_BASE+(PQUEUE_LEN*sizeof(pqueue)))
//Use DRAM for queue
//#define PQUEUE_BUFFER_SPACE_BASE	XPAR_DDR3_SODIMM_S_AXI_BASEADDR



void queue_init();

void enqueue_after_end(u16 queue_sel, pqueue_list* ring);
pqueue_list dequeue_from_beginning(u16 queue_sel, u16 num_pqueue);


//Functions for checking in and out pqueues from the free ring
pqueue_list queue_checkout(u16 num_pqueue);
void queue_checkin(pqueue_list* ring);
inline u32 queue_num_free();
inline u32 queue_num_queued(u16 queue_sel);

//Doubly linked list helper functions
void pqueue_insertAfter(pqueue_list* ring, pqueue* bd, pqueue* bd_new);
void pqueue_insertBefore(pqueue_list* ring, pqueue* bd, pqueue* bd_new);
void pqueue_insertBeginning(pqueue_list* ring, pqueue* bd_new);
void pqueue_insertEnd(pqueue_list* ring, pqueue* bd_new);
void pqueue_remove(pqueue_list* ring, pqueue* bd);
int queue_total_size();

pqueue_list pqueue_list_init();
void pqueue_print(pqueue_list* ring);
void queue_dram_present(u8 present);


#endif /* WLAN_MAC_QUEUE_H_ */
