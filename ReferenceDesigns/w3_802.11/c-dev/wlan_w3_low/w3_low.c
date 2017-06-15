/** @file wlan_platform_low.c
 *  @brief Platform abstraction for CPU Low
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

// SDK includes
#include "xil_types.h"
#include "xstatus.h"
#include "xparameters.h"
#include "stdarg.h"

// Platform includes
#include "w3_mac_phy_regs.h"
#include "w3_low.h"
#include "w3_common.h"
#include "wlan_platform_common.h"
#include "wlan_platform_debug_hdr.h"
#include "wlan_platform_low.h"
#include "w3_userio_util.h"
#include "w3_phy_util.h"

// Low framework includes
#include "wlan_phy_util.h"
#include "wlan_mac_low.h"
#include "wlan_mac_common.h"
#include "wlan_mac_mailbox_util.h"

// WARP v3 hardware includes
#include "w3_userio.h"
#include "w3_ad_controller.h"
#include "w3_clock_controller.h"
#include "radio_controller.h"
#include "w3_phy_util.h"

static channel_band_t gl_current_band;

/*****************************************************************************
 * Public functions - the functions below are exported to the low framework
 *****************************************************************************/


void wlan_platform_userio_disp_status(userio_disp_status_t status, ...){
   va_list valist;
   static u8 red_led_index = 0;
   static u8 green_led_index = 0;

   /* initialize valist for num number of arguments */
   va_start(valist, status);

   switch(status){

   	   case USERIO_DISP_STATUS_GOOD_FCS_EVENT: {
           green_led_index = (green_led_index + 1) % 4;
           userio_write_leds_green(USERIO_BASEADDR, (1<<green_led_index));
   	   } break;

   	   case USERIO_DISP_STATUS_BAD_FCS_EVENT: {
           red_led_index = (red_led_index + 1) % 4;
           userio_write_leds_red(USERIO_BASEADDR, (1<<red_led_index));
   	   } break;
   	   case USERIO_DISP_STATUS_CPU_ERROR: {
   		   u32 error_code = va_arg(valist, u32);
   		   if (error_code != WLAN_ERROR_CPU_STOP) {
   			   // Print error message
   			   xil_printf("\n\nERROR:  CPU is halting with error code: E%X\n\n", (error_code & 0xF));

   			   // Set the error code on the hex display
   			   set_hex_display_error_status(error_code & 0xF);

   			   // Enter infinite loop blinking the hex display
   			   blink_hex_display(0, 250000);
			} else {
				// Stop execution
				while (1) {};
			}
   	   } break;

   	   default:
	   break;
   }

   /* clean memory reserved for valist */
   va_end(valist);
   return;
}

int wlan_platform_low_init() {
	int status;

	status = w3_node_init();
	if(status != 0) {
		xil_printf("ERROR in w3_node_init(): %d\n", status);
		return status;
	}

	w3_radio_init();
	w3_agc_init();

	return 0;
}

void wlan_platform_low_param_handler(u8 mode, u32* payload) {
	if(mode != IPC_REG_WRITE_MODE) {
		xil_printf("ERROR wlan_platform_low_param_handler: unrecognized mode (%d) - mode must be WRITE\n", mode);
		return;
	}

	switch(payload[0]) {
		case LOW_PARAM_PKT_DET_MIN_POWER: {
			int min_pwr_arg = payload[1] & 0xFF;
			if(min_pwr_arg == 0) {
				// wlan_exp uses 0 to disable min power det logic
				wlan_platform_set_pkt_det_min_power(0);
			} else {
				//The value sent from wlan_exp will be unsigned with 0 representing PKT_DET_MIN_POWER_MIN
				wlan_platform_set_pkt_det_min_power(min_pwr_arg + PKT_DET_MIN_POWER_MIN);
			}
		}
		break;

		default:
		break;
	}

	return;
}

void wlan_platform_low_set_rx_ant_mode(u32 ant_mode) {

    // Disable PHY control of all RF interfaces - selected interfaces to re-enabled below
    radio_controller_setCtrlSource(RC_BASEADDR, RC_ALL_RF, RC_REG0_RXEN_CTRLSRC, RC_CTRLSRC_REG);

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
            radio_controller_setCtrlSource(RC_BASEADDR, RC_RFA, RC_REG0_RXEN_CTRLSRC, RC_CTRLSRC_HW);
        break;

        case RX_ANTMODE_SISO_ANTB:
            radio_controller_setCtrlSource(RC_BASEADDR, RC_RFB, RC_REG0_RXEN_CTRLSRC, RC_CTRLSRC_HW);
        break;

        case RX_ANTMODE_SISO_ANTC:
            radio_controller_setCtrlSource(RC_BASEADDR, RC_RFC, RC_REG0_RXEN_CTRLSRC, RC_CTRLSRC_HW);
        break;

        case RX_ANTMODE_SISO_ANTD:
            radio_controller_setCtrlSource(RC_BASEADDR, RC_RFD, RC_REG0_RXEN_CTRLSRC, RC_CTRLSRC_HW);
        break;

        case RX_ANTMODE_SISO_SELDIV_2ANT:
            // By enabling the antenna switching, the I/Q stream is automatically switched for Rx PHY
            radio_controller_setCtrlSource(RC_BASEADDR, (RC_RFA | RC_RFB), RC_REG0_RXEN_CTRLSRC, RC_CTRLSRC_HW);
        break;

        case RX_ANTMODE_SISO_SELDIV_4ANT:
            // By enabling the antenna switching, the I/Q stream is automatically switched for Rx PHY
            radio_controller_setCtrlSource(RC_BASEADDR, RC_ALL_RF, RC_REG0_RXEN_CTRLSRC, RC_CTRLSRC_HW);
        break;

        default:
            // Default to SISO on A if user provides invalid mode
            xil_printf("wlan_platform_low_set_rx_ant_mode ERROR: Invalid Mode - Defaulting to SISO on A\n");

            radio_controller_setCtrlSource(RC_BASEADDR, RC_RFA, RC_REG0_RXEN_CTRLSRC, RC_CTRLSRC_HW);
        break;
    }
	return;
}

