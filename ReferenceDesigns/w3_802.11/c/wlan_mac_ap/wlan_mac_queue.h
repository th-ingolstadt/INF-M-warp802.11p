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

typedef struct packet_bd packet_bd;
struct packet_bd{
	station_info* station_info_ptr;
	packet_bd* next;
	packet_bd* prev;
	tx_packet_buffer* pktbuf_ptr;
};

typedef struct {
	packet_bd* first;
	packet_bd* last;
	u16 length;
} packet_bd_list;

#define PQUEUE_MAX_FRAME_SIZE	0x800 	//2KB

//If using BRAM for packet_bds
//#define PQUEUE_LEN				20 		//Total Queue Size (bytes) = PQUEUE_LEN*(PQUEUE_MAX_FRAME_SIZE + sizeof(packet_bd))

//If using DRAM for packet_bds//
//#define PQUEUE_LEN				1000 		//Total Queue Size (bytes) = PQUEUE_LEN*(PQUEUE_MAX_FRAME_SIZE + sizeof(packet_bd))

//Bottom 48kB of data BRAM is used for PQUEUE
#define PQUEUE_MEM_BASE				(XPAR_MB_HIGH_DATA_BRAM_CTRL_S_AXI_BASEADDR)

//First section of packet_bd memory space is the packet_bd buffer descriptors
#define PQUEUE_SPACE_BASE		PQUEUE_MEM_BASE

//Second section of packet_bd memory space is the raw buffer space for payloads
//Use BRAM for queue
//#define PQUEUE_BUFFER_SPACE_BASE	(PQUEUE_MEM_BASE+(PQUEUE_LEN*sizeof(packet_bd)))
//Use DRAM for queue
//#define PQUEUE_BUFFER_SPACE_BASE	XPAR_DDR3_SODIMM_S_AXI_BASEADDR



void queue_init();

void enqueue_after_end(u16 queue_sel, packet_bd_list* ring);
packet_bd_list dequeue_from_beginning(u16 queue_sel, u16 num_packet_bd);


//Functions for checking in and out packet_bds from the free ring
packet_bd_list queue_checkout(u16 num_packet_bd);
void queue_checkin(packet_bd_list* ring);
inline u32 queue_num_free();
inline u32 queue_num_queued(u16 queue_sel);

//Doubly linked list helper functions
void packet_bd_insertAfter(packet_bd_list* ring, packet_bd* bd, packet_bd* bd_new);
void packet_bd_insertBefore(packet_bd_list* ring, packet_bd* bd, packet_bd* bd_new);
void packet_bd_insertBeginning(packet_bd_list* ring, packet_bd* bd_new);
void packet_bd_insertEnd(packet_bd_list* ring, packet_bd* bd_new);
void packet_bd_remove(packet_bd_list* ring, packet_bd* bd);
int queue_total_size();

packet_bd_list packet_bd_list_init();
void packet_bd_print(packet_bd_list* ring);
void queue_dram_present(u8 present);


#endif /* WLAN_MAC_QUEUE_H_ */
