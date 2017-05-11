/** @file ./src/wlan_exp_ip_udp_config.c
 *  @brief Mango IP/UDP Library (Config)
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Reference Design license (https://mangocomm.com/802.11/license)
 */


// NOTE:  Many data structures in the Mango IP/UDP Library must be accessible to DMAs
// and other system level masters.  Therefore, we have put those variables in their
// own linker section, WLAN_EXP_IP_UDP_ETH_BUFFERS_LINKER_SECTION, so that it is easy to put that section into an 
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

// Xilinx Hardware includes
#include <xaxidma.h>
#include <xaxiethernet.h>

// MAC High Framework includes
#include "wlan_platform_high.h"

// Mango IP/UDP Library includes
#include "wlan_exp_ip_udp.h"
#include "wlan_exp_ip_udp_internal.h"


platform_high_dev_info_t	 platform_high_dev_info;

/************************* Ethernet 0 Definitions ****************************/

// Variables for ETH 0
//   NONE - Mango IP/UDP library configured to ignore ETH 0

// Aligned memory segments to be used for buffer descriptors
//
//   NONE - Mango IP/UDP library configured to ignore ETH 0

// Memory allocations for buffers
//
//   NONE - Mango IP/UDP library configured to ignore ETH 0


/************************* Ethernet 1 Definitions ****************************/

// Variables for ETH 1
XAxiEthernet                 ETH_1_instance;
XAxiDma                      ETH_1_dma_instance;

// Aligned memory segments to be used for buffer descriptors
//     NOTE:  Memory for the buffer descriptors must be accessible by the DMA
//
u8                           ETH_1_rx_bd_space[WLAN_EXP_IP_UDP_ETH_1_RXBD_SPACE_BYTES] __attribute__ ((aligned(WLAN_EXP_IP_UDP_BD_ALIGNMENT))) __attribute__ ((section (WLAN_EXP_IP_UDP_ETH_BUFFERS_LINKER_SECTION)));
u8                           ETH_1_tx_bd_space[WLAN_EXP_IP_UDP_ETH_1_TXBD_SPACE_BYTES] __attribute__ ((aligned(WLAN_EXP_IP_UDP_BD_ALIGNMENT))) __attribute__ ((section (WLAN_EXP_IP_UDP_ETH_BUFFERS_LINKER_SECTION)));

// Memory allocations for buffers
//     NOTE:  Memory for the buffer data must be accessible by the DMA
//
wlan_exp_ip_udp_buffer           ETH_1_recv_buffers[WLAN_EXP_IP_UDP_ETH_1_NUM_RECV_BUF];
u32                          ETH_1_recv_buffer[WLAN_EXP_IP_UDP_ETH_1_NUM_RECV_BUF * (WLAN_EXP_IP_UDP_ETH_BUF_SIZE >> 2)] __attribute__ ((aligned(WLAN_EXP_IP_UDP_BUFFER_ALIGNMENT))) __attribute__ ((section (WLAN_EXP_IP_UDP_ETH_BUFFERS_LINKER_SECTION)));


/*************************** Variable Definitions ****************************/

// Ethernet device structure
ethernet_device              eth_device[WLAN_EXP_IP_UDP_NUM_ETH_DEVICES];


/******************************** Functions **********************************/

/*****************************************************************************/
/**
 * Initialize the information about the Ethernet device
 *
 * @param   eth_dev_num      - Ethernet device number
 *
 * @return  None
 *
 ******************************************************************************/

