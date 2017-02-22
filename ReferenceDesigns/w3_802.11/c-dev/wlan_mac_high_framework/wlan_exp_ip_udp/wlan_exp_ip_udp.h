/** @file  wlan_exp_ip_udp.h
 *  @brief Mango wlan_exp IP/UDP Library
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the WARP license  (http://warpproject.org/license)
 */

// How to use:
//
//   To use the Mango wlan_exp IP/UDP Library, code should include the following files:
//
// #include "wlan_exp_ip_udp.h"
// #include "wlan_exp_ip_udp_device.h"
//
// These files define the public API for the library.
//
//
// Ethernet resources:
//
//   - http://en.wikipedia.org/wiki/Ethernet_frame
//   - http://en.wikipedia.org/wiki/EtherType
//   - http://en.wikipedia.org/wiki/IPv4
//   - http://en.wikipedia.org/wiki/User_Datagram_Protocol
//   - http://en.wikipedia.org/wiki/Jumbo_frame
//   - http://en.wikipedia.org/wiki/Address_Resolution_Protocol
//   - http://en.wikipedia.org/wiki/Internet_Control_Message_Protocol
//   - http://en.wikipedia.org/wiki/Network_socket
//
// The Mango wlan_exp IP/UDP library supports jumbo and non-jumbo Ethernet frames carrying IP/UDP traffic.  This library
// does not support TCP traffic, but does support ARP and IMCP to allow usage with standard host OSes (Window, OSX, 
// Linux).  It also implements a partial socket API to manage connections to hosts.  
//
// NOTE:  The CRC/FCS checksum for an Ethernet frame will be computed by the Ethernet hardware module. 
// NOTE:  Currently, the Mango wlan_exp IP/UDP Library only supports IPv4
// 
//
// Design Considerations:
// 
//   Due to processing alignment constraints, WARP nodes require most data to be at least word aligned (ie 32 bit 
// aligned).  However, a standard UDP/IP Ethernet Header is 42 bytes (ie. 14 bytes for the Ethernet header; 
// 20 bytes for IP header; 8 bytes for the UDP header) which is not 32 bit aligned.  Therefore, one issue that 
// the library has to deal with is what to do with the two bytes to align the data.  In previous versions of the
// library, the two bytes were considered part of the WARPLab / WARPNet transport header.  This was fine because
// the library would always have contiguous data for Ethernet packets.  In this version of the library, to
// reduce processing overhead for large data transfers, we are using the scatter gather capabilities of the AXI DMA.
// Therefore, it becomes necessary to consider the padding as part of the UDP/IP Ethernet Header since that will
// align the pieces of data correctly.  
// 
//   Previously, when transmitting data, the WARPLab / WARPNet transports would copy the data from the given memory,
// such as DDR, into the Ethernet send buffer, usually via the CDMA, before using the AXI DMA to copy the data 
// to the Ethernet controller to be sent out over the wire.  This double copy of data is not really necessary 
// given the AXI DMA has scatter gather capabilities.  However, the WARPxilnet driver required the double copy 
// because it had to support multiple Ethernet interface peripherals (ie AXI FIFO, TEMAC, etc).  Given the WARP
// IP/UDP Library only supports AXI DMA, there is no longer a need to enforce the double copy.  However, this requires
// that the library align each piece of data for the Ethernet packet to make sure that there are not alignment
// processing issues in the rest of the software framework.
//
//   Therefore, for transmitting data, the library will generally split the Ethernet packet into two or three
// segments:  
//   1) Mango wlan_exp IP/UDP Header (includes Ethernet, IP, and UDP headers along with a 2 byte delimiter) - 44 bytes
//   2) WARPNet Header(s) (includes WARPNet Transport, Command / Response, and other headers) - (12 to 32 bytes)
//   3) Packet Data (could be contiguous or non-contiguous with the WARPNet Header(s))
//
// with each segment starting on a 32-bit aligned value.
//
//   However, when receiving data, we will always have to perform a double copy due to limitations with the Xilinx
// IP.  The AXI DMA requires that all of the buffer space for the Ethernet packet be specified a priori.  This 
// means that unless the library processes the AXI stream directly through a dedicated peripheral, it is
// impossible to decode the packet such that the library could direct the data to its final resting place with
// only a single copy unless some restrictions are introduced to communication between the host and the WARP node
// that are not suitable for a reference design.
// 
//
//
// Naming Conventions:
//   
//   In general, the library tries to be as explicit as possible when naming variables.  There is a tradeoff between
// name length and specificity so the library tries to balance those competing forces.  One area that needs some
// greater clarification is the use of "length" vs "size".  In general, the library will use the term "length" to
// indicate the number of contiguous items (ie how long an array or structure is), while the term "size" refers to
// the number of allocated items (ie the space of an array or structure).  For example, in the wlan_exp_ip_udp_buffer:
//
//   - "max_size" refers to the number of bytes allocated by the library for the buffer (ie if you access
//         data[max_size], you will have overflowed the buffer since C arrays are zero indexed).
//   - "size" refers to the number of bytes populated in the buffer (ie the size of the data in buffer)
//   - "length" refers to the number of remaining bytes from the offset (ie the length of the remaining bytes in the
//         buffer).  This value should be adjusted as a buffer is processed and the offset changes
// 
// Hopefully, this does not cause too much confusion.
//
//
//
// Structure:
// 
//   In general, the library tries to follow standard socket programming conventions.  The WARP node acts like a
// socket server listening on the multiple sockets.  For example, in WARPLab, the node will listen on a unicast socket 
// for direct node messages and a broadcast socket for triggers and other broadcast messages (ie the server looks for 
// messages on two different ports).  The Mango wlan_exp IP/UDP Library supports two use cases:
//
//     1) The node is acting as a server receiving and responding to commands from a client (this includes responses 
//        that may contain multiple Ethernet frames)
//
//     2) The node is asynchronously sending data to a destination address. 
// 
// The second use case is considered a slight extension to the first use case in that the data send asynchronously 
// are not commands that require responses (ie the library does not support a socket client model, where it sends
// commands and expects responses from a server).  This simplified use model and some hardware limitations results
// in some situations where the library has to deviate from standard socket conventions.
//
//   In a standard OS environment, there is enough memory and buffering, that the OS is able to keep packets around 
// long enough to support the polling of multiple sockets in series (see socket recv / recvfrom which only checks a 
// single port to see if it has data).  However, to support WARP based reference designs, the Mango wlan_exp IP/UDP Library must
// limit its memory / compute footprint so that as many resources as possible are available to the reference design.  Also,
// given that most WARP reference designs look for messages on multiple ports, the library needs to shift the focus
// of how messages are received.  Therefore, the library receive processing is built around a given Ethernet device
// (ie, the physical Ethernet peripheral, for example Eth A or Eth B on the WARP v3 hardware) vs a given socket.  When 
// the library executes a socket_recvfrom_eth() call, this will first check that there is a Ethernet frame on the given
// Ethernet device, and then as part of the packet processing determine what socket the packet is associated with.  This
// allows the library to more efficiently process packets destined to multiple sockets.
// 
//   One of the consequences of this Ethernet device centric processing is that the concept of binding sockets is a 
// bit different.  In a standard OS environment, sockets are able to simplify programming in multihomed hosts (ie, hosts 
// that have more than one network interface and address) through the use of the constant INADDR_ANY for the IP address.  
// When receiving, a socket bound to this address receives packets from all interfaces.  When sending, a socket bound with 
// INADDR_ANY binds to the default IP address, which is that of the lowest-numbered interface.  However, to reduce the 
// potential for confusion, the Mango wlan_exp IP/UDP Library requires explicit binding of sockets to an Ethernet device.  This means
// that the INADDR_ANY functionality is not supported and the application would need to create an individual socket for
// each Ethernet device.  
// 
//   However, on the send path, we can follow standard socket programming.  Since the socket is bound to an Ethernet 
// device, when sending data on that socket, it will be sent to the associated Ethernet device.  This is why there is
// not a socket_sendto_eth() function, only socket_sendto().  This allows the library to reduce the number of arguments
// that must be passed around since only the socket index is required for the library to know how to send the packet.
// 
// 
// 
// Extensions:
// 
//   Given the current structure of the Mango wlan_exp IP/UDP Library, it would be straightforward to abstract away the Ethernet
// device centric nature of the receive processing chain from the WARP applications.  In the current polling framework, 
// this would add processing overhead since it would require checking both Ethernet devices on any given poll.  If 
// Ethernet processing was moved from a polling framework to an interrupt based framework, this would be a logical 
// extension to implement since it would no longer require the additional overhead.  
//
//   Additionally, it would be straightforward to move the framework from a polling based to an interrupt based 
// framework.  The easiest way to implement this would be to create a global queue of packets to be processed 
// that would be fed by the ISRs.  For example, you could have an ISR similar to this to feed packet into a global
// processing queue:
//  
// void transport_isr_eth_A() {
//     int                     recv_bytes     = 0;
//     int                     socket_index;
//     wlan_exp_ip_udp_buffer    * recv_buffer    = transport_alloc_transport_buffer();
//     sockaddr              * from           = transport_alloc_sockaddr();
//     
//     // Check the socket to see if there is data    
//     recv_bytes = socket_recvfrom_eth(ETH_A_MAC, &socket_index, from, recv_buffer);
// 
//     // If we have received data, then we need to process it    
//     if (recv_bytes > 0) {
//         // Add the packet to the global queue to be processed
//         transport_add_packet(socket_index, from, recv_buffer);
//     }
// }
// 
// Then in the main processing loop, you can poll a function like this to process a packet:
//
// void transport_process_packets() {
//     int                     socket_index;
//     wlan_exp_transport_packet * recv_packet;
//
//     // Check that there is a packet in the global queue
//     if (transport_has_packet()) {
//         
//         // Allocate a send buffer from the transport driver
//         send_buffer = socket_alloc_send_buffer();
//
//         // Get data from the global packet queue
//         recv_packet = tranport_get_packet();
//
//         // Process the received packet
//         transport_receive(recv_packet->socket_index, recv_packet->from, recv_packet->buffer, send_buffer);
//         
//         // Need to communicate to the transport driver that the buffers can now be reused
//         socket_free_rcvd_buffer(socket_index, recv_buffer);
//         socket_free_send_buffer(send_buffer);
//         transport_free_packet(recv_packet);
//     }
// }
//
// where a wlan_exp_transport_packet would be defined as:
//
// typedef struct {
//     int                   socket_index;
//     struct sockaddr     * from;
//     wlan_exp_ip_udp_buffer  * buffer;
// } wlan_exp_transport_packet;
//
//
// The thing to remember is that you should not perform the processing of the packet by the WARP application 
// within the ISR.  One other challenge is to make sure that there is enough buffering within the library
// and global data structures so that no packets are lost.  Currently, the library uses static memory allocation
// based on the BSP configuration, but using a larger memory space, like DDR, and moving to a dynamic allocation
// scheme could help with this.
//
//   Additionally, it would not be difficult to add the INADDR_ANY functionality as part of these extensions
// to the library.  This would require modification to the udp_process_packet() function to apply the port
// check to sockets both for the current Ethernet device as well as sockets for INADDR_ANY.  However, it would 
// require the application to understand which Ethernet device packets were going to be sent on since that would 
// move from an explicitly defined value to an implicitly defined value in the case where a socket had an IP 
// address of INADDR_ANY.
//
//
//
//


