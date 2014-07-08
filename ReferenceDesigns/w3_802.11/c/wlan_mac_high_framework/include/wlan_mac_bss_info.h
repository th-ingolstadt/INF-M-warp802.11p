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

#define MAX_NUM_BSS_INFO 100 ///< Maximum number of BSS information structs. -1 will fill the allotted memory for BSS info.

#define NUM_BASIC_RATES_MAX	10
typedef struct{
	u8   bssid[6];
	u8   chan;
	u8   flags;
	u64  timestamp;
	char ssid[SSID_LEN_MAX + 1];
	u8   padding[3];
	u8   num_basic_rates;
	char rx_power;
	u8   basic_rates[NUM_BASIC_RATES_MAX];
} bss_info;

#define BSS_FLAGS_IS_ASSOCIATED 0x01 //FIXME: I don't have a good way of cleanly handling this. Why do we need it?
#define BSS_FLAGS_IS_PRIVATE    0x02

void bss_info_init();
void bss_info_init_finish();
dl_entry* bss_info_checkout();
void bss_info_checkin(dl_entry* bsi);
inline void bss_info_rx_process(void* pkt_buf_addr, u8 rate, u16 length);
void print_bss_info();
void bss_info_timestamp_check();
dl_entry* wlan_mac_high_find_bss_info_SSID(dl_list* list, char* ssid);
