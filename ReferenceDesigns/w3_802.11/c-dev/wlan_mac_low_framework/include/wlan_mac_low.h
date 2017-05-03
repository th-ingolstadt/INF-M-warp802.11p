/** @file wlan_mac_high.h
 *  @brief Low-level WLAN MAC High Framework
 *
 *  This contains the low-level code for accessing the WLAN MAC Low Framework.
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *                See LICENSE.txt included in the design archive or
 *                at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */


/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_LOW_H_
#define WLAN_MAC_LOW_H_

#include "wlan_mac_pkt_buf_util.h"
#include "wlan_mac_mailbox_util.h"

//-----------------------------------------------
// MAC Header defines
#define MAC_HW_LASTBYTE_ADDR1 (9)
#define MAC_HW_LASTBYTE_ADDR2 (15)

//-----------------------------------------------
// Power defines
//
#define PKT_DET_MIN_POWER_MIN -90
#define PKT_DET_MIN_POWER_MAX -30

//-----------------------------------------------
// Channel band defines
//
#define CHAN_BAND_24GHz		0
#define CHAN_BAND_5GHz		1



//-----------------------------------------------
// Macros wrapping register read/writes in MAC core
//
#define wlan_mac_reset(x)                      			  Xil_Out32(WLAN_MAC_REG_CONTROL, (Xil_In32(WLAN_MAC_REG_CONTROL) & ~WLAN_MAC_CTRL_MASK_RESET) | ((x) ? WLAN_MAC_CTRL_MASK_RESET : 0))

#define wlan_mac_set_postTx_timer1(d)                     (Xil_Out32(WLAN_MAC_REG_POST_TX_TIMERS, ((Xil_In32(WLAN_MAC_REG_POST_TX_TIMERS) & ~WLAN_MAC_POST_TX_TIMERS_MASK_TIMER1_COUNTTO) | ((d)         & WLAN_MAC_POST_TX_TIMERS_MASK_TIMER1_COUNTTO))))
#define wlan_mac_set_postTx_timer2(d)                     (Xil_Out32(WLAN_MAC_REG_POST_TX_TIMERS, ((Xil_In32(WLAN_MAC_REG_POST_TX_TIMERS) & ~WLAN_MAC_POST_TX_TIMERS_MASK_TIMER2_COUNTTO) | (((d) << 16) & WLAN_MAC_POST_TX_TIMERS_MASK_TIMER2_COUNTTO))))
#define wlan_mac_set_postRx_timer1(d)                     (Xil_Out32(WLAN_MAC_REG_POST_RX_TIMERS, ((Xil_In32(WLAN_MAC_REG_POST_RX_TIMERS) & ~WLAN_MAC_POST_RX_TIMERS_MASK_TIMER1_COUNTTO) | ((d)         & WLAN_MAC_POST_RX_TIMERS_MASK_TIMER1_COUNTTO))))
#define wlan_mac_set_postRx_timer2(d)                     (Xil_Out32(WLAN_MAC_REG_POST_RX_TIMERS, ((Xil_In32(WLAN_MAC_REG_POST_RX_TIMERS) & ~WLAN_MAC_POST_RX_TIMERS_MASK_TIMER2_COUNTTO) | (((d) << 16) & WLAN_MAC_POST_RX_TIMERS_MASK_TIMER2_COUNTTO))))

#define wlan_mac_postTx_timer1_en(d)                      (Xil_Out32(WLAN_MAC_REG_POST_TX_TIMERS, ((Xil_In32(WLAN_MAC_REG_POST_TX_TIMERS) & ~WLAN_MAC_POST_TX_TIMERS_MASK_TIMER1_EN) | ((d) ? WLAN_MAC_POST_TX_TIMERS_MASK_TIMER1_EN : 0))))
#define wlan_mac_postTx_timer2_en(d)                      (Xil_Out32(WLAN_MAC_REG_POST_TX_TIMERS, ((Xil_In32(WLAN_MAC_REG_POST_TX_TIMERS) & ~WLAN_MAC_POST_TX_TIMERS_MASK_TIMER2_EN) | ((d) ? WLAN_MAC_POST_TX_TIMERS_MASK_TIMER2_EN : 0))))
#define wlan_mac_postRx_timer1_en(d)                      (Xil_Out32(WLAN_MAC_REG_POST_RX_TIMERS, ((Xil_In32(WLAN_MAC_REG_POST_RX_TIMERS) & ~WLAN_MAC_POST_RX_TIMERS_MASK_TIMER1_EN) | ((d) ? WLAN_MAC_POST_RX_TIMERS_MASK_TIMER1_EN : 0))))
#define wlan_mac_postRx_timer2_en(d)                      (Xil_Out32(WLAN_MAC_REG_POST_RX_TIMERS, ((Xil_In32(WLAN_MAC_REG_POST_RX_TIMERS) & ~WLAN_MAC_POST_RX_TIMERS_MASK_TIMER2_EN) | ((d) ? WLAN_MAC_POST_RX_TIMERS_MASK_TIMER2_EN : 0))))

