/** @file wlan_mac_eth_util.h
 *  @brief Ethernet Framework
 *
 *  Contains code for using Ethernet, including encapsulation and de-encapsulation.
 *
 *  @copyright Copyright 2013-2015, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

/***************************** Include Files *********************************/

#include "xintc.h"


/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_ETH_UTIL_H_
#define WLAN_MAC_ETH_UTIL_H_

//-----------------------------------------------
// xparameter.h definitions
//
// Ethernet A
#define ETH_A_MAC_DEVICE_ID                                XPAR_ETH_A_MAC_DEVICE_ID
#define ETH_A_DMA_DEVICE_ID                                XPAR_ETH_A_DMA_DEVICE_ID

#define ETH_A_RX_INTR_ID                                   XPAR_INTC_0_AXIDMA_0_S2MM_INTROUT_VEC_ID
#define ETH_A_TX_INTR_ID                                   XPAR_INTC_0_AXIDMA_0_MM2S_INTROUT_VEC_ID

// Ethernet B
#define ETH_B_MAC_DEVICE_ID                                XPAR_ETH_B_MAC_DEVICE_ID
#define ETH_B_DMA_DEVICE_ID                                XPAR_ETH_B_DMA_DEVICE_ID

#define ETH_B_RX_INTR_ID                                   XPAR_INTC_0_AXIDMA_1_S2MM_INTROUT_VEC_ID
#define ETH_B_TX_INTR_ID                                   XPAR_INTC_0_AXIDMA_1_MM2S_INTROUT_VEC_ID


//-----------------------------------------------
// Ethernet PHY defines
//
#define ETH_A_MDIO_PHYADDR                                 0x6
#define ETH_B_MDIO_PHYADDR                                 0x7


//-----------------------------------------------
// WLAN Ethernet defines
//     NOTE:  Ethernet device associated with device ID must match Ethernet device associated
//         with MDIO PHY address
//
#define WLAN_ETH_DEV_ID                                    ETH_A_MAC_DEVICE_ID
#define WLAN_ETH_DMA_DEV_ID                                ETH_A_DMA_DEVICE_ID
#define WLAN_ETH_MDIO_PHYADDR                              ETH_A_MDIO_PHYADDR
#define WLAN_ETH_RX_INTR_ID                                ETH_A_RX_INTR_ID
#define WLAN_ETH_TX_INTR_ID                                ETH_A_TX_INTR_ID
#define WLAN_ETH_LINK_SPEED	                               1000
#define WLAN_ETH_PKT_BUF_SIZE                              0x800               // 2KB - space allocated per pkt


//-----------------------------------------------
// Magic numbers used for Ethernet/IP/UDP/DHCP/ARP packet interpretation
//
#define DHCP_BOOTP_FLAGS_BROADCAST                         0x8000
#define DHCP_MAGIC_COOKIE                                  0x63825363
#define DHCP_OPTION_TAG_TYPE                               53
#define DHCP_OPTION_TYPE_DISCOVER                          1
#define DHCP_OPTION_TYPE_OFFER                             2
#define DHCP_OPTION_TYPE_REQUEST                           3
#define DHCP_OPTION_TYPE_ACK                               5
#define DHCP_OPTION_TAG_IDENTIFIER                         61
#define DHCP_OPTION_END                                    255
#define DHCP_HOST_NAME                                     12

#define IPV4_PROT_UDP                                      0x11

#define UDP_SRC_PORT_BOOTPC                                68
#define UDP_SRC_PORT_BOOTPS                                67

#define ETH_TYPE_ARP                                       0x0608
#define ETH_TYPE_IP                                        0x0008

#define LLC_SNAP                                           0xAA
#define LLC_CNTRL_UNNUMBERED                               0x03
#define LLC_TYPE_ARP                                       0x0608
#define LLC_TYPE_IP                                        0x0008
#define LLC_TYPE_WLAN_LTG                                  0x9090              // Non-standard type for LTG packets



/*********************** Global Structure Definitions ************************/

// The struct definitions below are used to interpret packet payloads.  The
// code never creates instances of these structs

//
// See definitions in WARP_ip_udp.h for:
//     ethernet_header
//     ipv4_header
//     udp_header
//     arp_ipv4_packet
//

typedef struct{
	u8  op;
	u8  htype;
	u8  hlen;
	u8  hops;
	u32 xid;
	u16 secs;
	u16 flags;
	u8  ciaddr[4];
	u8  yiaddr[4];
	u8  siaddr[4];
	u8  giaddr[4];
	u8  chaddr[6];
	u8  chaddr_padding[10];
	u8  padding[192];
	u32 magic_cookie;
} dhcp_packet;


/*************************** Function Prototypes *****************************/

int           wlan_eth_init();
int           wlan_eth_setup_interrupt(XIntc* intc);

void          wlan_mac_util_set_eth_rx_callback(void(*callback)());
void          wlan_mac_util_set_eth_encap_mode(u8 mode);

inline int    eth_get_num_rx_bd();

int           wlan_mpdu_eth_send(void* mpdu, u16 length, u8 pre_llc_offset);

void          wlan_eth_dma_update();


#endif /* WLAN_MAC_ETH_UTIL_H_ */
