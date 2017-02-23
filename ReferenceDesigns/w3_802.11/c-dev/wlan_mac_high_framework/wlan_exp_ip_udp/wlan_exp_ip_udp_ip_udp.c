/** @file  wlan_exp_ip_udp_ip_udp.c
 *  @brief Mango wlan_exp IP/UDP Library (IP/UDP/ARP/IMCP Processing)
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the WARP license  (http://warpproject.org/license)
 */

/***************************** Include Files *********************************/

// Xilinx / Standard library includes
#include <xparameters.h>
#include <xstatus.h>
#include <xil_io.h>
#include <stdlib.h>
#include <stdio.h>

// Mango wlan_exp IP/UDP Library includes
#include "wlan_exp_ip_udp.h"
#include "wlan_exp_ip_udp_internal.h"


/*************************** Constant Definitions ****************************/



/*********************** Global Variable Definitions *************************/



/*************************** Variable Definitions ****************************/

u16           ipv4_id_counter = 0;



/*************************** Function Prototypes *****************************/

void arp_reply(u32 eth_dev_num, wlan_exp_ip_udp_buffer * arp_request);

void imcp_echo_reply(u32 eth_dev_num, wlan_exp_ip_udp_buffer * echo_request);


/******************************** Functions **********************************/


/**********************************************************************************************************************/
/**
 * @brief IP Functions 
 *
 **********************************************************************************************************************/


/*****************************************************************************/
/**
 * Initialize the IPv4 global variables
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void ipv4_init() {

    // Initialize the ID counter
    ipv4_id_counter = 0;

}



/*****************************************************************************/
/**
 * Process the IP packet
 *
 * @param   eth_dev_num - Ethernet device the message came in on
 * @param   packet      - wlan_exp IP/UDP Buffer containing the IP packet
 *
 * @return  int         - Number of bytes of data in the processed packet; 0 if the packet could not be processed
 *
 * @note    This function assumes that both Ethernet device and buffer are valid.
 *
 *****************************************************************************/
int ipv4_process_packet(u32 eth_dev_num, wlan_exp_ip_udp_buffer * packet) {

    ipv4_header       * header         = (ipv4_header *) packet->offset;

    u32                 addr_check     = 0;
    u8                * src_ip_addr    = (u8 *)&(header->src_ip_addr);
    u8                * dest_ip_addr   = (u8 *)&(header->dest_ip_addr);

    u8                * my_ip_addr     = eth_device[eth_dev_num].ip_addr;

    // Get the Ethernet header so we have the source MAC address for the ARP cache
    ethernet_header   * eth_header     = (ethernet_header *) packet->data;

    // Check the address of the IP packet
    //     - If the node has not been initialized (eg the node address is 10.0.0.0), then accept broadcast packets from 10.0.X.255
    //     - If the node has been initialized, then accept unicast packets and broadcast packets on the given subnet
    //
    //
    // !!! TBD !!! - Future addition:  The address check should really be more configurable (ie it should be a callback
    //                                 that can be set by the application that uses the library).
    //
    //
    if (my_ip_addr[3] == 0) {

        // Accept broadcast packets from 10.0.X.255
        if ((my_ip_addr[0] == dest_ip_addr[0]) && (my_ip_addr[1] == dest_ip_addr[1]) && (dest_ip_addr[3] == 255)) {
             addr_check = 1;
        }
    } else {
        // Accept unicast packets and broadcast packets on the given subnet
        if ((my_ip_addr[0] == dest_ip_addr[0]) &&
            (my_ip_addr[1] == dest_ip_addr[1]) &&
            (my_ip_addr[2] == dest_ip_addr[2]) &&
            ((my_ip_addr[3] == dest_ip_addr[3]) || (dest_ip_addr[3] == 255))) {
            addr_check = 1;
        }
    }

    //
    // 
    // !!! TBD !!! - Future consideration:  Additional packet checks - Length & Checksum
    //     u16            packet_length  = packet->length;
    //     u16            ip_length      = Xil_Ntohs(header->total_length);
    //
    //
    
    if (addr_check == 1) {
    
        // The Xilinx Ethernet / DMA hardware does not support fragmented Ethernet packets.  However, the 
        // library will still pass the first fragment of a packet up to the higher level transport for
        // processing so that the host that sent the fragmented packet does not have a transport timeout 
        // (This is important when trying to determine the maximum packet size supported by the transport).  
        // If this behavior needs to change the below code will cause the Mango wlan_exp IP/UDP Library to discard
        // packet fragments.
        // 
        // The 'fragment offset field' is 16 bits (see http://en.wikipedia.org/wiki/IPv4#Packet_structure 
        // for more information).  The 'DF' flag can be legitimately set to '1' so we need to mask that 
        // bit before we decide if we can discard the packet.  This means that the frag_off field can 
        // have valid values of either 0x0000 or 0x4000 (big endian).  However, we have to convert to 
        // little endian so the valid values are 0x0000 and 0x0040 (ie byte swapped).
        // 
        // if ((header->fragment_offset & 0xFFBF) != 0x0000) {
        //     xil_printf("ERROR:  Library does not support fragmented packets.\n");
        //     return 0;
        // }

        // Update ARP table (Maps IP address to MAC address)
        arp_update_cache(eth_dev_num, eth_header->src_mac_addr, src_ip_addr);
        
        // Update the offset within the buffer to after the IP header
        packet->offset += IP_HEADER_LEN_BYTES;
        packet->length -= IP_HEADER_LEN_BYTES;

        // Process the IP packet
        switch (header->protocol) {
            // UDP packets
            case IP_PROTOCOL_UDP:
                return udp_process_packet(eth_dev_num, packet);
            break;
            
            // IMCP packets
            case IP_PROTOCOL_IMCP:
                return imcp_process_packet(eth_dev_num, packet);
            break;
            
            // If a packet has made it here, the it is destined for the node but cannot be processed
            // by the library.  Therefore, we need to print an error message.
            default:
                xil_printf("ERROR:  Unknown IP protocol:  %d\n", header->protocol);
            break;
        }
    }

    return 0;
}



