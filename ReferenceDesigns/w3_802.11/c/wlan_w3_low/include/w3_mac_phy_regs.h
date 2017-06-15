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

/********************************************************************************
 * Register definitions for wlan_phy_rx core
********************************************************************************/
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
#define WLAN_RX_REG_CFG_ENABLE_VHT_DET     0x02000000     // Enables VHT phy_mode detecection; when disabled VHT waveforms are detected as NONHT

//-----------------------------------------------
// RX STATUS
//
#define WLAN_RX_REG_STATUS_OFDM_FCS_GOOD        	 0x00000001
#define WLAN_RX_REG_STATUS_DSSS_FCS_GOOD        	 0x00000002
#define WLAN_RX_REG_STATUS_ACTIVE_ANT_MASK           0x0000000C // 2-bits: [0,1,2,3] = RF[A,B,C,D]
#define WLAN_RX_REG_STATUS_OFDM_PKT_DET_STATUS_MASK  0x000001F0 // 5 bits: [ext, RF D/C/B/A]
#define WLAN_RX_REG_STATUS_DSSS_PKT_DET_STATUS_MASK  0x00001E00 // 4 bits: [RF D/C/B/A]


/********************************************************************************
 * Register definitions for wlan_phy_tx core
********************************************************************************/
#define WLAN_TX_REG_STATUS           XPAR_WLAN_PHY_TX_MEMMAP_STATUS
#define WLAN_TX_REG_CFG              XPAR_WLAN_PHY_TX_MEMMAP_CONFIG
#define WLAN_TX_REG_PKT_BUF_SEL      XPAR_WLAN_PHY_TX_MEMMAP_PKT_BUF_SEL
#define WLAN_TX_REG_SCALING          XPAR_WLAN_PHY_TX_MEMMAP_OUTPUT_SCALING
#define WLAN_TX_REG_START            XPAR_WLAN_PHY_TX_MEMMAP_TX_START
#define WLAN_TX_REG_FFT_CFG          XPAR_WLAN_PHY_TX_MEMMAP_FFT_CONFIG
#define WLAN_TX_REG_TIMING           XPAR_WLAN_PHY_TX_MEMMAP_TIMING

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


/********************************************************************************
 * Register definitions for wlan_mac_hw core
********************************************************************************/

// RO:
#define WLAN_MAC_REG_STATUS                                XPAR_WLAN_MAC_HW_MEMMAP_STATUS
#define WLAN_MAC_REG_LATEST_RX_BYTE                        XPAR_WLAN_MAC_HW_MEMMAP_LATEST_RX_BYTE
#define WLAN_MAC_REG_PHY_RX_PHY_HDR_PARAMS                 XPAR_WLAN_MAC_HW_MEMMAP_PHY_RX_PARAMS
#define WLAN_MAC_REG_TX_A_BACKOFF_COUNTER                  XPAR_WLAN_MAC_HW_MEMMAP_TX_A_BACKOFF_COUNTER
#define WLAN_MAC_REG_TX_CD_BACKOFF_COUNTERS                XPAR_WLAN_MAC_HW_MEMMAP_TX_CD_BACKOFF_COUNTERS
#define WLAN_MAC_REG_RX_TIMESTAMP_LSB                      XPAR_WLAN_MAC_HW_MEMMAP_RX_START_TIMESTAMP_LSB
#define WLAN_MAC_REG_RX_TIMESTAMP_MSB                      XPAR_WLAN_MAC_HW_MEMMAP_RX_START_TIMESTAMP_MSB
#define WLAN_MAC_REG_TX_TIMESTAMP_LSB                      XPAR_WLAN_MAC_HW_MEMMAP_TX_START_TIMESTAMP_LSB
#define WLAN_MAC_REG_TX_TIMESTAMP_MSB                      XPAR_WLAN_MAC_HW_MEMMAP_TX_START_TIMESTAMP_MSB
#define WLAN_MAC_REG_TXRX_TIMESTAMPS_FRAC                  XPAR_WLAN_MAC_HW_MEMMAP_TXRX_START_TIMESTAMPS_FRAC
#define WLAN_MAC_REG_NAV_VALUE                             XPAR_WLAN_MAC_HW_MEMMAP_NAV_VALUE
#define WLAN_MAC_REG_TX_CTRL_STATUS						   XPAR_WLAN_MAC_HW_MEMMAP_TX_CTRL_STATUS

