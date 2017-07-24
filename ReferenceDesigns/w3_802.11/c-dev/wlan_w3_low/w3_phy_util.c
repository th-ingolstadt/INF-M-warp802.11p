/** @file wlan_phy_util.c
 *  @brief Physical Layer Utility
 *
 *  This contains code for configuring low-level parameters in the PHY and hardware.
 *
 *  @copyright Copyright 2013-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */
/***************************** Include Files *********************************/

// Xilinx SDK includes
#include "xio.h"
#include "xparameters.h"

// WLAN includes
#include "wlan_platform_low.h"
#include "wlan_platform_common.h"
#include "wlan_mac_mailbox_util.h"
#include "wlan_phy_util.h"
#include "wlan_mac_low.h"
#include "wlan_mac_common.h"
#include "w3_low.h"
#include "w3_mac_phy_regs.h"
#include "wlan_common_types.h"
#include "w3_phy_util.h"
#include "wlan_Mac_pkt_buf_util.h"


/*****************************************************************************/
/**
 * Initialize the PHY
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void wlan_phy_init() {

    // Assert Tx and Rx resets
    REG_SET_BITS(WLAN_RX_REG_CTRL, WLAN_RX_REG_CTRL_RESET);
    REG_SET_BITS(WLAN_TX_REG_CFG, WLAN_TX_REG_CFG_RESET);

    /************ PHY Rx ************/

    // Set the max Tx/Rx packet sizes to 2KB (sane default for standard 802.11a/g links)
    wlan_phy_rx_set_max_pkt_len_kB( MAX_PKT_SIZE_KB );
    wlan_phy_rx_set_max_pktbuf_addr( PKT_BUF_SIZE - sizeof(rx_frame_info_t) - PHY_RX_PKT_BUF_PHY_HDR_SIZE );

    // WLAN_RX_DSSS_CFG reg
    // Configure the DSSS Rx pipeline
    //  wlan_phy_DSSS_rx_config(code_corr, despread_dly, sfd_timeout)
    wlan_phy_DSSS_rx_config(0x30, 5, 140);

    // WLAN_RX_PKT_DET_DSSS_CFG reg
    // Configure the DSSS auto-correlation packet detector
    //  wlan_phy_pktDet_autoCorr_dsss_cfg(corr_thresh, energy_thresh, timeout_ones, timeout_count)
    //
    // To effectively disable DSSS detection with high thresholds, substitute with the following line:
    //     wlan_phy_rx_pktDet_autoCorr_dsss_cfg(0xFF, 0x3FF, 30, 40);
    wlan_phy_rx_pktDet_autoCorr_dsss_cfg(0x60, 400, 30, 40);

    // WLAN_RX_PKT_DET_OFDM_CFG reg
	// args: (corr_thresh, energy_thresh, min_dur, post_wait)
	// Using defaults from set_phy_samp_rate(20)
	wlan_phy_rx_pktDet_autoCorr_ofdm_cfg(200, 9, 4, 0x3F);

    // WLAN_RX_REG_CFG reg
    // Configure DSSS Rx to wait for AGC lock, then hold AGC lock until Rx completes or times out
    REG_SET_BITS(WLAN_RX_REG_CFG, (WLAN_RX_REG_CFG_DSSS_RX_AGC_HOLD | WLAN_RX_REG_CFG_DSSS_RX_REQ_AGC));

    // Enable LTS-based CFO correction
    REG_CLEAR_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_CFO_EST_BYPASS);

    // Enable byte order swap for payloads and chan ests
    REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_PKT_BUF_WEN_SWAP);
    REG_CLEAR_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_CHAN_EST_WEN_SWAP);

    // Enable writing OFDM chan ests to Rx pkt buffer
    REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_RECORD_CHAN_EST);

    // The rate/length BUSY logic should hold the pkt det signal high to avoid
    //  spurious AGC and detection events during an unsupported waveform
    REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_BUSY_HOLD_PKT_DET);

    // Block Rx inputs during Tx
    REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_USE_TX_SIG_BLOCK);

    // Enable HTMF waveform (11n waveform) detection in the PHY Rx
    //  Disabling HTMF detection reverts the PHY to <v1.3 behavior where
    //  every reception is handled as NONHT (11a)
    REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_ENABLE_HTMF_DET);

    // Enable VHT waveform detection - the PHY can't decode VHT waveforms
    //  but enabling detection allows early termination with a header error
    //  instead of attempting to decode the VHT waveform as NONHT
    REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_ENABLE_VHT_DET);

    // Keep CCA.BUSY asserted when DSSS Rx is active
    REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_DSSS_ASSERTS_CCA);

    // WLAN_RX_FFT_CFG reg
    // FFT config
    wlan_phy_rx_config_fft(64, 16);
    wlan_phy_rx_set_fft_window_offset(7);
    wlan_phy_rx_set_fft_scaling(5);

    // WLAN_RX_LTS_CFG reg
    // Set LTS correlation threshold, timeout and allowed peak separation times
    //     1023 disables LTS threshold switch (one threshold worked across SNRs in our testing)
    //     Timeout value is doubled in hardware (350/2 becomes a timeout of 350 sample periods)
    //     Peak separation is 3-bit mask, allowing 63/64/65 sample periods between peaks
    wlan_phy_rx_lts_corr_config(1023 * PHY_RX_RSSI_SUM_LEN, 350/2, 0x7);

    // WLAN_RX_LTS_THRESH reg
    // LTS correlation thresholds (low NSR, high SNR)
    wlan_phy_rx_lts_corr_thresholds(9000, 9000);

    // WLAN_RX_LTS_PEAKTYPE_THRESH reg
    // LTS correlation peak-type (big vs small) thresholds (low NSR, high SNR)
    // TODO: We can explore the peaktype dimension further in a future release
    wlan_phy_rx_lts_corr_peaktype_thresholds(0xFFFF, 0xFFFF);

    // WLAN_RX_PKT_DET_OFDM_CFG reg
    // Configure RSSI pkt det
    //     RSSI pkt det disabled by default (auto-corr detection worked across SNRs in our testing)
    //     The summing logic realizes a sum of the length specified + 1
    wlan_phy_rx_pktDet_RSSI_cfg( (PHY_RX_RSSI_SUM_LEN-1), ( PHY_RX_RSSI_SUM_LEN * 1023), 1);

    // WLAN_RX_PHY_CCA_CFG reg
    // Set physical carrier sensing threshold
    // CS thresh set by wlan_platform_set_phy_cs_thresh() - register field set to 0xFFFF here to disable
    //  PHY CS assert until MAC code configures the desired threshold
    wlan_phy_rx_set_cca_thresh(0xFFFF);
    wlan_phy_rx_set_extension((6*20) - 64); // Overridden later by set_phy_samp_rate()

    // WLAN_RX_FEC_CFG reg
    // Set pre-quantizer scaling for decoder inputs
    //  These values were found empirically by vs PER by sweeping scaling and attenuation
    wlan_phy_rx_set_fec_scaling(15, 15, 18, 22);

    // WLAN_RX_PKT_BUF_SEL reg
    // Configure channel estimate capture (64 subcarriers, 4 bytes each)
    //     Chan ests start at sizeof(rx_frame_info) - sizeof(chan_est)
    wlan_phy_rx_pkt_buf_h_est_offset((PHY_RX_PKT_BUF_PHY_HDR_OFFSET - (64*4)));

    // WLAN_RX_CHAN_EST_SMOOTHING reg
    //Disable channel estimate smoothing
    wlan_phy_rx_chan_est_smoothing(0xFFF, 0x0);
    wlan_phy_rx_phy_mode_det_thresh(12);

    // WLAN_RX_PKT_BUF_MAXADDR reg
    wlan_phy_rx_set_max_pktbuf_addr(3800);

    // Configure the default antenna selections as SISO Tx/Rx on RF A
    wlan_rx_config_ant_mode(RX_ANTMODE_SISO_ANTA);

    /************ PHY Tx ************/

    // De-assert all starts
    REG_CLEAR_BITS(WLAN_TX_REG_START, 0xFFFFFFFF);

    // TX_OUTPUT_SCALING register
    // Set digital scaling of preamble/payload signals before DACs (UFix12_0)
    wlan_phy_tx_set_scaling(0x2000, 0x2000); // Scaling of 2.0

    // TX_CONFIG register
    // Enable the Tx PHY 4-bit TxEn port that captures the MAC's selection of active antennas per Tx
    REG_SET_BITS(WLAN_TX_REG_CFG, WLAN_TX_REG_CFG_USE_MAC_ANT_MASKS);

    // TX_FFT_CONFIG register
    // Configure the IFFT scaling and control logic
    //  Current PHY design requires 64 subcarriers, 16-sample cyclic prefix
    wlan_phy_tx_config_fft(0x2A, 64, 16);

    // TX_TIMING register
    //  Timing values overridden later in set_phy_samp_rate()
	wlan_phy_tx_set_extension(112);
	wlan_phy_tx_set_txen_extension(50);
	wlan_phy_tx_set_rx_invalid_extension(150);

	// TX_PKT_BUF_SEL register
    wlan_phy_tx_pkt_buf_phy_hdr_offset(PHY_TX_PKT_BUF_PHY_HDR_OFFSET);
    wlan_phy_tx_pkt_buf(0);


    /************ Wrap Up ************/


    // De-assert resets
    REG_CLEAR_BITS(WLAN_RX_REG_CTRL, WLAN_RX_REG_CTRL_RESET);
    REG_CLEAR_BITS(WLAN_TX_REG_CFG, WLAN_TX_REG_CFG_RESET);

    // Let PHY Tx take control of radio TXEN/RXEN
    REG_CLEAR_BITS(WLAN_TX_REG_CFG, WLAN_TX_REG_CFG_SET_RC_RXEN);
    REG_SET_BITS(WLAN_TX_REG_CFG, WLAN_TX_REG_CFG_SET_RC_RXEN);

    return;
}

