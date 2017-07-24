/** @file wlan_mac_nomac.c
 *  @brief Simple MAC that does nothing but transmit and receive
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/***************************** Include Files *********************************/

// Xilinx SDK includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xio.h"
#include "xil_cache.h"

// WLAN includes
#include "wlan_platform_common.h"
#include "wlan_platform_low.h"
#include "wlan_mac_low.h"
#include "wlan_mac_pkt_buf_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_phy_util.h"
#include "wlan_platform_low.h"
#include "wlan_mac_nomac.h"
#include "xparameters.h"
#include "wlan_mac_common.h"
#include "wlan_mac_mailbox_util.h"

// WLAN Exp includes
#include "wlan_exp.h"


/*************************** Constant Definitions ****************************/
#define WLAN_EXP_TYPE_DESIGN_80211_CPU_LOW WLAN_EXP_TYPE_DESIGN_80211_CPU_LOW_NOMAC

#define DEFAULT_TX_ANTENNA_MODE TX_ANTMODE_SISO_ANTA


/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/
static u8 eeprom_addr[MAC_ADDR_LEN];

// Common Platform Device Info
platform_common_dev_info_t platform_common_dev_info;


/*************************** Functions Prototypes ****************************/

int process_low_param(u8 mode, u32* payload);


/******************************** Functions **********************************/

int main(){
	// Initialize Microblaze --
	//  these functions should be called before anything
	//  else is executed
	Xil_DCacheDisable();
	Xil_ICacheDisable();
	microblaze_enable_exceptions();

    wlan_mac_hw_info_t* hw_info;
    compilation_details_t compilation_details;
    bzero(&compilation_details, sizeof(compilation_details_t));

    xil_printf("\f");
    xil_printf("----- Mango 802.11 Reference Design -----\n");
    xil_printf("----- v1.7.1 ----------------------------\n");
    xil_printf("----- wlan_mac_nomac --------------------\n");
    xil_printf("Compiled %s %s\n\n", __DATE__, __TIME__);
	strncpy(compilation_details.compilation_date, __DATE__, 12);
	strncpy(compilation_details.compilation_time, __TIME__, 9);

    xil_printf("Note: this UART is currently printing from CPU_LOW. To view prints from\n");
    xil_printf("and interact with CPU_HIGH, raise the right-most User I/O DIP switch bit.\n");
    xil_printf("This switch can be toggled live while the design is running.\n\n");
    xil_printf("------------------------\n");

    wlan_mac_common_malloc_init();

    // Initialize the Low Framework
    wlan_mac_low_init(WLAN_EXP_TYPE_DESIGN_80211_CPU_LOW, compilation_details);

    // Get the device info
	platform_common_dev_info = wlan_platform_common_get_dev_info();

    // Get the node's HW address
    hw_info = get_mac_hw_info();
    memcpy(eeprom_addr, hw_info->hw_addr_wlan, MAC_ADDR_LEN);

    // Set up the TX / RX callbacks
    wlan_mac_low_set_frame_rx_callback((void*)frame_receive);
    wlan_mac_low_set_ipc_low_param_callback((void*)process_low_param);
    wlan_mac_low_set_handle_tx_pkt_buf_ready((void*)handle_tx_pkt_buf_ready);
    // wlan_mac_low_set_sample_rate_change_callback() not used at this time.

    // Finish Low Framework initialization
    wlan_mac_low_init_finish();

    // Set the MAC HW:
    //     - Ignore carrier sensing
    //     - Ignore NAV
    //
    REG_SET_BITS(WLAN_MAC_REG_CONTROL, (WLAN_MAC_CTRL_MASK_CCA_IGNORE_PHY_CS | WLAN_MAC_CTRL_MASK_CCA_IGNORE_NAV));

    // Print NOMAC information to the terminal
    xil_printf("------------------------\n");
    xil_printf("WLAN MAC NOMAC boot complete: \n");
    xil_printf("  Serial Number     : W3-a-%05d\n", hw_info->serial_number);
    xil_printf("  Wireless MAC Addr : %02x:%02x:%02x:%02x:%02x:%02x\n\n", eeprom_addr[0], eeprom_addr[1], eeprom_addr[2], eeprom_addr[3], eeprom_addr[4], eeprom_addr[5]);

    while(1){
        // Poll PHY RX start
        wlan_mac_low_poll_frame_rx();

        // Poll IPC rx
        wlan_mac_low_poll_ipc_rx();
    }

    return 0;
}



