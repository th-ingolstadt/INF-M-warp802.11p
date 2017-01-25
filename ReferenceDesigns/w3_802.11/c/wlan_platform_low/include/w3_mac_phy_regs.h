/** @file w3_mac_phy_regs.h
 *  @brief Platform abstraction header for CPU Low
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

#ifndef W3_MAC_PHY_REGS_H_
#define W3_MAC_PHY_REGS_H_

/***********************************************************************
 * Register renames
 *
 * The macros below rename the per-register macros defined in xparameters.h
 * for the System Generator cores in the 802.11 ref design. Each macro
 * below encodes the memory address (as seen by CPU Low) for each register
 * in the PHY Tx, PHY Rx and MAC Hw cores. The low-level MAC and PHY code
 * access these registers directly with Xil_In/Out32() in order to
 * minimize latency in time-sensitive polling/update loops in the MAC code.
 *
 ***********************************************************************/

// Rx PHY regiseters
#define WLAN_RX_REG_CTRL             XPAR_WLAN_PHY_RX_MEMMAP_CONTROL
#define WLAN_RX_REG_CFG              XPAR_WLAN_PHY_RX_MEMMAP_CONFIG
#define WLAN_RX_STATUS               XPAR_WLAN_PHY_RX_MEMMAP_STATUS
#define WLAN_RX_PKT_BUF_SEL          XPAR_WLAN_PHY_RX_MEMMAP_PKT_BUF_SEL
#define WLAN_RX_FEC_CFG              XPAR_WLAN_PHY_RX_MEMMAP_FEC_CONFIG
#define WLAN_RX_LTS_CFG              XPAR_WLAN_PHY_RX_MEMMAP_LTS_CORR_CONFIG
#define WLAN_RX_LTS_THRESH           XPAR_WLAN_PHY_RX_MEMMAP_LTS_CORR_THRESH
#define WLAN_RX_LTS_PEAKTYPE_THRESH  XPAR_WLAN_PHY_RX_MEMMAP_LTS_CORR_PEAKTYPE_THRESH
#define WLAN_RX_FFT_CFG              XPAR_WLAN_PHY_RX_MEMMAP_FFT_CONFIG
#define WLAN_RX_RSSI_THRESH          XPAR_WLAN_PHY_RX_MEMMAP_RSSI_THRESH
#define WLAN_RX_PKTDET_RSSI_CFG      XPAR_WLAN_PHY_RX_MEMMAP_PKTDET_RSSI_CONFIG
#define WLAN_RX_PHY_CCA_CFG          XPAR_WLAN_PHY_RX_MEMMAP_PHY_CCA_CONFIG
#define WLAN_RX_PKT_RSSI_AB          XPAR_WLAN_PHY_RX_MEMMAP_RX_PKT_RSSI_AB
#define WLAN_RX_PKT_RSSI_CD          XPAR_WLAN_PHY_RX_MEMMAP_RX_PKT_RSSI_CD
#define WLAN_RX_PKT_AGC_GAINS        XPAR_WLAN_PHY_RX_MEMMAP_RX_PKT_AGC_GAINS
#define WLAN_RX_DSSS_CFG             XPAR_WLAN_PHY_RX_MEMMAP_DSSS_RX_CONFIG
#define WLAN_RX_PKT_DET_OFDM_CFG     XPAR_WLAN_PHY_RX_MEMMAP_PKTDET_AUTOCORR_CONFIG
#define WLAN_RX_PKT_DET_DSSS_CFG     XPAR_WLAN_PHY_RX_MEMMAP_PKTDET_DSSS_CONFIG
#define WLAN_RX_PKT_BUF_MAXADDR      XPAR_WLAN_PHY_RX_MEMMAP_PKTBUF_MAX_WRITE_ADDR
#define WLAN_RX_CFO_EST_TIME_DOMAIN  XPAR_WLAN_PHY_RX_MEMMAP_CFO_EST_TIME_DOMAIN
#define WLAN_RX_CHAN_EST_SMOOTHING	 XPAR_WLAN_PHY_RX_MEMMAP_CHAN_EST_SMOOTHING