/*****************************************************************************/
/**
 * Initialize the IP Header
 *
 * @param   header           - Pointer to the IP header
 * @param   src_ip_addr      - Source IP address for IP packet (
 *
 * @return  None
 *
 *****************************************************************************/
void ipv4_init_header(ipv4_header * header, u8 * src_ip_addr) {

    u32 ip_addr;

    // Update the following fields because they are static for the header:
    //   - Version / Internet Header Length
    //   - DSCP / ECN
    //   - TTL
    //   - Fragmentation offset
    //   - Source IP address
    //
    header->version_ihl      = (IP_VERSION_4 << 4) +  IP_HEADER_LEN;
    header->dscp_ecn         = (IP_DSCP_CS0 << 2) + IP_ECN_NON_ECT;
    header->fragment_offset  = IP_NO_FRAGMENTATION;
    header->ttl              = IP_DEFAULT_TTL;
    
    // Convert IP address to u32 (big endian)
    ip_addr = (src_ip_addr[3] << 24) + (src_ip_addr[2] << 16) + (src_ip_addr[1] << 8) + src_ip_addr[0];

    header->src_ip_addr      = ip_addr;
}



/*****************************************************************************/
/**
 * Update the IP Header
 *
 * @param   header           - Pointer to the IP header
 * @param   dest_ip_addr     - Destination IP address for IP packet (big-endian)
 * @param   ip_length        - Length of the IP packet (includes IP header) (little-endian)
 * @param   protocol         - Protocol of the IP packet
 *
 * @return  None
 *
 *****************************************************************************/
void ipv4_update_header(ipv4_header * header, u32 dest_ip_addr, u16 ip_length, u8 protocol) {

    // Update the following fields:
    //   - Length
    //   - Identification
    //   - Protocol
    //   - Checksum
    //   - Destination IP address
    //
    // NOTE:  We do not need to update the following fields because they are static for the socket:
    //   - Version / Internet Header Length
    //   - DSCP / ECN
    //   - TTL
    //   - Source IP address
    //
    header->total_length     = Xil_Htons(ip_length);
    header->identification   = Xil_Htons(ipv4_id_counter++);
    header->protocol         = protocol;
    header->header_checksum  = 0;                                    // Set to zero for checksum calculation   
    header->dest_ip_addr     = dest_ip_addr;
   
    // Update the checksum with 1's complement of 1's complement 16-bit sum
    header->header_checksum  = Xil_Htons(ipv4_compute_checksum((u8 *)header, sizeof(ipv4_header)));
}



/*****************************************************************************/
/**
 * Compute IP Checksum
 *
 * The ones' complement of the ones' complement sum of the data's 16-bit words
 *
 * @param   data            - Pointer to the data words
 * @param   size            - Size of the data to use
 *
 * @return  u16             - Checksum value
 *
 * @note    IP Checksum Algorithm:  http://en.wikipedia.org/wiki/IPv4_header_checksum
 *
 *****************************************************************************/
