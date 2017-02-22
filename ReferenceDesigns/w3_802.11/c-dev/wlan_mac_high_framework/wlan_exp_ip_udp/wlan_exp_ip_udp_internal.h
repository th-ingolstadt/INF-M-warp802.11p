/** @file  wlan_exp_ip_udp_internal.h
 *  @brief Mango wlan_exp IP/UDP Library (Internal Structures / Functions)
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the WARP license  (http://warpproject.org/license)
 */


/***************************** Include Files *********************************/

// Xilinx / Standard library includes
#include <xil_types.h>

// Mango wlan_exp IP/UDP Library includes
#include "wlan_exp_ip_udp_config.h"
#include "wlan_exp_ip_udp_device.h"


/*************************** Constant Definitions ****************************/
#ifndef WLAN_EXP_IP_UDP_INTERNAL_H_
#define WLAN_EXP_IP_UDP_INTERNAL_H_


// **********************************************************************
// Mango wlan_exp IP/UDP Library Common Defines
//

// Mango wlan_exp IP/UDP Library Buffer State defines
#define WLAN_EXP_IP_UDP_BUFFER_FREE                            0                   // Buffer is free to be used
#define WLAN_EXP_IP_UDP_BUFFER_IN_USE                          1                   // Buffer is currently in use


// **********************************************************************
// Mango wlan_exp IP/UDP Library ARP Defines
//

// Mango wlan_exp IP/UDP Library ARP Table States
#define ARP_TABLE_UNUSED                                   0                   // ARP Table Entry is not in use
#define ARP_TABLE_USED                                     1                   // ARP Table Entry is in use


// **********************************************************************
// Mango wlan_exp IP/UDP Library Socket Defines
//

// Mango wlan_exp IP/UDP Library Socket States
#define SOCKET_CLOSED                                      0                   // Socket cannot be used
#define SOCKET_ALLOCATED                                   1                   // Socket has been allocated but not bound
#define SOCKET_OPEN                                        2                   // Socket is bound and can be used


/***************************** Macro Definitions *****************************/


/*********************** Global Structure Definitions ************************/

// **********************************************************************
// Mango wlan_exp IP/UDP Library Ethernet Structures
//

// NOTE:  The reason that we use void pointers vs pointers to actual defined types is so that 
//     downstream software doesn't have to have any hardware specific include files if they do 
//     not need to deal with these pointers.
//
typedef struct  {
    u32                      initialized;                                      // Is the Ethernet device initialized

    // Ethernet variables
    u32                      eth_id;                                           // XPAR ID for Ethernet device
    void                   * eth_ptr;                                          // Pointer to Ethernet instance
    void                   * eth_cfg_ptr;                                      // Pointer to Ethernet config instance
    
    // Ethernet DMA variables
    u32                      dma_id;                                           // XPAR ID for Ethernet DMA
    void                   * dma_ptr;                                          // Pointer to Ethernet DMA instance
    void                   * dma_cfg_ptr;                                      // Pointer to Ethernet DMA config instance
    
    void                   * dma_rx_ring_ptr;                                  // Pointer to RX ring
    void                   * dma_rx_bd_ptr;                                    // Pointer to RX buffer descriptor
    int                      dma_rx_bd_cnt;                                    // Number of RX buffer descriptors
    
    void                   * dma_tx_ring_ptr;                                  // Pointer to TX ring
    void                   * dma_tx_bd_ptr;                                    // Pointer to TX buffer descriptor
    int                      dma_tx_bd_cnt;                                    // Number of TX buffer descriptors

    // Ethernet device information
    u8                       hw_addr[ETH_ADDR_LEN];                            // Ethernet device MAC address
    u16                      padding;                                          // Padding to align hw_addr
    u8                       ip_addr[IP_ADDR_LEN];                             // Ethernet device IP address

    // Buffers for receiving data
    //   NOTE:  Buffers are allocated based on the configuration in the BSP.  For DMA interfaces, it
    //          is recommended to have at least 2 receive buffers so that the AXI DMA can use a
    //          ping pong buffer scheme.
    //   NOTE:  Since buffers for sending data are not specific to an Ethernet device, there are a pool
    //          that can be allocated in the library.
    //
    u32                      num_recv_buffers;
    wlan_exp_ip_udp_buffer     * recv_buffers;
} ethernet_device;


