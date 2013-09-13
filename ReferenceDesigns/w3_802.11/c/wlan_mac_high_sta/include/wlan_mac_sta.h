////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_sta.h
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

#ifndef WLAN_MAC_STA_H_
#define WLAN_MAC_STA_H_

#define SSID_LEN_MAX 32
#define NUM_BASIC_RATES_MAX 10

typedef struct{
	u8 bssid[6];
	u8 chan;
	u8 private;
	char ssid[SSID_LEN_MAX];
	u8 num_basic_rates;
	u8 basic_rates[NUM_BASIC_RATES_MAX];
	char rx_power;
} ap_info;

int main();
void print_ap_list();
int ethernet_receive(packet_bd_list* tx_queue_list, u8* eth_dest, u8* eth_src, u16 tx_length);
void mpdu_rx_process(void* pkt_buf_addr, u8 rate, u16 length);
void mpdu_transmit_done(tx_frame_info* tx_mpdu);
void uart_rx(u8 rxByte);
void print_menu();
void attempt_authentication();
void probe_req_transmit();
void check_tx_queue();
int str2num(char* str);
void print_station_status();
void reset_station_statistics();
void ltg_event(u32 id);

#endif /* WLAN_MAC_STA_H_ */
