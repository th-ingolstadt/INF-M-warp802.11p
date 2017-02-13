/** @file wlan_mac_eth_util.c
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




#include "wlan_mac_high_sw_config.h"

#if WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE

#include "stdlib.h"
#include "stddef.h"
#include "xil_types.h"
#include "xintc.h"
#include "string.h"

#include "wlan_platform_high.h"
#include "wlan_mac_common.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_station_info.h"



/*********************** Global Variable Definitions *************************/

// Controls whether or not portal behaviors will be enabled
static u8 gl_portal_en;

/*************************** Variable Definitions ****************************/

// Ethernet encapsulation mode
//     See:  http://warpproject.org/trac/wiki/802.11/MAC/Upper/MACHighFramework/EthEncap
static application_role_t    eth_encap_mode;

// Ethernet Station MAC Address
//
//   The station code's implementation of encapsulation and de-encapsulation has an important
// limitation: only one device may be plugged into the station's Ethernet port. The station
// does not provide NAT. It assumes that the last received Ethernet src MAC address is used
// as the destination MAC address on any Ethernet transmissions. This is fine when there is
// only one device on the station's Ethernet port, but will definitely not work if the station
// is plugged into a switch with more than one device.
//
static u8                    eth_sta_mac_addr[6];

// Callback for top-level processing of Ethernet received packets
static function_ptr_t        eth_rx_callback;
static function_ptr_t        eth_rx_early_rejection_callback;


/*************************** Functions Prototypes ****************************/


#if PERF_MON_ETH_BD
void     print_bd_high_water_mark() { xil_printf("BD HWM = %d\n", bd_high_water_mark); }
#endif


/******************************** Functions **********************************/

int      wlan_eth_encap(u8* mpdu_start_ptr, u8* eth_dest, u8* eth_src, u8* eth_start_ptr, u32 eth_rx_len);

/*****************************************************************************/
/**
 * @brief Initialize the Ethernet utility sub-system
 *
 * Initialize Ethernet A hardware and framework for handling Ethernet Tx/Rx via
 * the DMA.  This function must be called once at boot, before any Ethernet
 * configuration or Tx/Rx operations.
 *
 * @return 0 on success
 */
int wlan_eth_util_init() {

	// Initialize Callback
	eth_rx_callback = (function_ptr_t)wlan_null_callback;
	eth_rx_early_rejection_callback = (function_ptr_t)wlan_null_callback;

    // Enable bridging of the wired-wireless networks (a.k.a. the "portal" between networks)
	wlan_eth_portal_en(1);
    return 0;
}

/*****************************************************************************/
/**
 * @brief Sets the MAC callback to process Ethernet receptions
 *
 * The framework will call the MAC's callback for each Ethernet reception that is a candidate
 * for wireless transmission. The framework may reject some packets for malformed or unrecognized
 * Ethernet headers. The MAC may reject more, based on Ethernet address or packet contents.
 *
 * @param void(*callback)
 *  -Function pointer to the MAC's Ethernet Rx callback
 */
void wlan_mac_util_set_eth_rx_callback(void(*callback)()) {
    eth_rx_callback = (function_ptr_t)callback;
}


/*****************************************************************************/
/**
 * @brief Sets the MAC callback to provide early rejection feedback to the framework
 *
 * Prior to encapsulating a received Ethernet frame, the framework will call the
 * early rejection callback if it has been attached by the application. If the application
 * so chooses, the application can provide feedback to the framework to bypass the
 * encapsulation procedure and drop the packet. This is purely optional and is intended
 * to provided performance improvements in busy unswitched Ethernet networks.
 *
 * @param void(*callback)
 *  -Function pointer to the MAC's Ethernet Rx Early Rejection callback
 */
void wlan_mac_util_set_eth_rx_early_rejection_callback(void(*callback)()) {
	eth_rx_early_rejection_callback = (function_ptr_t)callback;
}