int wlan_platform_low_set_samp_rate(phy_samp_rate_t phy_samp_rate) {
	if((phy_samp_rate != PHY_10M) && (phy_samp_rate != PHY_20M) && (phy_samp_rate != PHY_40M)) {
		xil_printf("Invalid PHY samp rate (%d)\n", phy_samp_rate);
		return -1;
	}

    // Assert PHY Tx/Rx and MAC Resets
    REG_SET_BITS(WLAN_RX_REG_CTRL, WLAN_RX_REG_CTRL_RESET);
    REG_SET_BITS(WLAN_TX_REG_CFG, WLAN_TX_REG_CFG_RESET);
    wlan_mac_reset(1);

	// Set RF interface clocking and interp/decimation filters
	switch(phy_samp_rate){
		case PHY_40M:
			// Set ADC_CLK=DAC_CLK=40MHz, interp_rate=decim_rate=1
			clk_config_dividers(CLK_BASEADDR, 2, (CLK_SAMP_OUTSEL_AD_RFA | CLK_SAMP_OUTSEL_AD_RFB));
			ad_config_filters(AD_BASEADDR, AD_ALL_RF, 1, 1);
			ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x32, 0x2F);
			ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x33, 0x00);
			ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x33, 0x08);
		break;
		case PHY_20M:
			// Set ADC_CLK=DAC_CLK=40MHz, interp_rate=decim_rate=2
			clk_config_dividers(CLK_BASEADDR, 2, (CLK_SAMP_OUTSEL_AD_RFA | CLK_SAMP_OUTSEL_AD_RFB));

			ad_config_filters(AD_BASEADDR, AD_ALL_RF, 2, 2);
			ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x32, 0x27);
			ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x33, 0x00);
			ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x33, 0x08);
		break;
		case PHY_10M:
			// Set ADC_CLK=DAC_CLK=20MHz, interp_rate=decim_rate=2
			clk_config_dividers(CLK_BASEADDR, 4, (CLK_SAMP_OUTSEL_AD_RFA | CLK_SAMP_OUTSEL_AD_RFB));
			ad_config_filters(AD_BASEADDR, AD_ALL_RF, 2, 2);
			ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x32, 0x27);
			ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x33, 0x00);
			ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x33, 0x08);
		break;
	}

    switch(phy_samp_rate){
    	case PHY_40M:
    	    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_RXLPF_BW, 3);
    	    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_TXLPF_BW, 3);
		break;
    	case PHY_10M:
    	case PHY_20M:
    	    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_RXLPF_BW, 1);
    	    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_TXLPF_BW, 1);
    	break;
    }

    // AGC timing: capt_rssi_1, capt_rssi_2, capt_v_db, agc_done
    switch(phy_samp_rate){
    	case PHY_40M:
    		wlan_agc_set_AGC_timing(10, 30, 90, 96);
		break;
    	case PHY_10M:
    	case PHY_20M:
    		wlan_agc_set_AGC_timing(1, 30, 90, 96);
    	break;
    }

	
    // Configure auto-correlation packet detection
    //  wlan_phy_rx_pktDet_autoCorr_ofdm_cfg(corr_thresh, energy_thresh, min_dur, post_wait)
    switch(phy_samp_rate){
    	case PHY_40M:
    		//TODO: The 2 value is suspiciously low
    		wlan_phy_rx_pktDet_autoCorr_ofdm_cfg(200, 2, 15, 0x3F);
		break;
    	case PHY_10M:
    	case PHY_20M:
    		wlan_phy_rx_pktDet_autoCorr_ofdm_cfg(200, 9, 4, 0x3F);
    	break;
    }

    // Set post Rx extension
    //  Number of sample periods post-Rx the PHY waits before asserting Rx END. The additional
    //   -3 usec accounts for latency through the Rx RF chain.
    switch(phy_samp_rate){
    	case PHY_40M:
    		// 6us Extension
    		wlan_phy_rx_set_extension((6*40) - 128);
		break;
    	case PHY_20M:
    		// 6us Extension
    		wlan_phy_rx_set_extension((6*20) - 64);
    	break;
    	case PHY_10M:
    		// 6us Extension
    		wlan_phy_rx_set_extension((6*10) - 32);
    	break;
    }

    // Set Tx duration extension, in units of sample periods
	switch(phy_samp_rate){
		case PHY_40M:
			// 224 40MHz sample periods -- aligns TX_END to RX_END
			wlan_phy_tx_set_extension(224);

			// Set extension from last samp output to RF Tx -> Rx transition
			//     This delay allows the Tx pipeline to finish driving samples into DACs
			//     and for DAC->RF frontend to finish output Tx waveform
			wlan_phy_tx_set_txen_extension(100);

			// Set extension from RF Rx -> Tx to un-blocking Rx samples
			wlan_phy_tx_set_rx_invalid_extension(300);
		break;
		case PHY_20M:
			// 112 20MHz sample periods -- aligns TX_END to RX_END
			wlan_phy_tx_set_extension(112);

			// Set extension from last samp output to RF Tx -> Rx transition
			//     This delay allows the Tx pipeline to finish driving samples into DACs
			//     and for DAC->RF frontend to finish output Tx waveform
			wlan_phy_tx_set_txen_extension(50);

			// Set extension from RF Rx -> Tx to un-blocking Rx samples
			wlan_phy_tx_set_rx_invalid_extension(150);
		break;
		case PHY_10M:
			// 56 10MHz sample periods -- aligns TX_END to RX_END
			wlan_phy_tx_set_extension(56);

			// Set extension from last samp output to RF Tx -> Rx transition
			//     This delay allows the Tx pipeline to finish driving samples into DACs
			//     and for DAC->RF frontend to finish output Tx waveform
			wlan_phy_tx_set_txen_extension(25);

			// Set extension from RF Rx -> Tx to un-blocking Rx samples
			wlan_phy_tx_set_rx_invalid_extension(75);
		break;
	}
	
    // Deassert PHY Tx/Rx and MAC Resets
    REG_CLEAR_BITS(WLAN_RX_REG_CTRL, WLAN_RX_REG_CTRL_RESET);
    REG_CLEAR_BITS(WLAN_TX_REG_CFG, WLAN_TX_REG_CFG_RESET);
    wlan_mac_reset(0);

    // Let PHY Tx take control of radio TXEN/RXEN
    REG_CLEAR_BITS(WLAN_TX_REG_CFG, WLAN_TX_REG_CFG_SET_RC_RXEN);
    REG_SET_BITS(WLAN_TX_REG_CFG, WLAN_TX_REG_CFG_SET_RC_RXEN);
	
	return 0;
}



