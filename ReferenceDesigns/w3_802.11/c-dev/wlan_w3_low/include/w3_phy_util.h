/** @file wlan_phy_util.h
 *  @brief Physical Layer Utility
 *
 *  This contains code for configuring low-level parameters in the PHY and hardware.
 *
 *  @copyright Copyright 2013-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *                See LICENSE.txt included in the design archive or
 *                at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/*************************** Constant Definitions ****************************/
#ifndef W3_PHY_UTIL_H_
#define W3_PHY_UTIL_H_

#include "xil_types.h"

//Forward declarations
enum phy_samp_rate_t;

// ****************************************************************************
// Define standard macros for base addresses and device IDs
//     XPAR_ names will change with instance names in hardware
//
#define CLK_BASEADDR         XPAR_W3_CLOCK_CONTROLLER_BASEADDR
#define DRAM_BASEADDR        XPAR_DDR3_2GB_SODIMM_MPMC_BASEADDR
#define RC_BASEADDR          XPAR_RADIO_CONTROLLER_BASEADDR
#define AD_BASEADDR          XPAR_W3_AD_CONTROLLER_BASEADDR

// ****************************************************************************
// Timing Constants
// Empirically measured a 2.2usec latency from RC PHY start to observable waveform
#define TX_RC_PHYSTART_DLY 0

#define TX_PHY_DLY_100NSEC (((TX_RC_PHYSTART_DLY)/4) + 22)

// RX PHY-level constants
#define PHY_RX_RSSI_SUM_LEN       4
#define PHY_RX_RSSI_SUM_LEN_BITS  2 // LOG2(PHY_RX_RSSI_SUM_LEN)

// ****************************************************************************
// RATE field values for SIGNAL/L-SIG in PHY preamble (IEEE 802.11-2012 18.3.4.2)
//  DSSS 1M rate code is non-standard, used by our code to indicate DSSS Rx
#define WLAN_PHY_RATE_DSSS_1M  0x1

// ****************************************************************************
// Data bytes per OFDM symbol
//     NOTE:  Values from Table 17-3 of 2007 IEEE 802.11
//
#define N_DBPS_R6    24
#define N_DBPS_R9    36
#define N_DBPS_R12   48
#define N_DBPS_R18   72
#define N_DBPS_R24   96
#define N_DBPS_R36  144
#define N_DBPS_R48  192
#define N_DBPS_R54  216


// ****************************************************************************
// Currently active antenna defines
//
#define RX_ACTIVE_ANTA  0x0
#define RX_ACTIVE_ANTB  0x1
#define RX_ACTIVE_ANTC  0x2
#define RX_ACTIVE_ANTD  0x3

// ****************************************************************************
// PHY Register definitions
//





/**************************** Macro Definitions ******************************/

#define REG_CLEAR_BITS(addr, mask) Xil_Out32(addr, (Xil_In32(addr) & ~(mask)))
#define REG_SET_BITS(addr, mask)   Xil_Out32(addr, (Xil_In32(addr) | (mask)))


// Macros to calculate the SIGNAL / L-SIG field in the PHY preamble (IEEE 802.11-2012 18.3.4)
#define WLAN_TX_SIGNAL_CALC(rate, length) (((rate) & 0xF) | (((length) & 0xFFF) << 5) | (WLAN_TX_SIGNAL_PARITY_CALC(rate,length)))
#define WLAN_TX_SIGNAL_PARITY_CALC(rate, length) ((0x1 & (ones_in_chars[(rate)] + ones_in_chars[(length) & 0xFF] + ones_in_chars[(length) >> 8])) << 17)

//-----------------------------------------------
// PHY Macros
//
#define wlan_phy_select_rx_antenna(d) Xil_Out32(WLAN_RX_REG_CFG, ((Xil_In32(WLAN_RX_REG_CFG) & ~WLAN_RX_REG_CFG_ANT_SEL_MASK) | (((d) & 0x3) << 15)))

