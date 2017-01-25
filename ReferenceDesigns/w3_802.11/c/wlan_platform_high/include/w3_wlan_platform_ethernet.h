#ifndef WLAN_PLATFORM_ETHERNET_H_
#define WLAN_PLATFORM_ETHERNET_H_


#include "xintc.h"
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

int w3_wlan_platform_ethernet_init();
int w3_wlan_platform_ethernet_setup_interrupt(XIntc* intc);
void w3_wlan_platform_ethernet_set_rx_callback(function_ptr_t callback);
void w3_wlan_platform_ethernet_free_queue_entry_notify();

#endif //WLAN_PLATFORM_ETHERNET_H_
