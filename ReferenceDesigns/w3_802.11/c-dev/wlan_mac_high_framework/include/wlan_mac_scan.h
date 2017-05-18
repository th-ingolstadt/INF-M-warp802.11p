/** @file wlan_mac_scan.h
 *  @brief Scan FSM
 *
 *  This contains code for the active scan finite state machine.
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/***************************** Include Files *********************************/


/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_SCAN_H_
#define WLAN_MAC_SCAN_H_

#include "wlan_mac_high_types.h"

// Scan Timing Parameters
//     These defines set the scan timing parameters at boot.
//
#define DEFAULT_SCAN_PROBE_TX_INTERVAL_USEC                20000
#define DEFAULT_SCAN_TIME_PER_CHANNEL_USEC                 150000


/*************************** Function Prototypes *****************************/

int  wlan_mac_scan_init();

void wlan_mac_scan_set_tx_probe_request_callback(function_ptr_t callback);
void wlan_mac_scan_set_state_change_callback(function_ptr_t callback);

volatile scan_parameters_t* wlan_mac_scan_get_parameters();

void wlan_mac_scan_start();
void wlan_mac_scan_stop();
void wlan_mac_scan_pause();
void wlan_mac_scan_resume();

u32  wlan_mac_scan_is_scanning();
int  wlan_mac_scan_get_num_scans();


#endif