#define wlan_phy_enable_req_both_pkt_det()  Xil_Out32(WLAN_RX_REG_CFG, (Xil_In32(WLAN_RX_REG_CFG) | (WLAN_RX_REG_CFG_REQ_BOTH_PKT_DET_OFDM | WLAN_RX_REG_CFG_REQ_BOTH_PKT_DET_DSSS)))
#define wlan_phy_disable_req_both_pkt_det() Xil_Out32(WLAN_RX_REG_CFG, (Xil_In32(WLAN_RX_REG_CFG) & ~(WLAN_RX_REG_CFG_REQ_BOTH_PKT_DET_OFDM | WLAN_RX_REG_CFG_REQ_BOTH_PKT_DET_DSSS)))

#define wlan_phy_rx_set_max_pkt_len_kB(d) Xil_Out32(WLAN_RX_REG_CFG, (Xil_In32(WLAN_RX_REG_CFG) & ~WLAN_RX_REG_CFG_MAX_PKT_LEN_MASK) | (((d) << 17) & WLAN_RX_REG_CFG_MAX_PKT_LEN_MASK))
#define wlan_phy_tx_set_max_pkt_len_kB(d) Xil_Out32(WLAN_TX_REG_CFG, (Xil_In32(WLAN_TX_REG_CFG) & ~WLAN_TX_REG_CFG_MAX_PKT_LEN_MASK) | (((d) << 8) & WLAN_TX_REG_CFG_MAX_PKT_LEN_MASK))

#define wlan_phy_rx_set_max_pktbuf_addr(a) Xil_Out32(WLAN_RX_PKT_BUF_MAXADDR, (a))

// The PHY header offsets deal in units of u64 words, so the << 13 is like a << 16 and >> 3 to convert u8 words to u64 words
#define wlan_phy_rx_pkt_buf_phy_hdr_offset(d) Xil_Out32(WLAN_RX_PKT_BUF_SEL, ((Xil_In32(WLAN_RX_PKT_BUF_SEL) & (~0x00FF0000)) | (((d) << 13) & 0x00FF0000)))
#define wlan_phy_tx_pkt_buf_phy_hdr_offset(d) Xil_Out32(WLAN_TX_REG_PKT_BUF_SEL, ((Xil_In32(WLAN_TX_REG_PKT_BUF_SEL) & (~0x00FF0000)) | (((d) << 13) & 0x00FF0000)))

// Chan est offset is specified in units of u64 words; this macros converts a byte offset to u64 offset (hence the implicit >> 3, ie the 21 is (24 - 3))
#define wlan_phy_rx_pkt_buf_h_est_offset(d) Xil_Out32(WLAN_RX_PKT_BUF_SEL, ((Xil_In32(WLAN_RX_PKT_BUF_SEL) & (~0xFF000000)) | (((d) << 21) & 0xFF000000)))

#define wlan_phy_tx_set_scaling(pre, pay) Xil_Out32(WLAN_TX_REG_SCALING, (((pre) & 0xFFFF) | (((pay) & 0xFFFF) << 16)))

#define wlan_phy_rx_pkt_buf_dsss(d) Xil_Out32(WLAN_RX_PKT_BUF_SEL, ((Xil_In32(WLAN_RX_PKT_BUF_SEL) & (~0x00000F00)) | (((d) << 8) & 0x00000F00)))
#define wlan_phy_rx_pkt_buf_ofdm(d) Xil_Out32(WLAN_RX_PKT_BUF_SEL, ((Xil_In32(WLAN_RX_PKT_BUF_SEL) & (~0x0000000F)) | ((d) & 0x0000000F)))

#define wlan_phy_tx_pkt_buf(d) Xil_Out32(WLAN_TX_REG_PKT_BUF_SEL, ((Xil_In32(WLAN_TX_REG_PKT_BUF_SEL) & (~0x0000000F)) | ((d) & 0x0000000F)))