// Tx PHY registers
#define WLAN_TX_REG_STATUS           XPAR_WLAN_PHY_TX_MEMMAP_STATUS
#define WLAN_TX_REG_CFG              XPAR_WLAN_PHY_TX_MEMMAP_CONFIG
#define WLAN_TX_REG_PKT_BUF_SEL      XPAR_WLAN_PHY_TX_MEMMAP_PKT_BUF_SEL
#define WLAN_TX_REG_SCALING          XPAR_WLAN_PHY_TX_MEMMAP_OUTPUT_SCALING
#define WLAN_TX_REG_START            XPAR_WLAN_PHY_TX_MEMMAP_TX_START
#define WLAN_TX_REG_FFT_CFG          XPAR_WLAN_PHY_TX_MEMMAP_FFT_CONFIG
#define WLAN_TX_REG_TIMING           XPAR_WLAN_PHY_TX_MEMMAP_TIMING


// AGC registers
#define WLAN_AGC_REG_RESET           XPAR_WLAN_AGC_MEMMAP_RESET
#define WLAN_AGC_REG_TIMING_AGC      XPAR_WLAN_AGC_MEMMAP_TIMING_AGC
#define WLAN_AGC_REG_TIMING_DCO      XPAR_WLAN_AGC_MEMMAP_TIMING_DCO
#define WLAN_AGC_REG_TARGET          XPAR_WLAN_AGC_MEMMAP_TARGET
#define WLAN_AGC_REG_CONFIG          XPAR_WLAN_AGC_MEMMAP_CONFIG
#define WLAN_AGC_REG_RSSI_PWR_CALIB  XPAR_WLAN_AGC_MEMMAP_RSSI_PWR_CALIB
#define WLAN_AGC_REG_IIR_COEF_B0     XPAR_WLAN_AGC_MEMMAP_IIR_COEF_B0
#define WLAN_AGC_REG_IIR_COEF_A1     XPAR_WLAN_AGC_MEMMAP_IIR_COEF_A1
#define WLAN_AGC_TIMING_RESET        XPAR_WLAN_AGC_MEMMAP_TIMING_RESET

/***************************************************************
 * Register fields
 *
 * The macros below define the contents of various PHY/MAC registers.
 * These definitions must match the actual PHY/MAC hardware designs.
 ****************************************************************/

//-----------------------------------------------
// RX CONTROL
//
#define WLAN_RX_REG_CTRL_RESET                             0x00000001