/*****************************************************************************/
/**
 * @brief Sets the wired-wireless encapsulation mode
 *
 * @param application_role_t mode		- The role of the high-level application (AP, STA, or IBSS)
 */
void wlan_mac_util_set_eth_encap_mode(application_role_t mode) {
    eth_encap_mode = mode;
}

/*****************************************************************************/
/**
 * @brief Process an Ethernet packet that has been received by the ETH DMA
 *
 * This function processes an Ethernet DMA buffer descriptor.  This design assumes
 * a 1-to-1 correspondence between buffer descriptors and Ethernet packets.  For
 * each packet, this function encapsulates the Ethernet packet and calls the
 * MAC's callback to either enqueue (for eventual wireless Tx) or reject the packet.
 *
 * NOTE:  Processed ETH DMA buffer descriptors are freed but not resubmitted to
 * hardware for use by future Ethernet receptions.  This is the responsibility of
 * higher level code (see comment at the end of the function if this behavior needs
 * to change).
 *
 * This function requires the MAC implement a function (assigned to the eth_rx_callback
 * function pointer) that returns 0 or 1, indicating the MAC's handling of the packet:
 *     0: Packet was not enqueued and will not be processed by wireless MAC; framework
 *        should immediately discard
 *     1: Packet was enqueued for eventual wireless transmission; MAC will check in
 *        occupied queue entry when finished
 *
 * @param void* eth_rx_buf
 *  - Pointer buffer containing Ethernet payload
 * @param u32 eth_rx_len
 *  - Length of received Ethernet frame
 *
 */
u32 wlan_process_eth_rx(void* eth_rx_buf, u32 eth_rx_len) {
    u8                * mpdu_start_ptr;
    u8                * eth_start_ptr;
    dl_entry*           curr_tx_queue_element;
    u32                 mpdu_tx_len;
    tx_queue_buffer_t*	tx_queue_buffer;
    ethernet_header_t*  eth_hdr;

    u32					return_value = 0;
    int                 packet_is_queued;

    u8                  eth_dest[6];
    u8                  eth_src[6];

#if PERF_MON_ETH_PROCESS_RX
    wlan_mac_set_dbg_hdr_out(0x4);
#endif

    // Check arguments
    if ((eth_rx_buf == NULL) || (eth_rx_len == 0)) {
        xil_printf("ERROR:  Tried to process NULL Ethernet packet\n");
        return return_value;
    }

    // Process Ethernet packet
    packet_is_queued = 0;

	// The start of the MPDU is before the first byte of the DMA transfer. We can work our way backwards from this point.
	mpdu_start_ptr = (void*)( (u8*)eth_rx_buf - ETH_PAYLOAD_OFFSET );

    // Get the TX queue entry pointer. This is located further back in the tx_queue_buffer_t struct.
	//
	tx_queue_buffer = (tx_queue_buffer_t*)( (u8*)mpdu_start_ptr - offsetof(tx_queue_buffer_t,frame));
	curr_tx_queue_element = tx_queue_buffer->tx_queue_entry;

	eth_start_ptr  = (u8*)eth_rx_buf;

	eth_hdr = (ethernet_header_t*)eth_start_ptr;

	if(eth_rx_early_rejection_callback(eth_hdr->dest_mac_addr, eth_hdr->src_mac_addr) == 0){
		// Encapsulate the Ethernet packet
		mpdu_tx_len    = wlan_eth_encap(mpdu_start_ptr, eth_dest, eth_src, eth_start_ptr, eth_rx_len);

		if ((gl_portal_en == 0) || (mpdu_tx_len == 0)) {
			// Encapsulation failed (Probably because of an unknown ETHERTYPE value)
			//     Don't pass the invalid frame to the MAC - just cleanup and return
			packet_is_queued = 0;
		} else {
			// Call the MAC's callback to process the packet
			//     MAC will either enqueue the packet for eventual transmission or reject the packet
			packet_is_queued = eth_rx_callback(curr_tx_queue_element, eth_dest, eth_src, mpdu_tx_len);
		}
	} else {
		packet_is_queued = 0;
	}

    // If the packet was not successfully enqueued, discard it and return its queue entry to the free pool
    //     For packets that are successfully enqueued, this cleanup is part of the post-wireless-Tx handler
    if (packet_is_queued == 0) {
        // Either the packet was invalid, or the MAC code failed to enqueue this packet
        //     The MAC will fail if the appropriate queue was full or the Ethernet addresses were not recognized.

        // Return the occupied queue entry to the free pool
        queue_checkin(curr_tx_queue_element);
    } else {
    	return_value |= WLAN_PROCESS_ETH_RX_RETURN_IS_ENQUEUED;
    }

#if PERF_MON_ETH_PROCESS_RX
    wlan_mac_clear_dbg_hdr_out(0x4);
#endif

    return return_value;
}



