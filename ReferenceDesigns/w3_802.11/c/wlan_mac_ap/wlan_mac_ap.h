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

int main();
void ethernet_receive(packet_queue_element* tx_queue, u8* eth_dest, u8* eth_src, u16 tx_length);
void mpdu_rx_process(void* pkt_buf_addr, u8 rate, u16 length);
int is_tx_buffer_empty();
void mpdu_transmit(packet_queue_element* tx_queue);
void beacon_transmit();
void process_ipc_msg_from_low(wlan_ipc_msg* msg);
void print_associations();
void queue_size_print();


#endif /* WLAN_MAC_LOW_H_ */