#define wlan_mac_reset_tx_ctrl_A(x)             Xil_Out32(WLAN_MAC_REG_CONTROL, (Xil_In32(WLAN_MAC_REG_CONTROL) & ~WLAN_MAC_CTRL_MASK_RESET_TX_CTRL_A) | ((x) ? WLAN_MAC_CTRL_MASK_RESET_TX_CTRL_A : 0))
#define wlan_mac_reset_tx_ctrl_B(x)             Xil_Out32(WLAN_MAC_REG_CONTROL, (Xil_In32(WLAN_MAC_REG_CONTROL) & ~WLAN_MAC_CTRL_MASK_RESET_TX_CTRL_B) | ((x) ? WLAN_MAC_CTRL_MASK_RESET_TX_CTRL_B : 0))
#define wlan_mac_reset_tx_ctrl_C(x)             Xil_Out32(WLAN_MAC_REG_CONTROL, (Xil_In32(WLAN_MAC_REG_CONTROL) & ~WLAN_MAC_CTRL_MASK_RESET_TX_CTRL_C) | ((x) ? WLAN_MAC_CTRL_MASK_RESET_TX_CTRL_C : 0))
#define wlan_mac_reset_tx_ctrl_D(x)             Xil_Out32(WLAN_MAC_REG_CONTROL, (Xil_In32(WLAN_MAC_REG_CONTROL) & ~WLAN_MAC_CTRL_MASK_RESET_TX_CTRL_D) | ((x) ? WLAN_MAC_CTRL_MASK_RESET_TX_CTRL_D : 0))

#define wlan_mac_set_A_backoff_reset(x)         Xil_Out32(WLAN_MAC_REG_CONTROL, (Xil_In32(WLAN_MAC_REG_CONTROL) & ~WLAN_MAC_CTRL_MASK_RESET_A_BACKOFF) | ((x) ? WLAN_MAC_CTRL_MASK_RESET_A_BACKOFF : 0))
#define wlan_mac_set_C_backoff_reset(x)         Xil_Out32(WLAN_MAC_REG_CONTROL, (Xil_In32(WLAN_MAC_REG_CONTROL) & ~WLAN_MAC_CTRL_MASK_RESET_C_BACKOFF) | ((x) ? WLAN_MAC_CTRL_MASK_RESET_C_BACKOFF : 0))
#define wlan_mac_set_D_backoff_reset(x)         Xil_Out32(WLAN_MAC_REG_CONTROL, (Xil_In32(WLAN_MAC_REG_CONTROL) & ~WLAN_MAC_CTRL_MASK_RESET_D_BACKOFF) | ((x) ? WLAN_MAC_CTRL_MASK_RESET_D_BACKOFF : 0))

#define wlan_mac_pause_tx_ctrl_A(x)     Xil_Out32(WLAN_MAC_REG_CONTROL, (Xil_In32(WLAN_MAC_REG_CONTROL) & ~WLAN_MAC_CTRL_MASK_PAUSE_TX_A) | ((x) ? WLAN_MAC_CTRL_MASK_PAUSE_TX_A : 0))
#define wlan_mac_pause_tx_ctrl_C(x)     Xil_Out32(WLAN_MAC_REG_CONTROL, (Xil_In32(WLAN_MAC_REG_CONTROL) & ~WLAN_MAC_CTRL_MASK_PAUSE_TX_C) | ((x) ? WLAN_MAC_CTRL_MASK_PAUSE_TX_C : 0))
#define wlan_mac_pause_tx_ctrl_D(x)     Xil_Out32(WLAN_MAC_REG_CONTROL, (Xil_In32(WLAN_MAC_REG_CONTROL) & ~WLAN_MAC_CTRL_MASK_PAUSE_TX_D) | ((x) ? WLAN_MAC_CTRL_MASK_PAUSE_TX_D : 0))