/*****************************************************************************/
/**
 * @brief Set the radio channel
 *
 * This function will set the radio channel for CPU LOW
 *
 * @param   channel     - Radio channel
 * @return  None
 *
 */
int wlan_platform_low_set_radio_channel(u32 channel) {

	if(channel <= 14) {
		// 2.4GHz channel
		radio_controller_setCenterFrequency(RC_BASEADDR, (RC_ALL_RF), RC_24GHZ, w3_wlan_chan_to_rc_chan(channel));
		gl_current_band = BAND_24GHZ;
	} else {
		// 5GHz channel
		radio_controller_setCenterFrequency(RC_BASEADDR, (RC_ALL_RF), RC_5GHZ, w3_wlan_chan_to_rc_chan(channel));
		gl_current_band = BAND_5GHZ;
	}


	return 0;
}


/********************************************************************************
 * Private functions - the functions below are not exported to the low framework
 ********************************************************************************/


/*****************************************************************************/
/**
 * Initialize the WARP v3 node
 *
 * @param   None
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 *****************************************************************************/
int w3_node_init() {

    int            ret_val             = XST_SUCCESS;
    int            status;
    u32            clkmod_status;

    // Initialize w3_clock_controller hardware and AD9512 buffers
    //   NOTE:  The clock initialization will set the clock divider to 2 (for 40MHz clock) to RF A/B AD9963's
    status = clk_init(CLK_BASEADDR, 2);
    if(status != XST_SUCCESS) {
        xil_printf("ERROR: (w3_node_init) Clock initialization failed with error code: %d\n", status);
        ret_val = XST_FAILURE;
    }

    // Check for a clock module and configure clock inputs, outputs and dividers as needed
    clkmod_status = clk_config_read_clkmod_status(CLK_BASEADDR);

    switch(clkmod_status & CM_STATUS_SW) {
        case CM_STATUS_DET_NOCM:
        case CM_STATUS_DET_CMPLL_BYPASS:
            // No clock module - default config from HDL/driver is good as-is
            xil_printf("No clock module detected - selecting on-board clocks\n\n");
        break;

        case CM_STATUS_DET_CMMMCX_CFG_A:
            // CM-MMCX config A:
            //     Samp clk: on-board, RF clk: on-board
            //     Samp MMCX output: 80MHz, RF MMCX output: 80MHz
            xil_printf("CM-MMCX Config A Detected:\n");
            xil_printf("  RF: On-board\n  Samp: On-board\n  MMCX Outputs: Enabled\n\n");

            clk_config_outputs(CLK_BASEADDR, CLK_OUTPUT_ON, (CLK_SAMP_OUTSEL_CLKMODHDR | CLK_RFREF_OUTSEL_CLKMODHDR));
            clk_config_dividers(CLK_BASEADDR, 1, CLK_SAMP_OUTSEL_CLKMODHDR | CLK_RFREF_OUTSEL_CLKMODHDR);
        break;

        case CM_STATUS_DET_CMMMCX_CFG_B:
            // CM-MMCX config B:
            //     Samp clk: off-board, RF clk: off-board
            //     Samp MMCX output: 80MHz, RF MMCX output: 80MHz
            xil_printf("CM-MMCX Config B Detected:\n");
            xil_printf("  RF: Off-board\n  Samp: Off-board\n  MMCX Outputs: Enabled\n\n");

            clk_config_input_rf_ref(CLK_BASEADDR, CLK_INSEL_CLKMOD);
            clk_config_outputs(CLK_BASEADDR, CLK_OUTPUT_ON, (CLK_SAMP_OUTSEL_CLKMODHDR | CLK_RFREF_OUTSEL_CLKMODHDR));
            clk_config_dividers(CLK_BASEADDR, 1, (CLK_SAMP_OUTSEL_CLKMODHDR | CLK_RFREF_OUTSEL_CLKMODHDR));
        break;

        case CM_STATUS_DET_CMMMCX_CFG_C:
            // CM-MMCX config C:
            //     Samp clk: off-board, RF clk: off-board
            //     Samp MMCX output: Off, RF MMCX output: Off
            xil_printf("CM-MMCX Config C Detected:\n");
            xil_printf("  RF: Off-board\n  Samp: Off-board\n  MMCX Outputs: Disabled\n\n");

            clk_config_input_rf_ref(CLK_BASEADDR, CLK_INSEL_CLKMOD);
            clk_config_outputs(CLK_BASEADDR, CLK_OUTPUT_OFF, (CLK_SAMP_OUTSEL_CLKMODHDR | CLK_RFREF_OUTSEL_CLKMODHDR));
        break;

        case CM_STATUS_DET_CMPLL_CFG_A:
            // CM-PLL config A:
            //     Samp clk: clock module PLL
            //     RF clk: on-board
            xil_printf("CM-PLL Config A Detected:\n");
            xil_printf("  RF: On-board\n  Samp: clock module PLL\n");

            // No changes from configuration applied by HDL and clk_init()
        break;

        case CM_STATUS_DET_CMPLL_CFG_B:
            // CM-PLL config B:
            //     Samp clk: clock module PLL
            //     RF clk: clock module PLL
            xil_printf("CM-PLL Config B Detected:\n");
            xil_printf("  RF: clock module PLL\n  Samp: clock module PLL\n");

            clk_config_input_rf_ref(CLK_BASEADDR, CLK_INSEL_CLKMOD);
        break;

        case CM_STATUS_DET_CMPLL_CFG_C:
            // CM-PLL config C:
            //     Samp clk: clock module PLL
            //     RF clk: clock module PLL
            xil_printf("CM-PLL Config C Detected:\n");
            xil_printf("  RF: clock module PLL\n  Samp: clock module PLL\n");

            clk_config_input_rf_ref(CLK_BASEADDR, CLK_INSEL_CLKMOD);
        break;

        default:
            // Should be impossible
            xil_printf("ERROR: (w3_node_init) Invalid clock module switch settings! (0x%08x)\n", clkmod_status);
            ret_val = XST_FAILURE;
        break;
    }

#ifdef WLAN_4RF_EN
    // Turn on clocks to FMC
    clk_config_outputs(CLK_BASEADDR, CLK_OUTPUT_ON, (CLK_SAMP_OUTSEL_FMC | CLK_RFREF_OUTSEL_FMC));

    // FMC samp clock divider = 2 (40MHz sampling reference, same as on-board AD9963 ref clk)
    clk_config_dividers(CLK_BASEADDR, 2, CLK_SAMP_OUTSEL_FMC);

    // FMC RF ref clock divider = 2 (40MHz RF reference, same as on-board MAX2829 ref clk)
    clk_config_dividers(CLK_BASEADDR, 2, CLK_RFREF_OUTSEL_FMC);
#endif

    // Initialize the AD9963 ADCs/DACs for on-board RF interfaces
    ad_init(AD_BASEADDR, AD_ALL_RF, 3);

    // Disable AD9963 Duty Cycle Stabilizer (recommended when ADCCLK < 75MHz)
    ad_config_clocks(AD_BASEADDR, AD_ALL_RF, AD_DACCLKSRC_EXT, AD_ADCCLKSRC_EXT, AD_ADCCLKDIV_1, AD_DCS_OFF);

    if(status != XST_SUCCESS) {
        xil_printf("ERROR: (w3_node_init) ADC/DAC initialization failed with error code: %d\n", status);
        ret_val = XST_FAILURE;
    }

    // Initialize the radio_controller core and MAX2829 transceivers for on-board RF interfaces
    status = radio_controller_init(RC_BASEADDR, RC_ALL_RF, 1, 1);

    if(status != XST_SUCCESS) {
        xil_printf("ERROR: (w3_node_init) Radio controller initialization failed with error code: %d\n", status);

        // Comment out the line below to allow the node to boot even if a radio PLL is unlocked
        ret_val = XST_FAILURE;
    }

#ifdef WLAN_4RF_EN
    //FIXME
    iic_eeprom_init(FMC_EEPROM_BASEADDR, 0x64);
#endif

    // Give the PHY control of the red user LEDs (PHY counts 1-hot on SIGNAL errors)
    //
    // NOTE: Uncommenting this line will make the RED LEDs controlled by hardware.
    //     This will move the LEDs on PHY bad signal events
    //
    // userio_set_ctrlSrc_hw(USERIO_BASEADDR, W3_USERIO_CTRLSRC_LEDS_RED);

	// Initialize Debug Header
    //  Configure pins 15:12 as software controlled outputs, pins 11:0 as hardware controlled
    //  This configuration is applied only by CPU Low to avoid races between CPUs at boot
    //  Both CPUs can control the software-controlled pins
	wlan_mac_set_dbg_hdr_ctrlsrc(DBG_HDR_CTRLSRC_HW, 0x0FFF);
	wlan_mac_set_dbg_hdr_ctrlsrc(DBG_HDR_CTRLSRC_SW, 0xF000);
	wlan_mac_set_dbg_hdr_dir(DBG_HDR_DIR_OUTPUT, 0xF000);

    return ret_val;
}