u16 ipv4_compute_checksum(u8 * data, u32 size) {

    u32   i;
    u32   sum  = 0;
    u16   word = 0;
   
    // Sum all 16-bit words in the header (big-endian)
    for (i = 0; i < size; i = i + 2) {
        word = ((data[i] << 8) & 0xFF00) + (data[i+1] & 0x00FF);
        sum  = sum + ((u32) word);
    }

    // 1's complement 16-bit sum, formed by "end around carry" of 32-bit 2's complement sum
    sum = ((sum & 0xFFFF0000) >> 16) + (sum & 0x0000FFFF);

    // Return the 1's complement of 1's complement 16-bit sum
    return (~sum);
}

 


/**********************************************************************************************************************/
/**
 * @brief UDP Functions 
 *
 **********************************************************************************************************************/


/*****************************************************************************/
/**
 * Process the UDP packet
 *
 * @param   eth_dev_num - Ethernet device the message came in on
 * @param   packet      - Mango wlan_exp IP/UDP Buffer containing the UDP packet
 *
 * @return  int         - Number of bytes of data in the UDP packet; 0 if the packet could not be processed
 *
 * @note    This function assumes that both Ethernet device and buffer are valid.
 *
 *****************************************************************************/
int udp_process_packet(u32 eth_dev_num, wlan_exp_ip_udp_buffer * packet) {

    udp_header   * header         = (udp_header *) packet->offset;    
    
    u32            port_check     = 0;
    u16            dest_port      = Xil_Ntohs(header->dest_port);

    // See if there is a socket that corresponds to the UDP packet
    //     - Check all open sockets to see if one matches the port / eth_dev_num
    // 
    if (socket_find_index_by_eth(eth_dev_num, dest_port) != SOCKET_INVALID_SOCKET) {
    
        port_check = 1;
    }
    
    //
    // 
    // !!! TBD !!! - Future consideration:  Additional packet checks - Length & Checksum
    //     u16            packet_length  = packet->length;
    //     u16            udp_length     = Xil_Ntohs(header->length);
    //
    // 

    if (port_check == 1) {

        // Update the offset within the buffer to after the UDP header
        packet->offset += UDP_HEADER_LEN;
        packet->length -= UDP_HEADER_LEN;

        // Return the length of the remaining data bytes
        return packet->length;
    }

    return 0;
}



/*****************************************************************************/
/**
 * Initialize the UDP Header
 *
 * @param   header           - Pointer to the UDP header
 * @param   src_port         - Source port for UDP packet
 *
 * @return  None
 *
 *****************************************************************************/
void udp_init_header(udp_header * header, u16 src_port) {

    // Update the following fields that are static for the socket:
    //   - Source port
    //
    header->src_port  = Xil_Htons(src_port);
}



/*****************************************************************************/
/**
 * Update the UDP Header
 *
 * @param   header           - Pointer to the UDP header
 * @param   dest_port        - Destination port for UDP packet (big endian)
 * @param   udp_length       - Length of the UDP packet (includes UDP header)
 *
 * @return  None
 *
 *****************************************************************************/
void udp_update_header(udp_header * header, u16 dest_port, u16 udp_length) {

    // Update the following fields:
    //   - Destination port
    //   - Length
    //
    // NOTE:  We do not need to update the following fields because they are static for the socket:
    //   - Source port
    //
    header->dest_port = dest_port;
    header->length    = Xil_Htons(udp_length);
   
    // Currently, the Mango wlan_exp IP/UDP Library does not use the UDP checksum capabilities.  This is primarily
    // due to the amount of time required to compute the checksum.  Also, given that communication 
    // between hosts and WARP nodes is, in general, fairly localized, there is not as much of a need for 
    // the data integrity check that the UDP checksum provides.
    //
    header->checksum  = UDP_NO_CHECKSUM;
}



/**********************************************************************************************************************/
/**
 * @brief ARP Functions 
 *
 **********************************************************************************************************************/

 
/*****************************************************************************/
/**
 * Process the ARP packet
 *
 * @param   eth_dev_num - Ethernet device the message came in on
 * @param   packet      - Mango wlan_exp IP/UDP Buffer containing the ARP packet
 *
 * @return  int         - Always returns 0 since we don't want higher level transports
 *                        to process this packet
 *
 * @note    This function assumes that both Ethernet device and buffer are valid.
 *
 *****************************************************************************/
