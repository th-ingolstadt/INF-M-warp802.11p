////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_ap.h
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

#ifndef WLAN_MAC_HIGH_H_
#define WLAN_MAC_HIGH_H_

int main();
int ethernet_receive(packet_bd_list* tx_queue_list, u8* eth_dest, u8* eth_src, u16 tx_length);
void mpdu_rx_process(void* pkt_buf_addr, u8 rate, u16 length);
void beacon_transmit();
void print_associations();
void print_queue_status();
void mpdu_transmit_done(tx_frame_info* tx_mpdu);
void association_timestamp_check();
void enable_associations();
void disable_associations();
void animate_hex();
void up_button();
void uart_rx(u8 rxByte);
void print_menu();
void print_station_status();
void reset_station_statistics();
void deauthenticate_stations();
void check_tx_queue();

#endif /* WLAN_MAC_LOW_H_ */