/*****************************************************************************/
/**
 * Initialize the Radio Controller
 *
 * @param   None
 *
 * @return  None
 *
 * @note    This function supports both 2RF and 4RF configurations.  Also, there are
 *     sections (ie #if sections) that have been added to assist if clocking changes
 *     are needed.
 *
 *****************************************************************************/
void w3_radio_init() {

#if 1

	// Setup clocking and filtering (20MSps, 2x interp/decimate in AD9963)
	clk_config_dividers(CLK_BASEADDR, 2, (CLK_SAMP_OUTSEL_AD_RFA | CLK_SAMP_OUTSEL_AD_RFB));
	ad_config_filters(AD_BASEADDR, AD_ALL_RF, 2, 2);
	ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x32, 0x27);
	ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x33, 0x08);

#else
    // Setup clocking and filtering:
    //     80MHz ref clk to AD9963
    //     20MSps Tx/Rx at FPGA
    //     80MHz DAC, 4x interp in AD9963
    //     40MHz ADC, 2x decimate in AD9963
    //
    clk_config_dividers(CLK_BASEADDR, 1, (CLK_SAMP_OUTSEL_AD_RFA | CLK_SAMP_OUTSEL_AD_RFB));

    ad_config_clocks(AD_BASEADDR, AD_ALL_RF, AD_DACCLKSRC_EXT, AD_ADCCLKSRC_EXT, AD_ADCCLKDIV_2, AD_DCS_OFF);

    ad_config_filters(AD_BASEADDR, AD_ALL_RF, 4, 2);