/*****************************************************************************/
/**
 * @brief Encapsulates Ethernet packets for wireless transmission
 *
 * This function implements the encapsulation process for 802.11 transmission of
 * Ethernet packets
 *
 * The encapsulation process depends on the node's role:
 *
 * AP:
 *     - Copy original packet's source and destination addresses to temporary space
 *     - Add an LLC header (8 bytes) in front of the Ethernet payload
 *         - LLC header includes original packet's ETHER_TYPE field; only IPV4 and ARP
 *           are currently supported
 *
 * STA:
 *    - Copy original packet's source and destination addresses to temporary space
 *    - Add an LLC header (8 bytes) in front of the Ethernet payload
 *        - LLC header includes original packet's ETHER_TYPE field; only IPV4 and ARP
 *          are currently supported
 *    - If packet is ARP Request, overwrite ARP header's source address with STA
 *      wireless MAC address
 *    - If packet is UDP packet containing a DHCP request
 *        - Assert DHCP header's BROADCAST flag
 *        - Disable the UDP packet checksum (otherwise it would be invliad after
 *          modifying the BROADCAST flag)
 *
 * Refer to the 802.11 Reference Design user guide for more details:
 *     http://warpproject.org/trac/wiki/802.11/MAC/Upper/MACHighFramework/EthEncap
 *
 * @param u8* mpdu_start_ptr
 *  - Pointer to the first byte of the MPDU payload (first byte of the eventual MAC
 *    header)
 * @param u8* eth_dest
 *  - Pointer to 6 bytes of free memory; will be overwritten with Ethernet packet's
 *    destination address
 * @param u8* eth_src
 *  - Pointer to 6 bytes of free memory; will be overwritten with Ethernet packet's
 *    source address
 * @param u8* eth_start_ptr
 *  - Pointer to first byte of received Ethernet packet's header
 * @param u32 eth_rx_len
 *  - Length (in bytes) of the packet payload
 *
 * @return 0 for if packet type is unrecognized (failed encapsulation), otherwise
 *     returns length of encapsulated packet (in bytes)
 */