#define wlan_phy_rx_get_active_rx_ant() ((Xil_In32(WLAN_RX_STATUS) & WLAN_RX_REG_STATUS_ACTIVE_ANT_MASK) >> 2)

#define wlan_phy_rx_get_pkt_det_status() ((Xil_In32(WLAN_RX_STATUS) & WLAN_RX_REG_STATUS_PKT_DET_STATUS_MASK) >> 4)

#define wlan_phy_rx_set_fec_scaling(sc_bpsk, sc_qpsk, sc_16qam, sc_64qam) Xil_Out32(WLAN_RX_FEC_CFG, \
		 ((sc_bpsk) & 0x1F) | \
		(((sc_qpsk) & 0x1F) << 5) | \
		(((sc_16qam) & 0x1F) << 10) | \
		(((sc_64qam) & 0x1F) << 15))

// WLAN_RX_FFT_CFG register fields:
//     [ 7: 0] - Number of subcarriers (MUST BE 64 - OTHER VALUES UNTESTED)
//     [15: 8] - Cyclic prefix length (MUST BE 16 - OTHER VALUES UNTESTED)
//     [23:16] - FFT window offset - number of samples of CP to use on average (must be in [0,CP_LENGTH))
//     [31:24] - FFT scaling - UFix6_0 value; see Xilinx FFT datasheet for scaling details
//
#define wlan_phy_rx_set_fft_window_offset(d)   Xil_Out32(WLAN_RX_FFT_CFG, ((Xil_In32(WLAN_RX_FFT_CFG) & 0xFF00FFFF) | (((d) & 0xFF) << 16)))
#define wlan_phy_rx_set_fft_scaling(d)         Xil_Out32(WLAN_RX_FFT_CFG, ((Xil_In32(WLAN_RX_FFT_CFG) & 0x00FFFFFF) | (((d) & 0xFF) << 24)))
#define wlan_phy_rx_config_fft(num_sc, cp_len) Xil_Out32(WLAN_RX_FFT_CFG, ((Xil_In32(WLAN_RX_FFT_CFG) & 0xFFFF0000) | ( (((num_sc) & 0xFF) << 0))) | (((cp_len) & 0xFF) << 8))

#define wlan_phy_tx_config_fft(scaling, num_sc, cp_len) Xil_Out32(WLAN_TX_REG_FFT_CFG, \
		(((scaling) & 0x3F) << 24) | \
		(((cp_len) & 0xFF)  <<  8) | \
		((num_sc) & 0xFF))

#ifdef WLAN_RX_PKT_RSSI_AB
// RSSI register files:
//     WLAN_RX_PKT_RSSI_AB:
//         [15: 0] - RFA
//         [31:16] - RFB
//     WLAN_RX_PKT_RSSI_CD:
//         [15: 0] - RFC
//         [31:16] - RFD
//
// NOTE: The final << 1 accounts for the fact that this register actually returns the summed RSSI divided by 2
//
#define wlan_phy_rx_get_pkt_rssi(ant) ((((ant) == 0) ?  (Xil_In32(WLAN_RX_PKT_RSSI_AB)        & 0xFFFF) : \
                                        ((ant) == 1) ? ((Xil_In32(WLAN_RX_PKT_RSSI_AB) >> 16) & 0xFFFF) : \
                                        ((ant) == 2) ?  (Xil_In32(WLAN_RX_PKT_RSSI_CD)        & 0xFFFF) : \
                                       ((Xil_In32(WLAN_RX_PKT_RSSI_CD) >> 16) & 0xFFFF)) << 1)

