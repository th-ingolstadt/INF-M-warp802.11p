#ifndef WLAN_MAC_SCAN_FSM_H_
#define WLAN_MAC_SCAN_FSM_H_

int wlan_mac_set_scan_channels(u8* channel_vec, u32 len);
void wlan_mac_set_scan_timings(u32 dwell_usec, u32 idle_usec);
void wlan_mac_scan_enable(u8* bssid, char* ssid_str);
void wlan_mac_scan_disable();
void wlan_mac_scan_state_transition();
void wlan_mac_scan_probe_req_transmit();

#endif
