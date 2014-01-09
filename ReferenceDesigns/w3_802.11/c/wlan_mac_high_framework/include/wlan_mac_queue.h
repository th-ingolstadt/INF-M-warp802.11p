/** @file wlan_mac_queue.h
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

#ifndef WLAN_MAC_QUEUE_H_
#define WLAN_MAC_QUEUE_H_

#include "wlan_mac_dl_list.h"

//In spirit, packet_bd is derived from dl_node. Since C
//lacks a formal notion of inheritance, we adopt a popular
//alternative idiom for inheritance where the dl_node
//is the first entry in the new structure. Since structures
//will never be padded before their first entry, it is safe
//to cast back and forth between the packet_bd and dl_node.
typedef struct packet_bd packet_bd;
struct packet_bd{
	dl_node node;
	void* metadata_ptr;
	void* buf_ptr;
};

#define PQUEUE_MAX_FRAME_SIZE	0x800 	//2KB

//Bottom 48kB of data BRAM is used for PQUEUE
#define PQUEUE_MEM_BASE				(XPAR_MB_HIGH_AUX_BRAM_CTRL_S_AXI_BASEADDR)

//First section of packet_bd memory space is the packet_bd buffer descriptors
#define PQUEUE_SPACE_BASE		PQUEUE_MEM_BASE

//Second section of packet_bd memory space is the raw buffer space for payloads
//Use BRAM for queue
//#define PQUEUE_BUFFER_SPACE_BASE	(PQUEUE_MEM_BASE+(PQUEUE_LEN*sizeof(packet_bd)))
//Use DRAM for queue
//#define PQUEUE_BUFFER_SPACE_BASE	XPAR_DDR3_SODIMM_S_AXI_BASEADDR



int queue_init();

void enqueue_after_end(u16 queue_sel, dl_list* list);
void dequeue_from_beginning(dl_list* new_list, u16 queue_sel, u16 num_packet_bd);


//Functions for checking in and out packet_bds from the free ring
void queue_checkout(dl_list* new_list, u16 num_packet_bd);
void queue_checkin(dl_list* list);
inline u32 queue_num_free();
inline u32 queue_num_queued(u16 queue_sel);

int queue_total_size();
void purge_queue(u16 queue_sel);

void queue_dram_present(u8 present);
inline int wlan_mac_queue_poll(u16 queue_sel);

#endif /* WLAN_MAC_QUEUE_H_ */