#endif

    // Setup all RF interfaces
    radio_controller_TxRxDisable(RC_BASEADDR, RC_ALL_RF);

    radio_controller_apply_TxDCO_calibration(AD_BASEADDR, EEPROM_BASEADDR, (RC_RFA | RC_RFB));
#ifdef WLAN_4RF_EN
    radio_controller_apply_TxDCO_calibration(AD_BASEADDR, FMC_EEPROM_BASEADDR, (RC_RFC | RC_RFD));
#endif

    radio_controller_setCenterFrequency(RC_BASEADDR, RC_ALL_RF, RC_24GHZ, 4);

    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_RSSI_HIGH_BW_EN, 0);

    // Filter bandwidths
    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_RXHPF_HIGH_CUTOFF_EN, 1);

#if 0
    // To set the gains manually for all radios:
    radio_controller_setCtrlSource(RC_BASEADDR, RC_ALL_RF, RC_REG0_RXHP_CTRLSRC, RC_CTRLSRC_REG);
    radio_controller_setRxHP(RC_BASEADDR, RC_ALL_RF, RC_RXHP_OFF);
    radio_controller_setRxGainSource(RC_BASEADDR, RC_ALL_RF, RC_GAINSRC_SPI);

    // Set Rx gains
    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_RXGAIN_RF, 1);
    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_RXGAIN_BB, 8);

#else
    // AGC
    radio_controller_setCtrlSource(RC_BASEADDR, RC_ALL_RF, RC_REG0_RXHP_CTRLSRC, RC_CTRLSRC_HW);
    radio_controller_setRxGainSource(RC_BASEADDR, RC_ALL_RF, RC_GAINSRC_HW);
#endif

    // Set Tx gains
    //
    // NOTE:  To use software to control the Tx gains, use the following lines:
    //     radio_controller_setTxGainSource(RC_BASEADDR, RC_ALL_RF, RC_GAINSRC_REG); //Used for software control of gains
    //     radio_controller_setTxGainTarget(RC_BASEADDR, RC_ALL_RF, 45);
    //
    radio_controller_setTxGainSource(RC_BASEADDR, RC_ALL_RF, RC_GAINSRC_HW); //Used for hardware control of gains

    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_TXGAIN_BB, 1);

    // Set misc radio params
    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_TXLINEARITY_PADRIVER, 2);
    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_TXLINEARITY_VGA, 0);
    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_TXLINEARITY_UPCONV, 0);

    // Set Tx state machine timing
    //
    // NOTE:  radio_controller_setTxDelays(dly_GainRamp, dly_PA, dly_TX, dly_PHY)
    //
    radio_controller_setTxDelays(RC_BASEADDR, 40, 20, 0, TX_RC_PHYSTART_DLY); //240 PA time after 180 PHY time is critical point

    // Configure the radio_controller Tx/Rx enable control sources
    //     The Tx PHY drives a 4-bit TxEn, one bit per RF interface
    //     The Tx PHY drives a 1-bit RxEn, common to all RF interfaces
    //     MAC software should select active Rx interface by changing RFA/RFB RxEn ctrl src between _HW and _REG
    radio_controller_setCtrlSource(RC_BASEADDR, RC_RFA, (RC_REG0_RXEN_CTRLSRC), RC_CTRLSRC_HW);
    radio_controller_setCtrlSource(RC_BASEADDR, RC_RFB, (RC_REG0_RXEN_CTRLSRC), RC_CTRLSRC_REG);

    radio_controller_setCtrlSource(RC_BASEADDR, (RC_RFA | RC_RFB), (RC_REG0_TXEN_CTRLSRC), RC_CTRLSRC_HW);

