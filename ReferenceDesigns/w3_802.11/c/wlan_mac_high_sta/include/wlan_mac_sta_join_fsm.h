#ifndef WLAN_MAC_STA_JOIN_FSM_H_
#define WLAN_MAC_STA_JOIN_FSM_H_

#define BSS_SEARCH_POLL_INTERVAL_USEC 100000
#define BSS_ATTEMPT_POLL_INTERVAL_USEC 50000

void wlan_mac_sta_scan_and_join(char* ssid, u32 to_sec);
void wlan_mac_sta_join(bss_info* bss_description, u32 to_sec);
void wlan_mac_sta_bss_search_poll(u32 schedule_id);
void wlan_mac_sta_bss_attempt_poll(u32 schedule_id);
void wlan_mac_sta_scan_auth_transmit();
void wlan_mac_sta_scan_assoc_req_transmit();
void wlan_mac_sta_return_to_idle();

#endif