int wlan_eth_encap(u8* mpdu_start_ptr, u8* eth_dest, u8* eth_src, u8* eth_start_ptr, u32 eth_rx_len) {
	ethernet_header_t        * eth_hdr;
    ipv4_header_t            * ip_hdr;
    arp_ipv4_packet_t        * arp;
    udp_header_t             * udp;
    dhcp_packet            * dhcp;
    llc_header_t           * llc_hdr;
    u32                      mpdu_tx_len;

    // Calculate actual wireless Tx len (eth payload - eth header + wireless header)
    mpdu_tx_len = eth_rx_len - sizeof(ethernet_header_t) + sizeof(llc_header_t) + sizeof(mac_header_80211) + WLAN_PHY_FCS_NBYTES;

    // Helper pointers to interpret/fill fields in the new MPDU
    eth_hdr = (ethernet_header_t*)eth_start_ptr;
    llc_hdr = (llc_header_t*)(mpdu_start_ptr + sizeof(mac_header_80211));

    // Copy the src/dest addresses from the received Eth packet to temp space
    memcpy(eth_src, eth_hdr->src_mac_addr, 6);
    memcpy(eth_dest, eth_hdr->dest_mac_addr, 6);

    // Prepare the MPDU LLC header
    llc_hdr->dsap          = LLC_SNAP;
    llc_hdr->ssap          = LLC_SNAP;
    llc_hdr->control_field = LLC_CNTRL_UNNUMBERED;
    bzero((void *)(llc_hdr->org_code), 3); //Org Code 0x000000: Encapsulated Ethernet

    switch(eth_encap_mode) {
    	default:
    		return 0;
    	break;
        // ------------------------------------------------
        case APPLICATION_ROLE_AP:
            switch(eth_hdr->ethertype) {
                case ETH_TYPE_ARP:
                    llc_hdr->type = LLC_TYPE_ARP;
                    arp = (arp_ipv4_packet_t*)((void*)eth_hdr + sizeof(ethernet_header_t));
                break;

                case ETH_TYPE_IP:
                    llc_hdr->type = LLC_TYPE_IP;
                break;

                default:
                    // Unknown/unsupported EtherType; don't process the Eth frame
                    return 0;
                break;
            }
        break;

        // ------------------------------------------------
        case APPLICATION_ROLE_IBSS:
        case APPLICATION_ROLE_STA:
            // Save this ethernet src address
            memcpy(eth_sta_mac_addr, eth_src, 6);
            memcpy(eth_src, get_mac_hw_addr_wlan(), 6);

            switch(eth_hdr->ethertype) {
                case ETH_TYPE_ARP:
                    llc_hdr->type = LLC_TYPE_ARP;

                    // Overwrite ARP request source MAC address field with the station's wireless MAC address.
                    arp = (arp_ipv4_packet_t*)((void*)eth_hdr + sizeof(ethernet_header_t));
                    memcpy(arp->sender_haddr, get_mac_hw_addr_wlan(), 6);
                break;

                case ETH_TYPE_IP:
                    llc_hdr->type = LLC_TYPE_IP;

                    // Check if IPv4 packet is a DHCP Discover in a UDP frame
                    ip_hdr = (ipv4_header_t*)((void*)eth_hdr + sizeof(ethernet_header_t));

                    if (ip_hdr->protocol == IPV4_PROT_UDP) {
                        udp = (udp_header_t*)((void*)ip_hdr + 4*((u8)(ip_hdr->version_ihl) & 0xF));

                        // Check if this is a DHCP Discover packet, which contains the source hardware
                        // address deep inside the packet (in addition to its usual location in the Eth
                        // header). For STA encapsulation, we need to overwrite this address with the
                        // MAC addr of the wireless station.
                        if ((Xil_Ntohs(udp->src_port) == UDP_SRC_PORT_BOOTPC) ||
                            (Xil_Ntohs(udp->src_port) == UDP_SRC_PORT_BOOTPS)) {

                            // Disable the checksum since this will change the bytes in the packet
                            udp->checksum = 0;

                            dhcp = (dhcp_packet*)((void*)udp + sizeof(udp_header_t));

                            if (Xil_Ntohl(dhcp->magic_cookie) == DHCP_MAGIC_COOKIE) {
                                // Assert the DHCP Discover's BROADCAST flag; this signals to any DHCP
                                // severs that their responses should be sent to the broadcast address.
                                // This is necessary for the DHCP response to propagate back through the
                                // wired-wireless portal at the AP, through the STA Rx MAC filters, back
                                // out the wireless-wired portal at the STA, and finally into the DHCP
                                // listener at the wired device
                                dhcp->flags = Xil_Htons(DHCP_BOOTP_FLAGS_BROADCAST);

                            } // END is DHCP valid
                        } // END is DHCP
                    } // END is UDP
                break;

                default:
                    // Unknown/unsupported EtherType; don't process the Eth frame
                    return 0;
                break;
            } //END switch(pkt type)
        break;
    } // END switch(encap mode)

    // If we got this far, the packet was successfully encapsulated; return the post-encapsulation length
    return mpdu_tx_len;
}