#ifdef WLAN_4RF_EN
    radio_controller_setCtrlSource(RC_BASEADDR, (RC_RFC | RC_RFD), (RC_REG0_TXEN_CTRLSRC), RC_CTRLSRC_HW);
    radio_controller_setCtrlSource(RC_BASEADDR, (RC_RFC | RC_RFD), (RC_REG0_RXEN_CTRLSRC), RC_CTRLSRC_REG);
#else
    // Disable any hardware control of RFC/RFD
    radio_controller_setCtrlSource(RC_BASEADDR, (RC_RFC | RC_RFD), (RC_REG0_RXEN_CTRLSRC | RC_REG0_TXEN_CTRLSRC), RC_CTRLSRC_REG);
#endif

    return;
}

int w3_agc_init() {

	// Post Rx_done reset delays for [rxhp, g_rf, g_bb]
	wlan_agc_set_reset_timing(4, 250, 250);

	// AGC config:
	//     RFG Thresh 3->2, 2->1, Avg_len_sel, V_DB_Adj, Init G_BB
	wlan_agc_set_config((256 - 56), (256 - 37), 0, 6, 24);

	// AGC RSSI->Rx power offsets
	wlan_agc_set_RSSI_pwr_calib(100, 85, 70);

	// AGC timing: start_dco, en_iir_filt
	wlan_agc_set_DCO_timing(100, (100 + 34));

	// AGC target output power (log scale)
	wlan_agc_set_target((64 - 16));

#if 0
	// To set the gains manually:

	//xil_printf("Switching to MGC\n");
	radio_controller_setCtrlSource(RC_BASEADDR, RC_ALL_RF, RC_REG0_RXHP_CTRLSRC, RC_CTRLSRC_REG);
	radio_controller_setRxHP(RC_BASEADDR, RC_ALL_RF, RC_RXHP_OFF);
	radio_controller_setRxGainSource(RC_BASEADDR, RC_ALL_RF, RC_GAINSRC_SPI);

	// Set Rx gains
	radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_RXGAIN_RF, 2);
	radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_RXGAIN_BB, 8);
#endif

    return 0;
}



/*****************************************************************************/
/**
 * @brief Map the WLAN channel frequencies onto the convention used by the radio controller
 */
inline u32 w3_wlan_chan_to_rc_chan(u32 mac_channel) {
    int return_value = 0;

    switch(mac_channel){
        // 2.4GHz channels
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
            return_value = mac_channel;
        break;

        // 5GHz channels
        case 36: // 5180 MHz
            return_value = 1;
        break;
        case 38: // 5190 MHz
            return_value = 2;
        break;
        case 40: // 5200 MHz
            return_value = 3;
        break;
        case 44: // 5220 MHz
            return_value = 4;
        break;
        case 46: // 5230 MHz
            return_value = 5;
        break;
        case 48: // 5240 MHz
            return_value = 6;
        break;
#if 0 // Disable these channels by default
        case 52: // 5260 MHz
            return_value = 7;
        break;
        case 54: // 5270 MHz
            return_value = 8;
        break;
        case 56: // 5280 MHz
            return_value = 9;
        break;
        case 60: // 5300 MHz
            return_value = 10;
        break;
        case 62: // 5310 MHz
            return_value = 11;
        break;
        case 64: // 5320 MHz
            return_value = 12;
        break;
        case 100: // 5500 MHz
            return_value = 13;
        break;
        case 102: // 5510 MHz
            return_value = 14;
        break;
        case 104: // 5520 MHz
            return_value = 15;
        break;
        case 108: // 5540 MHz
            return_value = 16;
        break;
        case 110: // 5550 MHz
            return_value = 17;
        break;
        case 112: // 5560 MHz
            return_value = 18;
        break;
        case 116: // 5580 MHz
            return_value = 19;
        break;
        case 118: // 5590 MHz
            return_value = 20;
        break;
        case 120: // 5600 MHz
            return_value = 21;
        break;
        case 124: // 5620 MHz
            return_value = 22;
        break;
        case 126: // 5630 MHz
            return_value = 23;
        break;
        case 128: // 5640 MHz
            return_value = 24;
        break;
        case 132: // 5660 MHz
            return_value = 25;
        break;
        case 134: // 5670 MHz
            return_value = 26;
        break;
        case 136: // 5680 MHz
            return_value = 27;
        break;
        case 140: // 5700 MHz
            return_value = 28;
        break;
        case 142: // 5710 MHz
            return_value = 29;
        break;
        case 144: // 5720 MHz
            return_value = 30;
        break;
        case 149: // 5745 MHz
            return_value = 31;
        break;
        case 151: // 5755 MHz
            return_value = 32;
        break;
        case 153: // 5765 MHz
            return_value = 33;
        break;
        case 157: // 5785 MHz
            return_value = 34;
        break;
        case 159: // 5795 MHz
            return_value = 35;
        break;
        case 161: // 5805 MHz
            return_value = 36;
        break;
        case 165: // 5825 MHz
            return_value = 37;
        break;
        case 172: // 5860 MHz
            return_value = 38;
        break;
        case 174: // 5870 MHz
        	return_value = 39;
		break;
        case 175: // 5875 MHz
        	return_value = 40;
		break;
        case 176: // 5880 MHz
        	return_value = 41;
		break;
        case 177: // 5885 MHz
        	return_value = 42;
		break;
        case 178: // 5890 MHz
        	return_value = 43;
		break;
#endif
    }

    return return_value;
}