// RW:
#define WLAN_MAC_REG_TX_START                              XPAR_WLAN_MAC_HW_MEMMAP_TX_START
#define WLAN_MAC_REG_CALIB_TIMES                           XPAR_WLAN_MAC_HW_MEMMAP_CALIB_TIMES
#define WLAN_MAC_REG_IFS_1                                 XPAR_WLAN_MAC_HW_MEMMAP_IFS_INTERVALS1
#define WLAN_MAC_REG_IFS_2                                 XPAR_WLAN_MAC_HW_MEMMAP_IFS_INTERVALS2
#define WLAN_MAC_REG_CONTROL                               XPAR_WLAN_MAC_HW_MEMMAP_CONTROL
#define WLAN_MAC_REG_SW_BACKOFF_CTRL                       XPAR_WLAN_MAC_HW_MEMMAP_BACKOFF_CTRL
#define WLAN_MAC_REG_TX_CTRL_A_PARAMS                      XPAR_WLAN_MAC_HW_MEMMAP_TX_CTRL_A_PARAMS
#define WLAN_MAC_REG_TX_CTRL_A_GAINS                       XPAR_WLAN_MAC_HW_MEMMAP_TX_CTRL_A_GAINS
#define WLAN_MAC_REG_TX_CTRL_B_PARAMS                      XPAR_WLAN_MAC_HW_MEMMAP_TX_CTRL_B_PARAMS
#define WLAN_MAC_REG_TX_CTRL_B_GAINS                       XPAR_WLAN_MAC_HW_MEMMAP_TX_CTRL_B_GAINS
#define WLAN_MAC_REG_TX_CTRL_C_PARAMS                      XPAR_WLAN_MAC_HW_MEMMAP_TX_CTRL_C_PARAMS
#define WLAN_MAC_REG_TX_CTRL_C_GAINS                       XPAR_WLAN_MAC_HW_MEMMAP_TX_CTRL_C_GAINS
#define WLAN_MAC_REG_TX_CTRL_D_PARAMS                      XPAR_WLAN_MAC_HW_MEMMAP_TX_CTRL_D_PARAMS
#define WLAN_MAC_REG_TX_CTRL_D_GAINS                       XPAR_WLAN_MAC_HW_MEMMAP_TX_CTRL_D_GAINS
#define WLAN_MAC_REG_POST_TX_TIMERS                        XPAR_WLAN_MAC_HW_MEMMAP_POST_TX_TIMERS
#define WLAN_MAC_REG_POST_RX_TIMERS                        XPAR_WLAN_MAC_HW_MEMMAP_POST_RX_TIMERS
#define WLAN_MAC_REG_NAV_CHECK_ADDR_1                      XPAR_WLAN_MAC_HW_MEMMAP_NAV_MATCH_ADDR_1
#define WLAN_MAC_REG_NAV_CHECK_ADDR_2                      XPAR_WLAN_MAC_HW_MEMMAP_NAV_MATCH_ADDR_2
#define WLAN_MAC_REG_TU_TARGET_LSB                         XPAR_WLAN_MAC_HW_MEMMAP_TU_TARGET_LSB
#define WLAN_MAC_REG_TU_TARGET_MSB                         XPAR_WLAN_MAC_HW_MEMMAP_TU_TARGET_MSB

//-----------------------------------------------
// WLAN MAC HW - Tx/Rx timer bit masks / macros
//
#define WLAN_MAC_POST_TX_TIMERS_MASK_TIMER1_COUNTTO        0x00007FFF
#define WLAN_MAC_POST_TX_TIMERS_MASK_TIMER1_EN             0x00008000
#define WLAN_MAC_POST_TX_TIMERS_MASK_TIMER2_COUNTTO        0x7FFF0000
#define WLAN_MAC_POST_TX_TIMERS_MASK_TIMER2_EN             0x80000000

#define WLAN_MAC_POST_RX_TIMERS_MASK_TIMER1_COUNTTO        0x00007FFF
#define WLAN_MAC_POST_RX_TIMERS_MASK_TIMER1_EN             0x00008000
#define WLAN_MAC_POST_RX_TIMERS_MASK_TIMER2_COUNTTO        0x7FFF0000
#define WLAN_MAC_POST_RX_TIMERS_MASK_TIMER2_EN             0x80000000