void eth_init_device_info(u32 eth_dev_num) {

    int  i;
    u32  temp;

    platform_high_dev_info   = wlan_platform_high_get_dev_info();

    // Initialize Ethernet device
    switch (eth_dev_num) {
        case WLAN_EXP_IP_UDP_ETH_0:
            eth_device[eth_dev_num].initialized            = 0;
            eth_device[eth_dev_num].eth_id                 = 0;
            eth_device[eth_dev_num].eth_ptr                = NULL;
            eth_device[eth_dev_num].eth_cfg_ptr            = NULL;
            eth_device[eth_dev_num].dma_id                 = 0;
            eth_device[eth_dev_num].dma_ptr                = NULL;
            eth_device[eth_dev_num].dma_cfg_ptr            = NULL;
            eth_device[eth_dev_num].dma_rx_ring_ptr        = NULL;
            eth_device[eth_dev_num].dma_rx_bd_ptr          = NULL;
            eth_device[eth_dev_num].dma_rx_bd_cnt          = 0;
            eth_device[eth_dev_num].dma_tx_ring_ptr        = NULL;
            eth_device[eth_dev_num].dma_tx_bd_ptr          = NULL;
            eth_device[eth_dev_num].dma_tx_bd_cnt          = 0;
            eth_device[eth_dev_num].padding                = 0;
            eth_device[eth_dev_num].num_recv_buffers       = 0;
            eth_device[eth_dev_num].recv_buffers           = NULL;

            bzero(eth_device[eth_dev_num].hw_addr, ETH_ADDR_LEN);
            bzero(eth_device[eth_dev_num].ip_addr, IP_ADDR_LEN);
        break;

        case WLAN_EXP_IP_UDP_ETH_1:
            eth_device[eth_dev_num].initialized            = 1;
            eth_device[eth_dev_num].eth_id                 = platform_high_dev_info.wlan_exp_eth_mac_dev_id;
            eth_device[eth_dev_num].eth_ptr                = &ETH_1_instance;
            eth_device[eth_dev_num].eth_cfg_ptr            = (void *) XAxiEthernet_LookupConfig(platform_high_dev_info.wlan_exp_eth_mac_dev_id);
            eth_device[eth_dev_num].dma_id                 = platform_high_dev_info.wlan_exp_eth_dma_dev_id;
            eth_device[eth_dev_num].dma_ptr                = &ETH_1_dma_instance;
            eth_device[eth_dev_num].dma_cfg_ptr            = (void *) XAxiDma_LookupConfig(platform_high_dev_info.wlan_exp_eth_dma_dev_id);
            eth_device[eth_dev_num].dma_rx_ring_ptr        = (void *) XAxiDma_GetRxRing(&ETH_1_dma_instance);
            eth_device[eth_dev_num].dma_rx_bd_ptr          = &ETH_1_rx_bd_space;
            eth_device[eth_dev_num].dma_rx_bd_cnt          = WLAN_EXP_IP_UDP_ETH_1_RXBD_CNT;
            eth_device[eth_dev_num].dma_tx_ring_ptr        = (void *) XAxiDma_GetTxRing(&ETH_1_dma_instance);
            eth_device[eth_dev_num].dma_tx_bd_ptr          = &ETH_1_tx_bd_space;
            eth_device[eth_dev_num].dma_tx_bd_cnt          = WLAN_EXP_IP_UDP_ETH_1_TXBD_CNT;
            eth_device[eth_dev_num].padding                = 0;
            eth_device[eth_dev_num].num_recv_buffers       = WLAN_EXP_IP_UDP_ETH_1_NUM_RECV_BUF;
            eth_device[eth_dev_num].recv_buffers           = ETH_1_recv_buffers;

            bzero(eth_device[eth_dev_num].hw_addr, ETH_ADDR_LEN);
            bzero(eth_device[eth_dev_num].ip_addr, IP_ADDR_LEN);

            // Initialize the buffers for the device
            //     NOTE:  For the receive buffers, they are all being used by the library and cannot be allocated
            //
            for (i = 0; i < WLAN_EXP_IP_UDP_ETH_1_NUM_RECV_BUF; i++) {
                temp = (u32)(((u8*)(&ETH_1_recv_buffer) + (i * (WLAN_EXP_IP_UDP_ETH_BUF_SIZE)) + WLAN_EXP_IP_UDP_ETH_RX_BUF_ALIGNMENT));

                //xil_printf("temp_%d = 0x%08x, full buff size = %d, &ETH_0_recv_buffer = 0x%08x\n", i, temp, sizeof(ETH_0_recv_buffer), &ETH_0_recv_buffer);

                ETH_1_recv_buffers[i].state                = WLAN_EXP_IP_UDP_BUFFER_IN_USE;
                ETH_1_recv_buffers[i].max_size             = WLAN_EXP_IP_UDP_ETH_BUF_SIZE;
                ETH_1_recv_buffers[i].size                 = 0;
                ETH_1_recv_buffers[i].data                 = (u8 *) temp;
                ETH_1_recv_buffers[i].offset               = (u8 *) temp;
                ETH_1_recv_buffers[i].length               = 0;
                ETH_1_recv_buffers[i].descriptor           = NULL;
            }
        break;

        default:
            xil_printf("  **** ERROR:  Ethernet device %d not configured in hardware.", (eth_dev_num + 1));
        break;
    }
}

