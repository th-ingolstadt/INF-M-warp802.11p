/** @file wlan_mac_dcf.h
 *  @brief Distributed Coordination Function
 *
 *  This contains code to implement the 802.11 DCF.
 *
 *  @copyright Copyright 2014, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 *  @bug
 *  - EIFS value is currently aggressive (DIFS). Needs calibration.
 */

#ifndef WLAN_MAC_DCF_H_
#define WLAN_MAC_DCF_H_

#define TX_PKT_BUF_ACK 15

//RTS/CTS is not currently supported. This threshold must remain larger than any outgoing MPDU
#define RTS_THRESHOLD 2000

//TODO: In a future release, control of these parameters will be made available to CPU_HIGH through IPC commands
#define DCF_CW_EXP_MIN 4
#define DCF_CW_EXP_MAX 10

//CW Update Reasons
#define DCF_CW_UPDATE_MPDU_TX_ERR 0
#define DCF_CW_UPDATE_MPDU_RX_ACK 1
#define DCF_CW_UPDATE_BCAST_TX 2

typedef struct{
	u8 frame_control_1;
	u8 frame_control_2;
	u16 duration_id;
	u8 address_ra[6];
} mac_header_80211_ACK;


//MAC Timing Parameters
#define T_SLOT 9
#define T_SIFS 16
#define T_DIFS (T_SIFS + 2*T_SLOT)
//#define T_EIFS 128
#define T_EIFS T_DIFS
#define T_TIMEOUT 80

#define MAC_HW_LASTBYTE_ADDR1 (13)

int main();
int phy_to_mac_rate(u8 rate_in);
int mac_to_phy_rate(u8 rate_in);
int frame_transmit(u8 pkt_buf, u8 rate, u16 length);
u32 frame_receive(void* pkt_buf_addr, u8 rate, u16 length);
void mac_dcf_init();
void wlan_mac_dcf_hw_unblock_rx_phy();
void process_ipc_msg_from_high(wlan_ipc_msg* msg);
inline u32 poll_mac_rx();
inline u32 wlan_mac_dcf_hw_rx_finish();
inline int update_cw(u8 reason, u8 pkt_buf);
inline unsigned int rand_num_slots();
void wlan_mac_dcf_hw_start_backoff(u16 num_slots);
int wlan_create_ack_frame(void* pkt_buf, u8* address_ra);
inline void lock_empty_rx_pkt_buf();
inline u64 get_usec_timestamp();
inline u64 get_rx_start_timestamp();
inline void send_exception(u32 reason);
void process_config_rf_ifc(ipc_config_rf_ifc* config_rf_ifc);
void process_config_mac(ipc_config_mac* config_mac);
inline int calculate_rx_power(u8 band, u16 rssi, u8 lna_gain);



#endif /* WLAN_MAC_DCF_H_ */