/*****************************************************************************/
/**
 * @brief De-encapsulates a wireless reception and prepares it for transmission
 *     via Ethernet
 *
 * This function implements the de-encapsulation process for the wireless-wired
 * portal. See the 802.11 Reference Design user guide for more details:
 *     http://warpproject.org/trac/wiki/802.11/MAC/Upper/MACHighFramework/EthEncap
 *
 * In addition to de-encapsulation this function implements one extra behavior. When
 * the AP observes a DHCP request about to be transmitted via Ethernet, it inspects
 * the DHCP payload to extract the hostname field. When the source packet was a
 * wireless transmission from an associated STA, this hostname field reflects the
 * user-specified name of the device which generated the DHCP request. For most
 * devices this name is human readable (like "jacks-phone"). The AP copies this
 * hostname to the station_info.hostname field in the STA's entry in the association
 * table.
 *
 * This functionality is purely for user convenience (i.e. the AP only displays the
 * hostname, it never makes state decisions based on its value. The hostname can be
 * bogus or missing without affecting the behavior of the AP.
 *
 * @param u8* mpdu
 *  - Pointer to the first byte of the packet received from the wireless interface
 * @param u32 length
 *  - Length (in bytes) of the packet to send
 *
 * @return 0 for successful de-encapsulation, -1 otherwise; failure usually indicates
 *     malformed or unrecognized LLC header
*/
int wlan_mpdu_eth_send(void* mpdu, u16 length, u8 pre_llc_offset) {
    int                      status;
    u8                     * eth_mid_ptr;

    rx_frame_info_t        * rx_frame_info;

    mac_header_80211       * rx80211_hdr;
    llc_header_t           * llc_hdr;

    ethernet_header_t        * eth_hdr;
    ipv4_header_t            * ip_hdr;
    udp_header_t             * udp;

    arp_ipv4_packet_t        * arp;
    dhcp_packet            * dhcp;

    u8                       continue_loop;
    u8                       is_dhcp_req         = 0;

    u8                       addr_cache[6];
    u32                      len_to_send;

    u32                      min_pkt_len = sizeof(mac_header_80211) + sizeof(llc_header_t);

    if(gl_portal_en == 0) return 0;

    if(length < (min_pkt_len + pre_llc_offset + WLAN_PHY_FCS_NBYTES)){
        xil_printf("Error in wlan_mpdu_eth_send: length of %d is too small... must be at least %d\n", length, min_pkt_len);
        return -1;
    }

    // Get helper pointers to various byte offsets in the packet payload
    rx80211_hdr = (mac_header_80211*)((void *)mpdu);
    llc_hdr     = (llc_header_t*)((void *)mpdu + sizeof(mac_header_80211) + pre_llc_offset);
    eth_hdr     = (ethernet_header_t*)((void *)mpdu + sizeof(mac_header_80211) + sizeof(llc_header_t) + pre_llc_offset - sizeof(ethernet_header_t));

    // Calculate length of de-encapsulated Ethernet packet
    len_to_send = length - min_pkt_len - pre_llc_offset - WLAN_PHY_FCS_NBYTES + sizeof(ethernet_header_t);

    // Perform de-encapsulation of wireless packet
    switch(eth_encap_mode) {
    	default:
    		return 0;
    	break;
        // ------------------------------------------------
        case APPLICATION_ROLE_AP:
            // Map the 802.11 header address fields to Ethernet header address fields
            //     For AP de-encapsulation, (eth.dest == wlan.addr3) and (eth.src == wlan.addr2)
            memmove(eth_hdr->dest_mac_addr, rx80211_hdr->address_3, 6);
            memmove(eth_hdr->src_mac_addr,  rx80211_hdr->address_2, 6);

            // Set the ETHER_TYPE field in the Ethernet header
            switch(llc_hdr->type){
                case LLC_TYPE_ARP:
                    eth_hdr->ethertype = ETH_TYPE_ARP;
                break;

                case LLC_TYPE_IP:
                    eth_hdr->ethertype = ETH_TYPE_IP;

                    // Check if this is a DHCP discover packet in a UDP packet
                    //     If so, extract the hostname field from the DHCP discover payload and
                    //         update the corresponding STA association table entry
                    //     This hostname is purely for convenience- the hostname is easier to
                    //          recognize than the STA MAC address. The hostname can be blank
                    //          without affecting any AP functionality.
                    ip_hdr = (ipv4_header_t*)((void*)eth_hdr + sizeof(ethernet_header_t));

                    if (ip_hdr->protocol == IPV4_PROT_UDP) {
                        udp = (udp_header_t*)((void*)ip_hdr + 4*((u8)(ip_hdr->version_ihl) & 0xF));

                        if ((Xil_Ntohs(udp->src_port) == UDP_SRC_PORT_BOOTPC) ||
                            (Xil_Ntohs(udp->src_port) == UDP_SRC_PORT_BOOTPS)) {
                            dhcp = (dhcp_packet*)((void*)udp + sizeof(udp_header_t));

                            if (Xil_Ntohl(dhcp->magic_cookie) == DHCP_MAGIC_COOKIE) {
                                eth_mid_ptr = (u8*)((void*)dhcp + sizeof(dhcp_packet));

                                // Iterate over all tagged parameters in the DHCP request, looking for the hostname parameter
                                //     NOTE:  Stop after 20 tagged parameters (handles case of mal-formed DHCP packets missing END tag)
                                continue_loop = 20;

                                while(continue_loop) {
                                    continue_loop--;
                                    switch(eth_mid_ptr[0]) {

                                        case DHCP_OPTION_TAG_TYPE:
                                            if((eth_mid_ptr[2] == DHCP_OPTION_TYPE_DISCOVER) ||
                                               (eth_mid_ptr[2] == DHCP_OPTION_TYPE_REQUEST)) {
                                                    is_dhcp_req = 1;
                                            }
                                        break;

                                        case DHCP_HOST_NAME:
                                            if (is_dhcp_req) {
                                                // Look backwards from the MPDU payload to find the wireless Rx pkt metadata (the rx_frame_info struct)
                                                rx_frame_info = (rx_frame_info_t*)((u8*)mpdu  - PHY_RX_PKT_BUF_MPDU_OFFSET);

                                                if ((void*)(rx_frame_info->additional_info) != NULL) {
                                                    // rx_frame_info has pointer to STA entry in association table - fill in that entry's hostname field

                                                    // Zero out the hostname field of the station_info
                                                    //     NOTE: This will effectively Null-terminate the string
                                                    bzero(((station_info_t*)(rx_frame_info->additional_info))->hostname, STATION_INFO_HOSTNAME_MAXLEN+1);

                                                    // Copy the string from the DHCP payload into the hostname field
                                                    memcpy(((station_info_t*)(rx_frame_info->additional_info))->hostname,
                                                            &(eth_mid_ptr[2]),
                                                            min(STATION_INFO_HOSTNAME_MAXLEN, eth_mid_ptr[1]));
                                                }
                                            }
                                        break;

                                        case DHCP_OPTION_END:
                                            continue_loop = 0;
                                        break;
                                    } // END switch(DHCP tag type)

                                    // Increment by size of current tagged parameter
                                    eth_mid_ptr += (2+eth_mid_ptr[1]);

                                } // END iterate over DHCP tags
                            }  // END is DHCP_MAGIC_COOKIE correct?
                        } // END is DHCP?
                    } // END is UDP?
                break; // END case(IP packet)

                default:
                    // Invalid or unsupported Eth type; give up and return
                    return -1;
                break;
            } // END switch(LLC type)
        break;

        // ------------------------------------------------
        case APPLICATION_ROLE_STA:
            if (wlan_addr_eq(rx80211_hdr->address_3, get_mac_hw_addr_wlan())) {
                // This case handles the behavior of an AP reflecting a station-sent broadcast
                // packet back out over the air.  Without this filtering, a station would forward
                // the packet it just transmitted back to its wired interface.  This messes up
                // DHCP and ARP behavior on the connected PC.
                return -1;
            }

            // Make temp copy of the 802.11 header address 3 field
            memcpy(addr_cache, rx80211_hdr->address_3, 6);

            // If this packet is addressed to this STA, use the wired device's MAC address as the Eth dest address
            if (wlan_addr_eq(rx80211_hdr->address_1, get_mac_hw_addr_wlan())) {
                memcpy(eth_hdr->dest_mac_addr, eth_sta_mac_addr, 6);
            } else {
                memmove(eth_hdr->dest_mac_addr, rx80211_hdr->address_1, 6);
            }

            // Insert the Eth source, from the 802.11 header address 1 field
            memcpy(eth_hdr->src_mac_addr, addr_cache, 6);

            switch(llc_hdr->type){
                case LLC_TYPE_ARP:
                    eth_hdr->ethertype = ETH_TYPE_ARP;

                    // If the ARP packet is addressed to this STA wireless address, replace the ARP dest address
                    // with the connected wired device's MAC address
                    arp = (arp_ipv4_packet_t *)((void*)eth_hdr + sizeof(ethernet_header_t));
                    if (wlan_addr_eq(arp->target_haddr, get_mac_hw_addr_wlan())) {
                        memcpy(arp->target_haddr, eth_sta_mac_addr, 6);
                    }
                break;

                case ETH_TYPE_IP:
                    eth_hdr->ethertype = ETH_TYPE_IP;
                    ip_hdr = (ipv4_header_t*)((void*)eth_hdr + sizeof(ethernet_header_t));
                break;

                default:
                    // Invalid or unsupported Eth type; give up and return
                    return -1;
                break;
            }
        break;

        // ------------------------------------------------
        case APPLICATION_ROLE_IBSS:
            // Make temp copy of the 802.11 header address 2 field
            memcpy(addr_cache, rx80211_hdr->address_2, 6);

            // If this packet is addressed to this STA, use the wired device's MAC address as the Eth dest address
            if(wlan_addr_eq(rx80211_hdr->address_1, get_mac_hw_addr_wlan())) {
                memcpy(eth_hdr->dest_mac_addr, eth_sta_mac_addr, 6);
            } else {
                memmove(eth_hdr->dest_mac_addr, rx80211_hdr->address_1, 6);
            }

            // Insert the Eth source, from the 802.11 header address 2 field
            memcpy(eth_hdr->src_mac_addr, addr_cache, 6);

            switch(llc_hdr->type){
                case LLC_TYPE_ARP:
                    eth_hdr->ethertype = ETH_TYPE_ARP;

                    // If the ARP packet is addressed to this STA wireless address, replace the ARP dest address
                    // with the connected wired device's MAC address
                    arp = (arp_ipv4_packet_t*)((void*)eth_hdr + sizeof(ethernet_header_t));
                    if (wlan_addr_eq(arp->target_haddr, get_mac_hw_addr_wlan())) {
                        memcpy(arp->target_haddr, eth_sta_mac_addr, 6);
                    }
                break;

                case ETH_TYPE_IP:
                    eth_hdr->ethertype = ETH_TYPE_IP;
                    ip_hdr = (ipv4_header_t*)((void*)eth_hdr + sizeof(ethernet_header_t));
                break;

                default:
                    // Invalid or unsupported Eth type; give up and return
                    return -1;
                break;
            }
        break;
    }

    status = wlan_platform_ethernet_send((u8*)eth_hdr, len_to_send);
    if (status != 0) { xil_printf("Error in wlan_platform_ethernet_send! Err = %d\n", status); return -1; }

    return 0;
}

void wlan_eth_portal_en(u8 enable){
	gl_portal_en = enable;
}

#endif /* WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE */

