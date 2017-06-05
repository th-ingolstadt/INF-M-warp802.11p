/** @file wlan_mac_low.c
 *  @brief Low-level WLAN MAC High Framework
 *
 *  This contains the low-level code for accessing the WLAN MAC Low Framework.
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */
/***************************** Include Files *********************************/

// Xilinx / Standard library includes
#include <xil_io.h>
#include <xio.h>
#include <stdlib.h>
#include <string.h>
#include <xstatus.h>

// WLAN includes
#include "xparameters.h"
#include "wlan_platform_common.h"
#include "wlan_platform_low.h"
#include "wlan_mac_mailbox_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_phy_util.h"
#include "wlan_mac_low.h"
#include "wlan_mac_common.h"
#include "wlan_mac_pkt_buf_util.h"


// WLAN Exp includes
#include "wlan_exp.h"


/*************************** Constant Definitions ****************************/

#define DBG_PRINT  0


/*********************** Global Variable Definitions *************************/

/*************************** Functions Prototypes ****************************/


/*************************** Variable Definitions ****************************/
static volatile phy_samp_rate_t   gl_phy_samp_rate;                                 ///< Current PHY sampling rate
static volatile u32               mac_param_chan;                                   ///< Current channel of the lower-level MAC
static volatile u8                mac_param_band;                                   ///< Current band of the lower-level MAC
static volatile u8                mac_param_dsss_en;                                ///< Enable / Disable DSSS when possible
static volatile s8                mac_param_ctrl_tx_pow;                            ///< Current transmit power (dBm) for control packets
static volatile u32               mac_param_rx_filter;                              ///< Current filter applied to packet receptions
static volatile u8                rx_pkt_buf;                                       ///< Current receive buffer of the lower-level MAC

static u32                        cpu_low_status;                                   ///< Status flags that are reported to upper-level MAC
static u32						  cpu_low_type;										///< wlan_exp CPU_LOW type that is reported to upper-level MAC
static compilation_details_t	  cpu_low_compilation_details;

// Common Platform Device Info
platform_common_dev_info_t	 platform_common_dev_info;

static wlan_ipc_msg_t        	  ipc_msg_from_high;                                          ///< Buffer for incoming IPC messages
static u32                   	  ipc_msg_from_high_payload[MAILBOX_BUFFER_MAX_NUM_WORDS];    ///< Buffer for payload of incoming IPC messages

// Callback function pointers
static function_ptr_t        frame_rx_callback;                                     ///< User callback frame receptions

static function_ptr_t		 beacon_txrx_config_callback;
static function_ptr_t		 mcast_buffer_enable_callback;
static function_ptr_t		 mactime_change_callback;
static function_ptr_t		 sample_rate_change_callback;
static function_ptr_t		 handle_tx_pkt_buf_ready;

static function_ptr_t        ipc_low_param_callback;                                ///< User callback for IPC_MBOX_LOW_PARAM ipc calls

// Unique transmit sequence number
static volatile u64	         unique_seq;

// Constant LUTs for MCS
static const u16 mcs_to_n_dbps_nonht_lut[] = {24, 36, 48, 72, 96, 144, 192, 216};
static const u16 mcs_to_n_dbps_htmf_lut[] = {26, 52, 78, 104, 156, 208, 234, 260};

/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * @brief Initialize MAC Low Framework
 *
 * This function initializes the MAC Low Framework by setting
 * up the hardware and other subsystems in the framework.
 *
 * @param   type                - Lower-level MAC type
 * @param	compilation_details - compilation_details_t struct from low-level application
 * @return  int                 - Initialization status (0 = success)
 */
int wlan_mac_low_init(u32 type, compilation_details_t compilation_details){
    u32 		     status;
    rx_frame_info_t* rx_frame_info;
	tx_frame_info_t* tx_frame_info;
    u32			     i;

    /**********************************************************************************
     * Initialize the low platform first - this must happen before the low application
     *  attempts to use any hardware resources
     **********************************************************************************/
    status = wlan_platform_common_init();
    if(status != 0) {
        xil_printf("Error in wlan_platform_common_init()! Exiting\n");
        return -1;
    }
    status = wlan_platform_low_init();

    if(status != 0) {
        xil_printf("Error in wlan_platform_low_init()! Exiting\n");
        return -1;
    }
    /**********************************************************************************/

    // Get the device info
	platform_common_dev_info = wlan_platform_common_get_dev_info();

    /**********************************************************************************
     * Initialize the MAC and PHY cores - this must happen before the low application
     *  attempts any wireless Tx/Rx operations
     * These calls will reset the MAC and PHY cores, safely interrupting any ongoing
     *  Tx/Rx events and clearing old MAC state that may remain from a previous boot
     **********************************************************************************/
    wlan_phy_init();
    wlan_mac_hw_init();


    mac_param_dsss_en        = 1;
    mac_param_band           = CHAN_BAND_24GHz;
    mac_param_ctrl_tx_pow    = 10;
    cpu_low_status           = 0;
    cpu_low_type			 = type;
    cpu_low_compilation_details = compilation_details;

    unique_seq = 0;

    //Set the TU Target to the max value
    wlan_mac_set_tu_target(0xFFFFFFFFFFFFFFFFULL);

    mac_param_rx_filter      = (RX_FILTER_FCS_ALL | RX_FILTER_HDR_ALL);

    frame_rx_callback            = (function_ptr_t) wlan_null_callback;
    ipc_low_param_callback       = (function_ptr_t) wlan_null_callback;
    beacon_txrx_config_callback  = (function_ptr_t) wlan_null_callback;
    mcast_buffer_enable_callback = (function_ptr_t) wlan_null_callback;
    mactime_change_callback		 = (function_ptr_t) wlan_null_callback;
    sample_rate_change_callback  = (function_ptr_t) wlan_null_callback;
    handle_tx_pkt_buf_ready		 = (function_ptr_t) wlan_null_callback;

    // Initialize mailbox
	init_mailbox();

    // Initialize packet buffers
	init_pkt_buf();

	// ***************************************************
	// Initialize Transmit Packet Buffers
	// ***************************************************
	for(i = 0; i < NUM_TX_PKT_BUFS; i++){
		tx_frame_info = (tx_frame_info_t*)CALC_PKT_BUF_ADDR(platform_common_dev_info.tx_pkt_buf_baseaddr, i);
		switch(i){
			case TX_PKT_BUF_MPDU_1:
			case TX_PKT_BUF_MPDU_2:
			case TX_PKT_BUF_MPDU_3:
				switch(tx_frame_info->tx_pkt_buf_state){
					case TX_PKT_BUF_UNINITIALIZED:
					case TX_PKT_BUF_HIGH_CTRL:
						// CPU High will initialize
						break;
					break;
					case TX_PKT_BUF_READY:
					case TX_PKT_BUF_DONE:
			            // CPU Low rebooted after finishing old Tx
			            // No way to know if CPU Low sent TX_DONE(p) message - must reset p.state here
			            //  Two potential races:
			            //   -CPU High just rebooted and will also attempt setting p.state=TX_PKT_BUF_HIGH_CTRL
			            //      No problem if both CPUs set state to TX_PKT_BUF_HIGH_CTRL
			            //   -CPU High did not reboot and will attempt tx_done_handler(p)
			            //      If p.state=TX_PKT_BUF_HIGH_CTRL when tx_done_handler(p) runs, CPU High will fail gracefully
			            //      If p.state set to TX_PKT_BUF_HIGH_CTRL during tx_done_handler(p), CPU High will succeed normally
					case TX_PKT_BUF_LOW_CTRL:
			            // CPU Low rebooted after CPU High submitted packet for Tx
			            //  Release lock and reset state
			            //  CPU High will find this TX_PKT_BUF_HIGH_CTRL buffer in next ping/pong update
					default:
						// Something went wrong if tx_pkt_buf_state is something
						// other than one of the tx_pkt_buf_state_t enums. We'll
						// attempt to resolve the problem by explicitly setting
						// the state.
						tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_HIGH_CTRL;
						unlock_tx_pkt_buf(i);
					break;
				}
			break;
			case TX_PKT_BUF_BEACON:
				unlock_tx_pkt_buf(i);
			break;
			case TX_PKT_BUF_RTS:
			case TX_PKT_BUF_ACK_CTS:
				force_lock_tx_pkt_buf(i);
				tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_LOW_CTRL;
			default:
			break;
		}
	}
	// ***************************************************
	// Initialize Receive Packet Buffers
	// ***************************************************
	for(i = 0; i < NUM_RX_PKT_BUFS; i++){
		rx_frame_info = (rx_frame_info_t*)CALC_PKT_BUF_ADDR(platform_common_dev_info.rx_pkt_buf_baseaddr, i);
		switch(rx_frame_info->rx_pkt_buf_state){
		   case RX_PKT_BUF_UNINITIALIZED:
		   case RX_PKT_BUF_LOW_CTRL:
		   default:
				// Something went wrong if rx_pkt_buf_state is something
				// other than one of the rx_pkt_buf_state_t enums. We'll
				// attempt to resolve the problem by explicitly setting
				// the state.
			   force_lock_rx_pkt_buf(i);
			   rx_frame_info->rx_pkt_buf_state = RX_PKT_BUF_LOW_CTRL;
		   break;
		   case RX_PKT_BUF_HIGH_CTRL:
		   case RX_PKT_BUF_READY:
	           // CPU Low rebooted after submitting packet for de-encapsulation/logging
	           //  Will be handled by CPU High, either because CPU High is about
	           //  to de-encapsulate/log p or just rebooted and will clean up
		   break;
		}
	}

    // Create IPC message to receive into
    ipc_msg_from_high.payload_ptr = &(ipc_msg_from_high_payload[0]);

    // Point the PHY to an empty Rx Pkt Buffer
    wlan_mac_low_lock_empty_rx_pkt_buf();

    // Move the PHY's starting address into the packet buffers by PHY_XX_PKT_BUF_PHY_HDR_OFFSET.
    // This accounts for the metadata located at the front of every packet buffer (Xx_mpdu_info)
    wlan_phy_rx_pkt_buf_phy_hdr_offset(PHY_RX_PKT_BUF_PHY_HDR_OFFSET);
    wlan_phy_tx_pkt_buf_phy_hdr_offset(PHY_TX_PKT_BUF_PHY_HDR_OFFSET);


	// Unpause MAC Tx Controllers
	wlan_mac_pause_tx_ctrl_A(0);
	wlan_mac_pause_tx_ctrl_C(0);
	wlan_mac_pause_tx_ctrl_D(0);

    // Initialize the HW info structure
    init_mac_hw_info();

    // Set the NAV ignore addr to this HW address
    wlan_mac_low_set_nav_check_addr(get_mac_hw_addr_wlan());

    return 0;
}



/*****************************************************************************/
/**
 * @brief Finish Initializing MAC Low Framework
 *
 * This function finishes the initialization and notifies the upper-level
 * MAC that it has finished booting.
 *
 * @param   None
 * @return  None
 */
void wlan_mac_low_init_finish(){

    //Set the default PHY sample rate to 20 MSps
	set_phy_samp_rate(PHY_20M);

    // Update the CPU Low status
    cpu_low_status |= CPU_STATUS_INITIALIZED;

    wlan_mac_low_send_status(CPU_STATUS_REASON_BOOTED);

}

void wlan_mac_low_send_status(u8 cpu_status_reason){
	wlan_ipc_msg_t ipc_msg_to_high;
	u32            ipc_msg_to_high_payload[2+(sizeof(compilation_details_t)/sizeof(u32))];

	// Send a message to other processor to say that this processor is initialized and ready
	ipc_msg_to_high.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_CPU_STATUS);
	ipc_msg_to_high.arg0			  = cpu_status_reason;
	ipc_msg_to_high.num_payload_words = 2+(sizeof(compilation_details_t)/sizeof(u32));
	ipc_msg_to_high.payload_ptr       = &(ipc_msg_to_high_payload[0]);
	ipc_msg_to_high_payload[0]        = cpu_low_status;
	ipc_msg_to_high_payload[1]        = cpu_low_type;
	memcpy((u8*)&(ipc_msg_to_high_payload[2]), (u8*)&cpu_low_compilation_details, sizeof(compilation_details_t));

	write_mailbox_msg(&ipc_msg_to_high);
}


/*****************************************************************************/
/**
 * @brief Set the PHY Sampling Rate
 *
 * This function should be called to switch the PHY sampling rate between
 * 10/20/40 MSps.
 *
 * @param   phy_samp_rate_t phy_samp_rate
 * @return  None
 */
void set_phy_samp_rate(phy_samp_rate_t phy_samp_rate) {

    // Check sample rate argument
    //     - Must be in [PHY_10M, PHY_20M, PHY_40M]
    if (!((phy_samp_rate == PHY_10M) || (phy_samp_rate == PHY_20M) || (phy_samp_rate == PHY_40M))) {
        return;
    }

    // Set global sample rate variable
	gl_phy_samp_rate = phy_samp_rate;

    // Call the platform's set_samp_rate first
    wlan_platform_low_set_samp_rate(phy_samp_rate);

    // DSSS Rx only supported at 20Msps
    switch(phy_samp_rate){
		case PHY_10M:
    	case PHY_40M:
    		// Always disable DSSS when PHY sample rate is not 20 MSps
    		wlan_phy_DSSS_rx_disable();
    	break;
    	case PHY_20M:
			// Enable DSSS if global variable indicates it should be enabled and RF band allows it
    		if ((mac_param_dsss_en) && (mac_param_band == CHAN_BAND_24GHz)) {
    			wlan_phy_DSSS_rx_enable();
			}
    	break;
    }

    // Call user callback so it can deal with any changes that need to happen due to a change in sampling rate
	sample_rate_change_callback(gl_phy_samp_rate);

}

/*****************************************************************************/
/**
 * @brief Initialize the DCF Hardware Core
 *
 * This function initializes the DCF hardware core.
 *
 * @param   None
 * @return  None
 */
void wlan_mac_hw_init(){

	// Reset the MAC core - this clears any stale state in the Tx controllers, NAV counter, backoff counters, etc.
	wlan_mac_reset(1);

    // Enable blocking of the Rx PHY following good-FCS receptions and bad-FCS receptions
    //     BLOCK_RX_ON_VALID_RXEND will block the Rx PHY on all RX_END events following valid RX_START events
    //     This allows the wlan_exp framework to count and log bad FCS receptions
    //
    REG_SET_BITS(WLAN_MAC_REG_CONTROL, WLAN_MAC_CTRL_MASK_BLOCK_RX_ON_TX);

    // Enable the NAV counter
    REG_CLEAR_BITS(WLAN_MAC_REG_CONTROL, (WLAN_MAC_CTRL_MASK_DISABLE_NAV));

    // Set sane defaults for MAC timing values. These will be overwritten by
    // low-level applications that need to specify these times (e.g. the DCF)
    wlan_mac_set_slot(9*10);
	wlan_mac_set_DIFS(28*10);
	wlan_mac_set_TxDIFS((28*10) - (TX_PHY_DLY_100NSEC));
	wlan_mac_postTx_timer1_en(0);
	wlan_mac_postRx_timer2_en(0);
	wlan_mac_set_NAV_adj(0*10);
	wlan_mac_set_EIFS(88*10);

    // Set the TU target to 2^32-1 (max value) and hold TU_LATCH in reset
    //  MAC Low application should re-enabled if needed
    wlan_mac_set_tu_target(0xFFFFFFFF);
    wlan_mac_reset_tu_target_latch(1);

    // Clear any stale Rx events
    wlan_mac_hw_clear_rx_started();

    // Clear the reset
    wlan_mac_reset(0);

    return;
}



