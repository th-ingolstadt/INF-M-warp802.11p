////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_eth_util.h
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

#ifndef WLAN_MAC_ETH_UTIL_H_
#define WLAN_MAC_ETH_UTIL_H_

#define ETH_A_DMA_DEV_ID	XPAR_MB_HIGH_ETH_DMA_DEVICE_ID

//Ethernet MAC-PHY link speed - must match PHY's actual link speed, as auto-negotiated at run time
#define ETH_A_LINK_SPEED	1000

//Memory space allocated per Ethernet packet
#define ETH_A_PKT_BUF_SIZE	0x800 //2KB

//Number of Tx and Rx DMA buffer descriptors
#define ETH_A_NUM_TX_BD		1
//#define ETH_A_NUM_RX_BD		200

//Current layout of DATA_BRAM:
// [0:48KB] : Tx Queue (managed by wlan_mac_queue.c/h)
// [48:64KB]:
//   Eth Tx Buffer Descriptor(s)
//   Eth Rx Buffer Descriptors
//   Eth Rx Packet Buffers
//
// All Eth Tx come from wireless Rx packet buffers, so no extra buffer space is required here

#define ETH_A_BUF_MEM_BASE		(XPAR_MB_HIGH_DATA_BRAM_CTRL_S_AXI_BASEADDR + (48*1024)) //bottom 48kB are Tx queues

#define ETH_A_TX_BD_SPACE_BASE	(ETH_A_BUF_MEM_BASE)
#define ETH_A_RX_BD_SPACE_BASE	(ETH_A_TX_BD_SPACE_BASE + (ETH_A_NUM_TX_BD * XAXIDMA_BD_MINIMUM_ALIGNMENT)) //safer than sizeof(XAxiDma_Bd)?


int wlan_eth_init();
int wlan_eth_dma_init();
int wlan_eth_send(void* mpdu, u16 length);
int wlan_eth_dma_send(u8* pkt_ptr, u32 length);
inline void wlan_poll_eth();
void wlan_eth_dma_update();

#endif /* WLAN_MAC_ETH_UTIL_H_ */
