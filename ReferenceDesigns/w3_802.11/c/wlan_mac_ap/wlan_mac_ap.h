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
void ethernet_receive(u8* eth_dest, u8* eth_src, u16 tx_length);
void mpdu_process(void* pkt_buf_addr, u8 rate, u16 length);
int is_tx_buffer_empty();
void mpdu_transmit(u16 length, u8 rate, u8 retry_max, u8 flags);
void beacon_transmit();
void wait_for_tx_accept();
void process_ipc_msg_from_low(wlan_ipc_msg* msg);
void print_associations();
void write_hex_display(u8 val);

#endif /* WLAN_MAC_LOW_H_ */
