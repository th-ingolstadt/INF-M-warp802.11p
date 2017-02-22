/** @file  wlan_exp_ip_udp_init.c
 *  @brief Mango wlan_exp IP/UDP Library (Initialization)
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the WARP license  (http://warpproject.org/license)
 */

// NOTE:  Many data structures in the Mango wlan_exp IP/UDP Library must be accessible to DMAs
// and other system level masters.  Therefore, we have put those variables in their
// own linker section, ".ip_udp_eth_buffers", so that it is easy to put that section into an
// appropriate memory within the system.
//
//   This will require custom modification of the linker script since the Xilinx SDK
// can not detect these section headers ahead of time so that they can be placed as 
// part of the GUI section placement.
//

/***************************** Include Files *********************************/

// Xilinx / Standard library includes
#include <xparameters.h>
#include <xstatus.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Mango wlan_exp IP/UDP Library includes
#include "wlan_exp_ip_udp.h"
#include "wlan_exp_ip_udp_internal.h"


/*************************** Constant Definitions ****************************/


/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/

// Send buffers for all Ethernet devices
u32                          ETH_allocated_send_buffers;
u32                          ETH_send_buffer[WLAN_EXP_IP_UDP_ETH_NUM_SEND_BUF * (WLAN_EXP_IP_UDP_ETH_BUF_SIZE >> 2)] __attribute__ ((aligned(WLAN_EXP_IP_UDP_BUFFER_ALIGNMENT))) __attribute__ ((section (".ip_udp_eth_buffers")));
wlan_exp_ip_udp_buffer           ETH_send_buffers[WLAN_EXP_IP_UDP_ETH_NUM_SEND_BUF];

// Memory for minimum length dummy Ethernet Frame
//     NOTE:  Memory for the frame must be accessible by the DMA
//
u8                           ETH_dummy_frame[ETH_MIN_FRAME_LEN] __attribute__ ((aligned(WLAN_EXP_IP_UDP_BUFFER_ALIGNMENT))) __attribute__ ((section (".ip_udp_eth_buffers")));

// Socket data structures
//     NOTE:  Memory for the IP/UDP headers must be accessible by the DMA
//
wlan_exp_ip_udp_header           ETH_headers[WLAN_EXP_IP_UDP_NUM_SOCKETS] __attribute__ ((aligned(WLAN_EXP_IP_UDP_BUFFER_ALIGNMENT))) __attribute__ ((section (".ip_udp_eth_buffers")));
wlan_exp_ip_udp_socket           ETH_sockets[WLAN_EXP_IP_UDP_NUM_SOCKETS];

// ARP Table data structures
//     NOTE:  There is only a single ARP table for all Ethernet devices.
//
arp_cache_entry              ETH_arp_cache[WLAN_EXP_IP_UDP_NUM_ARP_ENTRIES];


/*************************** Function Prototypes *****************************/


/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * Initialize the global Ethernet structures
 *
 * @param   None
 *
 * @return  None
 *
 ******************************************************************************/
void eth_init_global_structures() {
    u32 i;
    u32 temp;
    
    // Initialize dummy Ethernet frame
    bzero(ETH_dummy_frame, ETH_MIN_FRAME_LEN);

    // Initialize the send buffer pool
    ETH_allocated_send_buffers = 0;
        
    // Initialize each IP/UDP buffer in the send buffer pool
    for (i = 0; i < WLAN_EXP_IP_UDP_ETH_NUM_SEND_BUF; i++) {
        temp = ((u32)(&ETH_send_buffer) + (i * (WLAN_EXP_IP_UDP_ETH_BUF_SIZE >> 2)) + WLAN_EXP_IP_UDP_ETH_TX_BUF_ALIGNMENT);
    
        ETH_send_buffers[i].state                = WLAN_EXP_IP_UDP_BUFFER_FREE;
        ETH_send_buffers[i].max_size             = WLAN_EXP_IP_UDP_ETH_BUF_SIZE;
        ETH_send_buffers[i].size                 = 0;
        ETH_send_buffers[i].data                 = (u8 *) temp;
        ETH_send_buffers[i].offset               = (u8 *) temp;
        ETH_send_buffers[i].length               = 0;
        ETH_send_buffers[i].descriptor           = NULL;
    }
}



/*****************************************************************************/
/**
 * Initialize the Socket structures
 *
 * @param   None
 *
 * @return  None
 *
 ******************************************************************************/
void socket_init_sockets() {
    u32 i;
    
    // Initialize all the sockets
    for (i = 0; i < WLAN_EXP_IP_UDP_NUM_SOCKETS; i++) {
        ETH_sockets[i].index       = i;
        ETH_sockets[i].state       = SOCKET_CLOSED;
        ETH_sockets[i].eth_dev_num = WLAN_EXP_IP_UDP_INVALID_ETH_DEVICE;
        ETH_sockets[i].hdr         = &(ETH_headers[i]);
    }
}



/*****************************************************************************/
/**
 * Initialize the ARP cache structure
 *
 * @param   None
 *
 * @return  None
 *
 ******************************************************************************/
void arp_init_cache() {
    u32 i;

    // Initialize ARP table
    for (i = 0; i < WLAN_EXP_IP_UDP_NUM_ARP_ENTRIES; i++) {
        // Zero out the entry
        bzero((void *)&(ETH_arp_cache[i]), sizeof(arp_cache_entry));
        
        // Set the Ethernet device to the invalid Ethernet device
        ETH_arp_cache[i].eth_dev_num = WLAN_EXP_IP_UDP_INVALID_ETH_DEVICE;
    }
}



/*****************************************************************************/
/**
 * Initialize the IP/UDP Library
 *
 * This function will initialize all subsystems within the library:
 *   - Global Ethernet structures
 *   - Socket data structures
 *   - ARP cache
 *   - IP V4 global structures (ie ID counter)
 *
 * @param   None
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 ******************************************************************************/
int wlan_exp_ip_udp_init() {

    // Initialize the global ethernet structures
    eth_init_global_structures();
    
    // Initialize the sockets
    socket_init_sockets();
    
    // Initialize the ARP cache
    arp_init_cache();
   
    // Initialize the IP v4 global structures
    ipv4_init();

    // Return success
    return XST_SUCCESS;
}