// AGC gains register fields:
//     [ 4: 0] - RF A BBG
//     [ 6: 5] - RF A RFG
//         [7] - 0
//     [12: 8] - RF B BBG
//     [14:13] - RF B RFG
//        [15] - 0
//     [20:16] - RF C BBG
//     [22:21] - RF C RFG
//        [23] - 0
//     [28:24] - RF D BBG
//     [30:29] - RF D RFG
//        [31] - 0
//
#define wlan_phy_rx_get_agc_RFG(ant) ((((ant) == 0) ? (Xil_In32(WLAN_RX_PKT_AGC_GAINS) >>  5) : \
                                       ((ant) == 1) ? (Xil_In32(WLAN_RX_PKT_AGC_GAINS) >> 13) : \
                                       ((ant) == 2) ? (Xil_In32(WLAN_RX_PKT_AGC_GAINS) >> 21) : \
                                       (Xil_In32(WLAN_RX_PKT_AGC_GAINS) >> 29)) & 0x3)

#define wlan_phy_rx_get_agc_BBG(ant) ((((ant) == 0) ? (Xil_In32(WLAN_RX_PKT_AGC_GAINS) >>  0) : \
                                       ((ant) == 1) ? (Xil_In32(WLAN_RX_PKT_AGC_GAINS) >>  8) : \
                                       ((ant) == 2) ? (Xil_In32(WLAN_RX_PKT_AGC_GAINS) >> 16) : \
                                       (Xil_In32(WLAN_RX_PKT_AGC_GAINS) >> 24)) & 0x1F)
#else
int wlan_phy_rx_get_pkt_rssi(u8 ant);
int wlan_phy_rx_get_agc_BBG(u8 ant);

//#define wlan_phy_rx_get_pkt_rssi(ant) (0)
//#define wlan_phy_rx_get_agc_BBG(ant)  (0)
#define wlan_phy_rx_get_agc_RFG(ant)  (0)
#endif

#define wlan_phy_DSSS_rx_enable()  Xil_Out32(WLAN_RX_REG_CFG, Xil_In32(WLAN_RX_REG_CFG) | WLAN_RX_REG_CFG_DSSS_RX_EN)
#define wlan_phy_DSSS_rx_disable() Xil_Out32(WLAN_RX_REG_CFG, Xil_In32(WLAN_RX_REG_CFG) & ~WLAN_RX_REG_CFG_DSSS_RX_EN)

// Rx PHY captures time-domain CFO est
// Fix20_21 value, sign extended to Fix32_31 in this register
#define wlan_phy_rx_get_cfo_est() Xil_In32(WLAN_RX_CFO_EST_TIME_DOMAIN)

#define wlan_phy_rx_pktDet_RSSI_cfg(sum_len, sum_thresh, min_dur) \
    Xil_Out32(WLAN_RX_PKTDET_RSSI_CFG, (((sum_len) & 0x1F) | (((sum_thresh) & 0x7FFF) << 5) | (((min_dur) & 0x1F) << 20)))

// WLAN_RX_DSSS_CFG register fields:
//     [ 7: 0] - UFix8_0 SYNC matching score thresh
//     [15: 8] - UFix8_0 SYNC matching timeout (samples, multiplied by 32x in hw)
//     [23:16] - UFix8_0 SFD matching timeout (samples, multiplied by 32x in hw)
//     [31:24] - UFix8_0 SYNC search time (samples)
#define wlan_phy_DSSS_rx_config(thresh, sync_timeout, sfd_timeout, search_time) \
    Xil_Out32(WLAN_RX_DSSS_CFG, (((thresh) & 0xFF) | (((sync_timeout) & 0xFF) << 8) | (((sfd_timeout) & 0xFF) << 16) | (((search_time) & 0xFF) << 24)))

// WLAN_RX_PKT_DET_DSSS_CFG register fields:
//     [ 7: 0] - Correlation threshold as UFix8_6
//     [17: 8] - Energy threshold as UFix10_0
//
#define wlan_phy_rx_pktDet_autoCorr_dsss_cfg(corr_thresh, energy_thresh) \
    Xil_Out32(WLAN_RX_PKT_DET_DSSS_CFG, (((corr_thresh) & 0xFF) | (((energy_thresh) & 0x3FF) << 8)))

