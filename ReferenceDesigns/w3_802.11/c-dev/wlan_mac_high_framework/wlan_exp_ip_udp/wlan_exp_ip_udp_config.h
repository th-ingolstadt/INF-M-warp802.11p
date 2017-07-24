/** @file ./src/wlan_exp_ip_udp_config.h
 *  @brief Mango wlan_exp IP/UDP Library (Config)
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Reference Design license (https://mangocomm.com/802.11/license)
 */


#ifndef WLAN_EXP_IP_UDP_CONFIG_H
#define WLAN_EXP_IP_UDP_CONFIG_H


/***************************** Include Files *********************************/

// Xilinx / Standard library includes
#include <xparameters.h>


/*************************** Constant Definitions ****************************/

// Global Defines
#define WLAN_EXP_IP_UDP_BD_ALIGNMENT                 XAXIDMA_BD_MINIMUM_ALIGNMENT       // Buffer Descriptor alignment
#define WLAN_EXP_IP_UDP_BUFFER_ALIGNMENT             0x40                               // Buffer alignment (64 byte boundary)

// Define global DMA constants
#define WLAN_EXP_IP_UDP_TXBD_CNT                     10                                 // Number of TX buffer descriptors (per instance)

// Define UDP connections
#define WLAN_EXP_IP_UDP_NUM_SOCKETS                  5                                  // Number of UDP sockets (global pool)
#define WLAN_EXP_IP_UDP_NUM_ARP_ENTRIES              10                                 // Number of ARP entries (global pool)

// Define global Ethernet constants
#define WLAN_EXP_IP_UDP_ETH_BUF_SIZE                 ((9014 + 31) & 0xFFFFFFE0)         // Align parameter to a 32 byte boundary
#define WLAN_EXP_IP_UDP_ETH_NUM_SEND_BUF             2                                  // Number Ethernet transmit buffers (global pool)
#define WLAN_EXP_IP_UDP_ETH_RX_BUF_ALIGNMENT         4                                  // Alignment of packet within the RX buffer
#define WLAN_EXP_IP_UDP_ETH_TX_BUF_ALIGNMENT         0                                  // Alignment of packet within the TX buffer


/************************* Ethernet 0 Definitions ****************************/

// Ethernet device 0
#define WLAN_EXP_IP_UDP_ETH_0_DEFAULT_SPEED          1000
#define WLAN_EXP_IP_UDP_ETH_0_NUM_RECV_BUF           0
#define WLAN_EXP_IP_UDP_ETH_0_RXBD_CNT               0                             // Number of RX descriptors to use
#define WLAN_EXP_IP_UDP_ETH_0_TXBD_CNT               0                             // Number of TX descriptors to use
#define WLAN_EXP_IP_UDP_ETH_0_RXBD_SPACE_BYTES       0
#define WLAN_EXP_IP_UDP_ETH_0_TXBD_SPACE_BYTES       0


/************************* Ethernet 1 Definitions ****************************/

// Ethernet device 1
#define WLAN_EXP_IP_UDP_ETH_1_DEFAULT_SPEED          1000
#define WLAN_EXP_IP_UDP_ETH_1_NUM_RECV_BUF           3
#define WLAN_EXP_IP_UDP_ETH_1_RXBD_CNT               WLAN_EXP_IP_UDP_ETH_1_NUM_RECV_BUF     // Number of RX descriptors to use
#define WLAN_EXP_IP_UDP_ETH_1_TXBD_CNT               WLAN_EXP_IP_UDP_TXBD_CNT               // Number of TX descriptors to use
#define WLAN_EXP_IP_UDP_ETH_1_RXBD_SPACE_BYTES      (XAxiDma_BdRingMemCalc(WLAN_EXP_IP_UDP_BD_ALIGNMENT, WLAN_EXP_IP_UDP_ETH_1_RXBD_CNT))
#define WLAN_EXP_IP_UDP_ETH_1_TXBD_SPACE_BYTES      (XAxiDma_BdRingMemCalc(WLAN_EXP_IP_UDP_BD_ALIGNMENT, WLAN_EXP_IP_UDP_ETH_1_TXBD_CNT))


/*********************** Global Variable Definitions *************************/



/*************************** Function Prototypes *****************************/

// Initialization functions
void eth_init_device_info(u32 eth_dev_num);

#endif // WLAN_EXP_IP_UDP_CONFIG_H_

