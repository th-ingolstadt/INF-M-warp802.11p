////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_util.h
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

#ifndef WLAN_MAC_UTIL_H_
#define WLAN_MAC_UTIL_H_

#define ETH_A_MAC_DEVICE_ID			XPAR_ETH_A_MAC_DEVICE_ID
#define ETH_A_FIFO_DEVICE_ID		XPAR_ETH_A_FIFO_DEVICE_ID
#define TIMESTAMP_GPIO_DEVICE_ID 	XPAR_MB_HIGH_TIMESTAMP_GPIO_DEVICE_ID
#define TIMESTAMP_GPIO_LSB_CHAN 1
#define TIMESTAMP_GPIO_MSB_CHAN 2


//RATE ADAPTATION PARAMETERS
//Rate will attempt an increase after MIN_CONSECUTIVE_GOOD_ACKS consecutive good ACKs are received
//Rate will attempt a decrease after MIN_TOTAL_MISSED_ACKS total missed ACKs occur
#define MIN_CONSECUTIVE_GOOD_ACKS	10
#define MIN_TOTAL_MISSED_ACKS		50

#define RATE_ADAPT_MAX_RATE WLAN_MAC_RATE_QPSK34
#define RATE_ADAPT_MIN_RATE WLAN_MAC_RATE_BPSK12


typedef struct{
	u16 AID;
	u16 seq;
	u8 addr[6];
	u16 total_missed_acks;
	u16 consecutive_good_acks;
	u8 tx_rate;
} station_info;

typedef struct{
	u8 address_destination[6];
	u8 address_source[6];
	u16 type;
} ethernet_header;


#define ETH_TYPE_ARP 	0x0608
#define ETH_TYPE_IP 	0x0008

typedef struct{
	u8 dsap;
	u8 ssap;
	u8 control_field;
	u8 org_code[3];
	u16 type;
} llc_header;

#define LLC_SNAP 						0xAA
#define LLC_CNTRL_UNNUMBERED			0x03
#define LLC_TYPE_ARP					0x0608
#define LLC_TYPE_IP						0x0008


void wlan_mac_util_init();
void gpio_timestamp_initialize();
inline u64 get_usec_timestamp();
void wlan_eth_init();
void wlan_mac_send_eth(void* mpdu, u16 length);
inline void wlan_mac_poll_eth(u8 tx_pkt_buf);
void wlan_mac_util_set_eth_rx_callback(void(*callback)());
void wlan_mac_schedule_event(u32 delay, void(*callback)());
inline void poll_schedule();

void wlan_mac_util_process_tx_done(tx_frame_info* frame,station_info* station);
u8 wlan_mac_util_get_tx_rate(station_info* station);

#endif /* WLAN_MAC_UTIL_H_ */