/***************************** Include Files *********************************/

// Xilinx / Standard library includes
#include <xil_types.h>


/*************************** Constant Definitions ****************************/
#ifndef WLAN_EXP_IP_UDP_H_
#define WLAN_EXP_IP_UDP_H_


// **********************************************************************
// Mango wlan_exp IP/UDP Library Version Information
//

// Version info (vMAJOR_MINOR_REV)
//     MAJOR and MINOR are both u8, while REV is char
//
#define WLAN_EXP_IP_UDP_VER_MAJOR                              1
#define WLAN_EXP_IP_UDP_VER_MINOR                              0
#define WLAN_EXP_IP_UDP_VER_REV                                a



// **********************************************************************
// Mango wlan_exp IP/UDP Library Common Defines
//

#define WLAN_EXP_IP_UDP_DELIM                                  0xFFFF              // Value of transport delimiter
#define WLAN_EXP_IP_UDP_DELIM_LEN                              2                   // Length of transport delimiter (ie padding) (in bytes)

#define WLAN_EXP_IP_UDP_SUCCESS                                0                   // Return value for library success
#define WLAN_EXP_IP_UDP_FAILURE                                -1                  // Return value for library failure

// Ethernet device defines
#define WLAN_EXP_IP_UDP_INVALID_ETH_DEVICE                     0xFFFF              // Invalid Ethernet device number
#define WLAN_EXP_IP_UDP_ALL_ETH_DEVICES                        0xFFFFFFFF          // All Ethernet devices