// WLAN MAC HW - TX_CTRL_STATUS register bit masks
#define WLAN_MAC_TXCTRL_STATUS_MASK_TX_A_PENDING                  0x00000001     // b[0]
#define WLAN_MAC_TXCTRL_STATUS_MASK_TX_A_DONE                     0x00000002     // b[1]
#define WLAN_MAC_TXCTRL_STATUS_MASK_TX_A_RESULT                   0x0000000C     // b[3:2]
#define WLAN_MAC_TXCTRL_STATUS_MASK_TX_A_STATE                    0x00000070     // b[6:4]
#define WLAN_MAC_TXCTRL_STATUS_MASK_TX_B_PENDING                  0x00000080     // b[7]
#define WLAN_MAC_TXCTRL_STATUS_MASK_TX_B_DONE                     0x00000100     // b[8]
#define WLAN_MAC_TXCTRL_STATUS_MASK_TX_B_RESULT                   0x00000600     // b[10:9]
#define WLAN_MAC_TXCTRL_STATUS_MASK_TX_B_STATE                    0x00003800     // b[13:11]
#define WLAN_MAC_TXCTRL_STATUS_MASK_TX_C_PENDING                  0x00004000     // b[14]
#define WLAN_MAC_TXCTRL_STATUS_MASK_TX_C_DONE                     0x00008000     // b[15]
#define WLAN_MAC_TXCTRL_STATUS_MASK_TX_C_STATE                    0x00070000     // b[18:16]
#define WLAN_MAC_TXCTRL_STATUS_MASK_TX_D_PENDING                  0x00080000     // b[19]
#define WLAN_MAC_TXCTRL_STATUS_MASK_TX_D_DONE                     0x00100000     // b[20]
#define WLAN_MAC_TXCTRL_STATUS_MASK_TX_D_STATE                    0x00E00000     // b[23:21]
#define WLAN_MAC_TXCTRL_STATUS_MASK_POSTTX_TIMER2_RUNNING         0x01000000     // b[24]
#define WLAN_MAC_TXCTRL_STATUS_MASK_POSTTX_TIMER1_RUNNING         0x02000000     // b[25]
#define WLAN_MAC_TXCTRL_STATUS_MASK_POSTRX_TIMER2_RUNNING         0x04000000     // b[26]
#define WLAN_MAC_TXCTRL_STATUS_MASK_POSTRX_TIMER1_RUNNING         0x08000000     // b[27]

#define WLAN_MAC_TXCTRL_STATUS_TX_A_RESULT_NONE                  (0 << 2)        // FSM idle or still running
#define WLAN_MAC_TXCTRL_STATUS_TX_A_RESULT_TIMEOUT               (1 << 2)        // FSM completed with postTx timer expiration
#define WLAN_MAC_TXCTRL_STATUS_TX_A_RESULT_RX_STARTED            (2 << 2)        // FSM completed with PHY Rx starting

#define WLAN_MAC_TXCTRL_STATUS_TX_A_STATE_IDLE                   (0 << 4)
#define WLAN_MAC_TXCTRL_STATUS_TX_A_STATE_PRE_TX_WAIT            (1 << 4)
#define WLAN_MAC_TXCTRL_STATUS_TX_A_STATE_START_BO               (2 << 4)
#define WLAN_MAC_TXCTRL_STATUS_TX_A_STATE_DEFER                  (3 << 4)
#define WLAN_MAC_TXCTRL_STATUS_TX_A_STATE_DO_TX                  (4 << 4)
#define WLAN_MAC_TXCTRL_STATUS_TX_A_STATE_POST_TX                (5 << 4)
#define WLAN_MAC_TXCTRL_STATUS_TX_A_STATE_POST_TX_WAIT           (6 << 4)
#define WLAN_MAC_TXCTRL_STATUS_TX_A_STATE_DONE                   (7 << 4)

#define WLAN_MAC_TXCTRL_STATUS_TX_B_RESULT_NONE                  (0 << 9)        // FSM idle or still running
#define WLAN_MAC_TXCTRL_STATUS_TX_B_RESULT_DID_TX                (1 << 9)        // FSM completed with PHY Tx
#define WLAN_MAC_TXCTRL_STATUS_TX_B_RESULT_NO_TX                 (2 << 9)        // FSM completed, skipped PHY Tx

#define WLAN_MAC_TXCTRL_STATUS_TX_B_STATE_IDLE                   (0 << 11)
#define WLAN_MAC_TXCTRL_STATUS_TX_B_STATE_PRE_TX_WAIT            (1 << 11)
#define WLAN_MAC_TXCTRL_STATUS_TX_B_STATE_CHECK_NAV              (2 << 11)
#define WLAN_MAC_TXCTRL_STATUS_TX_B_STATE_DO_TX                  (3 << 11)
#define WLAN_MAC_TXCTRL_STATUS_TX_B_STATE_DONE                   (4 << 11)