/*****************************************************************************/
/**
 * @brief Handles reception of a wireless packet
 *
 * This function is called after a good SIGNAL field is detected by either PHY (OFDM or DSSS)
 *
 * It is the responsibility of this function to wait until a sufficient number of bytes have been received
 * before it can start to process those bytes. When this function is called the eventual checksum status is
 * unknown. In NOMAC, this function doesn't need to do any kind of filtering or operations like transmitting
 * an acknowledgment.  This should be modified to fit the user's needs.
 *
 * NOTE: The timing of this function is critical for correct operation of the 802.11. It is not
 *     safe to add large delays to this function (e.g. xil_printf or wlan_usleep)
 *
 * @param   rx_pkt_buf       - Index of the Rx packet buffer containing the newly recevied packet
 * @param   phy_details      - Pointer to phy_rx_details struct containing PHY mode, MCS, and Length
 * @return  u32              - Bit mask of flags indicating various results of the reception
 *
 * @note    Default NOMAC implementation always returns 0
 */
u32 frame_receive(u8 rx_pkt_buf, phy_rx_details_t* phy_details){

    void* pkt_buf_addr = (void*) CALC_PKT_BUF_ADDR(platform_common_dev_info.rx_pkt_buf_baseaddr, rx_pkt_buf);
    rx_frame_info_t* rx_frame_info = (rx_frame_info_t*) pkt_buf_addr;

    // Wait for the Rx PHY to finish receiving this packet
	if(wlan_mac_hw_rx_finish() == 1){
		//FCS was good
		rx_frame_info->flags |= RX_FRAME_INFO_FLAGS_FCS_GOOD;
	} else {
		//FCS was bad
		rx_frame_info->flags &= ~RX_FRAME_INFO_FLAGS_FCS_GOOD;
	}

    // Increment the LEDs based on the FCS status
    if(rx_frame_info->flags & RX_FRAME_INFO_FLAGS_FCS_GOOD){
    	wlan_platform_low_userio_disp_status(USERIO_DISP_STATUS_GOOD_FCS_EVENT);
    } else {
    	wlan_platform_low_userio_disp_status(USERIO_DISP_STATUS_BAD_FCS_EVENT);
    }

    rx_frame_info->rx_pkt_buf_state = RX_PKT_BUF_READY;
	if (unlock_rx_pkt_buf(rx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS) {
		xil_printf("Error: unable to unlock RX pkt_buf %d\n", rx_pkt_buf);
		wlan_mac_low_send_exception(WLAN_ERROR_CODE_CPU_LOW_RX_MUTEX);
	} else {
		wlan_mac_low_frame_ipc_send();

		// Find a free packet buffer and begin receiving packets there (blocks until free buf is found)
		wlan_mac_low_lock_empty_rx_pkt_buf();
	}


    return 0;
}

int handle_tx_pkt_buf_ready(u8 pkt_buf){
	if( wlan_mac_low_prepare_frame_transmit(pkt_buf) == 0 ){
		frame_transmit(pkt_buf);
		wlan_mac_low_finish_frame_transmit(pkt_buf);
		return 0;
	} else {
		return -1;
	}
}

/*****************************************************************************/
/**
 * @brief Handles transmission of a wireless packet
 *
 * This function is called to transmit a new packet via the PHY. While the code does utilize the wlan_mac_dcf_hw core,
 * it bypasses any of the DCF-specific state in order to directly transmit the frame. This function should be called
 * once per packet and will return immediately following that transmission. It will not perform any DCF-like retransmissions.
 *
 * This function is called once per IPC_MBOX_TX_MPDU_READY message from CPU High. The IPC_MBOX_TX_MPDU_DONE message will be
 * sent back to CPU High when this function returns.
 *
 * @param   pkt_buf          - Index of the Tx packet buffer containing the packet to transmit
 * @return  int              - Transmission result
 */
int frame_transmit(u8 pkt_buf) {

    u32 mac_hw_status;
    u32 mac_tx_ctrl_status;
    u8 tx_gain;
    wlan_mac_low_tx_details_t low_tx_details;

    tx_frame_info_t* tx_frame_info = (tx_frame_info_t*) (CALC_PKT_BUF_ADDR(platform_common_dev_info.tx_pkt_buf_baseaddr, pkt_buf));
    u8 mpdu_tx_ant_mask = 0;

    // Extract waveform params from the tx_frame_info
    u8  mcs = tx_frame_info->params.phy.mcs;
    u8  phy_mode = (tx_frame_info->params.phy.phy_mode & (PHY_MODE_HTMF | PHY_MODE_NONHT));
    u16 length = tx_frame_info->length;

    // Write the PHY premable (SIGNAL or L-SIG/HT-SIG) to the packet buffer
    write_phy_preamble(pkt_buf, phy_mode, mcs, length);

    // Set the antenna mode
    switch(tx_frame_info->params.phy.antenna_mode) {
        case TX_ANTMODE_SISO_ANTA:  mpdu_tx_ant_mask |= 0x1;  break;
        case TX_ANTMODE_SISO_ANTB:  mpdu_tx_ant_mask |= 0x2;  break;
        case TX_ANTMODE_SISO_ANTC:  mpdu_tx_ant_mask |= 0x4;  break;
        case TX_ANTMODE_SISO_ANTD:  mpdu_tx_ant_mask |= 0x8;  break;
        default:                    mpdu_tx_ant_mask  = 0x1;  break;      // Default to RF_A
    }

    // Fill in the number of attempts to transmit the packet
    tx_frame_info->num_tx_attempts = 1;

    // Update tx_frame_info with current PHY sampling rate
    tx_frame_info->phy_samp_rate = (u8)wlan_mac_low_get_phy_samp_rate();

    // Convert the requested Tx power (dBm) to a Tx gain setting for the radio
    tx_gain = wlan_mac_low_dbm_to_gain_target(tx_frame_info->params.phy.power);

    // Set the MAC HW control parameters
    //  args: (pktBuf, antMask, preTx_backoff_slots, preWait_postRxTimer1, preWait_postTxTimer1, postWait_postTxTimer2, phy_mode)
    wlan_mac_tx_ctrl_A_params(pkt_buf, mpdu_tx_ant_mask, 0, 0, 0, 0, phy_mode);

    // Set Tx Gains - use same gain for all RF interfaces
    wlan_mac_tx_ctrl_A_gains(tx_gain, tx_gain, tx_gain, tx_gain);

    // Before we mess with any PHY state, we need to make sure it isn't actively
    //  transmitting. For example, it may be sending an ACK when we get to this part of the code
    while (wlan_mac_get_status() & WLAN_MAC_STATUS_MASK_TX_PHY_ACTIVE) {}

    // Submit the MPDU for transmission - this starts the MAC hardware's MPDU Tx state machine
    wlan_mac_tx_ctrl_A_start(1);
    wlan_mac_tx_ctrl_A_start(0);

    // Fill in the Tx low details
	low_tx_details.tx_details_type = TX_DETAILS_MPDU;
	low_tx_details.phy_params_mpdu.mcs = mcs;
	low_tx_details.phy_params_mpdu.phy_mode = phy_mode;
	low_tx_details.phy_params_mpdu.power = tx_frame_info->params.phy.power;
	low_tx_details.phy_params_mpdu.antenna_mode = tx_frame_info->params.phy.antenna_mode;
	low_tx_details.chan_num = wlan_mac_low_get_active_channel();
	low_tx_details.num_slots = 0;
	low_tx_details.cw = 0;
	low_tx_details.attempt_number = 1;

    // Wait for the PHY Tx to finish
    do{
        // Get the MAC HW status
        mac_hw_status = wlan_mac_get_status();
        mac_tx_ctrl_status = wlan_mac_get_tx_ctrl_status();

        // If the MAC HW is done, fill in the remaining Tx low details and return
        if (mac_tx_ctrl_status & WLAN_MAC_TXCTRL_STATUS_MASK_TX_A_DONE) {

			low_tx_details.tx_start_timestamp_mpdu = wlan_mac_low_get_tx_start_timestamp();
			low_tx_details.tx_start_timestamp_frac_mpdu = wlan_mac_low_get_tx_start_timestamp_frac();

			// Send IPC message containing the details about this low-level transmission
			wlan_mac_low_send_low_tx_details(pkt_buf, &low_tx_details);


            // Set return value based on Tx A result
            //  This is easy for NoMAC - all transmissions are immediately successful
            switch (mac_tx_ctrl_status & WLAN_MAC_TXCTRL_STATUS_MASK_TX_A_RESULT) {
                case WLAN_MAC_TXCTRL_STATUS_TX_A_RESULT_NONE:
                default:
                    return TX_FRAME_INFO_RESULT_SUCCESS;
                break;
            }
        }
    } while (mac_hw_status & WLAN_MAC_STATUS_MASK_TX_A_PENDING);


	// NoMAC Tx is always "successful"
    return TX_FRAME_INFO_RESULT_SUCCESS;
}



/*****************************************************************************/
/**
 * @brief Process NOMAC Low Parameters
 *
 * This method is part of the IPC_MBOX_LOW_PARAM parameter processing in the low framework.  It
 * will process NOMAC specific low parameters.
 *
 * @param   mode             - Mode to process parameter:  IPC_REG_WRITE_MODE or IPC_REG_READ_MODE
 * @param   payload          - Pointer to parameter and arguments
 * @return  int              - Status
 */
int process_low_param(u8 mode, u32* payload) {

    switch(mode){
        case IPC_REG_WRITE_MODE:
            switch(payload[0]){

#if 0
                //---------------------------------------------------------------------
                case <Parameter #define in wlan_mac_nomac.h>: {
                    // Implementation of parameter write
                }
                break;
#endif

                //---------------------------------------------------------------------
                default: {}
                break;
            }
        break;

        case IPC_REG_READ_MODE: {
            // Not supported.  See comment in wlan_mac_low.c for IPC_REG_READ_MODE mode.
        }
        break;

        default: {
            xil_printf("Unknown mode 0x%08x\n", mode);
        }
        break;
    }

    return 0;
}