#define WLAN_EXP_IP_UDP_ETH_BUFFERS_LINKER_SECTION              ".ip_udp_eth_buffers"


// **********************************************************************
// Mango wlan_exp IP/UDP Library Ethernet Defines
//

#define ETH_ADDR_LEN                                       6                   // Length of Ethernet MAC address (in bytes)
#define ETH_MAC_ADDR_LEN                                   ETH_ADDR_LEN        // Length of Ethernet MAC address (in bytes) (Legacy.  Remove on next update)
#define ETH_HEADER_LEN                                     14                  // Length of Ethernet Header (in bytes)

#define ETH_MIN_FRAME_LEN                                  60                  // Length of minimum Ethernet frame (in bytes)
#define ETH_MAX_FRAME_LEN                                  9014                // Length of maximum Ethernet frame (in bytes)
                                                                               //   - Support for jumbo frames

#define ETHERTYPE_IP_V4                                    0x0800              // EtherType:  IPv4 packet
#define ETHERTYPE_ARP                                      0x0806              // EtherType:  ARP packet


// **********************************************************************
// Mango wlan_exp IP/UDP Library IP Defines
//

#define IP_VERSION_4                                       4                   // IP version 4
#define IP_ADDR_LEN                                        4                   // Length of IP address (in bytes)

