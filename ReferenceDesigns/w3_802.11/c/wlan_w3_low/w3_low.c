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

// Platform includes
#include "w3_mac_phy_regs.h"
#include "w3_low.h"
#include "wlan_platform_common.h"

// Low framework includes
#include "wlan_phy_util.h"

// WARP v3 hardware includes
#include "w3_userio.h"
#include "w3_ad_controller.h"
#include "w3_clock_controller.h"
#include "radio_controller.h"

// Common Platform Device Info
static platform_common_dev_info_t	 platform_common_dev_info;

/*****************************************************************************
 * Public functions - the functions below are exported to the low framework
 *****************************************************************************/
int wlan_platform_low_init() {
	int status;

	// Get the device info
	platform_common_dev_info = wlan_platform_common_get_dev_info();

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
	//FIXME: implement switch/case over W3-specific low param IDs that used to be in wlan_mac_low.c

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
	} else {
		// 5GHz channel
		radio_controller_setCenterFrequency(RC_BASEADDR, (RC_ALL_RF), RC_5GHZ, w3_wlan_chan_to_rc_chan(channel));
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

    // Enable excpetions
    microblaze_enable_exceptions();

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

    radio_controller_apply_TxDCO_calibration(AD_BASEADDR, platform_common_dev_info.eeprom_baseaddr, (RC_RFA | RC_RFB));
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
