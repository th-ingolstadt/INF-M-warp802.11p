/** @file wlan_platform_high.h
 *  @brief High Platform Defines for 802.11 Ref design
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *                See LICENSE.txt included in the design archive or
 *                at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

#ifndef WLAN_PLATFORM_HIGH_H_
#define WLAN_PLATFORM_HIGH_H_

#include "xintc.h"
#include "wlan_mac_common.h"
#include "wlan_exp_common.h"

//---------------------------------------
// Platform information struct
typedef struct{
	u32		dlmb_baseaddr;
	u32		dlmb_size;
	u32		ilmb_baseaddr;
	u32		ilmb_size;
	u32		aux_bram_baseaddr;
	u32		aux_bram_size;
	u32		dram_baseaddr;
	u32		dram_size;
	u32		intc_dev_id;
	u32		timer_dev_id;
	u32		timer_int_id;
	u32		timer_freq;
	u32		cdma_dev_id;
	u32 	mailbox_int_id;
	u32		wlan_exp_eth_mac_dev_id;
	u32		wlan_exp_eth_dma_dev_id;
} platform_high_dev_info_t;

//---------------------------------------
// Platform configuration struct
typedef struct{
	XIntc* intc;
	function_ptr_t  eth_rx_callback;
	function_ptr_t  uart_rx_callback;
	function_ptr_t  userio_inputs_callback;
} platform_high_config_t;

//---------------------------------------
// Public functions that WLAN MAC High Framework directly calls
platform_high_dev_info_t wlan_platform_high_get_dev_info();
int wlan_platform_high_init(platform_high_config_t platform_high_config);
void wlan_platform_free_queue_entry_notify();
int wlan_platform_wlan_exp_process_node_cmd(u8* cmd_processed, u32 cmd_id, int socket_index, void * from, cmd_resp * command, cmd_resp * response, u32 max_resp_len);

// Functions implemented in files other than wlan_platform_high.c
#if WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE
int wlan_platform_ethernet_send(u8* pkt_ptr, u32 length);
#endif //WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE

#endif /* WLAN_PLATFORM_HIGH_H_ */
