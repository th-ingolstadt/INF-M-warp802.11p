/** @file wlan_mac_bss_info.h
 *  @brief BSS Info Subsystem
 *
 *  This contains code tracking BSS information. It also contains code for managing
 *  the active scan state machine.
 *
 *  @copyright Copyright 2014, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 *  @bug No known bugs.
 */

#ifndef WLAN_MAC_BSS_INFO_H_
#define WLAN_MAC_BSS_INFO_H_

#define MAX_NUM_BSS_INFO 1000 ///< Maximum number of BSS information structs. -1 will fill the allotted memory for BSS info.

#define NUM_BASIC_RATES_MAX	10
typedef struct{
	u8   	bssid[6];
	u8   	chan;
	u8   	flags;
	u64  	timestamp;
	char 	ssid[SSID_LEN_MAX + 1];
	u8 	 	state;
	u16		capabilities;
	u16		beacon_interval;
	u8   	num_basic_rates;
	u8 	 	padding0;
	u8   	basic_rates[NUM_BASIC_RATES_MAX];
	u16		padding1;
	dl_list associated_stations;
} bss_info;

#define BSS_STATE_UNAUTHENTICATED	1
#define BSS_STATE_AUTHENTICATED		2
#define BSS_STATE_ASSOCIATED		4
#define BSS_STATE_OWNED				5

void bss_info_init();
void bss_info_init_finish();
dl_entry* bss_info_checkout();
void bss_info_checkin(dl_entry* bsi);
inline void bss_info_rx_process(void* pkt_buf_addr, u8 rate, u16 length);
void print_bss_info();
void bss_info_timestamp_check();
dl_entry* wlan_mac_high_find_bss_info_SSID(char* ssid);
dl_entry* wlan_mac_high_find_bss_info_BSSID(u8* bssid);
bss_info* wlan_mac_high_create_bss_info(u8* bssid, char* ssid, u8 chan);
inline dl_list* wlan_mac_high_get_bss_info_list();

#endif
