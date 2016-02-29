/** @file wlan_mac_sta_join.h
 *  @brief Join FSM
 *
 *  This contains code for the STA join process.
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


#ifndef WLAN_MAC_STA_JOIN_FSM_H_
#define WLAN_MAC_STA_JOIN_FSM_H_

typedef struct {
	char*	ssid;		//Mandatory: SSID string
	u8 		bssid[6];	//Optional: BSSID address, 00-00-00-00-00-00 interpreted as "ignore"
	u8		channel;	//Optional: Channel number, 0 interpreted as "ignore"
} join_parameters_t;

#define BSS_SEARCH_POLL_INTERVAL_USEC            10000
#define BSS_ATTEMPT_POLL_INTERVAL_USEC           50000

void wlan_mac_sta_join_init();
volatile join_parameters_t* wlan_mac_sta_get_join_parameters();
void wlan_mac_sta_set_join_success_callback(function_ptr_t callback);
int  wlan_mac_is_joining();
void wlan_mac_sta_join();
void wlan_mac_sta_join_l();
void wlan_mac_sta_bss_search_poll(u32 schedule_id);
void wlan_mac_sta_bss_attempt_poll(u32 arg);
void wlan_mac_sta_scan_auth_transmit();
void wlan_mac_sta_scan_assoc_req_transmit();
void wlan_mac_sta_return_to_idle();

#endif