/*****************************************************************************/
/**
 * @brief Send Exception to Upper-Level MAC
 *
 * This function generates an IPC message for the upper-level MAC
 * to tell it that something has gone wrong
 *
 * @param u32 reason
 *  - reason code for the exception
 * @return None
 */
inline void wlan_mac_low_send_exception(u32 reason){
    wlan_ipc_msg_t ipc_msg_to_high;
    u32            ipc_msg_to_high_payload[2];

    // Update CPU Low status
    cpu_low_status |= CPU_STATUS_EXCEPTION;

    // Send an exception to CPU_HIGH along with a reason
    ipc_msg_to_high.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_CPU_STATUS);
    ipc_msg_to_high.arg0			  = (u8)CPU_STATUS_REASON_EXCEPTION;
    ipc_msg_to_high.num_payload_words = 2;
    ipc_msg_to_high.payload_ptr       = &(ipc_msg_to_high_payload[0]);
    ipc_msg_to_high_payload[0]        = cpu_low_status;
    ipc_msg_to_high_payload[1]        = reason;

    write_mailbox_msg(&ipc_msg_to_high);

    // Set the Hex display with the reason code and flash the LEDs
    wlan_platform_userio_disp_status(USERIO_DISP_STATUS_CPU_ERROR,reason);
}

/*****************************************************************************/
/**
 * @brief Poll the Receive Frame Start
 *
 * This function will poll the hardware to see if the PHY is currently receiving or
 * has finished receiving a packet.  This will then dispatch the current RX packet
 * buffer and PHY details to the frame_rx_callback(). The callback is responsible for
 * updating the current Rx packet buffer, typically required if the received packet
 * is passed to CPU High for further processing.
 *
 * @param   None
 * @return  u32              - Status (See MAC Polling defines in wlan_mac_low.h)
 */
inline u32 wlan_mac_low_poll_frame_rx(){
    phy_rx_details_t 	phy_details;
    u8                  active_rx_ant;

    volatile u32 mac_hw_status;
    volatile u32 phy_hdr_params;

    void* pkt_buf_addr = (void *) CALC_PKT_BUF_ADDR(platform_common_dev_info.rx_pkt_buf_baseaddr, rx_pkt_buf);
    rx_frame_info_t* rx_frame_info = (rx_frame_info_t *) pkt_buf_addr;

    int i = 0;

    u32 return_status = 0;

    // Read the MAC/PHY status
    mac_hw_status = wlan_mac_get_status();

    // Check if PHY has started a new reception
    if(mac_hw_status & WLAN_MAC_STATUS_MASK_RX_PHY_STARTED) {

    	// Fill in rx_frame_info_t metadata
        active_rx_ant = (wlan_phy_rx_get_active_rx_ant());

    	rx_frame_info->flags          = 0;
		rx_frame_info->channel        = wlan_mac_low_get_active_channel();
		rx_frame_info->phy_samp_rate  = (u8)wlan_mac_low_get_phy_samp_rate();
		rx_frame_info->timestamp      = wlan_mac_low_get_rx_start_timestamp();
		rx_frame_info->timestamp_frac = wlan_mac_low_get_rx_start_timestamp_frac();
	    rx_frame_info->ant_mode       = active_rx_ant;
	    rx_frame_info->rx_gain_index  = wlan_platform_get_rx_pkt_gain(active_rx_ant);
	    rx_frame_info->rx_power       = wlan_platform_get_rx_pkt_pwr(active_rx_ant);


    	// Check whether this is an OFDM or DSSS Rx
        if(wlan_mac_get_rx_phy_sel() == WLAN_MAC_PHY_RX_PHY_HDR_PHY_SEL_DSSS) {
            // DSSS Rx - PHY Rx length is already valid, other params unused for DSSS
        	phy_details.phy_mode = PHY_MODE_DSSS;
        	phy_details.N_DBPS   = 0;

            // Strip off extra pre-MAC-header bytes used in DSSS frames; this adjustment allows the next
            //     function to treat OFDM and DSSS payloads the same
        	phy_details.length   = wlan_mac_get_rx_phy_length() - 5;
        	phy_details.mcs      = 0;

    	    rx_frame_info->cfo_est		  = 0;;
        	rx_frame_info->phy_details    = phy_details;


            // Call the user callback to handle this Rx, capture return value
        	return_status |= FRAME_RX_RET_STATUS_RECEIVED_PKT;
        	return_status |= frame_rx_callback(rx_pkt_buf, &phy_details);

        } else {
        	// OFDM Rx - must wait for valid PHY header
        	// Order of operations is critical here
			//  1) Read status first
			//  2) Read PHY header register second
			//  3) Check for complete PHY header - continue if complete
			//  4) Else check for early PHY reset - quit if reset

        	while (1) {
				mac_hw_status = wlan_mac_get_status();
				phy_hdr_params = wlan_mac_get_rx_phy_hdr_params();

				if(i++ > 1000000) {
					xil_printf("Stuck in OFDM Rx PHY hdr check!\n");
					xil_printf(" MAC HW Status: 0x%08x\n", wlan_mac_get_status());
					xil_printf(" Rx Hdr Params: 0x%08x\n", wlan_mac_get_rx_phy_hdr_params());
					xil_printf(" Rx PHY Status: 0x%08x\n", Xil_In32(WLAN_RX_STATUS));
				}

				if(phy_hdr_params & WLAN_MAC_PHY_RX_PHY_HDR_READY) {
					// Rx PHY received enough bytes to decode PHY header
					//  Exit loop and check PHY header params
					break;
				}
				if((mac_hw_status & WLAN_MAC_STATUS_MASK_RX_PHY_ACTIVE) == 0) {
					// Rx PHY went idle before asserting RX_PHY_HDR_READY
					//  This only happens if the PHY is reset externally, possible if MAC starts a Tx during Rx
					//  Only option is to reset RX_STARTED and wait for next Rx

					// There is a 1-cycle race in this case, because RX_END asserts 1 cycle before RX_PHY_HDR_READY in the
					//  case of an invalid HT-SIG. The invalid HT-SIG generates an RX_END_ERROR which causes
					//  RX_END to assert. The simple workaround used below is to re-read phy_hdr_params one last time
					//  before concluding that the Rx PHY was reset unexpectedly
					break;
				}
			}

        	//  Re-read phy_hdr_params to resolve 1-cycle ambiguity in case of HT-SIG error
			phy_hdr_params = wlan_mac_get_rx_phy_hdr_params();

			// Decide how to handle this waveform
			if(phy_hdr_params & WLAN_MAC_PHY_RX_PHY_HDR_READY) {
                // Received PHY header - decide whether to call MAC callback
                if( (phy_hdr_params & WLAN_MAC_PHY_RX_PHY_HDR_MASK_UNSUPPORTED) ||
                	(wlan_mac_get_rx_phy_mode() > 0x2) ) {
                    // Valid HT-SIG but unsupported waveform
                    //  Rx PHY will hold ACTIVE until last samp but will not write payload
                	//  HT-SIG fields (MCS, length) can be safely read here if desired
                	// Or detected VHT waveform (not supported), did not attempt decoding VHT-SIG
        			//xil_printf("Quitting - WLAN_MAC_PHY_RX_PHY_HDR_MASK_UNSUPPORTED (MCS = %d, Length = %d)", wlan_mac_get_rx_phy_mcs(), wlan_mac_get_rx_phy_length());

                } else if(phy_hdr_params & WLAN_MAC_PHY_RX_PHY_HDR_MASK_RX_ERROR) {
                    // Invalid HT-SIG (CRC error, invalid RESERVED or TAIL bits, invalid LENGTH, etc)
                    //  Rx PHY has already released ACTIVE and will not write payload
                	//  HT-SIG fields (MCS, length) should not be trusted in this case
        			//xil_printf("Quitting - WLAN_MAC_PHY_RX_PHY_HDR_MASK_RX_ERROR");

                } else {
                    // NONHT waveform or HTMF waveform with supported HT-SIG - PHY will write payload
                    //  Call lower MAC Rx callback
                    //  Callback can safely return anytime (before or after RX_END)

                	phy_details.phy_mode = wlan_mac_get_rx_phy_mode();
                	phy_details.length   = wlan_mac_get_rx_phy_length();
                	phy_details.mcs      = wlan_mac_get_rx_phy_mcs();
                	phy_details.N_DBPS   = wlan_mac_low_mcs_to_n_dbps(phy_details.mcs, phy_details.phy_mode);

            	    rx_frame_info->cfo_est		  = wlan_phy_rx_get_cfo_est();
                    rx_frame_info->phy_details    = phy_details;

                	return_status |= FRAME_RX_RET_STATUS_RECEIVED_PKT;
                	return_status |= frame_rx_callback(rx_pkt_buf, &phy_details);
                }
            } else {
            	// PHY went idle before PHY_HDR_DONE, probably due to external reset
            	//  The Rx PHY can be reset from software (only used in wlan_phy_init()) or hardware.
            	//  The hardware reset is asserted by the MAC core during Tx. Asserting Tx during Rx is
            	//  impossible with the normal DCF code, as packet det is treated as a busy medium. With a
            	//  custom MAC implementation that allows Tx during Rx this code block will catch the
            	//  unexpected reset events.

            	// PHY header cannot be trusted in this case - do nothing and return

            }//END if(PHY_HDR_DONE)
        } //END if(OFDM Rx)

    	// Clear the MAC status register RX_STARTED bit unless the MAC application requested it stay asserted
    	//  By this point the framework and MAC are typically done with the current waveform, however it was handled.
    	//  The MAC application can request the RX_STARTED latch stay asserted by returning the FRAME_RX_RET_SKIP_RX_STARTED_RESET flag
        //  In this case the application *must* clear the latch directly whenever it completes its Rx processing
        if((return_status & FRAME_RX_RET_SKIP_RX_STARTED_RESET) == 0) wlan_mac_hw_clear_rx_started();

    } //END if(PHY_RX_STARTED)

    return return_status;
}