// NOTE:  For all transmitted IP packets, IHL == 5 (ie the library always uses the minimum IP header length)
#define IP_HEADER_LEN                                      5                   // Length of IP header (in 32-bit words)
#define IP_HEADER_LEN_BYTES                                20                  // Length of IP header (in bytes) (ie 5 words * 4 bytes / word)

// The Mango wlan_exp IP/UDP Library is best effort
//     See http://en.wikipedia.org/wiki/Differentiated_services
//
#define IP_DSCP_CS0                                        0                   // IP Precedence:  Best Effort 

// The Mango wlan_exp IP/UDP Library is not ECN capable
//     See http://en.wikipedia.org/wiki/Explicit_Congestion_Notification
//
#define IP_ECN_NON_ECT                                     0                   // Non ECN-Capable Transport

// Fragmentation
//
#define IP_NO_FRAGMENTATION                                0                   // No fragmentation
#define IP_DF_FRAGMENT                                     0x4000              // "Don't Fragment" bit

// Default TTL
//     See http://en.wikipedia.org/wiki/Time_to_live
#define IP_DEFAULT_TTL                                     0x40                // Default TTL is 64 per recommendation

// Supported IP protocols
//     See http://en.wikipedia.org/wiki/List_of_IP_protocol_numbers
//
#define IP_PROTOCOL_IMCP                                   0x01                // Internet Control Message Protocol (IMCP)
#define IP_PROTOCOL_UDP                                    0x11                // User Datagram Protocol (UDP)


