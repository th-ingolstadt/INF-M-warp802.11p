/** @file wlan_mac_nomac.c
 *  @brief Simple MAC that does nothing but transmit and receive
 *
 *  @copyright Copyright 2014-2015, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

#ifndef WLAN_MAC_NOMAC_H_
#define WLAN_MAC_NOMAC_H_

///////// TOKEN MAC EXTENSION /////////
#define TX_PKT_BUF_TOKEN	   7

#define MAC_HW_LASTBYTE_TOKEN (sizeof(mac_frame_custom_token)+3)

typedef struct{
	u8 frame_control_1;
	u8 frame_control_2;
	u16 duration_id;
	u8 address_ra[6];
	u8 address_ta[6];
	u32 res_duration_usec;
} mac_frame_custom_token;
///////// TOKEN MAC EXTENSION /////////

int main();
int frame_transmit(u8 pkt_buf, u8 rate, u16 length, wlan_mac_low_tx_details* low_tx_details);
u32 frame_receive(u8 rx_pkt_buf, phy_rx_details* phy_details);

///////// TOKEN MAC EXTENSION /////////
void token_new_reservation(ipc_token_new_reservation* new_reservation);
void poll_reservation_time();
int wlan_create_token_offer_frame(void* pkt_buf_addr, u8* address_ra, u8* address_ta, u16 duration, u32 res_duration);
void adjust_reservation_ts_end(s64 adjustment);
int wlan_create_token_response_frame(void* pkt_buf_addr, u8* address_ra, u8* address_ta, u16 duration, u32 res_duration);
///////// TOKEN MAC EXTENSION /////////

#endif /* WLAN_MAC_NOMAC_H_ */