/*****************************************************************************/
/**
 * @brief Poll for IPC Receptions
 *
 * This function is a non-blocking poll for IPC receptions from the upper-level MAC.
 *
 * @param   None
 * @return  int				- 0 when mailbox was empty, 1 when one message was processed
 */
inline int wlan_mac_low_poll_ipc_rx(){
    // Poll mailbox read msg
    if (read_mailbox_msg(&ipc_msg_from_high) == IPC_MBOX_SUCCESS) {
        wlan_mac_low_process_ipc_msg(&ipc_msg_from_high);
        return 1;
    }
    return 0;
}


/*****************************************************************************/
/**
 * @brief Process IPC Reception
 *
 * This is an internal function to the WLAN MAC Low framework to process
 * received IPC messages and call the appropriate callback.
 *
 * @param   None
 * @return  None
 */
void wlan_mac_low_process_ipc_msg(wlan_ipc_msg_t * msg){
    wlan_ipc_msg_t           ipc_msg_to_high;

    switch(IPC_MBOX_MSG_ID_TO_MSG(msg->msg_id)){
		//---------------------------------------------------------------------
		case IPC_MBOX_TX_PKT_BUF_READY: {
			u8 tx_pkt_buf;
			tx_pkt_buf = msg->arg0;

			if(tx_pkt_buf < NUM_TX_PKT_BUFS){

				// Lock and change state of packet buffer
				wlan_mac_low_lock_tx_pkt_buf(tx_pkt_buf);

				// Message is an indication that a Tx Pkt Buf needs processing
				handle_tx_pkt_buf_ready(tx_pkt_buf);
				// TODO: check return status and inform CPU_HIGH of error?

			}
		}
		break;

    	//---------------------------------------------------------------------
    	case IPC_MBOX_SET_MAC_TIME:
    		switch(msg->arg0){
				default:
				case 0:
					//Payload is an absolute MAC time that must be applied
					set_mac_time_usec( *(u64*)(msg->payload_ptr) );
					mactime_change_callback( (*(s64*)(msg->payload_ptr))-((s64)get_mac_time_usec()) );
    			break;
				case 1:
					//Payload is a MAC time delta that must be applied
					apply_mac_time_delta_usec( *(s64*)(msg->payload_ptr));
					mactime_change_callback( *(s64*)(msg->payload_ptr));
				break;
    		}
    	break;

    	//---------------------------------------------------------------------
    	case IPC_MBOX_TXRX_BEACON_CONFIGURE: {
    		beacon_txrx_config_callback(msg->payload_ptr);
    	}
    	break;

		//---------------------------------------------------------------------
        case IPC_MBOX_CPU_STATUS: {
        	if(msg->arg0 == (u8)CPU_STATUS_REASON_BOOTED){
        		// If CPU_HIGH just booted, we should re-inform it of our CPU status
        		wlan_mac_low_send_status(CPU_STATUS_REASON_RESPONSE);
        	}
        }
        break;

        //---------------------------------------------------------------------
        case IPC_MBOX_MEM_READ_WRITE: {
            switch(msg->arg0){
                case IPC_REG_WRITE_MODE: {

                	u32    * payload_to_write = (u32*)((u8*)ipc_msg_from_high_payload + sizeof(ipc_reg_read_write_t));

                	// IMPORTANT: this memcpy assumes the payload provided by CPU high is ready as-is
                    //     Any byte swapping (i.e. for payloads that arrive over Ethernet) *must* be performed
                    //     before the payload is passed to this function

                    // Implement memcpy with only 32-bit writes, avoids byte-select issues in Sysgen AXI slaves
                    int word_idx;
                    u32 num_words =  ((ipc_reg_read_write_t*)ipc_msg_from_high_payload)->num_words;
                    u32 start_addr = (((ipc_reg_read_write_t*)ipc_msg_from_high_payload)->baseaddr) & 0xFFFFFFFC; //force 2LSB=0

                    for(word_idx = 0; word_idx < num_words; word_idx++) {
                    	// Increment target address by 4 bytes per iteration
                    	// payload_to_write already a u32 pointer, so use count-by-1 array index to access u32 words
                    	Xil_Out32((start_addr + word_idx*4), payload_to_write[word_idx]);
                    }
                }
                break;

                case IPC_REG_READ_MODE: {
                    /*
                    xil_printf("\nCPU Low Read:\n");
                    xil_printf(" Addr: 0x%08x\n", (u32*)((ipc_reg_read_write*)ipc_msg_from_high_payload)->baseaddr);
                    xil_printf(" N Wrds: %d\n", ((ipc_reg_read_write*)ipc_msg_from_high_payload)->num_words);

                    xil_printf("Mem[0x%08x] = 0x%08x\n",
                            (u32*)((ipc_reg_read_write*)ipc_msg_from_high_payload)->baseaddr,
                            Xil_In32((u32*)((ipc_reg_read_write*)ipc_msg_from_high_payload)->baseaddr));
                     */
                    ipc_msg_to_high.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_MEM_READ_WRITE);
                    ipc_msg_to_high.num_payload_words = ((ipc_reg_read_write_t*)ipc_msg_from_high_payload)->num_words;
                    ipc_msg_to_high.payload_ptr       = (u32*)((ipc_reg_read_write_t*)ipc_msg_from_high_payload)->baseaddr;

                    write_mailbox_msg(&ipc_msg_to_high);
                }
                break;
            }
        }
        break;

        //---------------------------------------------------------------------------------------
        case IPC_MBOX_LOW_PARAM: {
            switch(msg->arg0){
                case IPC_REG_WRITE_MODE: {
                    switch(ipc_msg_from_high_payload[0]){

                        case LOW_PARAM_PHY_SAMPLE_RATE: {
                            set_phy_samp_rate(ipc_msg_from_high_payload[1]);
                        }
                        break;

                        default: {
                        	// Low framework doesn't recognize this low param ID - call the application
                        	//  and platform handlers to process with this param write
                            ipc_low_param_callback(IPC_REG_WRITE_MODE, ipc_msg_from_high_payload);
                            wlan_platform_low_param_handler(IPC_REG_WRITE_MODE, ipc_msg_from_high_payload);
                        }
                        break;
                    }
                }
                break;

                case IPC_REG_READ_MODE: {
                    // Read Mode is not supported
                    //
                    // NOTE:  This is due to the fact that IPC messages in CPU low can take an infinitely long amount of
                    //     to return given that the sending and receiving of wireless data takes precedent.  Therefore,
                    //     it is not good to try to return values from CPU low since there is no guarantee when the values
                    //     will be available.
                    //
                    u32      ret_val                  = 0;

                    ipc_msg_to_high.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_LOW_PARAM);
                    ipc_msg_to_high.num_payload_words = 0;
                    ipc_msg_to_high.payload_ptr       = (u32 *)&ret_val;

                    write_mailbox_msg(&ipc_msg_to_high);
                }
                break;
            }
        }
        break;

        //---------------------------------------------------------------------
        case IPC_MBOX_MCAST_BUFFER_ENABLE: {
        	mcast_buffer_enable_callback(msg->arg0);
        }
        break;

        //---------------------------------------------------------------------
        case IPC_MBOX_CONFIG_CHANNEL: {
            wlan_mac_low_set_radio_channel(ipc_msg_from_high_payload[0]);
        }
        break;

        //---------------------------------------------------------------------
        case IPC_MBOX_LOW_RANDOM_SEED: {
            srand(ipc_msg_from_high_payload[0]);
        }
        break;

        //---------------------------------------------------------------------
        case IPC_MBOX_CONFIG_TX_CTRL_POW: {
            mac_param_ctrl_tx_pow = (s8)ipc_msg_from_high_payload[0];
        }
        break;

        //---------------------------------------------------------------------
        case IPC_MBOX_CONFIG_RX_FILTER: {
            u32    filter_mode_hi = (u32)ipc_msg_from_high_payload[0];
            u32    filter_mode_lo = 0;

            if((filter_mode_hi & RX_FILTER_FCS_MASK) == RX_FILTER_FCS_NOCHANGE){
                filter_mode_lo |= (mac_param_rx_filter & RX_FILTER_FCS_MASK);
            } else {
                filter_mode_lo |= (filter_mode_hi & RX_FILTER_FCS_MASK);
            }

            if((filter_mode_hi & RX_FILTER_HDR_NOCHANGE) == RX_FILTER_HDR_NOCHANGE){
                filter_mode_lo |= (mac_param_rx_filter & RX_FILTER_HDR_NOCHANGE);
            } else {
                filter_mode_lo |= (filter_mode_hi & RX_FILTER_HDR_NOCHANGE);
            }

            mac_param_rx_filter = filter_mode_lo;
        }
        break;

        //---------------------------------------------------------------------
        case IPC_MBOX_CONFIG_RX_ANT_MODE: {
            wlan_rx_config_ant_mode(ipc_msg_from_high_payload[0]);
        }
        break;

        //---------------------------------------------------------------------
        case IPC_MBOX_CONFIG_DSSS_EN: {
			if (ipc_msg_from_high_payload[0] == 1) {
				// xil_printf("Enabling DSSS\n");
				wlan_mac_low_DSSS_rx_enable();
			} else {
				// xil_printf("Disabling DSSS\n");
				wlan_mac_low_DSSS_rx_disable();
			}
        }
        break;

        //---------------------------------------------------------------------
        case IPC_MBOX_SET_DEFAULT_TX_POWER: {
        	s8 pwr = (s8)(msg->arg0);

        	wlan_platform_set_default_tx_power(pwr);
        }
        break;


    }
}