/*****************************************************************************/
/**
 * @brief Calculates Rx Power (in dBm)
 *
 * This function calculates receive power for a given band, RSSI and LNA gain. This
 * provides a reasonable estimate of Rx power, accurate to a few dB for standard waveforms.
 *
 * This function does not use the VGA gain setting or I/Q magnitudes. The PHY should use these
 * to refine its own power measurement if needed.
 *
 * NOTE:  These lookup tables were developed as part of the RF characterization.  See:
 *     http://warpproject.org/trac/wiki/802.11/Benchmarks/Rx_Char
 *
 *
 * @param   rssi             - RSSI value from RF frontend
 * @param   lna_gain         - Value of LNA gain stage in RF frontend
 * @return  int              - Power in dBm
 */
static const s8 pow_lookup_B24[128]  = {-90, -90, -89, -88, -88, -87, -87, -86, -86, -85, -84, -84, -83, -83, -82, -82,
                                        -81, -81, -80, -79, -79, -78, -78, -77, -77, -76, -75, -75, -74, -74, -73, -73,
                                        -72, -72, -71, -70, -70, -69, -69, -68, -68, -67, -66, -66, -65, -65, -64, -64,
                                        -63, -63, -62, -61, -61, -60, -60, -59, -59, -58, -58, -57, -56, -56, -55, -55,
                                        -54, -54, -53, -52, -52, -51, -51, -50, -50, -49, -49, -48, -47, -47, -46, -46,
                                        -45, -45, -44, -43, -43, -42, -42, -41, -41, -40, -40, -39, -38, -38, -37, -37,
                                        -36, -36, -35, -34, -34, -33, -33, -32, -32, -31, -31, -30, -29, -29, -28, -28,
                                        -27, -27, -26, -26, -25, -24, -24, -23, -23, -22, -22, -21, -20, -20, -19, -19};

static const s8 pow_lookup_B5[128]   = {-97, -97, -96, -96, -95, -94, -94, -93, -93, -92, -92, -91, -90, -90, -89, -89,
                                        -88, -88, -87, -87, -86, -85, -85, -84, -84, -83, -83, -82, -81, -81, -80, -80,
                                        -79, -79, -78, -78, -77, -76, -76, -75, -75, -74, -74, -73, -72, -72, -71, -71,
                                        -70, -70, -69, -69, -68, -67, -67, -66, -66, -65, -65, -64, -63, -63, -62, -62,
                                        -61, -61, -60, -60, -59, -58, -58, -57, -57, -56, -56, -55, -54, -54, -53, -53,
                                        -52, -52, -51, -51, -50, -49, -49, -48, -48, -47, -47, -46, -45, -45, -44, -44,
                                        -43, -43, -42, -42, -41, -40, -40, -39, -39, -38, -38, -37, -36, -36, -35, -35,
                                        -34, -34, -33, -32, -32, -31, -31, -30, -30, -29, -29, -28, -27, -27, -26, -26};


inline int w3_rssi_to_rx_power(u16 rssi, u8 lna_gain, channel_band_t band) {

    int            power     = -100;
    u16            adj_rssi  = 0;

    if(band == BAND_24GHZ) {
        switch(lna_gain) {
            case 0:
            case 1:
                // Low LNA Gain State
                adj_rssi = rssi + (440 << PHY_RX_RSSI_SUM_LEN_BITS);
            break;

            case 2:
                // Medium LNA Gain State
                adj_rssi = rssi + (220 << PHY_RX_RSSI_SUM_LEN_BITS);
            break;

            case 3:
                // High LNA Gain State
                adj_rssi = rssi;
            break;
        }

        power = pow_lookup_B24[(adj_rssi >> (PHY_RX_RSSI_SUM_LEN_BITS+POW_LOOKUP_SHIFT))];

    } else if(band == BAND_5GHZ) {
        switch(lna_gain){
            case 0:
            case 1:
                // Low LNA Gain State
                adj_rssi = rssi + (540 << PHY_RX_RSSI_SUM_LEN_BITS);
            break;

            case 2:
                // Medium LNA Gain State
                adj_rssi = rssi + (280 << PHY_RX_RSSI_SUM_LEN_BITS);
            break;

            case 3:
                // High LNA Gain State
                adj_rssi = rssi;
            break;
        }

        power = pow_lookup_B5[(adj_rssi >> (PHY_RX_RSSI_SUM_LEN_BITS+POW_LOOKUP_SHIFT))];
    }

    return power;
}



/*****************************************************************************/
/**
 * @brief Calculates RSSI from Rx power (in dBm)
 *
 * This function calculates receive power for a given band, RSSI and LNA gain. This
 * provides a reasonable estimate of Rx power, accurate to a few dB for standard waveforms.
 *
 * This function does not use the VGA gain setting or I/Q magnitudes. The PHY should use these
 * to refine its own power measurement if needed.
 *
 * NOTE:  These lookup tables were developed as part of the RF characterization.  See:
 *     http://warpproject.org/trac/wiki/802.11/Benchmarks/Rx_Char
 *
 *
 * @param   rx_pow           - Receive power in dBm
 * @return  u16              - RSSI value
 *
 * @note    rx_pow must be in the range [PKT_DET_MIN_POWER_MIN, PKT_DET_MIN_POWER_MAX] inclusive
 */
static const u16 rssi_lookup_B24[61] = {  1,  16,  24,  40,  56,  72,  80,  96, 112, 128, 144, 152, 168, 184, 200, 208,
                                        224, 240, 256, 272, 280, 296, 312, 328, 336, 352, 368, 384, 400, 408, 424, 440,
                                        456, 472, 480, 496, 512, 528, 536, 552, 568, 584, 600, 608, 624, 640, 656, 664,
                                        680, 696, 712, 728, 736, 752, 768, 784, 792, 808, 824, 840, 856};