// **********************************************************************
// Mango wlan_exp IP/UDP Library UDP Defines
//

#define UDP_HEADER_LEN                                     8                   // Length of UDP header (in bytes)

#define UDP_NO_CHECKSUM                                    0x0000              // Value if no checksum is generated by the transmitter


// **********************************************************************
// Mango wlan_exp IP/UDP Library ARP Defines
//

#define ARP_IPV4_PACKET_LEN                                28                  // Length of IPv4 ARP packet (in bytes)

// ARP Hardware Types
#define ARP_HTYPE_ETH                                      0x0001              // Hardware Type:  Ethernet (big endian)

// ARP Operation
#define ARP_REQUEST                                        0x0001              // ARP Request
#define ARP_REPLY                                          0x0002              // ARP Reply


// **********************************************************************
// Mango wlan_exp IP/UDP Library IMCP Defines
//

#define IMCP_HEADER_LEN                                    8                   // Length of IMCP header (in bytes) 

#define ICMP_ECHO_REQUEST_TYPE                             0x0008              // Echo Request (Ping)
#define ICMP_ECHO_REPLY_TYPE                               0x0000              // Echo Reply (Ping)
#define ICMP_ECHO_CODE                                     0x0000              // Echo Request / Reply code


// **********************************************************************
// Mango wlan_exp IP/UDP Library Socket Defines
//

// Socket Types
#define SOCK_STREAM                                        1                   // Socket stream (connection) (tcp)
#define SOCK_DGRAM                                         2                   // Socket datagram (connectionless) (udp)

// Address Families
#define AF_UNIX                                            1                   // Local to host (pipes, portals)
#define AF_INET                                            2                   // Inter-network: UDP, TCP, etc.

// Mango wlan_exp IP/UDP Library socket defines
#define SOCKET_INVALID_SOCKET                              -1                  // Socket is invalid


// **********************************************************************
// Mango wlan_exp IP/UDP Library Header Defines
//
#define WLAN_EXP_IP_UDP_HEADER_LEN                            (ETH_HEADER_LEN + IP_HEADER_LEN_BYTES + UDP_HEADER_LEN + WLAN_EXP_IP_UDP_DELIM_LEN)



/***************************** Macro Definitions *****************************/

// **********************************************************************
// Mango wlan_exp IP/UDP Library Ethernet Macros
//

// Convert Ethernet device number to WARP convention (ie ETH A or ETH B)
#define wlan_exp_conv_eth_dev_num(x)                           (char)(((int)'A') + x)



/*********************** Global Structure Definitions ************************/

// **********************************************************************
// Mango wlan_exp IP/UDP Library Common Structures
//

// Mango wlan_exp IP/UDP buffer
//     Describes a buffer of data with maximum length of 2^32 bytes
//
typedef struct {
    u32                      state;                                            // State of the buffer
    u32                      max_size;                                         // Maximum size of the buffer (in bytes) (ie number of bytes allocated; immutable)
    u32                      size;                                             // Size of the buffer data (in bytes) (ie total number of data bytes populated in the buffer)
    u8                     * data;                                             // Pointer to the buffer data
    u8                     * offset;                                           // Pointer to offset within the buffer data
    u32                      length;                                           // Length of remaining data in the buffer from the offset (ie length = (data + size) - offset)
    void                   * descriptor;                                       // A pointer to a buffer descriptor (optional)
} wlan_exp_ip_udp_buffer;


// **********************************************************************
// Mango wlan_exp IP/UDP Library Ethernet Structures
//