int arp_process_packet(u32 eth_dev_num, wlan_exp_ip_udp_buffer * packet) {

    arp_ipv4_packet   * arp            = (arp_ipv4_packet *) packet->offset;
    u8                * my_ip_addr     = eth_device[eth_dev_num].ip_addr;
    
    // Process the ARP packet
    //     - If the ARP is a request to the node, then update the ARP table and send a reply
    //     - If the ARP is a reply, then update the ARP table
    // 
    // NOTE:  The library does not currently process gratuitous ARPs since there are a limited number
    //        of ARP table entries.  However, this functionality would be easy to add in.
    //
    if ((Xil_Ntohs(arp->htype) == ARP_HTYPE_ETH)    &&                         // Hardware type is "Ethernet"
        (Xil_Ntohs(arp->ptype) == ETHERTYPE_IP_V4)  &&                         // Protocol type is "IP v4"
        (arp->hlen             == ETH_ADDR_LEN)     &&                         // Hardware length is "Ethernet"
        (arp->plen             == IP_ADDR_LEN)      ) {                        // Protocol length is "IP v4"

        //
        // 
        // !!! TBD !!! - Future addition:  To process gratuitous ARPs, check:
        //     - ARP Request and target_paddr == sender_paddr and target_haddr == {0, 0, 0, 0, 0, 0}
        //     - ARP Reply   and target_paddr == sender_paddr and target_haddr == sender_haddr
        //
        //

        // Check the ARP is for the node
        //     NOTE:  For ARP requests, the target haddr is ignored.  
        //
        if ((arp->target_paddr[0]  == my_ip_addr[0])    &&                         // IP address matches node IP address
            (arp->target_paddr[1]  == my_ip_addr[1])    && 
            (arp->target_paddr[2]  == my_ip_addr[2])    && 
            (arp->target_paddr[3]  == my_ip_addr[3])    ) {

            // Update the ARP table regardless of whether this is a request or a reply
            arp_update_cache(eth_dev_num, arp->sender_haddr, arp->sender_paddr);

            // Process the ARP operation
            //     NOTE:  In the case of an ARP reply, we have already updated the ARP cache, so there is nothing
            //            further to be done.  Therefore, we are just using an if statement to check if this is 
            //            an ARP request.  If needed, this can be changed to a case statement to process other ARP
            //            operations.  
            //
            if (Xil_Ntohs(arp->oper) == ARP_REQUEST) {
                // Send an ARP reply
                arp_reply(eth_dev_num, packet);
            }
        }
    }

    return 0;    // Upper layer stacks should not process this packet so return zero bytes
}



/*****************************************************************************/
/**
 * Send an ARP Reply
 *
 * @param   eth_dev_num - Ethernet device the message came in on
 * @param   arp_request - Mango wlan_exp IP/UDP Buffer containing the ARP packet
 *
 * @return  None
 *
 * @note    This function assumes that both Ethernet device and buffer are valid.
 *
 *****************************************************************************/