static const u16 rssi_lookup_B5[61]  = { 96, 112, 128, 144, 160, 168, 184, 200, 216, 224, 240, 256, 272, 288, 296, 312,
                                        328, 344, 352, 368, 384, 400, 416, 424, 440, 456, 472, 480, 496, 512, 528, 544,
                                        552, 568, 584, 600, 608, 624, 640, 656, 672, 680, 696, 712, 728, 736, 752, 768,
                                        784, 800, 808, 824, 840, 856, 864, 880, 896, 912, 920, 936, 952};


int w3_rx_power_to_rssi(s8 rx_pow, channel_band_t band) {
    u16 rssi_val = 0;

    if ((rx_pow <= PKT_DET_MIN_POWER_MAX) && (rx_pow >= PKT_DET_MIN_POWER_MIN)) {
        if(band == BAND_24GHZ){
            rssi_val = rssi_lookup_B24[rx_pow-PKT_DET_MIN_POWER_MIN];
        } else if(band == BAND_5GHZ){
            rssi_val = rssi_lookup_B5[rx_pow-PKT_DET_MIN_POWER_MIN];
        }

        return rssi_val;

    } else {
        return -1;
    }
}

int wlan_platform_get_rx_pkt_pwr(u8 ant) {
	u16 rssi;
	u8 lna_gain;
	int pwr;

	rssi = wlan_phy_rx_get_pkt_rssi(ant);
	lna_gain = wlan_phy_rx_get_agc_RFG(ant);
	pwr = w3_rssi_to_rx_power(rssi, lna_gain, gl_current_band);

	return pwr;
}

void wlan_platform_set_phy_cs_thresh(int power_thresh) {
	int safe_thresh;

	if(power_thresh == 0xFFFF) {
		// 0xFFFF means disable physical CS
		wlan_phy_rx_set_cca_thresh(0xFFFF);
		return;
	}

	if(power_thresh < -95) {
		safe_thresh = -95;
	} else if(power_thresh > -25) {
		safe_thresh = -25;
	} else {
		safe_thresh = power_thresh;
	}

	wlan_phy_rx_set_cca_thresh(PHY_RX_RSSI_SUM_LEN * w3_rx_power_to_rssi(safe_thresh, gl_current_band));

	return;
}

/*****************************************************************************/
/**
 * @brief Set the minimum power for packet detection
 *
 * @param   rx_pow           - Receive power in dBm
 * @return  int              - Status:  0 - Success; -1 - Failure
 */
int wlan_platform_set_pkt_det_min_power(int min_power) {
    int rssi_val;

    if(min_power == 0) {
    	// 0 disables the min-power logic
		wlan_phy_disable_req_both_pkt_det();
		return 0;

    } else {
		wlan_phy_enable_req_both_pkt_det();
	    rssi_val = w3_rx_power_to_rssi(min_power, gl_current_band);

	    if(rssi_val != -1) {
	        wlan_phy_rx_pktDet_RSSI_cfg( (PHY_RX_RSSI_SUM_LEN-1), (rssi_val << PHY_RX_RSSI_SUM_LEN_BITS), 1);

	        return  0;
	    } else {
	    	xil_printf("wlan_platform_set_pkt_det_min_power: invalid min_power argument: %d\n", min_power);
	        return -1;
	    }
    }
}

int wlan_platform_get_rx_pkt_gain(u8 ant) {
	u8 bb_gain;
	u8 rf_gain;

	switch(ant) {
		case 0: // RF A
			bb_gain = (Xil_In32(WLAN_RX_PKT_AGC_GAINS) >>  0) & 0x1F;
			rf_gain = (Xil_In32(WLAN_RX_PKT_AGC_GAINS) >>  5) & 0x03;
		break;
		case 1: // RF B
			bb_gain = (Xil_In32(WLAN_RX_PKT_AGC_GAINS) >>  8) & 0x1F;
			rf_gain = (Xil_In32(WLAN_RX_PKT_AGC_GAINS) >>  13) & 0x03;
		break;
		case 2: // RF C
			bb_gain = (Xil_In32(WLAN_RX_PKT_AGC_GAINS) >>  16) & 0x1F;
			rf_gain = (Xil_In32(WLAN_RX_PKT_AGC_GAINS) >>  21) & 0x03;
		break;
		case 3: // RF D
			bb_gain = (Xil_In32(WLAN_RX_PKT_AGC_GAINS) >>  24) & 0x1F;
			rf_gain = (Xil_In32(WLAN_RX_PKT_AGC_GAINS) >>  29) & 0x03;
		break;
		default:
			xil_printf("wlan_platform_get_rx_pkt_gain: invalid antenna ID: %d\n", ant);
			return -1;
		break;
	}

	// For MAX2829 RF interface construct 8-bit gain_index value as:
	//  [  8]: 0
	//  [6:5]: RF gain index (0, 1, 2)
	//  [4:0]: BB gain index (0, 1, ..., 31)
	return ((rf_gain << 5) | bb_gain);
}

int wlan_platform_set_radio_tx_power(s8 power) {
	// Empty function for WARP v3 - all Tx powers are configured per-packet via
	//  tx_frame_info and the mac_hw core driving the radio_controller's Tx gain pins

	return 0;
}