#define wlan_mac_reset_tu_target_latch(x)       Xil_Out32(WLAN_MAC_REG_CONTROL, (Xil_In32(WLAN_MAC_REG_CONTROL) & ~WLAN_MAC_CTRL_MASK_RESET_TU_LATCH) | ((x) ? WLAN_MAC_CTRL_MASK_RESET_TU_LATCH : 0))
#define wlan_mac_reset_rx_started(x)			Xil_Out32(WLAN_MAC_REG_CONTROL, (Xil_In32(WLAN_MAC_REG_CONTROL) & ~WLAN_MAC_CTRL_MASK_RESET_RX_STARTED_LATCH) | ((x) ? WLAN_MAC_CTRL_MASK_RESET_RX_STARTED_LATCH : 0))

// WLAN_MAC_REG_SW_BACKOFF_CTRL:
//     b[15:0]: Num Slots
//     b[31]  : Start
//
#define wlan_mac_set_backoff_num_slots(d) Xil_Out32(WLAN_MAC_REG_SW_BACKOFF_CTRL, ((Xil_In32(WLAN_MAC_REG_SW_BACKOFF_CTRL) & (~0x0000FFFF)) | ((d) & 0x0000FFFF)))
#define wlan_mac_backoff_start(x)         Xil_Out32(WLAN_MAC_REG_SW_BACKOFF_CTRL, ((Xil_In32(WLAN_MAC_REG_SW_BACKOFF_CTRL) & (~0x80000000)) | (((x) << 31) & 0x80000000)))

// WLAN_MAC_IFS_1:
//     b[9:0]  : Slot
//     b[29:20]: DIFS
//
#define wlan_mac_set_slot(d) Xil_Out32(WLAN_MAC_REG_IFS_1, ((Xil_In32(WLAN_MAC_REG_IFS_1) & (~0x000003FF)) | ((d) & 0x000003FF)))
#define wlan_mac_set_DIFS(d) Xil_Out32(WLAN_MAC_REG_IFS_1, ((Xil_In32(WLAN_MAC_REG_IFS_1) & (~0x3FF00000)) | (((d) << 20) & 0x3FF00000)))

// WLAN_MAC_IFS_2:
//     b[15:0] : EIFS
//     b[31:16]: ACK Timeout
//
#define wlan_mac_set_EIFS(d) Xil_Out32(WLAN_MAC_REG_IFS_2, ((Xil_In32(WLAN_MAC_REG_IFS_2) & (~0x0000FFFF)) | ((d) & 0x0000FFFF)))

// WLAN_MAC_CALIB_TIMES:
//     b[9:0]  : TxDIFS
//     b[31:24]: NAV Adj (Fix8_0 - signed char!)
//
#define wlan_mac_set_TxDIFS(d)  Xil_Out32(WLAN_MAC_REG_CALIB_TIMES, ((Xil_In32(WLAN_MAC_REG_CALIB_TIMES) & (~0x000003FF)) | ((d) & 0x000003FF)))
#define wlan_mac_set_NAV_adj(d) Xil_Out32(WLAN_MAC_REG_CALIB_TIMES, ((Xil_In32(WLAN_MAC_REG_CALIB_TIMES) & (~0xFF000000)) | (((d) << 24) & 0xFF000000)))

// TX_CTRL_A_PARAMS:
//     b[3:0] : Pkt buf
//     b[7:4] : Tx ant mask
//     b[23:8]: Num backoff slots
//     b[24]  : Pre-Wait for PostRx Timer 1
//     b[25]  : Pre-Wait for PostTx Timer 1
//     b[26]  : Post-Wait for PostTx Timer 2
//     b[29:27]: PHY mode
//
#define wlan_mac_tx_ctrl_A_params(pktBuf, antMask, preTx_backoff_slots, preWait_postRxTimer1, preWait_postTxTimer1, postWait_postTxTimer2, phy_mode) \
                Xil_Out32(WLAN_MAC_REG_TX_CTRL_A_PARAMS, \
                    (((pktBuf)                & 0xF   )        | \
                    (((antMask)               & 0xF   ) <<  4) | \
                    (((preTx_backoff_slots)   & 0xFFFF) <<  8) | \
                    (((preWait_postRxTimer1)  & 0x1   ) << 24) | \
                    (((preWait_postTxTimer1)  & 0x1   ) << 25) | \
                    (((postWait_postTxTimer2) & 0x1   ) << 26) | \
                    (((phy_mode)              & 0x7   ) << 27)))