// WLAN_RX_PKT_DET_OFDM_CFG register fields:
//     [ 7: 0] - Correlation threshold as UFix8_8
//     [21: 8] - Energy threshold as UFix14_8
//     [25:22] - Minimum duration (also used by DSSS det)
//     [31:26] - Post detection reset block (also used by DSSS det)
//
#define wlan_phy_rx_pktDet_autoCorr_ofdm_cfg(corr_thresh, energy_thresh, min_dur, post_wait) \
    Xil_Out32(WLAN_RX_PKT_DET_OFDM_CFG, (((corr_thresh) & 0xFF) | (((energy_thresh) & 0x3FFF) << 8) | (((min_dur) & 0xF) << 22) | (((post_wait) & 0x3F) << 26)))

#define wlan_phy_rx_lts_corr_thresholds(corr_thresh_low_snr, corr_thresh_high_snr) \
    Xil_Out32(WLAN_RX_LTS_THRESH, ((corr_thresh_low_snr) & 0xFFFF) | (((corr_thresh_high_snr) & 0xFFFF) << 16))


#define wlan_phy_rx_lts_corr_peaktype_thresholds(thresh_low_snr, thresh_high_snr) \
    Xil_Out32(WLAN_RX_LTS_PEAKTYPE_THRESH, ((thresh_low_snr) & 0xFFFF) | (((thresh_high_snr) & 0xFFFF) << 16))

#define wlan_phy_rx_lts_corr_config(snr_thresh, corr_timeout, dly_mask) Xil_Out32(WLAN_RX_LTS_CFG, ((((dly_mask) & 0x7) << 24) | ((corr_timeout) & 0xFF) | (((snr_thresh) & 0xFFFF) << 8)))

#define wlan_phy_rx_chan_est_smoothing(coef_a, coef_b) \
	Xil_Out32(WLAN_RX_CHAN_EST_SMOOTHING, ((Xil_In32(WLAN_RX_CHAN_EST_SMOOTHING) & 0xFF000000) | \
			(((coef_b) & 0xFFF) << 12) | ((coef_a) & 0xFFF)))

#define wlan_phy_rx_phy_mode_det_thresh(thresh) \
		Xil_Out32(WLAN_RX_CHAN_EST_SMOOTHING, ((Xil_In32(WLAN_RX_CHAN_EST_SMOOTHING) & 0xC0FFFFFF) | \
			(((thresh) & 0x3F) << 24)))

//Tx PHY TIMING register:
// [ 9: 0] Tx extension (time from last sample to TX_DONE)
// [19:10] TxEn extension (time from last sample to de-assertion of radio TXEN)
// [29:20] Rx invalid extension (time from last sample to de-assertion of RX_SIG_INVALID output)
#define wlan_phy_tx_set_extension(d)            Xil_Out32(WLAN_TX_REG_TIMING, ( (Xil_In32(WLAN_TX_REG_TIMING) & 0xFFFFFC00) | ((d) & 0x3FF)))
#define wlan_phy_tx_set_txen_extension(d)       Xil_Out32(WLAN_TX_REG_TIMING, ( (Xil_In32(WLAN_TX_REG_TIMING) & 0xFFF003FF) | (((d) & 0x3FF) << 10)))
#define wlan_phy_tx_set_rx_invalid_extension(d) Xil_Out32(WLAN_TX_REG_TIMING, ( (Xil_In32(WLAN_TX_REG_TIMING) & 0xC00FFFFF) | (((d) & 0x3FF) << 20)))

#define wlan_phy_rx_set_cca_thresh(d) Xil_Out32(WLAN_RX_PHY_CCA_CFG, ((Xil_In32(WLAN_RX_PHY_CCA_CFG) & 0xFFFF0000) | ((d) & 0xFFFF)))
#define wlan_phy_rx_set_extension(d)  Xil_Out32(WLAN_RX_PHY_CCA_CFG, ((Xil_In32(WLAN_RX_PHY_CCA_CFG) & 0xFF00FFFF) | (((d) << 16) & 0xFF0000)))