typedef struct {
    u8                       dest_mac_addr[ETH_ADDR_LEN];                      // Destination MAC address
    u8                       src_mac_addr[ETH_ADDR_LEN];                       // Source MAC address
    u16                      ethertype;                                        // EtherType
} ethernet_header;


// **********************************************************************
// Mango wlan_exp IP/UDP Library IP Structures
//

typedef struct {
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
} ipv4_header;


// **********************************************************************
// Mango wlan_exp IP/UDP Library UDP Structures
//

// udp header structure
typedef struct {
    u16                      src_port;                                         // Source port number
    u16                      dest_port;                                        // Destination port number
    u16                      length;                                           // Length of UDP header and UDP data (in bytes)
    u16                      checksum;                                         // Checksum
} udp_header;


// **********************************************************************
// Mango wlan_exp IP/UDP Library ARP Structures
//     - NOTE:  The Mango wlan_exp IP/UDP Library only support IPv4 ARP
//

typedef struct {
    u16                      htype;                                            // Hardware Type
    u16                      ptype;                                            // Protocol Type
    u8                       hlen;                                             // Length of Hardware address
    u8                       plen;                                             // Length of Protocol address
    u16                      oper;                                             // Operation
    u8                       sender_haddr[ETH_ADDR_LEN];                       // Sender hardware address
    u8                       sender_paddr[IP_ADDR_LEN];                        // Sender protocol address
    u8                       target_haddr[ETH_ADDR_LEN];                       // Target hardware address
    u8                       target_paddr[IP_ADDR_LEN];                        // Target protocol address
} arp_ipv4_packet;


// **********************************************************************
// Mango wlan_exp IP/UDP Library IMCP Structures
//     NOTE:  The Mango wlan_exp IP/UDP Library only support Echo Reply
//            http://en.wikipedia.org/wiki/Ping_(networking_utility) 
//

typedef struct {
    u8                       type;                                             // IMCP Type
    u8                       code;                                             // IMCP subtype
    u16                      checksum;                                         // Header checksum (only IMCP part of packet)
    u32                      rest;                                             // Rest of Header (4 bytes that vary based on IMCP type and code)
} imcp_header;


typedef struct {
    u8                       type;                                             // IMCP Type
    u8                       code;                                             // IMCP subtype
    u16                      checksum;                                         // Header checksum (only IMCP part of packet)
    u16                      identifier;                                       // Ping identifier
    u16                      seq_num;                                          // Ping sequence number
} imcp_echo_header;


// **********************************************************************
// Mango wlan_exp IP/UDP Library Combined Data Structures
//

// Mango wlan_exp IP/UDP Library header
//     Describes the header of a standard UDP/IP Ethernet packet, aligned to 32 bits
//
typedef struct __attribute__((__packed__)) {
    ethernet_header          eth_hdr;
    ipv4_header              ip_hdr;
    udp_header               udp_hdr;
    u16                      delimiter;
} wlan_exp_ip_udp_header;



// **********************************************************************
// Mango wlan_exp IP/UDP Library Socket Structures
//

// NOTE:  To ease processing, each UDP socket should keep track of the Mango wlan_exp IP/UDP Library header
//        used for the socket.  This header will be transmitted as part of each packet to the
//        socket (if indicated).  
//
// NOTE:  To ease processing, instead of maintaining a struct sockaddr_in within the socket, 
//        the library splits out the necessary structure fields.  This may need to be modified
//        in the future if the library needs to support more than UDP sockets.
//
// NOTE:  Given the library only supports AF_INET SOCK_DGRAM sockets, the socket does not need to
//        keep track of the socket domain or family.  However, the socket has the sin_family field 
//        to maintain 32-bit data alignment within the structure.
// 
typedef struct {
    u32                      index;                                            // Index of the socket
    u32                      state;                                            // State of the socket
    u32                      eth_dev_num;                                      // Ethernet device associated with the sockets
    
    // Necessary fields of struct sockaddr_in
    u16                      sin_family;                                       // Family of the socket (only used for data alignment)
    u16                      sin_port;                                         // Port of the socket
    u32                      sin_addr;                                         // IP address of the socket
    
    wlan_exp_ip_udp_header     * hdr;                                              // Mango wlan_exp IP/UDP Library header associated with the socket
} wlan_exp_ip_udp_socket;