void arp_reply(u32 eth_dev_num, wlan_exp_ip_udp_buffer * arp_request) {

    u32                      i;
    int                      status;
    
    arp_ipv4_packet        * request        = (arp_ipv4_packet *) arp_request->offset;
    u8                     * eth_ip_addr    = eth_device[eth_dev_num].ip_addr;
    u8                     * eth_hw_addr    = eth_device[eth_dev_num].hw_addr;
    
    u32                      arp_size       = ETH_HEADER_LEN + ARP_IPV4_PACKET_LEN;

    wlan_exp_ip_udp_buffer     * send_buffer    = NULL;
    arp_ipv4_packet            * arp_reply_packet;

    // Allocate a send buffer from the library
    send_buffer = socket_alloc_send_buffer();

    // If the packet was successfully allocated
    if (send_buffer != NULL) {
    
        // Initialize the send buffer
        send_buffer->size    = arp_size;
        send_buffer->length  = arp_size;
        
        // Initialize the Ethernet header
        //     NOTE:  We will not use a UDP socket to send this packet, since this reply occurs at a 
        //            lower level in the protocol stack than a UDP socket.
        //
        eth_init_header((ethernet_header *)(send_buffer->data), eth_hw_addr);
        
        // Update the offset / length of the buffer
        send_buffer->offset += ETH_HEADER_LEN;
        send_buffer->length -= ETH_HEADER_LEN;
        
        // Get the pointer to the ARP reply packet
        arp_reply_packet  = (arp_ipv4_packet *) send_buffer->offset;

        // Populate the ARP reply
        arp_reply_packet->htype     = Xil_Htons(ARP_HTYPE_ETH);
        arp_reply_packet->ptype     = Xil_Htons(ETHERTYPE_IP_V4);
        arp_reply_packet->hlen      = ETH_ADDR_LEN;
        arp_reply_packet->plen      = IP_ADDR_LEN;
        arp_reply_packet->oper      = Xil_Htons(ARP_REPLY);

        for (i = 0; i < ETH_ADDR_LEN; i++) {
        	arp_reply_packet->sender_haddr[i] = eth_hw_addr[i];
        	arp_reply_packet->target_haddr[i] = request->sender_haddr[i];
        }
      
        for (i = 0; i < IP_ADDR_LEN ; i++) {
        	arp_reply_packet->sender_paddr[i] = eth_ip_addr[i];
        	arp_reply_packet->target_paddr[i] = request->sender_paddr[i];
        }

        // Update the Ethernet header
        //     NOTE:  dest_hw_addr must be big-endian; ethertype must be little-endian
        //
        eth_update_header((ethernet_header *)(send_buffer->data), request->sender_haddr, ETHERTYPE_ARP);
        
        // Send the packet not using a socket
        status = eth_send_frame(eth_dev_num, NULL, &send_buffer, 0x1, 0x0);
        
        if (status != ETH_MIN_FRAME_LEN) {
            xil_printf("ERROR:  Issue sending ARP reply.  %d bytes sent.\n", status);
        }

        // Free the send buffer
        socket_free_send_buffer(send_buffer);
        
    } else {
        xil_printf("ERROR:  Could not allocate send buffer for ARP reply.\n");
    }
}



/*****************************************************************************/
/**
 * Send an ARP Request
 *
 * @param   eth_dev_num  - Ethernet device on which to send ARP request
 * @param   target_haddr - Target HW address for ARP
 * @param   target_paddr - Target Protocol (IP) address for ARP
 *
 * @return  None
 *
 * @note    This function assumes that both socket and buffer are valid.
 *
 *****************************************************************************/
void arp_request(u32 eth_dev_num, u8 * target_haddr, u8 * target_paddr) {

    u32                      i;
    
    u8                     * eth_ip_addr    = eth_device[eth_dev_num].ip_addr;
    u8                     * eth_hw_addr    = eth_device[eth_dev_num].hw_addr;
    
    u32                      arp_size       = ETH_HEADER_LEN + ARP_IPV4_PACKET_LEN;

    wlan_exp_ip_udp_buffer     * send_buffer    = NULL;
    arp_ipv4_packet        * arp_reply_packet;
    ethernet_header        * eth_header;
    
    // Allocate a send buffer
    send_buffer = socket_alloc_send_buffer();

    // If the packet was successfully allocated    
    if (send_buffer != NULL) {
    
        // Initialize the send buffer
        send_buffer->size    = arp_size;
        send_buffer->length  = arp_size;
        
        // Construct the Ethernet header
        eth_header = (ethernet_header *)(send_buffer->offset);
        
        for (i = 0; i < ETH_HEADER_LEN; i++) {
            eth_header->dest_mac_addr[i] = 0xFF;
            eth_header->src_mac_addr[i]  = eth_hw_addr[i];
        }
        
        eth_header->ethertype = Xil_Htons(ETHERTYPE_ARP);
        
        // Update the offset / length of the buffer
        send_buffer->offset += ETH_HEADER_LEN;
        send_buffer->length -= ETH_HEADER_LEN;
        
        // Get the pointer to the ARP reply packet
        arp_reply_packet  = (arp_ipv4_packet *) send_buffer->offset;

        // Populate the ARP request
        arp_reply_packet->htype     = Xil_Htons(ARP_HTYPE_ETH);
        arp_reply_packet->ptype     = Xil_Htons(ETHERTYPE_IP_V4);
        arp_reply_packet->hlen      = ETH_ADDR_LEN;
        arp_reply_packet->plen      = IP_ADDR_LEN;
        arp_reply_packet->oper      = Xil_Htons(ARP_REQUEST);

        for (i = 0; i < ETH_ADDR_LEN; i++) {
        	arp_reply_packet->sender_haddr[i] = eth_hw_addr[i];
        	arp_reply_packet->target_haddr[i] = target_haddr[i];
        }
      
        for (i = 0; i < IP_ADDR_LEN ; i++) {
        	arp_reply_packet->sender_paddr[i] = eth_ip_addr[i];
        	arp_reply_packet->target_paddr[i] = target_paddr[i];
        }

        // Send the packet not using a socket
        eth_send_frame(eth_dev_num, NULL, &send_buffer, 0x1, 0x0);

        // Free the send buffer
        socket_free_send_buffer(send_buffer);
        
    } else {
        xil_printf("ERROR:  Could not allocate send buffer for ARP request.\n");
    }
}