#define WLAN_MAC_TXCTRL_STATUS_TX_C_STATE_IDLE        (0 << 16)
#define WLAN_MAC_TXCTRL_STATUS_TX_C_STATE_START_BO    (1 << 16) //Starting backoff counter - 1 cycle
#define WLAN_MAC_TXCTRL_STATUS_TX_C_STATE_DEFER       (2 << 16) //Waiting for zero backoff - unbounded time
#define WLAN_MAC_TXCTRL_STATUS_TX_C_STATE_DO_TX       (3 << 16) //PHY Tx started, waiting on TX_DONE - TX_TIME
#define WLAN_MAC_TXCTRL_STATUS_TX_C_STATE_DONE        (4 << 16) //TX_DONE occurred - 1 cycle

#define WLAN_MAC_TXCTRL_STATUS_TX_D_STATE_IDLE        (0 << 21)
#define WLAN_MAC_TXCTRL_STATUS_TX_D_STATE_START_BO    (1 << 21) //Starting backoff counter - 1 cycle
#define WLAN_MAC_TXCTRL_STATUS_TX_D_STATE_DEFER       (2 << 21) //Waiting for zero backoff - unbounded time
#define WLAN_MAC_TXCTRL_STATUS_TX_D_STATE_DO_TX       (3 << 21) //PHY Tx started, waiting on TX_DONE - TX_TIME
#define WLAN_MAC_TXCTRL_STATUS_TX_D_STATE_DONE        (4 << 21) //TX_DONE occurred - 1 cycle

#define wlan_mac_get_tx_ctrl_status() (Xil_In32(WLAN_MAC_REG_TX_CTRL_STATUS))

// WLAN MAC HW - STATUS register bit masks
#define WLAN_MAC_STATUS_MASK_TX_A_PENDING                  0x00000001     // b[0]
#define WLAN_MAC_STATUS_MASK_TX_A_DONE                     0x00000002     // b[1]
#define WLAN_MAC_STATUS_MASK_TX_B_PENDING                  0x00000004     // b[2]
#define WLAN_MAC_STATUS_MASK_TX_B_DONE                     0x00000008     // b[3]
#define WLAN_MAC_STATUS_MASK_TX_C_PENDING                  0x00000010     // b[4]
#define WLAN_MAC_STATUS_MASK_TX_C_DONE                     0x00000020     // b[5]
#define WLAN_MAC_STATUS_MASK_TX_D_PENDING                  0x00000040     // b[6]
#define WLAN_MAC_STATUS_MASK_TX_D_DONE                     0x00000080     // b[7]

#define WLAN_MAC_STATUS_MASK_TX_PHY_ACTIVE                 0x00000100     // b[8]
#define WLAN_MAC_STATUS_MASK_RX_PHY_ACTIVE                 0x00000200     // b[9]
#define WLAN_MAC_STATUS_MASK_RX_PHY_STARTED                0x00000400     // b[10]
#define WLAN_MAC_STATUS_MASK_RX_FCS_GOOD                   0x00000800     // b[11]
#define WLAN_MAC_STATUS_MASK_RX_END_ERROR                  0x00007000     // b[14:12]
#define WLAN_MAC_STATUS_MASK_NAV_ADDR_MATCHED              0x00008000     // b[15]
#define WLAN_MAC_STATUS_MASK_NAV_BUSY                      0x00010000     // b[16]
#define WLAN_MAC_STATUS_MASK_CCA_BUSY                      0x00020000     // b[17]
#define WLAN_MAC_STATUS_MASK_TU_LATCH                      0x00040000     // b[18]
#define WLAN_MAC_STATUS_MASK_RX_PHY_WRITING_PAYLOAD        0x00080000     // b[19]

#define wlan_mac_get_status() (Xil_In32(WLAN_MAC_REG_STATUS))
#define wlan_mac_check_tu_latch() ((wlan_mac_get_status() & WLAN_MAC_STATUS_MASK_TU_LATCH) >> 16)

