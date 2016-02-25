/** @file wlan_mac_scan.h
 *  @brief Active Scan FSM
 *
 *  This contains code for the active scan finite state machine. This particular file
 *  is for the STA variant.
 *
 *  @copyright Copyright 2014-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */


#ifndef WLAN_MAC_SCAN_FSM_H_
#define WLAN_MAC_SCAN_FSM_H_

typedef struct {
	u32 	time_per_channel_usec;
	u32 	probe_tx_interval_usec;
	u8* 	channel_vec;
	u32 	channel_vec_len;
	char*	ssid;
} scan_parameters_t;

int wlan_mac_scan_init();
scan_parameters_t* wlan_mac_scan_get_parameters();
void wlan_mac_scan_start();
void wlan_mac_scan_pause();
void wlan_mac_scan_stop();
u32 wlan_mac_scan_is_scanning();
void wlan_mac_scan_state_transition();


#endif