void arp_send_announcement(u32 eth_dev_num) {
    
    // See http://en.wikipedia.org/wiki/Address_Resolution_Protocol#ARP_announcements 
    // for information about ARP announcements.  This implements the following ARP announcement:
    //     - ARP Request and target_paddr == sender_paddr and target_haddr == {0, 0, 0, 0, 0, 0}
    // 
    
    // Hardware address should be set to all zeros
    u8   haddr[ETH_ADDR_LEN] = {0, 0, 0, 0, 0, 0};
    
    // Protocol address should be the current IP address
    u8 * paddr                   = eth_device[eth_dev_num].ip_addr;

    arp_request(eth_dev_num, haddr, paddr);
}



/*****************************************************************************/
/**
 * Get the Hardware address associated with the Ethernet device / IP address from 
 * the ARP cache.
 *
 * @param   eth_dev_num  - Ethernet device to match in the cache
 * @param   hw_addr      - Hardware address (to be returned from the cache)
 * @param   ip_addr      - IP address to match in the cache
 *
 * @return  int          - Status of the command:
 *                             XST_SUCCESS - Command completed successfully
 *                             XST_FAILURE - There was an error in the command
 *
 * @note    The reason for the "strange" order of the arguments is to maintain 
 *          consistency when specifying HW address and IP address (ie all functions
 *          required HW address then IP address when both are part of the arguments).
 *          Since both addresses are (u8 *), the compiler cannot tell them apart 
 *          which makes it easy to get them reversed.
 *
 *****************************************************************************/
int arp_get_hw_addr(u32 eth_dev_num, u8 * hw_addr, u8 * ip_addr) {

    int                      i, j;

    // Look through the ARP table
    for (i = 0; i < WLAN_EXP_IP_UDP_NUM_ARP_ENTRIES; i++) {
    
        // If an entry is in use, then check the IP address
        if (ETH_arp_cache[i].state == ARP_TABLE_USED) {
        
            // If the IP address / eth_dev_num matches, then copy the hardware address
            if ((ETH_arp_cache[i].paddr[0] == ip_addr[0]) && 
                (ETH_arp_cache[i].paddr[1] == ip_addr[1]) && 
                (ETH_arp_cache[i].paddr[2] == ip_addr[2]) && 
                (ETH_arp_cache[i].paddr[3] == ip_addr[3]) &&
                (ETH_arp_cache[i].eth_dev_num == eth_dev_num)) {
                
                // Copy the hardware address 
                for (j = 0; j < ETH_ADDR_LEN; j++) {
                    hw_addr[j] = ETH_arp_cache[i].haddr[j];
                }
                
                return XST_SUCCESS;
            }
        }
    }

    return XST_FAILURE;
}



/*****************************************************************************/
/**
 * Update the ARP cache
 *
 * This cache uses Ethernet device and IP address as keys to index hardware addresses.  
 *
 * @param   eth_dev_num  - Ethernet device 
 * @param   hw_addr      - Hardware address 
 * @param   ip_addr      - IP address 
 *
 * @return  int          - Status of the command:
 *                             XST_SUCCESS - Command completed successfully
 *                             XST_FAILURE - There was an error in the command
 *
 * @note    This function assumes that both socket and buffer are valid.
 *
 *****************************************************************************/
