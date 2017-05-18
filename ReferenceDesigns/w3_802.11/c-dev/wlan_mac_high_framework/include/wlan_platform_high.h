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
#include "xaxiethernet.h"
#include "wlan_mac_high_types.h"
#include "wlan_exp_common.h" //FIXME: the declaration below needs the cmd_resp typedef from wlan_exp.
							 // we avoiding moving wlan_exp types into the flat wlan_mac_high_type.h list,
							 // but maybe we should reconsider.

//---------------------------------------
// Public functions that WLAN MAC High Framework directly calls
platform_high_dev_info_t wlan_platform_high_get_dev_info();
int wlan_platform_high_init(XIntc* intc);
void wlan_platform_free_queue_entry_notify();
int wlan_platform_wlan_exp_process_node_cmd(u8* cmd_processed, u32 cmd_id, int socket_index, void* from, cmd_resp* command, cmd_resp* response, u32 max_resp_len);
int wlan_platform_wlan_exp_eth_init(XAxiEthernet* eth_ptr);

// Functions implemented in files other than wlan_platform_high.c
#if WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE
int wlan_platform_ethernet_send(u8* pkt_ptr, u32 length);
#endif //WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE

#endif /* WLAN_PLATFORM_HIGH_H_ */