// TX_CTRL_A_GAINS
//     b[0:5]  : RFA Tx gain
//     b[6:11] : RFB Tx gain
//     b[12:17]: RFC Tx gain
//     b[18:23]: RFD Tx gain
//
#define wlan_mac_tx_ctrl_A_gains(rf_a, rf_b, rf_c, rf_d) \
                Xil_Out32(WLAN_MAC_REG_TX_CTRL_A_GAINS, \
                        (((rf_a) & 0x3F)        | \
                        (((rf_b) & 0x3F) <<  6) | \
                        (((rf_c) & 0x3F) << 12) | \
                        (((rf_d) & 0x3F) << 18)))

// TX_CTRL_B_PARAMS:
//     b[3:0]: Pkt buf
//     b[7:4]: Tx ant mask
//     b[8]: Pre-Wait for PostRx Timer 1
//     b[9]: Pre-Wait for PostRx Timer 2
//     b[10]: Pre-Wait for PostTx Timer 1
//     b[11]: Require NAV=0 at Tx time (otherwise skip Tx)
//     b[14:12]: PHY mode
//
#define wlan_mac_tx_ctrl_B_params(pktBuf, antMask, req_zeroNAV, preWait_postRxTimer1, preWait_postRxTimer2, preWait_postTxTimer1, phy_mode) \
                Xil_Out32(WLAN_MAC_REG_TX_CTRL_B_PARAMS, \
                    (((pktBuf)                & 0xF)        | \
                    (((antMask)               & 0xF) <<  4) | \
                    (((preWait_postRxTimer1)  & 0x1) <<  8) | \
                    (((preWait_postRxTimer2)  & 0x1) <<  9) | \
                    (((preWait_postTxTimer1)  & 0x1) << 10) | \
                    (((req_zeroNAV)           & 0x1) << 11) | \
                    (((phy_mode)              & 0x7) << 12)))

// TX_CTRL_B_GAINS
//     b[0:5]  : RFA Tx gain
//     b[6:11] : RFB Tx gain
//     b[12:17]: RFC Tx gain
//     b[18:23]: RFD Tx gain
//
#define wlan_mac_tx_ctrl_B_gains(rf_a, rf_b, rf_c, rf_d) \
                Xil_Out32(WLAN_MAC_REG_TX_CTRL_B_GAINS, \
                        (((rf_a) & 0x3F)        | \
                        (((rf_b) & 0x3F) <<  6) | \
                        (((rf_c) & 0x3F) << 12) | \
                        (((rf_d) & 0x3F) << 18)))

// TX_CTRL_C_PARAMS:
//     b[3:0]: Pkt buf
//     b[7:4]: Tx ant mask
//     b[8]: Require pre-Tx backoff
//     b[11:9]: PHY mode
//     b[27:12]: Num backoff slots
//
#define wlan_mac_tx_ctrl_C_params(pktBuf, antMask, req_backoff, phy_mode, num_slots) \
                Xil_Out32(WLAN_MAC_REG_TX_CTRL_C_PARAMS, \
                    (((pktBuf)           & 0xF)        | \
                    (((antMask)          & 0xF) <<  4) | \
                    (((req_backoff)      & 0x1) <<  8) | \
                    (((phy_mode)         & 0x7) <<  9) | \
                    (((num_slots)     & 0xFFFF) << 12)))

// TX_CTRL_C_GAINS
//     b[0:5]  : RFA Tx gain
//     b[6:11] : RFB Tx gain
//     b[12:17]: RFC Tx gain
//     b[18:23]: RFD Tx gain
//
#define wlan_mac_tx_ctrl_C_gains(rf_a, rf_b, rf_c, rf_d) \
                Xil_Out32(WLAN_MAC_REG_TX_CTRL_C_GAINS, \
                        (((rf_a) & 0x3F)        | \
                        (((rf_b) & 0x3F) <<  6) | \
                        (((rf_c) & 0x3F) << 12) | \
                        (((rf_d) & 0x3F) << 18)))