//Software-set packet buffer index only used for Tx events triggered via register writes
// Normal operation uses hardware-triggered Tx via the wlan_mac_hw core
#define wlan_tx_buffer_sel(n)  Xil_Out32(WLAN_TX_REG_PKT_BUF_SEL, ((Xil_In32(WLAN_TX_REG_PKT_BUF_SEL) & ~0xF) | ((n) & 0xF)) )

//Check if PHY Tx core is active - debug only, use wlan_mac_hw status register to ensure consistent MAC/PHY state
#define wlan_tx_isrunning() (Xil_In32(WLAN_TX_REG_STATUS) & WLAN_TX_REG_STATUS_TX_RUNNING))

//-----------------------------------------------
// AGC Macros
//
#define wlan_agc_set_AGC_timing(capt_rssi_1, capt_rssi_2, capt_v_db, agc_done) \
    Xil_Out32(WLAN_AGC_REG_TIMING_AGC, (((capt_rssi_1) & 0xFF) | ( ((capt_rssi_2) & 0xFF) << 8) | \
                                        (((capt_v_db) & 0xFF) << 16) | ( ((agc_done) & 0xFF) << 24)))

#define wlan_agc_set_DCO_timing(start_dco, en_iir_filt) Xil_Out32(WLAN_AGC_REG_TIMING_DCO, ((start_dco) & 0xFF) | ( ((en_iir_filt) & 0xFF)<<8))

#define wlan_agc_set_target(target_pwr) Xil_Out32(WLAN_AGC_REG_TARGET, ((target_pwr) & 0x3F))

#define wlan_agc_set_config(thresh32, thresh21, avg_len, v_db_adj, init_g_bb) \
		 Xil_Out32(WLAN_AGC_REG_CONFIG, (Xil_In32(WLAN_AGC_REG_CONFIG) & 0xE0000000) | \
									(((thresh32)  & 0xFF) <<  0) | \
                                    (((thresh21)  & 0xFF) <<  8) | \
                                    (((avg_len)   & 0x03) << 16) | \
                                    (((v_db_adj)  & 0x3F) << 18) | \
                                    (((init_g_bb) & 0x1F) << 24))

#define wlan_agc_set_rxhp_mode(m) Xil_Out32(WLAN_AGC_REG_CONFIG, (Xil_In32(WLAN_AGC_REG_CONFIG) & 0x1FFFFFFF) | ((m) ? WLAN_AGC_CONFIG_MASK_RXHP_MODE : 0))

#define wlan_agc_set_RSSI_pwr_calib(g3, g2, g1) Xil_Out32(WLAN_AGC_REG_RSSI_PWR_CALIB, (((g3) & 0xFF) | (((g2) & 0xFF) << 8) | (((g1) & 0xFF) << 16)))

#define wlan_agc_set_reset_timing(rxhp,g_rf, g_bb) Xil_Out32(WLAN_AGC_TIMING_RESET, (((rxhp) & 0xFF) | (((g_rf) & 0xFF) << 8) | (((g_bb) & 0xFF) << 16)))

/*********************** Global Variable Definitions *************************/

extern const u8    ones_in_chars[256];


/*************************** Function Prototypes *****************************/

// Initialization commands
int  w3_node_init();
void wlan_phy_init();
void wlan_radio_init();

// Configuration commands
void wlan_rx_config_ant_mode(u32 ant_mode);

// PHY commands
void write_phy_preamble(u8 pkt_buf, u8 phy_mode, u8 mcs, u16 length);

// TX debug commands
void wlan_tx_start();

// Calculate transmit times
u16 wlan_ofdm_calc_txtime(u16 length, u8 mcs, u8 phy_mode, enum phy_samp_rate_t phy_samp_rate);
u16 wlan_ofdm_calc_num_payload_syms(u16 length, u8 mcs, u8 phy_mode);

#endif /* W3_PHY_UTIL_H_ */
