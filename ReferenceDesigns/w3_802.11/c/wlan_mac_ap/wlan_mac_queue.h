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

#define MAX_PACKET_SIZE 2000
#define LOW_PRI_TX_QUEUE_LENGTH		21
#define HIGH_PRI_TX_QUEUE_LENGTH	7

#define HIGH_PRI_QUEUE_SEL	0
#define LOW_PRI_QUEUE_SEL	1

typedef struct{
	station_info* station_info_ptr;
	tx_frame_info frame_info;
	u8 phy_hdr_pad[8];
	u8 frame[MAX_PACKET_SIZE];
} packet_queue_element;

#define HIGH_PRI_QUEUE_BASEADDR	XPAR_MB_HIGH_DATA_BRAM_CTRL_S_AXI_BASEADDR
#define LOW_PRI_QUEUE_BASEADDR	(XPAR_MB_HIGH_DATA_BRAM_CTRL_S_AXI_BASEADDR + (sizeof(packet_queue_element)*HIGH_PRI_TX_QUEUE_LENGTH))

void wlan_mac_queue_init();
u16 wlan_mac_queue_get_size(u8 queue_sel);
packet_queue_element* wlan_mac_queue_get_write_element(u8 queue_sel);
packet_queue_element* wlan_mac_queue_get_read_element(u8 queue_sel);
void wlan_mac_enqueue(u8 queue_sel);
void wlan_mac_dequeue(u8 queue_sel);


#endif /* WLAN_MAC_QUEUE_H_ */