// TX_CTRL_D_PARAMS:
//     b[3:0]: Pkt buf
//     b[7:4]: Tx ant mask
//     b[8]: Require pre-Tx backoff
//     b[11:9]: PHY mode
//     b[27:12]: Num backoff slots
//
#define wlan_mac_tx_ctrl_D_params(pktBuf, antMask, req_backoff, phy_mode, num_slots) \
                Xil_Out32(WLAN_MAC_REG_TX_CTRL_D_PARAMS, \
                    (((pktBuf)           & 0xF)        | \
                    (((antMask)          & 0xF) <<  4) | \
                    (((req_backoff)      & 0x1) <<  8) | \
                    (((phy_mode)         & 0x7) <<  9) | \
                    (((num_slots)     & 0xFFFF) << 12)))

// TX_CTRL_D_GAINS
//     b[0:5]  : RFA Tx gain
//     b[6:11] : RFB Tx gain
//     b[12:17]: RFC Tx gain
//     b[18:23]: RFD Tx gain
//
#define wlan_mac_tx_ctrl_D_gains(rf_a, rf_b, rf_c, rf_d) \
                Xil_Out32(WLAN_MAC_REG_TX_CTRL_D_GAINS, \
                        (((rf_a) & 0x3F)        | \
                        (((rf_b) & 0x3F) <<  6) | \
                        (((rf_c) & 0x3F) << 12) | \
                        (((rf_d) & 0x3F) << 18)))

// TX_START
//     b[0]: Tx CTRL A Start
//     b[1]: Tx CTRL B Start
//     b[2]: Tx CTRL C Start
//     b[3]: Tx CTRL D Start
//
// NOTE:  Intrepret non-zero (x) as Tx start enable, zero (x) as Tx start disable
//     MAC core requires rising edge on either Tx start bit; software must set then clear for each Tx
//
#define wlan_mac_tx_ctrl_A_start(x) Xil_Out32(WLAN_MAC_REG_TX_START, ((Xil_In32(WLAN_MAC_REG_TX_START) & ~WLAN_MAC_START_REG_MASK_START_TX_A) | ((x) ? WLAN_MAC_START_REG_MASK_START_TX_A : 0x0)))
#define wlan_mac_tx_ctrl_B_start(x) Xil_Out32(WLAN_MAC_REG_TX_START, ((Xil_In32(WLAN_MAC_REG_TX_START) & ~WLAN_MAC_START_REG_MASK_START_TX_B) | ((x) ? WLAN_MAC_START_REG_MASK_START_TX_B : 0x0)))
#define wlan_mac_tx_ctrl_C_start(x) Xil_Out32(WLAN_MAC_REG_TX_START, ((Xil_In32(WLAN_MAC_REG_TX_START) & ~WLAN_MAC_START_REG_MASK_START_TX_C) | ((x) ? WLAN_MAC_START_REG_MASK_START_TX_C : 0x0)))
#define wlan_mac_tx_ctrl_D_start(x) Xil_Out32(WLAN_MAC_REG_TX_START, ((Xil_In32(WLAN_MAC_REG_TX_START) & ~WLAN_MAC_START_REG_MASK_START_TX_D) | ((x) ? WLAN_MAC_START_REG_MASK_START_TX_D : 0x0)))

// LATEST_RX_BYTE
//     b[15:0] : Last byte index
//     b[23:16]: Last byte
//
#define wlan_mac_get_last_byte_index() (Xil_In32(WLAN_MAC_REG_LATEST_RX_BYTE) & 0xFFFF)
#define wlan_mac_get_last_byte()      ((Xil_In32(WLAN_MAC_REG_LATEST_RX_BYTE) & 0xFF0000) >> 16)

// TX_A_BACKOFF_COUNTER
//     b[15:0]: A Backoff count
#define wlan_mac_get_backoff_count_A()  (Xil_In32(WLAN_MAC_REG_TX_A_BACKOFF_COUNTER) & 0x0000FFFF)

