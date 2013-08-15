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

#define MAX_ASSOCIATIONS 8

#define ASSOCIATION_TIMEOUT_S (300)
#define ASSOCIATION_TIMEOUT_US (ASSOCIATION_TIMEOUT_S*1000000)

#define ANIMATION_RATE_US (50000)

int main();
int ethernet_receive(pqueue_list* tx_queue_list, u8* eth_dest, u8* eth_src, u16 tx_length);
void mpdu_rx_process(void* pkt_buf_addr, u8 rate, u16 length);
int is_tx_buffer_empty();
void mpdu_transmit(pqueue* tx_queue);
void beacon_transmit();
void process_ipc_msg_from_low(wlan_ipc_msg* msg);
void print_associations();
void print_queue_status();
void association_timestamp_check();
void enable_associations();
void disable_associations();
void animate_hex();
void up_button();
void uart_rx(u8 rxByte);

#endif /* WLAN_MAC_LOW_H_ */
