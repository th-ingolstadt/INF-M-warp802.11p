/** @file wlan_mac_bss_info.h
 *  @brief BSS Info Subsystem
 *
 *  This contains code tracking BSS information. It also contains code for managing
 *  the active scan state machine.
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

#ifndef WLAN_MAC_NETWORK_INFO_H_
#define WLAN_MAC_NETWORK_INFO_H_

#include "wlan_mac_high_types.h"

//-----------------------------------------------
// Timeout used to remove inactive network_info_t structs
//     - Part of bss_info_timestamp_check()
//
#define NETWORK_INFO_TIMEOUT_USEC                              600000000


//-----------------------------------------------
// Field size defines
//
#define NUM_BASIC_RATES_MAX                                10


//-----------------------------------------------
// BSS Beacon Interval defines
//
#define BSS_MICROSECONDS_IN_A_TU                           1024
#define BEACON_INTERVAL_NO_BEACON_TX                       0x0
#define BEACON_INTERVAL_UNKNOWN                            0xFFFF

//-----------------------------------------------
// BSS Capabilities defines
//     - Uses some values from wlan_mac_802_11_defs.h for to map BSS
//       capabilities to the capabilities transmitted in a beacon.
//
#define BSS_CAPABILITIES_BEACON_MASK                      (CAPABILITIES_ESS | CAPABILITIES_IBSS | CAPABILITIES_PRIVACY)

#define BSS_CAPABILITIES_ESS                               CAPABILITIES_ESS
#define BSS_CAPABILITIES_IBSS                              CAPABILITIES_IBSS
#define BSS_CAPABILITIES_PRIVACY                           CAPABILITIES_PRIVACY


//-----------------------------------------------
// Network Flags defines
//
#define NETWORK_FLAGS_KEEP                                     0x0001


//-----------------------------------------------
// BSS Configuration Bit Masks
//
#define BSS_FIELD_MASK_BSSID                               0x00000001
#define BSS_FIELD_MASK_CHAN                                0x00000002
#define BSS_FIELD_MASK_SSID                                0x00000004
#define BSS_FIELD_MASK_BEACON_INTERVAL                     0x00000008
#define BSS_FIELD_MASK_HT_CAPABLE                          0x00000010
#define BSS_FIELD_MASK_DTIM_PERIOD		                   0x00000020
#define BSS_FIELD_MASK_ALL								   (BSS_FIELD_MASK_BSSID           | \
														    BSS_FIELD_MASK_CHAN            | \
														    BSS_FIELD_MASK_SSID            | \
														    BSS_FIELD_MASK_BEACON_INTERVAL | \
														    BSS_FIELD_MASK_HT_CAPABLE	   | \
														    BSS_FIELD_MASK_DTIM_PERIOD)


//-----------------------------------------------
// configure_bss() return error flags
//
#define BSS_CONFIG_FAILURE_BSSID_INVALID                   0x00000001
#define BSS_CONFIG_FAILURE_BSSID_INSUFFICIENT_ARGUMENTS    0x00000002
#define BSS_CONFIG_FAILURE_CHANNEL_INVALID                 0x00000004
#define BSS_CONFIG_FAILURE_BEACON_INTERVAL_INVALID         0x00000008
#define BSS_CONFIG_FAILURE_HT_CAPABLE_INVALID              0x00000010
#define BSS_CONFIG_FAILURE_DTIM_PERIOD_INVALID             0x00000020


/*************************** Function Prototypes *****************************/

void network_info_init();
void network_info_init_finish();

network_info_entry_t* network_info_checkout();
void network_info_checkin(network_info_entry_t* network_info_entry);

void network_info_rx_process(void* pkt_buf_addr);

void print_network_info();
void network_info_timestamp_check();

dl_list* wlan_mac_high_find_network_info_SSID(char* ssid);
network_info_entry_t* wlan_mac_high_find_network_info_BSSID(u8* bssid);

network_info_t* wlan_mac_high_create_network_info(u8* bssid, char* ssid, u8 chan);
void wlan_mac_high_reset_network_list();
void wlan_mac_high_clear_network_info(network_info_t* info);

dl_list* wlan_mac_high_get_network_info_list();

#endif