// TX_CD_BACKOFF_COUNTERS
//     b[15:0]: C Backoff count
//     b[31:16]: D Backoff count
#define wlan_mac_get_backoff_count_C()  (Xil_In32(WLAN_MAC_REG_TX_CD_BACKOFF_COUNTERS) & 0x0000FFFF)
#define wlan_mac_get_backoff_count_D() ((Xil_In32(WLAN_MAC_REG_TX_CD_BACKOFF_COUNTERS) & 0xFFFF0000) >> 16)

// RX_PHY_PARAMS Register:
//     b[15:0] : Length
//     b[22:16]: MCS
//     b[23]   : Unsupported
//     b[26:24]: Rx PHY Mode ([1,2,4] = [11a,11n,11ac])
//     b[27]   : Rx params valid
//     b[28]   : Rx PHY Sel (0=OFDM, 1=DSSS)
//     b[31:29]   : Rx Error
//
#define wlan_mac_get_rx_phy_hdr_params()    (Xil_In32(WLAN_MAC_REG_PHY_RX_PHY_HDR_PARAMS))
#define wlan_mac_get_rx_phy_length()        (Xil_In32(WLAN_MAC_REG_PHY_RX_PHY_HDR_PARAMS) & WLAN_MAC_PHY_RX_PHY_HDR_MASK_LENGTH)
#define wlan_mac_get_rx_phy_mcs()          ((Xil_In32(WLAN_MAC_REG_PHY_RX_PHY_HDR_PARAMS) & WLAN_MAC_PHY_RX_PHY_HDR_MASK_MCS) >> 16)
#define wlan_mac_get_rx_phy_sel()          ((Xil_In32(WLAN_MAC_REG_PHY_RX_PHY_HDR_PARAMS) & WLAN_MAC_PHY_RX_PHY_HDR_MASK_PHY_SEL))
#define wlan_mac_get_rx_phy_mode()         ((Xil_In32(WLAN_MAC_REG_PHY_RX_PHY_HDR_PARAMS) & WLAN_MAC_PHY_RX_PHY_HDR_MASK_PHY_MODE) >> 24)
#define wlan_mac_get_rx_error()            ((Xil_In32(WLAN_MAC_REG_PHY_RX_PHY_HDR_PARAMS) & WLAN_MAC_PHY_RX_PHY_HDR_MASK_RX_ERROR) >> 29)
#define wlan_mac_get_rx_phy_params_valid() ((Xil_In32(WLAN_MAC_REG_PHY_RX_PHY_HDR_PARAMS) & WLAN_MAC_PHY_RX_PHY_HDR_MASK_PARAMS_VALID))

// TXRX_TIMESTAMPS_FRAC register
// b[15:8]: Fractional part of RX_START microsecond timestamp
// b[ 7:0]: Fractional part of TX_START microsecond timestamp
#define wlan_mac_low_get_rx_start_timestamp_frac() ((Xil_In32(WLAN_MAC_REG_TXRX_TIMESTAMPS_FRAC) & 0xFF00) >> 8)
#define wlan_mac_low_get_tx_start_timestamp_frac()  (Xil_In32(WLAN_MAC_REG_TXRX_TIMESTAMPS_FRAC) & 0x00FF)

// TU Target register - UFix32_0 TU target
#define wlan_mac_get_tu_target()    (u64)(((u64)Xil_In32(WLAN_MAC_REG_TU_TARGET_MSB)<<32) + (u64)Xil_In32(WLAN_MAC_REG_TU_TARGET_LSB))

//-----------------------------------------------
// MAC Rx callback return defines
//
#define FRAME_RX_RET_STATUS_RECEIVED_PKT 		0x00000001     // b[0]
#define FRAME_RX_RET_STATUS_GOOD         		0x00000002     // b[1]
#define FRAME_RX_RET_ADDR_MATCH          		0x00000004     // b[2]
#define FRAME_RX_RET_CANCEL_TX          		0x00000008     // b[3]
#define FRAME_RX_RET_SKIP_RX_STARTED_RESET      0x00000010     // b[4]
#define FRAME_RX_RET_STATUS_TYPE         		0x0000FF00     // b[15:8]

#define FRAME_RX_RET_TYPE_DATA  (1 <<  8)
#define FRAME_RX_RET_TYPE_ACK   (1 <<  9)
#define FRAME_RX_RET_TYPE_CTS   (1 << 10)
#define FRAME_RX_RET_TYPE_OTHER (1 << 11)