inline u32 wlan_mac_low_lock_tx_pkt_buf(u16 tx_pkt_buf){
	u32 is_locked, owner;
	u32	iter = 0;
	tx_frame_info_t* tx_frame_info;

	tx_frame_info = (tx_frame_info_t*)CALC_PKT_BUF_ADDR(platform_common_dev_info.tx_pkt_buf_baseaddr, tx_pkt_buf);

	if( tx_frame_info->flags & TX_FRAME_INFO_FLAGS_WAIT_FOR_LOCK ){
		// This packet buffer has been flagged to ensure that CPU_LOW will wait until a mutex lock
		// is achieved before proceeding (as opposed to aborting and raising an error). The implicit
		// contract in this flag is that CPU_HIGH will only briefly lock the packet buffer while updating
		// its contents.
		if(tx_frame_info->tx_pkt_buf_state == TX_PKT_BUF_DONE){
			return PREPARE_FRAME_TRANSMIT_ERROR_UNEXPECTED_PKT_BUF_STATE;
		}

		while(lock_tx_pkt_buf(tx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
			// This frame was flagged with
			if(iter > 1000000) {
				xil_printf("ERROR (wlan_mac_low_lock_tx_pkt_buf): stuck waiting for CPU High to unlock Tx pkt buf %d\n", tx_pkt_buf);
			}
			else {
				iter++;
			}
		}

		if(tx_frame_info->tx_pkt_buf_state != TX_PKT_BUF_READY){
			return PREPARE_FRAME_TRANSMIT_ERROR_UNEXPECTED_PKT_BUF_STATE;
		}

	} else {
		// This packet buffer should be lockable and there is no need for CPU_LOW to wait on a mutex
		// lock if it is not. In that case, CPU_LOW should print an error and quit this function.

		if(tx_frame_info->tx_pkt_buf_state != TX_PKT_BUF_READY){
			if(tx_frame_info->tx_pkt_buf_state == TX_PKT_BUF_LOW_CTRL ){
				// CPU Low responsible for any LOW_CTRL buffers
				//  Don't transmit - just clean up and return
				tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_HIGH_CTRL;
			}
			//  CPU High will handle it eventually
			//  Ensure CPU Low doesn't own lock then return
			unlock_tx_pkt_buf(tx_pkt_buf);
			return PREPARE_FRAME_TRANSMIT_ERROR_UNEXPECTED_PKT_BUF_STATE;
		}

		// Attempt to lock the packet buffer. If this fails, we will not wait for it to succeed. Something
		// has gone wrong and we should print an error.
		if(lock_tx_pkt_buf(tx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
			wlan_printf(PL_ERROR, "Error: unable to lock TX pkt_buf %d\n", tx_pkt_buf);
			get_tx_pkt_buf_status(tx_pkt_buf, &is_locked, &owner);
			wlan_printf(PL_ERROR, "    TX pkt_buf %d status: isLocked = %d, owner = %d\n", tx_pkt_buf, is_locked, owner);
			tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_HIGH_CTRL;
			return PREPARE_FRAME_TRANSMIT_ERROR_LOCK_FAIL;
		}
	}
	// By this point in the function, we have verified that the packet buffer was in an expected state
	// and have successfully locked the mutex.

	tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_LOW_CTRL;
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
int wlan_mac_low_set_radio_channel(u32 channel){

	if (wlan_verify_channel(channel) == XST_SUCCESS) {
	mac_param_chan = channel;

		if(mac_param_chan <= 14){
			mac_param_band = CHAN_BAND_24GHz;

			// Enable DSSS if global variable indicates it should be enabled and PHY sample rate allows it
			if ((mac_param_dsss_en) && (gl_phy_samp_rate == PHY_20M)) {
				wlan_phy_DSSS_rx_enable();
			}
		} else {
			mac_param_band = CHAN_BAND_5GHz;

			// Always disable DSSS when in the 5 GHZ band
			wlan_phy_DSSS_rx_disable();
		}

		wlan_mac_reset_NAV_counter();

		return wlan_platform_low_set_radio_channel(channel);

	} else {
		xil_printf("Invalid channel selection %d\n", channel);
		return -1;
	}
}



/*****************************************************************************/
/**
 * @brief Enable / Disable DSSS RX
 *
 * DSSS RX must be disabled when in the 5 GHz band or when the PHY sample rate
 * is not 20 MSps.  However, the low framework will maintain what the state
 * should be when in the 2.4 GHz band and the PHY sample rate is 20 MSps
 *
 * @param   None
 * @return  None
 *
 */
void wlan_mac_low_DSSS_rx_enable() {
	mac_param_dsss_en = 1;

	// Only enable DSSS if in 2.4 GHz band and phy sample rate is 20
	if ((mac_param_band == CHAN_BAND_24GHz) && (gl_phy_samp_rate == PHY_20M)) {
		wlan_phy_DSSS_rx_enable();
	}
}


void wlan_mac_low_DSSS_rx_disable() {
	mac_param_dsss_en = 0;
	wlan_phy_DSSS_rx_disable();
}

int wlan_mac_low_finish_frame_transmit(u16 tx_pkt_buf){
	// This function should only be called on a packet buffer whose state is
	// TX_PKT_BUF_LOW_CTRL and is currently locked by CPU_LOW
	int 					 return_value = 0;
    tx_frame_info_t        * tx_frame_info;
    u32                      is_locked, owner;
    wlan_ipc_msg_t           ipc_msg_to_high;


    if(tx_pkt_buf >= NUM_TX_PKT_BUFS){
    	xil_printf("Error: Tx Pkt Buf index exceeds NUM_TX_PKT_BUFS\n");
    	return_value = -1;
    	return return_value;
    }

    tx_frame_info = (tx_frame_info_t*)CALC_PKT_BUF_ADDR(platform_common_dev_info.tx_pkt_buf_baseaddr, tx_pkt_buf);

	switch(tx_frame_info->tx_pkt_buf_state){
		case TX_PKT_BUF_LOW_CTRL:
			get_tx_pkt_buf_status(tx_pkt_buf, &is_locked, &owner);

			if( (is_locked == 0) || (owner != XPAR_CPU_ID)){
				wlan_printf(PL_ERROR, "TX pkt_buf %d is not locked by CPU_LOW\n", tx_pkt_buf);

				get_tx_pkt_buf_status(tx_pkt_buf, &is_locked, &owner);
				tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_HIGH_CTRL;

				return_value = -1;

			} else {
				//Record the time when we completed this MPDU
				tx_frame_info->timestamp_done = get_mac_time_usec();

				tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_DONE;

		    	// Note: at this point in the code, the packet buffer state has been modified to TX_PKT_BUF_DONE,
		    	// yet we have not sent the IPC_MBOX_TX_PKT_BUF_DONE message. If we happen to reboot here,
		    	// this packet buffer will be abandoned and won't be cleaned up in the boot process. This is a narrow
		    	// race in practice, but step-by-step debugging can accentuate the risk since there can be an arbitrary
		    	// amount of time spent in this window.

				//Revert the state of the packet buffer and return control to CPU High
				if(unlock_tx_pkt_buf(tx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
					wlan_printf(PL_ERROR, "Error: unable to unlock TX pkt_buf %d\n", tx_pkt_buf);
					wlan_mac_low_send_exception(WLAN_ERROR_CODE_CPU_LOW_TX_MUTEX);
					tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_HIGH_CTRL;
				} else {
					ipc_msg_to_high.msg_id =  IPC_MBOX_MSG_ID(IPC_MBOX_TX_PKT_BUF_DONE);

					ipc_msg_to_high.num_payload_words = 0;
					ipc_msg_to_high.payload_ptr = NULL;

					ipc_msg_to_high.arg0 = tx_pkt_buf;

					write_mailbox_msg(&ipc_msg_to_high);
				}
			}
		break;
		// ---- Something went wrong - packet buffer in unexpected state ----
		default:
		case TX_PKT_BUF_READY:
            // CPU Low responsible for any LOW_CTRL buffers
            //  Don't transmit - just clean up and return
			tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_HIGH_CTRL;
		case TX_PKT_BUF_UNINITIALIZED:
		case TX_PKT_BUF_DONE:
		case TX_PKT_BUF_HIGH_CTRL:
            //  CPU High will handle it eventually
            //  Ensure CPU Low doesn't own lock then return
            unlock_tx_pkt_buf(tx_pkt_buf);
		break;
	}
	return return_value;
}

int wlan_mac_low_prepare_frame_transmit(u16 tx_pkt_buf){
	tx_frame_info_t* 		 tx_frame_info;
	mac_header_80211       * tx_80211_header;
	ltg_packet_id_t*         pkt_id;
	if(tx_pkt_buf >= NUM_TX_PKT_BUFS){
		xil_printf("Error: Tx Pkt Buf index exceeds NUM_TX_PKT_BUFS\n");
		return PREPARE_FRAME_TRANSMIT_ERROR_INVALID_PKT_BUF;
	}

	tx_frame_info = (tx_frame_info_t*)CALC_PKT_BUF_ADDR(platform_common_dev_info.tx_pkt_buf_baseaddr, tx_pkt_buf);

	tx_frame_info->timestamp_accept = get_mac_time_usec();

	// Get pointer to start of MAC header in packet buffer
	tx_80211_header = (mac_header_80211*)(CALC_PKT_BUF_ADDR(platform_common_dev_info.tx_pkt_buf_baseaddr, tx_pkt_buf)+PHY_TX_PKT_BUF_MPDU_OFFSET);

	// Insert sequence number here
	tx_80211_header->sequence_control = ((tx_80211_header->sequence_control) & 0xF) | ( (unique_seq&0xFFF)<<4 );

	// Insert unique sequence into tx_frame_info
	tx_frame_info->unique_seq = unique_seq;

	if((tx_frame_info->flags) & TX_FRAME_INFO_FLAGS_FILL_UNIQ_SEQ){
		// Fill unique sequence number into LTG payload
		pkt_id             = (ltg_packet_id_t*)((u8*)tx_80211_header + sizeof(mac_header_80211));
		pkt_id->unique_seq = unique_seq;
	}

	//Increment the global unique sequence number
	unique_seq++;
	return 0;
}

void wlan_mac_low_send_low_tx_details(u8 pkt_buf, wlan_mac_low_tx_details_t* low_tx_details){
	wlan_ipc_msg_t           ipc_msg_to_high;

	ipc_msg_to_high.payload_ptr = (u32*)low_tx_details;
	ipc_msg_to_high.arg0 = pkt_buf;
	ipc_msg_to_high.num_payload_words = (sizeof(wlan_mac_low_tx_details_t) / sizeof(u32));

	ipc_msg_to_high.msg_id =  IPC_MBOX_MSG_ID(IPC_MBOX_PHY_TX_REPORT);
	write_mailbox_msg(&ipc_msg_to_high);
	return;
}



/*****************************************************************************/
/**
 * @brief Notify upper-level MAC of frame reception
 *
 * Sends an IPC message to the upper-level MAC to notify it that a frame has been
 * received and is ready to be processed
 *
 * @param   None
 * @return  None
 *
 * @note This function assumes it is called in the same context where rx_pkt_buf is still valid.
 */
void wlan_mac_low_frame_ipc_send(){
    wlan_ipc_msg_t ipc_msg_to_high;

    ipc_msg_to_high.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_RX_PKT_BUF_READY);
    ipc_msg_to_high.num_payload_words = 0;
    ipc_msg_to_high.arg0              = rx_pkt_buf;

    write_mailbox_msg(&ipc_msg_to_high);
}



/*****************************************************************************/
/**
 * @brief Set Frame Reception Callback
 *
 * Tells the framework which function should be called when the PHY begins processing a frame reception
 *
 * @param   callback         - Pointer to callback function
 * @return  None
 */
inline void wlan_mac_low_set_frame_rx_callback(function_ptr_t callback){
    frame_rx_callback = callback;
}

inline void wlan_mac_low_set_sample_rate_change_callback(function_ptr_t callback){
	sample_rate_change_callback = callback;
}

inline void wlan_mac_low_set_handle_tx_pkt_buf_ready(function_ptr_t callback){
	handle_tx_pkt_buf_ready = callback;
}

inline void wlan_mac_low_set_beacon_txrx_config_callback(function_ptr_t callback){
	beacon_txrx_config_callback = callback;
}

inline void wlan_mac_low_set_mcast_buffer_enable_callback(function_ptr_t callback){
	mcast_buffer_enable_callback = callback;
}

inline void wlan_mac_low_set_mactime_change_callback(function_ptr_t callback){
	mactime_change_callback = callback;
}

/*****************************************************************************/
/**
 * @brief Set IPC_MBOX_LOW_PARAM Callback
 *
 * Tells the framework which function should be called when an ipc message is received for
 * the IPC_MBOX_LOW_PARAM command.
 *
 * @param   callback         - Pointer to callback function
 * @return  None
 */
void wlan_mac_low_set_ipc_low_param_callback(function_ptr_t callback){
    ipc_low_param_callback = callback;
}



/*****************************************************************************/
/**
 * @brief Various Getter Methods
 *
 * These functions will get parameters from the low framework.
 *
 * @param   None
 * @return  (see individual function)
 */
inline u32 wlan_mac_low_get_active_channel(){
    return mac_param_chan;
}


inline s8 wlan_mac_low_get_current_ctrl_tx_pow(){
    return mac_param_ctrl_tx_pow;
}


inline u32 wlan_mac_low_get_current_rx_filter(){
    return mac_param_rx_filter;
}


inline phy_samp_rate_t wlan_mac_low_get_phy_samp_rate() {
    return gl_phy_samp_rate;
}



/*****************************************************************************/
/**
 * @brief Get the Rx Start Microsecond Timestamp
 *
 * This function returns the Rx start timestamp of the system
 *
 * @param   None
 * @return  u64              - microsecond timestamp
 */
inline u64 wlan_mac_low_get_rx_start_timestamp() {
    u32 timestamp_high_u32;
    u32 timestamp_low_u32;
    u64 timestamp_u64;

    // RX_START timestamp is captured once per reception - no race condition between 32-bit reads
    timestamp_high_u32 = Xil_In32(WLAN_MAC_REG_RX_TIMESTAMP_MSB);
    timestamp_low_u32 = Xil_In32(WLAN_MAC_REG_RX_TIMESTAMP_LSB);
    timestamp_u64 = (((u64)timestamp_high_u32)<<32) + ((u64)timestamp_low_u32);

    return timestamp_u64;
}



/*****************************************************************************/
/**
 * @brief Get the Tx Start Microsecond Timestamp
 *
 * This function returns the Tx start timestamp of the system
 *
 * @param   None
 * @return  u64              - microsecond timestamp
 */
inline u64 wlan_mac_low_get_tx_start_timestamp() {

    u32 timestamp_high_u32;
    u32 timestamp_low_u32;
    u64 timestamp_u64;

    // TX_START timestamp is captured once per transmission - no race condition between 32-bit reads
    timestamp_high_u32 = Xil_In32(WLAN_MAC_REG_TX_TIMESTAMP_MSB);
    timestamp_low_u32 = Xil_In32(WLAN_MAC_REG_TX_TIMESTAMP_LSB);
    timestamp_u64 = (((u64)timestamp_high_u32)<<32) + ((u64)timestamp_low_u32);

    return timestamp_u64;
}


/*****************************************************************************/
/**
 * @brief Sets the node's MAC address in the MAC core's NAV logic
 *
 * @param   addr - pointer to 6-byte MAC address
 * @return  None
 */
void wlan_mac_low_set_nav_check_addr(u8* addr) {
    Xil_Out32(WLAN_MAC_REG_NAV_CHECK_ADDR_1, *((u32*)&(addr[0])) );
    Xil_Out32(WLAN_MAC_REG_NAV_CHECK_ADDR_2, *((u32*)&(addr[4])) );
}

/*****************************************************************************/
/**
 * @brief Search for and Lock Empty Packet Buffer (Blocking)
 *
 * This is a blocking function for finding and locking an empty rx packet buffer. The low framework
 * calls this function after passing a new wireless reception up to CPU High for processing. CPU High
 * must unlock Rx packet buffers after processing the received packet. This function loops over all Rx
 * packet buffers until it finds one that has been unlocked by CPU High.
 *
 * By design this function prints a message if it fails to unlock the oldest packet buffer. When this
 * occurs it indicates that CPU Low has outrun CPU High, a situation that leads to dropped wireless
 * receptions with high probability. The node recovers gracefully from this condition and will
 * continue processing new Rx events after CPU High catches up. But seeing this message in the UART
 * for CPU Low is a strong indicator the CPU High code is not keeping up with wireless receptions.
 *
 * @param   None
 * @return  None
 *
 * @note    This function assumes it is called in the same context where rx_pkt_buf is still valid.
 */
inline void wlan_mac_low_lock_empty_rx_pkt_buf(){
    // This function blocks until it safely finds a packet buffer for the PHY RX to store a future reception
    rx_frame_info_t* rx_frame_info;
    u32 i = 1;

    while(1) {
    	//rx_pkt_buf is the global shared by all contexts which deal with wireless Rx
    	// Rx packet buffers are used in order. Thus incrementing rx_pkt_buf should
    	// select the "oldest" packet buffer, the one that is most likely to have already
    	// been processed and released by CPU High
    	rx_pkt_buf = (rx_pkt_buf+1) % NUM_RX_PKT_BUFS;
    	rx_frame_info    = (rx_frame_info_t*) CALC_PKT_BUF_ADDR(platform_common_dev_info.rx_pkt_buf_baseaddr, rx_pkt_buf);

        if((rx_frame_info->rx_pkt_buf_state) == RX_PKT_BUF_LOW_CTRL){

        	if(lock_rx_pkt_buf(rx_pkt_buf) == PKT_BUF_MUTEX_SUCCESS){
			// By default Rx pkt buffers are not zeroed out, to save the performance penalty of bzero'ing 2KB
			//     However zeroing out the pkt buffer can be helpful when debugging Rx MAC/PHY behaviors
			// bzero((void *)(RX_PKT_BUF_TO_ADDR(rx_pkt_buf)), 2048);

			// Set the OFDM and DSSS PHYs to use the same Rx pkt buffer
			wlan_phy_rx_pkt_buf_ofdm(rx_pkt_buf);
			wlan_phy_rx_pkt_buf_dsss(rx_pkt_buf);

			if (i > 1) { xil_printf("found in %d iterations.\n", i); }

			return;
        	} else {
        		xil_printf("Error: unable to lock Rx pkt_buf %d despite RX_PKT_BUF_LOW_CTRL\n", rx_pkt_buf);
        		unlock_rx_pkt_buf(rx_pkt_buf);
        }
        }

        if (i == 1) { xil_printf("Searching for empty packet buff ... "); }
        i++;
    }
}


/*****************************************************************************/
/**
 * @brief Finish PHY Reception
 *
 * This function polls the MAC status register until the Rx PHY goes idle. The
 * return value indicates whether the just-completed reception was good
 * (no Rx errors and matching checksum) or bad
 *
 * @param   None
 * @return  u32              - FCS status (RX_MPDU_STATE_FCS_GOOD or RX_MPDU_STATE_FCS_BAD)
 */
inline u32 wlan_mac_hw_rx_finish() {
    u32 mac_hw_status;
    int i = 0;

    // Wait for the packet to finish - Rx PHY is "active" if the demod/decoding pipeline is
    //  still writing bytes to the packet buffer. The FCS result is not known until the last
    //  payload byte has been written. The RX_PHY_ACTIVE bit might de-assert before this happens
    //  for some lengths/rates/bandwidths. This allows RX_END to start the SIFS timer at the right
    //  time independent of payload-specific PHY latencies.
    do{
        mac_hw_status = wlan_mac_get_status();
        if(i++>1000000) {
        	xil_printf("Stuck in wlan_mac_hw_rx_finish!\n", mac_hw_status);
			xil_printf(" MAC HW Status: 0x%08x\n", wlan_mac_get_status());
			xil_printf(" Rx Hdr Params: 0x%08x\n", wlan_mac_get_rx_phy_hdr_params());
			xil_printf(" Rx PHY Status: 0x%08x\n", Xil_In32(WLAN_RX_STATUS));
        }
    } while(mac_hw_status & (WLAN_MAC_STATUS_MASK_RX_PHY_ACTIVE | WLAN_MAC_STATUS_MASK_RX_PHY_WRITING_PAYLOAD));

    // Check RX_END_ERROR and FCS
    if( (mac_hw_status & WLAN_MAC_STATUS_MASK_RX_FCS_GOOD) &&
       ((mac_hw_status & WLAN_MAC_STATUS_MASK_RX_END_ERROR) == 0)) {
        return 1;

    } else {
        return 0;

    }
}



/*****************************************************************************/
/**
 * @brief Force reset backoff counter in MAC hardware
 */
inline void wlan_mac_reset_backoff_counter() {
    Xil_Out32(WLAN_MAC_REG_CONTROL, Xil_In32(WLAN_MAC_REG_CONTROL) | WLAN_MAC_CTRL_MASK_RESET_A_BACKOFF);
    Xil_Out32(WLAN_MAC_REG_CONTROL, Xil_In32(WLAN_MAC_REG_CONTROL) & ~WLAN_MAC_CTRL_MASK_RESET_A_BACKOFF);
}



/*****************************************************************************/
/**
 * @brief Force reset NAV counter in MAC hardware
 */
inline void wlan_mac_reset_NAV_counter() {
    Xil_Out32(WLAN_MAC_REG_CONTROL, Xil_In32(WLAN_MAC_REG_CONTROL) | WLAN_MAC_CTRL_MASK_RESET_NAV);
    Xil_Out32(WLAN_MAC_REG_CONTROL, Xil_In32(WLAN_MAC_REG_CONTROL) & ~WLAN_MAC_CTRL_MASK_RESET_NAV);
}



/*****************************************************************************/
/**
 * @brief Convert dBm to Tx Gain Target
 *
 * This function maps a transmit power (in dBm) to a radio gain target.
 *
 * @param   s8 power         - Power in dBm
 * @return  u8 gain_target   - Gain target in range of [0,63]
 */
inline u8 wlan_mac_low_dbm_to_gain_target(s8 power){
    s8 power_railed;
    u8 return_value;

    if(power > TX_POWER_MAX_DBM){
        power_railed = TX_POWER_MAX_DBM;
    } else if( power < TX_POWER_MIN_DBM){
        power_railed = TX_POWER_MIN_DBM;
    } else {
        power_railed = power;
    }

    // This is only save because 'power' is constrained to less than half the dynamic range of an s8 type
    return_value = (u8)((power_railed << 1) + 20);

    return return_value;
}





/*****************************************************************************/
/**
 * @brief Convert MCS to number of data bits per symbol
 */
inline u16 wlan_mac_low_mcs_to_n_dbps(u8 mcs, u8 phy_mode) {

	if(phy_mode == PHY_MODE_NONHT && mcs < (sizeof(mcs_to_n_dbps_nonht_lut)/sizeof(mcs_to_n_dbps_nonht_lut[0]))) {
		return mcs_to_n_dbps_nonht_lut[mcs];
	} else if(phy_mode == PHY_MODE_HTMF && mcs < (sizeof(mcs_to_n_dbps_htmf_lut)/sizeof(mcs_to_n_dbps_htmf_lut[0]))) {
		return mcs_to_n_dbps_htmf_lut[mcs];
	} else {
		xil_printf("ERROR (wlan_mac_low_mcs_to_n_dbps): Invalid PHY_MODE (%d) or MCS (%d)\n", phy_mode, mcs);
		return 1; // N_DBPS used as denominator, so better not return 0
	}
}



/*****************************************************************************/
/**
 * @brief Convert MCS to Control Response MCS
 */
inline u8 wlan_mac_low_mcs_to_ctrl_resp_mcs(u8 mcs, u8 phy_mode){
    // Returns the fastest NON-HT half-rate MCS lower than the provided MCS and no larger that 24Mbps.
	//  Valid return values are [0, 2, 4]
    u8 return_value = 0;

    if(phy_mode == PHY_MODE_NONHT){
    	return_value = mcs;
		if(return_value > 4){ return_value = 4; }
		if(return_value % 2){ return_value--;   }
    } else if(phy_mode == PHY_MODE_HTMF) {
    	switch(mcs){
    		default:
    		case 0:
    			return_value = 0;
			break;
    		case 1:
    			return_value = 2;
			break;
    		case 2:
    			return_value = 2;
			break;
    		case 3:
    		case 4:
    		case 5:
    		case 6:
    		case 7:
    			return_value = 4;
			break;
    	}
    }
    return return_value;
}

inline void wlan_mac_hw_clear_rx_started() {
	wlan_mac_reset_rx_started(1);
	wlan_mac_reset_rx_started(0);

	return;
}

void wlan_mac_set_tu_target(u64 tu_target) {
	Xil_Out32(WLAN_MAC_REG_TU_TARGET_MSB, ((u32)(tu_target >> 32)));
	Xil_Out32(WLAN_MAC_REG_TU_TARGET_LSB, ((u32)(tu_target & 0xFFFFFFFF)));
}

