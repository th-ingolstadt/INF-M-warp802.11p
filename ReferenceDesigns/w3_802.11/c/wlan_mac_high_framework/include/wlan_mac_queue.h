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
 */

#ifndef WLAN_MAC_QUEUE_H_
#define WLAN_MAC_QUEUE_H_

#include "wlan_mac_dl_list.h"
#include "wlan_mac_misc_util.h"

#define QUEUE_BUFFER_SIZE	0x1000 	//4KB

typedef struct{
	u8	  metadata_type;
	u8	  reserved[3];
	u32   metadata_ptr;
} tx_queue_metadata;

#define QUEUE_METADATA_TYPE_IGNORE			0x00
#define QUEUE_METADATA_TYPE_STATION_INFO	0x01
#define QUEUE_METADATA_TYPE_TX_PARAMS   	0x02


typedef struct{
	tx_queue_metadata metadata;
	tx_frame_info frame_info;
	u8 phy_hdr_pad[PHY_TX_PKT_BUF_PHY_HDR_SIZE];
	u8 frame[QUEUE_BUFFER_SIZE - PHY_TX_PKT_BUF_PHY_HDR_SIZE - sizeof(tx_frame_info) - sizeof(tx_queue_metadata)];
} tx_queue_buffer;

//Bottom 48kB of data BRAM is used for PACKET_BD
#define QUEUE_DL_ENTRY_MEM_BASE				(XPAR_MB_HIGH_AUX_BRAM_CTRL_S_AXI_BASEADDR)

//First section of packet_bd memory space is the packet_bd buffer descriptors
#define QUEUE_DL_ENTRY_SPACE_BASE		QUEUE_DL_ENTRY_MEM_BASE


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