//-----------------------------------------------
// WLAN MAC HW - RX_PHY_HDR_PARAMS bit masks
//
#define WLAN_MAC_PHY_RX_PHY_HDR_MASK_LENGTH                 0x0000FFFF     // b[15:0]
#define WLAN_MAC_PHY_RX_PHY_HDR_MASK_MCS                    0x007F0000     // b[22:16]
#define WLAN_MAC_PHY_RX_PHY_HDR_MASK_UNSUPPORTED            0x00800000     // b[23]
#define WLAN_MAC_PHY_RX_PHY_HDR_MASK_PHY_MODE               0x07000000     // b[26:24]
#define WLAN_MAC_PHY_RX_PHY_HDR_READY                       0x08000000     // b[27]
#define WLAN_MAC_PHY_RX_PHY_HDR_MASK_PHY_SEL                0x10000000     // b[28]
#define WLAN_MAC_PHY_RX_PHY_HDR_MASK_RX_ERROR               0xE0000000     // b[31:29]

#define WLAN_MAC_PHY_RX_PHY_HDR_PHY_SEL_DSSS                0x00000000
#define WLAN_MAC_PHY_RX_PHY_HDR_PHY_SEL_OFDM                0x10000000

#define WLAN_MAC_PHY_RX_PHY_HDR_PHY_MODE_11AG               0x1
#define WLAN_MAC_PHY_RX_PHY_HDR_PHY_MODE_11N                0x2
#define WLAN_MAC_PHY_RX_PHY_HDR_PHY_MODE_11AC               0x8


//-----------------------------------------------
// WLAN MAC HW - CONTROL bit masks / macros
//
#define WLAN_MAC_CTRL_MASK_RESET                           0x00000001
#define WLAN_MAC_CTRL_MASK_DISABLE_NAV                     0x00000002
#define WLAN_MAC_CTRL_MASK_RESET_NAV                       0x00000004
#define WLAN_MAC_CTRL_MASK_BLOCK_RX_ON_TX                  0x00000008
#define WLAN_MAC_CTRL_MASK_RESET_TU_LATCH                  0x00000010
#define WLAN_MAC_CTRL_MASK_CCA_IGNORE_PHY_CS               0x00000020
#define WLAN_MAC_CTRL_MASK_CCA_IGNORE_TX_BUSY              0x00000040
#define WLAN_MAC_CTRL_MASK_CCA_IGNORE_NAV                  0x00000080
#define WLAN_MAC_CTRL_MASK_FORCE_CCA_BUSY                  0x00000100
#define WLAN_MAC_CTRL_MASK_RESET_PHY_ACTIVE_LATCHES        0x00000200
#define WLAN_MAC_CTRL_MASK_RESET_RX_STARTED_LATCH          0x00000400
#define WLAN_MAC_CTRL_MASK_RESET_TX_CTRL_A                 0x00000800
#define WLAN_MAC_CTRL_MASK_RESET_TX_CTRL_B                 0x00001000
#define WLAN_MAC_CTRL_MASK_RESET_TX_CTRL_C                 0x00002000
#define WLAN_MAC_CTRL_MASK_RESET_TX_CTRL_D                 0x00004000
#define WLAN_MAC_CTRL_MASK_RESET_A_BACKOFF                 0x00008000
#define WLAN_MAC_CTRL_MASK_RESET_C_BACKOFF                 0x00010000
#define WLAN_MAC_CTRL_MASK_RESET_D_BACKOFF                 0x00020000
#define WLAN_MAC_CTRL_MASK_PAUSE_TX_A		               0x00040000
#define WLAN_MAC_CTRL_MASK_PAUSE_TX_C	                   0x00080000
#define WLAN_MAC_CTRL_MASK_PAUSE_TX_D		               0x00100000

//-----------------------------------------------
// WLAN MAC HW - START bit masks / macros
//
#define WLAN_MAC_START_REG_MASK_START_TX_A	0x1
#define WLAN_MAC_START_REG_MASK_START_TX_B	0x2
#define WLAN_MAC_START_REG_MASK_START_TX_C	0x4
#define WLAN_MAC_START_REG_MASK_START_TX_D	0x8

// TXRX_TIMESTAMPS_FRAC register
// b[15:8]: Fractional part of RX_START microsecond timestamp
// b[ 7:0]: Fractional part of TX_START microsecond timestamp
#define wlan_mac_low_get_rx_start_timestamp_frac() ((Xil_In32(WLAN_MAC_REG_TXRX_TIMESTAMPS_FRAC) & 0xFF00) >> 8)
#define wlan_mac_low_get_tx_start_timestamp_frac()  (Xil_In32(WLAN_MAC_REG_TXRX_TIMESTAMPS_FRAC) & 0x00FF)


#endif /* W3_MAC_PHY_REGS_H_ */
