/** @file wlan_mac_dcf.h
 *  @brief Distributed Coordination Function
 *
 *  This contains code to implement the 802.11 DCF.
 *
 *  @copyright Copyright 2015, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

#ifndef WLAN_MAC_DCF_H_
#define WLAN_MAC_DCF_H_

#define PKT_BUF_INVALID 0xFF

#define TX_PKT_BUF_CTRL 7

//CW Update Reasons
#define DCF_CW_UPDATE_MPDU_TX_ERR 0
#define DCF_CW_UPDATE_MPDU_RX_ACK 1
#define DCF_CW_UPDATE_BCAST_TX 2

#define MAC_HW_LASTBYTE_ADDR1 (13)

#define RAND_SLOT_REASON_STANDARD_ACCESS 0
#define RAND_SLOT_REASON_IBSS_BEACON     1

typedef enum {TX_WAIT_NONE, TX_WAIT_ACK, TX_WAIT_CTS} tx_wait_state_t;
typedef enum {TX_MODE_SHORT, TX_MODE_LONG} tx_mode_t;

int main();
int frame_transmit(u8 pkt_buf, u8 rate, u16 length, wlan_mac_low_tx_details* low_tx_details);
u32 frame_receive(u8 rx_pkt_buf, phy_rx_details* phy_details);
inline void increment_src_ssrc(u8* src_ptr);
inline void increment_lrc_slrc(u8* lrc_ptr);
inline void reset_cw();
inline void reset_ssrc();
inline void reset_slrc();
//inline void update_cw(u8 reason, u8 pkt_buf);
inline unsigned int rand_num_slots(u8 reason);
void wlan_mac_dcf_hw_start_backoff(u16 num_slots);
int wlan_create_ack_frame(void* pkt_buf_addr, u8* address_ra);
int wlan_create_rts_frame(void* pkt_buf_addr, u8* address_ra, u8* address_ta, u16 duration);



#endif /* WLAN_MAC_DCF_H_ */