//-----------------------------------------------
// WLAN Exp low parameter defines
//     NOTE:  Need to make sure that these values do not conflict with any of the LOW PARAM
//     callback defines
#define LOW_PARAM_BB_GAIN            0x00000001
#define LOW_PARAM_LINEARITY_PA       0x00000002
#define LOW_PARAM_LINEARITY_VGA      0x00000003
#define LOW_PARAM_LINEARITY_UPCONV   0x00000004
#define LOW_PARAM_AD_SCALING         0x00000005
#define LOW_PARAM_PKT_DET_MIN_POWER  0x00000006
#define LOW_PARAM_PHY_SAMPLE_RATE    0x00000008



/*************************** Function Prototypes *****************************/
int 			   wlan_mac_low_init(u32 type, compilation_details_t compilation_details);
void               wlan_mac_low_init_finish();
void 			   wlan_mac_low_send_status(u8 cpu_status_reason);
void 			   set_phy_samp_rate(phy_samp_rate_t phy_samp_rate);
void               wlan_mac_hw_init();
void               wlan_mac_low_init_hw_info(u32 type);

void        wlan_mac_low_send_exception(u32 reason);

u32         wlan_mac_low_poll_frame_rx();
int         wlan_mac_low_poll_ipc_rx();

void               wlan_mac_low_process_ipc_msg(wlan_ipc_msg_t * msg);
void               wlan_mac_low_frame_ipc_send();
void 			   wlan_mac_low_send_low_tx_details(u8 pkt_buf, wlan_mac_low_tx_details_t* low_tx_details);

void               wlan_mac_low_set_frame_rx_callback(function_ptr_t callback);
void 			   wlan_mac_low_set_beacon_txrx_config_callback(function_ptr_t callback);
void			   wlan_mac_low_set_mcast_buffer_enable_callback(function_ptr_t callback);
void 			   wlan_mac_low_set_mactime_change_callback(function_ptr_t callback);
void 			   wlan_mac_low_set_sample_rate_change_callback(function_ptr_t callback);
void 			   wlan_mac_low_set_handle_tx_pkt_buf_ready(function_ptr_t callback);
void               wlan_mac_low_set_ipc_low_param_callback(function_ptr_t callback);

u64         wlan_mac_low_get_rx_start_timestamp();
u64         wlan_mac_low_get_tx_start_timestamp();
u32         wlan_mac_low_get_active_channel();
s8          wlan_mac_low_get_current_ctrl_tx_pow();
u32         wlan_mac_low_get_current_rx_filter();
phy_samp_rate_t  wlan_mac_low_get_phy_samp_rate();

void               wlan_mac_low_set_nav_check_addr(u8* addr);
int                wlan_mac_low_set_radio_channel(u32 channel);
u32 			   wlan_mac_low_lock_tx_pkt_buf(u16 tx_pkt_buf);

void               wlan_mac_low_DSSS_rx_enable();
void               wlan_mac_low_DSSS_rx_disable();

#define			   PREPARE_FRAME_TRANSMIT_ERROR_INVALID_PKT_BUF				-1
#define			   PREPARE_FRAME_TRANSMIT_ERROR_UNEXPECTED_PKT_BUF_STATE	-2
#define 		   PREPARE_FRAME_TRANSMIT_ERROR_LOCK_FAIL					-3

int 			   wlan_mac_low_prepare_frame_transmit(u16 tx_pkt_buf);
int 			   wlan_mac_low_finish_frame_transmit(u16 tx_pkt_buf);



void        wlan_mac_low_lock_empty_rx_pkt_buf();

u32         wlan_mac_hw_rx_finish();

void        wlan_mac_reset_backoff_counter();
void        wlan_mac_reset_NAV_counter();

u8          wlan_mac_low_dbm_to_gain_target(s8 power);
u16         wlan_mac_low_mcs_to_n_dbps(u8 mcs, u8 phy_mode);
u8 		   wlan_mac_low_mcs_to_ctrl_resp_mcs(u8 mcs, u8 phy_mode);

void 	   wlan_mac_hw_clear_rx_started();

void 	   wlan_mac_set_tu_target(u64 tu_target);


#endif /* WLAN_MAC_LOW_H_ */