/*****************************************************************************/
/**
 * Configure the Rx Antenna Mode
 *
 * @param   ant_mode         - Antenna mode to set
 *
 * @return  None
 *
 * @note    There is no corresponding wlan_tx_config_ant_mode() because the transmit
 *     antenna is set by the MAC software (ie mac_sw -> mac_hw -> phy_tx) for every packet
 *
 *****************************************************************************/
void wlan_rx_config_ant_mode(u32 ant_mode) {

    // Hold the Rx PHY in reset before changing any pkt det or radio enables
    REG_SET_BITS(WLAN_RX_REG_CTRL, WLAN_RX_REG_CTRL_RESET);

    wlan_platform_low_set_rx_ant_mode(ant_mode);

    // Disable all Rx modes first; selectively re-enabled in switch below
    REG_CLEAR_BITS(WLAN_RX_REG_CFG, (WLAN_RX_REG_CFG_PKT_DET_EN_ANT_A |
                                     WLAN_RX_REG_CFG_PKT_DET_EN_ANT_B |
                                     WLAN_RX_REG_CFG_PKT_DET_EN_ANT_C |
                                     WLAN_RX_REG_CFG_PKT_DET_EN_ANT_D |
                                     WLAN_RX_REG_CFG_SWITCHING_DIV_EN |
                                     WLAN_RX_REG_CFG_PKT_DET_EN_EXT |
                                     WLAN_RX_REG_CFG_ANT_SEL_MASK));

    // Set the antenna mode
    //
    // For each antenna mode:
    //   - Enable packet detection
    //   - Select I/Q stream for Rx PHY
    //   - Give PHY control of Tx/Rx status
    //   - Configure AGC
    //
    switch (ant_mode) {
        case RX_ANTMODE_SISO_ANTA:
            REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_PKT_DET_EN_ANT_A);
            wlan_phy_select_rx_antenna(RX_ANTMODE_SISO_ANTA);
        break;

        case RX_ANTMODE_SISO_ANTB:
            REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_PKT_DET_EN_ANT_B);
            wlan_phy_select_rx_antenna(RX_ANTMODE_SISO_ANTB);
        break;

        case RX_ANTMODE_SISO_ANTC:
            REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_PKT_DET_EN_ANT_C);
            wlan_phy_select_rx_antenna(RX_ANTMODE_SISO_ANTC);
        break;

        case RX_ANTMODE_SISO_ANTD:
            REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_PKT_DET_EN_ANT_D);
            wlan_phy_select_rx_antenna(RX_ANTMODE_SISO_ANTD);
        break;

        case RX_ANTMODE_SISO_SELDIV_2ANT:
            REG_SET_BITS(WLAN_RX_REG_CFG, (WLAN_RX_REG_CFG_PKT_DET_EN_ANT_A | WLAN_RX_REG_CFG_PKT_DET_EN_ANT_B | WLAN_RX_REG_CFG_SWITCHING_DIV_EN));
        break;

        case RX_ANTMODE_SISO_SELDIV_4ANT:
            REG_SET_BITS(WLAN_RX_REG_CFG, (WLAN_RX_REG_CFG_PKT_DET_EN_ANT_A | WLAN_RX_REG_CFG_PKT_DET_EN_ANT_B | WLAN_RX_REG_CFG_PKT_DET_EN_ANT_C | WLAN_RX_REG_CFG_PKT_DET_EN_ANT_D | WLAN_RX_REG_CFG_SWITCHING_DIV_EN));
        break;

        default:
            // Default to SISO on A if user provides invalid mode
            xil_printf("wlan_rx_config_ant_mode ERROR: Invalid Mode - Defaulting to SISO on A\n");

            REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_PKT_DET_EN_ANT_A);
            wlan_phy_select_rx_antenna(RX_ANTMODE_SISO_ANTA);
        break;
    }

    // Release the PHY Rx reset
    REG_CLEAR_BITS(WLAN_RX_REG_CTRL, WLAN_RX_REG_CTRL_RESET);

    return;
}

/*****************************************************************************/
/**
 * Transmission debug methods
 *
 * @param   Variable
 *
 * @return  Variable
 *
 * @note    These function is for debug use only.
 *
 *****************************************************************************/
inline void wlan_tx_start() {
    // Start the PHY Tx immediately; this bypasses the mac_hw MPDU Tx state machine
    //     This should only be used for debug - normal transmissions should use mac_hw
    //
    REG_SET_BITS(WLAN_TX_REG_START, WLAN_TX_REG_START_VIA_RC);
    REG_CLEAR_BITS(WLAN_TX_REG_START, WLAN_TX_REG_START_VIA_RC);

    return;
}