// **********************************************************************
// Mango wlan_exp IP/UDP Library ARP Structures
//     - NOTE:  The Mango wlan_exp IP/UDP Library only support IPv4 ARP
//

// ARP Table entry
typedef struct {
    u32                      eth_dev_num;                                      // Ethernet device
    u32                      age;                                              // Age of the entry
    u16                      state;                                            // State of the entry
    u8                       haddr[ETH_ADDR_LEN];                              // Hardware address
    u8                       paddr[IP_ADDR_LEN];                               // Protocol address
} arp_cache_entry;



// **********************************************************************
// Ethernet Function pointer
//
typedef int (*eth_int_disable_func_ptr_t)();
typedef int (*eth_int_enable_func_ptr_t)(int);



/*********************** Global Variable Definitions *************************/

extern ethernet_device       eth_device[WLAN_EXP_IP_UDP_NUM_ETH_DEVICES];
extern u32                   ETH_allocated_send_buffers;
extern wlan_exp_ip_udp_buffer    ETH_send_buffers[WLAN_EXP_IP_UDP_ETH_NUM_SEND_BUF];
extern u8                    ETH_dummy_frame[ETH_MIN_FRAME_LEN];
extern wlan_exp_ip_udp_socket    ETH_sockets[WLAN_EXP_IP_UDP_NUM_SOCKETS];
extern arp_cache_entry       ETH_arp_cache[WLAN_EXP_IP_UDP_NUM_ARP_ENTRIES];


/*************************** Function Prototypes *****************************/

// Ethernet Device functions
void                    eth_init_header(ethernet_header * header, u8 * src_hw_addr);
void                    eth_update_header(ethernet_header * header, u8 * dest_hw_addr, u16 ethertype);

int                     eth_free_recv_buffers(u32 eth_dev_num, void * descriptors, u32 num_descriptors);

int                     eth_recv_frame(u32 eth_dev_num, wlan_exp_ip_udp_buffer * eth_frame);
int                     eth_send_frame(u32 eth_dev_num, wlan_exp_ip_udp_socket * socket, wlan_exp_ip_udp_buffer ** buffers, u32 num_buffers, u32 use_socket_header);

// IP functions
void                    ipv4_init();

int                     ipv4_process_packet(u32 eth_dev_num, wlan_exp_ip_udp_buffer * buffer);

void                    ipv4_init_header(ipv4_header * header, u8 * src_ip_addr);
// void                 ipv4_update_header(ipv4_header * header, u32 dest_ip_addr, u16 ip_length, u8 protocol);   // Defined in wlan_exp_ip_udp.h

u16                     ipv4_compute_checksum(u8 * data, u32 size);

// UDP functions
int                     udp_process_packet(u32 eth_dev_num, wlan_exp_ip_udp_buffer * packet);

void                    udp_init_header(udp_header * header, u16 src_port);
void                    udp_update_header(udp_header * header, u16 dest_port, u16 udp_length);

// ARP functions
int                     arp_process_packet(u32 eth_dev_num, wlan_exp_ip_udp_buffer * packet);

void                    arp_send_reply(u32 eth_dev_num, wlan_exp_ip_udp_buffer * arp_request);

// IMCP functions
int                     imcp_process_packet(u32 eth_dev_num, wlan_exp_ip_udp_buffer * packet);

void                    imcp_send_echo_reply(u32 eth_dev_num, wlan_exp_ip_udp_buffer * echo_request);

// Socket functions
int                     socket_find_index_by_eth(u32 eth_dev_num, u16 port);


#endif // WLAN_EXP_IP_UDP_INTERNAL_H_