// **********************************************************************
// Standard Socket Structures
//     NOTE:  These structures use standard socket naming conventions for compatibility.
//

// Internet (IP) address structure
struct in_addr {
   u32                       s_addr;
};

// Socket address structure
struct sockaddr {
    u16                      sa_family;
    u8                       sa_data[14];
};

// Internet (IP) socket address structure
struct sockaddr_in {
    u16                      sin_family;
    u16                      sin_port;
    struct in_addr           sin_addr;
    u8                       sin_zero[8];                                      // Padding to fill out to 16 bytes
};



/*************************** Function Prototypes *****************************/

// Mango wlan_exp IP/UDP Library functions
int                     wlan_exp_ip_udp_init();

// Ethernet Device functions
int                     eth_init(u32 eth_dev_num, u8 * hw_addr, u8 * ip_addr, u32 verbose);
int                     eth_start_device(u32 eth_dev_num);

void                    eth_set_interrupt_enable_callback(void(*callback)());
void                    eth_set_interrupt_disable_callback(void(*callback)());

int                     eth_set_ip_addr(u32 eth_dev_num, u8 * ip_addr);
int                     eth_get_ip_addr(u32 eth_dev_num, u8 * ip_addr);

int                     eth_set_hw_addr(u32 eth_dev_num, u8 * hw_addr);
int                     eth_get_hw_addr(u32 eth_dev_num, u8 * hw_addr);

int                     eth_set_mac_operating_speed(u32 eth_dev_num, u32 speed);
int                     eth_read_phy_reg(u32 eth_dev_num, u32 phy_addr, u32 reg_addr, u16 * reg_value);
int                     eth_write_phy_reg(u32 eth_dev_num, u32 phy_addr, u32 reg_addr, u16 reg_value);

int                     eth_get_num_tx_descriptors();

// IP functions
void                    ipv4_update_header(ipv4_header * header, u32 dest_ip_addr, u16 ip_length, u8 protocol);

// ARP functions
void                    arp_send_request(u32 eth_dev_num, u8 * target_haddr, u8 * target_paddr);
void                    arp_send_announcement(u32 eth_dev_num);

int                     arp_update_cache(u32 eth_dev_num, u8 * hw_addr, u8 * ip_addr);
int                     arp_get_hw_addr(u32 eth_dev_num, u8 * hw_addr, u8 * ip_addr);

// Socket functions
int                     socket_socket(int domain, int type, int protocol);
int                     socket_bind_eth(int socket_index, u32 eth_dev_num, u16 port);
int                     socket_sendto(int socket_index, struct sockaddr * to, wlan_exp_ip_udp_buffer ** buffers, u32 num_buffers);
int                     socket_sendto_raw(int socket_index, wlan_exp_ip_udp_buffer ** buffers, u32 num_buffers);
int                     socket_recvfrom_eth(u32 eth_dev_num, int * socket_index, struct sockaddr * from, wlan_exp_ip_udp_buffer * buffer);
void                    socket_close(int socket_index);

u32                     socket_get_eth_dev_num(int socket_index);
wlan_exp_ip_udp_header    * socket_get_wlan_exp_ip_udp_header(int socket_index);

wlan_exp_ip_udp_buffer    * socket_alloc_send_buffer();
void                    socket_free_send_buffer(wlan_exp_ip_udp_buffer * buffer);

void                    socket_free_recv_buffer(int socket_index, wlan_exp_ip_udp_buffer * buffer);

#endif // WLAN_EXP_IP_UDP_H_
