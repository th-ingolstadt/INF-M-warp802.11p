/** @file wlan_mac_eth_util.h
 *  @brief Ethernet Framework
 *
 *  Contains code for using Ethernet, including encapsulation and de-encapsulation.
 *
 *  @copyright Copyright 2013-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/***************************** Include Files *********************************/

#ifndef WLAN_MAC_ETH_UTIL_H_
#define WLAN_MAC_ETH_UTIL_H_

#include "wlan_mac_high_sw_config.h"
#include "wlan_mac_high_types.h"

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

#define ETH_PAYLOAD_OFFSET								   ( sizeof(mac_header_80211) + sizeof(llc_header_t) - sizeof(ethernet_header_t) )

#define WLAN_PROCESS_ETH_RX_RETURN_IS_ENQUEUED	0x0000001


#if WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE

/*************************** Function Prototypes *****************************/

int           wlan_eth_util_init();
void          wlan_mac_util_set_eth_rx_callback(void(*callback)());
void          wlan_mac_util_set_eth_encap_mode(application_role_t mode);
int           wlan_mpdu_eth_send(void* mpdu, u16 length, u8 pre_llc_offset);
void 	      wlan_eth_portal_en(u8 enable);
u32 	 	  wlan_process_eth_rx(void* eth_rx_buf, u32 eth_rx_len);

#endif /* WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE */

#endif /* WLAN_MAC_ETH_UTIL_H_ */
