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

#ifndef WLAN_MAC_ETH_UTIL_H_
#define WLAN_MAC_ETH_UTIL_H_

/***************************** Include Files *********************************/
#include "wlan_mac_high_sw_config.h"
#include "wlan_high_types.h"

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


#define ETH_ADDR_SIZE                                      6                   // Length of Ethernet MAC address (in bytes)
#define IP_ADDR_SIZE                                       4                   // Length of IP address (in bytes)

#define ETH_PAYLOAD_OFFSET								   ( sizeof(mac_header_80211) + sizeof(llc_header_t) - sizeof(ethernet_header_t) )

#define WLAN_PROCESS_ETH_RX_RETURN_IS_ENQUEUED	0x0000001


/*********************** Global Structure Definitions ************************/

// The struct definitions below are used to interpret packet payloads.  The
// code never creates instances of these structs

//
// See definitions in wlan_exp_ip_udp.h for:
//     ethernet_header
//     ipv4_header
//     udp_header
//     arp_ipv4_packet
//

typedef struct dhcp_packet{
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
	u8  chaddr[MAC_ADDR_LEN];
	u8  chaddr_padding[10];
	u8  padding[192];
	u32 magic_cookie;
} dhcp_packet;

typedef struct ethernet_header_t{
    u8                       dest_mac_addr[ETH_ADDR_SIZE];                      // Destination MAC address
    u8                       src_mac_addr[ETH_ADDR_SIZE];                       // Source MAC address
    u16                      ethertype;                                        // EtherType
} ethernet_header_t;

typedef struct ipv4_header_t{
    u8                       version_ihl;                                      // [7:4] Version; [3:0] Internet Header Length
    u8                       dscp_ecn;                                         // [7:2] Differentiated Services Code Point; [1:0] Explicit Congestion Notification
    u16                      total_length;                                     // Total Length (includes header and data - in bytes)
    u16                      identification;                                   // Identification
    u16                      fragment_offset;                                  // [15:14] Flags;   [13:0] Fragment offset
    u8                       ttl;                                              // Time To Live
    u8                       protocol;                                         // Protocol
    u16                      header_checksum;                                  // IP header checksum
    u32                      src_ip_addr;                                      // Source IP address (big endian)
    u32                      dest_ip_addr;                                     // Destination IP address (big endian)
} ipv4_header_t;

typedef struct arp_ipv4_packet_t{
    u16                      htype;                                            // Hardware Type
    u16                      ptype;                                            // Protocol Type
    u8                       hlen;                                             // Length of Hardware address
    u8                       plen;                                             // Length of Protocol address
    u16                      oper;                                             // Operation
    u8                       sender_haddr[ETH_ADDR_SIZE];                       // Sender hardware address
    u8                       sender_paddr[IP_ADDR_SIZE];                        // Sender protocol address
    u8                       target_haddr[ETH_ADDR_SIZE];                       // Target hardware address
    u8                       target_paddr[IP_ADDR_SIZE];                        // Target protocol address
} arp_ipv4_packet_t;

typedef struct udp_header_t{
    u16                      src_port;                                         // Source port number
    u16                      dest_port;                                        // Destination port number
    u16                      length;                                           // Length of UDP header and UDP data (in bytes)
    u16                      checksum;                                         // Checksum
} udp_header_t;

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