//-----------------------------------------------
// RX CONFIG
//
#define WLAN_RX_REG_CFG_DSSS_RX_EN         0x00000001     // Enable DSSS Rx
#define WLAN_RX_REG_CFG_USE_TX_SIG_BLOCK   0x00000002     // Force I/Q/RSSI signals to zero during Tx
#define WLAN_RX_REG_CFG_PKT_BUF_WEN_SWAP   0x00000004     // Swap byte order at pkt buf interface
#define WLAN_RX_REG_CFG_CHAN_EST_WEN_SWAP  0x00000008     // Swap the order of H est writes per u64 ([0,1] vs [1,0])
#define WLAN_RX_REG_CFG_DSSS_RX_AGC_HOLD   0x00000010     // Allow active DSSS Rx to keep AGC locked
#define WLAN_RX_REG_CFG_CFO_EST_BYPASS     0x00000020     // Bypass time-domain CFO correction
#define WLAN_RX_REG_CFG_RECORD_CHAN_EST    0x00000040     // Enable recording channel estimates to the Rx pkt buffer
#define WLAN_RX_REG_CFG_SWITCHING_DIV_EN   0x00000080     // Enable switching diversity per-Rx
#define WLAN_RX_REG_CFG_DSSS_RX_REQ_AGC    0x00000100     // DSSS Rx requires AGC be locked first
#define WLAN_RX_REG_CFG_PKT_DET_EN_ANT_A   0x00000200     // Enable pkt detection on RF A
#define WLAN_RX_REG_CFG_PKT_DET_EN_ANT_B   0x00000400     // Enable pkt detection on RF B
#define WLAN_RX_REG_CFG_PKT_DET_EN_ANT_C   0x00000800     // Enable pkt detection on RF C
#define WLAN_RX_REG_CFG_PKT_DET_EN_ANT_D   0x00001000     // Enable pkt detection on RF D
#define WLAN_RX_REG_CFG_PKT_DET_EN_EXT     0x00002000     // Enable pkt detection via pkt_det_in port
#define WLAN_RX_REG_CFG_PHY_CCA_MODE_SEL   0x00004000     // Selects any(0) or all(1) antenna requirement for PHY CCA BUSY
#define WLAN_RX_REG_CFG_ANT_SEL_MASK       0x00018000     // Selects antenna for PHY input when sel div is disabled ([0,1,2,3] = RF[A,B,C,D])
#define WLAN_RX_REG_CFG_MAX_PKT_LEN_MASK   0x001E0000     // Sets max SIGNAL.LENGTH value in kB
#define WLAN_RX_REG_CFG_REQ_BOTH_PKT_DET   0x00200000     // Requires both auto_corr and RSSI pkt det assertion to start Rx
#define WLAN_RX_REG_CFG_BUSY_HOLD_PKT_DET  0x00400000     // Valid SIGNAL holds pkt det for rate*lengh duration, even if unsupported
#define WLAN_RX_REG_CFG_DSSS_ASSERTS_CCA   0x00800000     // DSSS active holds CCA busy
#define WLAN_RX_REG_CFG_ENABLE_HTMF_DET	   0x01000000     // Enables 11n Rx support; when disabled all Rx are processed as 11a waveforms

//-----------------------------------------------
// RX STATUS
//
#define WLAN_RX_REG_STATUS_OFDM_FCS_GOOD        	 0x00000001
#define WLAN_RX_REG_STATUS_DSSS_FCS_GOOD        	 0x00000002
#define WLAN_RX_REG_STATUS_ACTIVE_ANT_MASK           0x0000000C // 2-bits: [0,1,2,3] = RF[A,B,C,D]
#define WLAN_RX_REG_STATUS_OFDM_PKT_DET_STATUS_MASK  0x000001F0 // 5 bits: [ext, RF D/C/B/A]
#define WLAN_RX_REG_STATUS_DSSS_PKT_DET_STATUS_MASK  0x00001E00 // 4 bits: [RF D/C/B/A]

//-----------------------------------------------
// TX CONFIG
//
#define WLAN_TX_REG_CFG_SET_RC_RXEN               0x00000001
#define WLAN_TX_REG_CFG_RESET_SCRAMBLING_PER_PKT  0x00000002
#define WLAN_TX_REG_CFG_ANT_A_TXEN                0x00000004
#define WLAN_TX_REG_CFG_ANT_B_TXEN                0x00000008
#define WLAN_TX_REG_CFG_ANT_C_TXEN                0x00000010
#define WLAN_TX_REG_CFG_ANT_D_TXEN                0x00000020
#define WLAN_TX_REG_CFG_USE_MAC_ANT_MASKS         0x00000040
#define WLAN_TX_REG_CFG_DELAY_DBG_TX_RUNNING      0x00000080
#define WLAN_TX_REG_CFG_PHY_MODE_SW_TX            0x00007000
#define WLAN_TX_REG_CFG_RESET                     0x80000000

//-----------------------------------------------
// TX STATUS
//
#define WLAN_TX_REG_STATUS_TX_RUNNING  0x00000001

//-----------------------------------------------
// TX START
//
#define WLAN_TX_REG_START_DIRECT  0x00000001
#define WLAN_TX_REG_START_VIA_RC  0x00000002


#endif /* W3_MAC_PHY_REGS_H_ */