int arp_update_cache(u32 eth_dev_num, u8 * hw_addr, u8 * ip_addr) {

    int                      i, j;
    int                      first_unused_entry = -1;
    int                      oldest_entry       = -1;
    int                      entry_age          = -1;
    int                      entry_to_use       = -1;
    
    // Look through the ARP table:
    //     - Check that current IP address to see if entry already exists for the Ethernet device;
    //       - Update the hw address
    //       - Set age to zero
    //     - Update the age of all entries that are being used
    //     - Record the first unused entry
    //     - Record the oldest entry
    //
    for (i = 0; i < WLAN_EXP_IP_UDP_NUM_ARP_ENTRIES; i++) {
        if (ETH_arp_cache[i].state == ARP_TABLE_USED) {

            // If this entry is older than the current oldest, then record it and update the age
            if (entry_age < ETH_arp_cache[i].age) {
                oldest_entry = i;
                entry_age    = ETH_arp_cache[i].age;
            }
        
            // Update the age of the used entry        
            ETH_arp_cache[i].age += 1;

            // If the IP address / eth_dev_num matches, then copy the hardware address
            if ((ETH_arp_cache[i].paddr[0] == ip_addr[0]) &&
                (ETH_arp_cache[i].paddr[1] == ip_addr[1]) &&
                (ETH_arp_cache[i].paddr[2] == ip_addr[2]) &&
                (ETH_arp_cache[i].paddr[3] == ip_addr[3]) &&
                (ETH_arp_cache[i].eth_dev_num == eth_dev_num)) {
        
                // Copy the hardware address 
                for (j = 0; j < ETH_ADDR_LEN; j++) {
                    ETH_arp_cache[i].haddr[j] = hw_addr[j];
                }

                // Set age to zero
                ETH_arp_cache[i].age = 0;
                
                // We are done updating the table
                return XST_SUCCESS;
            }
        
        } else {
            // Record first unused entry
            if (first_unused_entry < 0) {
                first_unused_entry = i;
            }
        }
    }    

    // If we reach, here we need to add the entry to the table
    //     - If there is an unused entry, then we should use that
    //     - If there are no unused entries, then we should use the oldest entry (LRU replacement policy)
    //
    if (first_unused_entry != -1) {
        entry_to_use = first_unused_entry;
        
        // Mark unused entry as used
        ETH_arp_cache[entry_to_use].state = ARP_TABLE_USED;
    } else {
        entry_to_use = oldest_entry;
    }

    // Copy the IP / HW addresses and eth_dev_num to entry
    if (entry_to_use != -1) {
        // Copy IP address
        for (i = 0; i < IP_ADDR_LEN; i++) {
            ETH_arp_cache[entry_to_use].paddr[i] = ip_addr[i];
        }
    
        // Copy HW address    
        for (i = 0; i < ETH_ADDR_LEN; i++) {
            ETH_arp_cache[entry_to_use].haddr[i] = hw_addr[i];
        }
        
        // Copy Ethernet device
        ETH_arp_cache[entry_to_use].eth_dev_num = eth_dev_num;
    
    } else {
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}




/**********************************************************************************************************************/
/**
 * @brief IMCP Functions 
 *
 **********************************************************************************************************************/


/*****************************************************************************/
/**
 * Process the IMCP packet
 *
 * @param   eth_dev_num - Ethernet device the message came in on
 * @param   packet      - Mango wlan_exp IP/UDP Buffer containing the IMCP packet
 *
 * @return  int         - Always returns 0 since we don't want higher level transports
 *                        to process this packet
 *
 * @note    The library only support Echo Request IMCP packets
 * @note    This function assumes that both Ethernet device and buffer are valid.
 *
 *****************************************************************************/
int imcp_process_packet(u32 eth_dev_num, wlan_exp_ip_udp_buffer * packet) {

    imcp_header  * imcp = (imcp_header *) packet->offset;
    
    // Check if this is an IMCP Echo Request to the node
    if ((imcp->type == ICMP_ECHO_REQUEST_TYPE) && (imcp->code == ICMP_ECHO_CODE)  ) {
        // Send an IMCP Echo Reply
        imcp_echo_reply(eth_dev_num, packet);
    }

    return 0;    // Upper layer stacks should not process this packet so return zero bytes
}



/*****************************************************************************/
/**
 * Send an IMCP Echo Reply
 *
 * @param   eth_dev_num      - Ethernet device the message came in on
 * @param   echo_request     - Mango wlan_exp IP/UDP Buffer containing the IMCP Echo Request
 *
 * @return  None
 *
 * @note    This function assumes that both socket and buffer are valid.
 *
 *****************************************************************************/
void imcp_echo_reply(u32 eth_dev_num, wlan_exp_ip_udp_buffer * echo_request) {

    u32                      i;
    
    u8                     * eth_ip_addr         = eth_device[eth_dev_num].ip_addr;
    u8                     * eth_hw_addr         = eth_device[eth_dev_num].hw_addr;
    
    // De-construct the input Echo Request
    //   NOTE:  Function expects that the echo_request->offset is pointing to the IMCP header 
    //          and that the buffer contains the entire received packet
    //
    imcp_echo_header       * recv_imcp_hdr       = (imcp_echo_header *) echo_request->offset;
    u8                     * recv_imcp_data      = (u8 *)(echo_request->offset + IMCP_HEADER_LEN);
    u32                      recv_imcp_data_len  = echo_request->length - IMCP_HEADER_LEN;
    ipv4_header            * recv_ip_hdr         = (ipv4_header *)(echo_request->offset - IP_HEADER_LEN_BYTES);
    ethernet_header        * recv_eth_hdr        = (ethernet_header *)(echo_request->offset - IP_HEADER_LEN_BYTES - ETH_HEADER_LEN);
    
    wlan_exp_ip_udp_buffer     * send_buffer         = NULL;
    imcp_echo_header       * send_imcp_hdr;

    
    // Allocate the send buffer
    send_buffer = socket_alloc_send_buffer();

    // If the packet was successfully allocated    
    if (send_buffer != NULL) {
    
        // Initialize the send buffer
        send_buffer->size    = echo_request->size;          // Size of reply is the same as the request
        send_buffer->length  = echo_request->size;

        // Initialize the Ethernet header
        //     NOTE:  We will not use a UDP socket to send this packet, since this reply occurs at a 
        //            lower level in the protocol stack than a UDP socket.
        //
        eth_init_header((ethernet_header *)(send_buffer->offset), eth_hw_addr);
        
        // Update the offset / length of the buffer
        send_buffer->offset += ETH_HEADER_LEN;
        send_buffer->length -= ETH_HEADER_LEN;

        // Initialize the IP header
        ipv4_init_header((ipv4_header *)(send_buffer->offset), eth_ip_addr);
        
        // Update the offset / length of the buffer
        send_buffer->offset += IP_HEADER_LEN_BYTES;
        send_buffer->length -= IP_HEADER_LEN_BYTES;
        
        // Get the pointer to the IMCP reply header
        send_imcp_hdr = (imcp_echo_header *)(send_buffer->offset);

        // Populate the IMCP reply
        send_imcp_hdr->type       = ICMP_ECHO_REPLY_TYPE;
        send_imcp_hdr->code       = ICMP_ECHO_CODE;
        send_imcp_hdr->checksum   = 0;
        send_imcp_hdr->identifier = recv_imcp_hdr->identifier;
        send_imcp_hdr->seq_num    = recv_imcp_hdr->seq_num;

        // Update the offset / length of the buffer
        send_buffer->offset += IMCP_HEADER_LEN;
        send_buffer->length -= IMCP_HEADER_LEN;
        
        // Copy all the data from the request packet
        for (i = 0; i < recv_imcp_data_len; i++) {
            send_buffer->offset[i] = recv_imcp_data[i];
        }

        // Update the buffer to include the IMCP header because checksum has to be 
        // calculated over IMCP header and payload
        send_buffer->offset -= IMCP_HEADER_LEN;
        send_buffer->length += IMCP_HEADER_LEN;
        
        // Calculate the ICMP checksum
        send_imcp_hdr->checksum   = Xil_Htons(ipv4_compute_checksum(send_buffer->offset, send_buffer->length));
        
        // Update the buffer to include the IP header
        send_buffer->offset -= IP_HEADER_LEN_BYTES;
        send_buffer->length += IP_HEADER_LEN_BYTES;
        
        // Update the IP header
        //     NOTE:  Requires dest_ip_addr to be big-endian; ip_length to be little-endian
        //
        ipv4_update_header((ipv4_header *)send_buffer->offset, recv_ip_hdr->src_ip_addr, send_buffer->length, IP_PROTOCOL_IMCP);
        
        // Update the Ethernet header
        //     NOTE:  dest_hw_addr must be big-endian; ethertype must be little-endian
        //
        eth_update_header((ethernet_header *)send_buffer->data, recv_eth_hdr->src_mac_addr, ETHERTYPE_IP_V4);
        
        // Send the packet not using a socket
        eth_send_frame(eth_dev_num, NULL, &send_buffer, 0x1, 0x0);
        
        // Free the send buffer
        socket_free_send_buffer(send_buffer);

    } else {
        xil_printf("ERROR:  Could not allocate send buffer for IMCP request.\n");
    }
}



