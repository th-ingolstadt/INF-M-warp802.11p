/** @file wlan_mac_high.h
 *  @brief Low-level WLAN MAC High Framework
 *
 *  This contains the low-level code for accessing the WLAN MAC Low Framework.
 *
 *  @copyright Copyright 2014, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

#include "xparameters.h"
#include "w3_userio.h"
#include "w3_ad_controller.h"
#include "w3_clock_controller.h"
#include "w3_iic_eeprom.h"
#include "radio_controller.h"

#include "wlan_mac_ipc_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_misc_util.h"
#include "wlan_phy_util.h"

#include "wlan_exp.h"
#include "wlan_mac_low.h"


static u32					mac_param_chan; 				///< Current channel of the lower-level MAC
static u8           		mac_param_band;					///< Current band of the lower-level MAC
static u8   				rx_pkt_buf;						///< Current receive buffer of the lower-level MAC
static u32  				cpu_low_status;					///< Status flags that are reported to upper-level MAC
static wlan_mac_hw_info    	hw_info;						///< Information about the hardware reported to upper-level MAC
static wlan_ipc_msg        	ipc_msg_from_high;				///< Buffer for incoming IPC messages
static u32                 	ipc_msg_from_high_payload[10];	///< Buffer for payload of incoming IPC messages

// Callback function pointers
function_ptr_t     frame_rx_callback;			///< User callback frame receptions
function_ptr_t     frame_tx_callback;			///< User callback frame transmissions


/**
 * @brief Initialize MAC Low Framework
 *
 * This function initializes the MAC Low Framework by setting
 * up the hardware and other subsystems in the framework.
 *
 * @param type
 * 	- lower-level MAC type
 * @return int status
 *  - initialization status (0 = success)
 */
int wlan_mac_low_init(u32 type){
	u32 status;
	rx_frame_info* rx_mpdu;
	wlan_ipc_msg ipc_msg_to_high;

	mac_param_band = RC_24GHZ;
	cpu_low_status = 0;

	frame_rx_callback	= (function_ptr_t)nullCallback;
	frame_tx_callback	= (function_ptr_t)nullCallback;

	status = w3_node_init();

	if(status != 0) {
		xil_printf("Error in w3_node_init()! Exiting\n");
		return -1;
	}

	wlan_lib_init();

	//create IPC message to receive into
	ipc_msg_from_high.payload_ptr = &(ipc_msg_from_high_payload[0]);

	//Begin by trying to lock packet buffer 0 for wireless receptions
	rx_pkt_buf = 0;
	if(lock_pkt_buf_rx(rx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
		warp_printf(PL_ERROR, "Error: unable to lock pkt_buf %d\n", rx_pkt_buf);
		wlan_mac_low_send_exception(EXC_MUTEX_TX_FAILURE);
		return -1;
	} else {
		rx_mpdu = (rx_frame_info*)RX_PKT_BUF_TO_ADDR(rx_pkt_buf);
		rx_mpdu->state = RX_MPDU_STATE_RX_PENDING;
		wlan_phy_rx_pkt_buf_ofdm(rx_pkt_buf);
		wlan_phy_rx_pkt_buf_dsss(rx_pkt_buf);
	}

	//Move the PHY's starting address into the packet buffers by PHY_XX_PKT_BUF_PHY_HDR_OFFSET.
	//This accounts for the metadata located at the front of every packet buffer (Xx_mpdu_info)
	wlan_phy_rx_pkt_buf_phy_hdr_offset(PHY_RX_PKT_BUF_PHY_HDR_OFFSET);
	wlan_phy_tx_pkt_buf_phy_hdr_offset(PHY_TX_PKT_BUF_PHY_HDR_OFFSET);

	wlan_radio_init();
	wlan_phy_init();
	wlan_mac_low_dcf_init();

	// Initialize the HW info structure
	wlan_mac_low_init_hw_info(type);

	// Send a message to other processor to identify hw info of cpu low
	ipc_msg_to_high.msg_id = IPC_MBOX_MSG_ID(IPC_MBOX_HW_INFO);
	ipc_msg_to_high.num_payload_words = 8;
	ipc_msg_to_high.payload_ptr = (u32 *) &(hw_info);

	ipc_mailbox_write_msg(&ipc_msg_to_high);

	return 0;
}

/**
 * @brief Finish Initializing MAC Low Framework
 *
 * This function finishes the initialization and notifies the upper-level
 * MAC that it has finished booting.
 *
 * @param None
 * @return None
 */
void wlan_mac_low_finish_init(){
	wlan_ipc_msg ipc_msg_to_high;
	u32 ipc_msg_to_high_payload[1];
	cpu_low_status |= CPU_STATUS_INITIALIZED;
	//Send a message to other processor to say that this processor is initialized and ready
	ipc_msg_to_high.msg_id = IPC_MBOX_MSG_ID(IPC_MBOX_CPU_STATUS);
	ipc_msg_to_high.num_payload_words = 1;
	ipc_msg_to_high.payload_ptr = &(ipc_msg_to_high_payload[0]);
	ipc_msg_to_high_payload[0] = cpu_low_status;
	ipc_mailbox_write_msg(&ipc_msg_to_high);
}

/**
 * @brief Initialize the DCF Hardware Core
 *
 * This function initializes the DCF hardware core.
 *
 * @param None
 * @return None
 */
void wlan_mac_low_dcf_init(){
	u16 i;
	rx_frame_info* rx_mpdu;

	//Enable blocking of the Rx PHY following good-FCS reception
	REG_SET_BITS(WLAN_MAC_REG_CONTROL, (WLAN_MAC_CTRL_MASK_RX_PHY_BLOCK_EN | WLAN_MAC_CTRL_MASK_BLOCK_RX_ON_TX ));
	REG_CLEAR_BITS(WLAN_MAC_REG_CONTROL, (WLAN_MAC_CTRL_MASK_DISABLE_NAV | WLAN_MAC_CTRL_MASK_BLOCK_RX_ON_VALID_RXEND));

	//MAC timing parameters are in terms of units of 100 nanoseconds
	wlan_mac_set_slot(T_SLOT*10);
	wlan_mac_set_DIFS((T_DIFS-PHY_RX_SIG_EXT_USEC)*10);
	wlan_mac_set_TxDIFS(((T_DIFS-PHY_RX_SIG_EXT_USEC)*10) - (TX_PHY_DLY_100NSEC));
	wlan_mac_set_timeout(T_TIMEOUT*10);

	//TODO: NAV adjust needs verification
	//NAV adjust time - signed char (Fix8_0) value
	wlan_mac_set_NAV_adj(0*10);
	wlan_mac_set_EIFS(T_EIFS*10);

	//Clear any stale Rx events
	wlan_mac_dcf_hw_unblock_rx_phy();

	for(i=0;i < NUM_RX_PKT_BUFS; i++){
		rx_mpdu = (rx_frame_info*)RX_PKT_BUF_TO_ADDR(i);
		rx_mpdu->state = RX_MPDU_STATE_EMPTY;
	}

}

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
	wlan_ipc_msg ipc_msg_to_high;
	u32 ipc_msg_to_high_payload[2];
	//Send an exception to CPU_HIGH along with a reason
	cpu_low_status |= CPU_STATUS_EXCEPTION;
	ipc_msg_to_high.msg_id = IPC_MBOX_MSG_ID(IPC_MBOX_CPU_STATUS);
	ipc_msg_to_high.num_payload_words = 2;
	ipc_msg_to_high.payload_ptr = &(ipc_msg_to_high_payload[0]);
	ipc_msg_to_high_payload[0] = cpu_low_status;
	ipc_msg_to_high_payload[1] = reason;
	ipc_mailbox_write_msg(&ipc_msg_to_high);

	userio_write_hexdisp_left(USERIO_BASEADDR, reason & 0xF);
	userio_write_hexdisp_right(USERIO_BASEADDR, (reason>>4) & 0xF);

	while(1){
		userio_write_leds_red(USERIO_BASEADDR, 0x5);
		usleep(250000);
		userio_write_leds_red(USERIO_BASEADDR, 0xA);
		usleep(250000);
	}
}

/**
 * @brief Poll for IPC Receptions
 *
 * This function is a non-blocking poll for IPC receptions from the
 * upper-level MAC.
 *
 * @param None
 * @return None
 */
inline void wlan_mac_low_poll_ipc_rx(){
	//Poll mailbox read msg
	if(ipc_mailbox_read_msg(&ipc_msg_from_high) == IPC_MBOX_SUCCESS){
		process_ipc_msg_from_high(&ipc_msg_from_high);
	}
}

/**
 * @brief Process IPC Reception
 *
 * This is an internal function to the WLAN MAC Low framework to process
 * received IPC messages and call the appropriate callback.
 *
 * @param None
 * @return None
 */
void process_ipc_msg_from_high(wlan_ipc_msg* msg){
	u16 tx_pkt_buf;
	u8 rate;
	tx_frame_info* tx_mpdu;
	wlan_ipc_msg ipc_msg_to_high;
	u32 status;
	mac_header_80211* tx_80211_header;
	beacon_probe_frame* beacon;
	u16 n_dbps;
	u32 isLocked, owner;
	u64 new_timestamp;

		switch(IPC_MBOX_MSG_ID_TO_MSG(msg->msg_id)){
			case IPC_MBOX_CONFIG_RF_IFC:
				process_config_rf_ifc((ipc_config_rf_ifc*)ipc_msg_from_high_payload);
			break;

			case IPC_MBOX_CONFIG_MAC:
				process_config_mac((ipc_config_mac*)ipc_msg_from_high_payload);
			break;

			case IPC_MBOX_SET_TIME:
				new_timestamp = *(u64*)ipc_msg_from_high_payload;
				wlan_mac_low_set_time(new_timestamp);
			break;

			case IPC_MBOX_CONFIG_PHY_TX:
				process_config_phy_tx((ipc_config_phy_tx*)ipc_msg_from_high_payload);
			break;

			case IPC_MBOX_CONFIG_PHY_RX:
				process_config_phy_rx((ipc_config_phy_rx*)ipc_msg_from_high_payload);
			break;

			case IPC_MBOX_TX_MPDU_READY:

				//Message is an indication that a Tx Pkt Buf needs processing
				tx_pkt_buf = msg->arg0;


				ipc_msg_to_high.msg_id = IPC_MBOX_MSG_ID(IPC_MBOX_TX_MPDU_ACCEPT);
				ipc_msg_to_high.num_payload_words = 0;
				ipc_msg_to_high.arg0 = tx_pkt_buf;
				ipc_mailbox_write_msg(&ipc_msg_to_high);


				if(lock_pkt_buf_tx(tx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
					warp_printf(PL_ERROR, "Error: unable to lock TX pkt_buf %d\n", tx_pkt_buf);

					status_pkt_buf_tx(tx_pkt_buf, &isLocked, &owner);

					warp_printf(PL_ERROR, "	TX pkt_buf %d status: isLocked = %d, owner = %d\n", tx_pkt_buf, isLocked, owner);

				} else {

					tx_mpdu = (tx_frame_info*)TX_PKT_BUF_TO_ADDR(tx_pkt_buf);

					tx_mpdu->delay_accept = (u32)(get_usec_timestamp() - tx_mpdu->timestamp_create);

					//Convert human-readable rates into PHY rates
					//n_dbps is used to calculate duration of received ACKs.
					//This rate selection is specified in 9.7.6.5.2 of 802.11-2012

					switch(tx_mpdu->rate){
						case WLAN_MAC_RATE_1M:
							warp_printf(PL_ERROR, "Error: DSSS rate was selected for transmission. Only OFDM transmissions are supported.\n");
						break;
						case WLAN_MAC_RATE_6M:
							rate = WLAN_PHY_RATE_BPSK12;
							n_dbps = N_DBPS_R6;
						break;
						case WLAN_MAC_RATE_9M:
							rate = WLAN_PHY_RATE_BPSK34;
							n_dbps = N_DBPS_R6;
						break;
						case WLAN_MAC_RATE_12M:
							rate = WLAN_PHY_RATE_QPSK12;
							n_dbps = N_DBPS_R12;
						break;
						case WLAN_MAC_RATE_18M:
							rate = WLAN_PHY_RATE_QPSK34;
							n_dbps = N_DBPS_R12;
						break;
						case WLAN_MAC_RATE_24M:
							rate = WLAN_PHY_RATE_16QAM12;
							n_dbps = N_DBPS_R24;
						break;
						case WLAN_MAC_RATE_36M:
							rate = WLAN_PHY_RATE_16QAM34;
							n_dbps = N_DBPS_R24;
						break;
						case WLAN_MAC_RATE_48M:
							rate = WLAN_PHY_RATE_64QAM23;
							n_dbps = N_DBPS_R24;
						break;
						case WLAN_MAC_RATE_54M:
							rate = WLAN_PHY_RATE_64QAM34;
							n_dbps = N_DBPS_R24;
						break;
						default:
							xil_printf("Rate %d\n", tx_mpdu->rate);
							xil_printf("Len %d\n", tx_mpdu->length);
						break;
					}

					if((tx_mpdu->flags) & TX_MPDU_FLAGS_FILL_DURATION){
						tx_80211_header = (mac_header_80211*)(TX_PKT_BUF_TO_ADDR(tx_pkt_buf)+PHY_TX_PKT_BUF_MPDU_OFFSET);
						tx_80211_header->duration_id = wlan_ofdm_txtime(sizeof(mac_header_80211_ACK)+WLAN_PHY_FCS_NBYTES, n_dbps) + T_SIFS;
					}

					if((tx_mpdu->flags) & TX_MPDU_FLAGS_FILL_TIMESTAMP){
						beacon = (beacon_probe_frame*)(TX_PKT_BUF_TO_ADDR(tx_pkt_buf)+PHY_TX_PKT_BUF_MPDU_OFFSET+sizeof(mac_header_80211));
						beacon->timestamp = get_usec_timestamp();
					}

					status = frame_tx_callback(tx_pkt_buf, rate, tx_mpdu->length);

					tx_mpdu->delay_done = (u32)(get_usec_timestamp() - (tx_mpdu->timestamp_create + (u64)(tx_mpdu->delay_accept)));

					if(status == 0){
						tx_mpdu->state_verbose = TX_MPDU_STATE_VERBOSE_SUCCESS;
					} else {
						tx_mpdu->state_verbose = TX_MPDU_STATE_VERBOSE_FAILURE;
					}

					tx_mpdu->state = TX_MPDU_STATE_EMPTY;

					if(unlock_pkt_buf_tx(tx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
						warp_printf(PL_ERROR, "Error: unable to unlock TX pkt_buf %d\n", tx_pkt_buf);
						wlan_mac_low_send_exception(EXC_MUTEX_TX_FAILURE);
					} else {
						ipc_msg_to_high.msg_id =  IPC_MBOX_MSG_ID(IPC_MBOX_TX_MPDU_DONE);
						ipc_msg_to_high.num_payload_words = 0;
						ipc_msg_to_high.arg0 = tx_pkt_buf;
						ipc_mailbox_write_msg(&ipc_msg_to_high);
					}



				}
			break;
		}
}

/**
 * @brief Set Time of System
 *
 * This function sets the microsecond counter common to the implementation to
 * a value specified as a parameter to the function.
 *
 * @param u64 new_time
 *  - the new base timestamp for the system
 * @return None
 */
void wlan_mac_low_set_time(u64 new_time) {
	Xil_Out32(WLAN_MAC_REG_SET_TIMESTAMP_LSB, (u32)new_time);
	Xil_Out32(WLAN_MAC_REG_SET_TIMESTAMP_MSB, (u32)(new_time>>32));

	Xil_Out32(WLAN_MAC_REG_CONTROL, (Xil_In32(WLAN_MAC_REG_CONTROL) & ~WLAN_MAC_CTRL_MASK_UPDATE_TIMESTAMP));
	Xil_Out32(WLAN_MAC_REG_CONTROL, (Xil_In32(WLAN_MAC_REG_CONTROL) | WLAN_MAC_CTRL_MASK_UPDATE_TIMESTAMP));
	Xil_Out32(WLAN_MAC_REG_CONTROL, (Xil_In32(WLAN_MAC_REG_CONTROL) & ~WLAN_MAC_CTRL_MASK_UPDATE_TIMESTAMP));
}

/**
 * @brief Process RF Interface Configuration
 *
 * This function process RF interface configurations and directly writes the radio controller.
 *
 * @param ipc_config_rf_ifc config_rf_ifc
 *  - configuration struct
 * @return None
 */
void process_config_rf_ifc(ipc_config_rf_ifc* config_rf_ifc){

	if((config_rf_ifc->channel)!=0xFF){
		mac_param_chan = config_rf_ifc->channel;
		//TODO: allow mac_param_chan to select 5GHz channels
		radio_controller_setCenterFrequency(RC_BASEADDR, (RC_ALL_RF), mac_param_band, mac_param_chan);
	}
}

/**
 * @brief Process MAC Configuration
 *
 * This function processes MAC configurations.
 *
 * @param ipc_config_mac config_mac
 *  - configuration struct
 * @return None
 */
void process_config_mac(ipc_config_mac* config_mac){
	//TODO
}

/**
 * @brief Initialize Hardware Info Struct
 *
 * This function initializes the hardware info struct with values read from the EEPROM.
 *
 * @param None
 * @return None
 */
void wlan_mac_low_init_hw_info( u32 type ) {

	// Initialize the wlan_mac_hw_info structure to all zeros
	//
	memset( (void*)( &hw_info ), 0x0, sizeof( wlan_mac_hw_info ) );

	// Set General Node information
	hw_info.type          = type;
    hw_info.serial_number = w3_eeprom_readSerialNum(EEPROM_BASEADDR);
    hw_info.fpga_dna[1]   = w3_eeprom_read_fpga_dna(EEPROM_BASEADDR, 1);
    hw_info.fpga_dna[0]   = w3_eeprom_read_fpga_dna(EEPROM_BASEADDR, 0);

    // Set HW Addresses
    //   - NOTE:  The w3_eeprom_readEthAddr() function handles the case when the WARP v3
    //     hardware does not have a valid Ethernet address
    //
	w3_eeprom_readEthAddr(EEPROM_BASEADDR, 0, hw_info.hw_addr_wlan);
	w3_eeprom_readEthAddr(EEPROM_BASEADDR, 1, hw_info.hw_addr_wn);

    // WARPNet will use ethernet device 1 unless you change this function
    hw_info.wn_exp_eth_device = 1;
}

/**
 * @brief Return Hardware Info Struct
 *
 * This function returns the hardware info struct stored in the MAC Low Framework
 *
 * @param None
 * @return None
 */
wlan_mac_hw_info* wlan_mac_low_get_hw_info(){
	return &hw_info;
}

/**
 * @brief Return Current Channel Selection
 *
 * This function returns the the current channel.
 *
 * @param None
 * @return None
 */
u32 wlan_mac_low_get_active_channel(){
	return mac_param_chan;
}


/**
 * @brief Calculates Rx Power (in dBm)
 *
 * This function calculates receive power for a given RSSI and LNA gain.
 *
 * @param None
 * @return None
 * @note This function's calculations change based on mac_param_band.
 */
inline int wlan_mac_low_calculate_rx_power(u16 rssi, u8 lna_gain){
#define RSSI_SLOPE_BITSHIFT		3 //Was 4 in old PHY; sliced 16MSB in v31
#define RSSI_OFFSET_LNA_LOW		(-61)
#define RSSI_OFFSET_LNA_MED		(-76)
#define RSSI_OFFSET_LNA_HIGH	(-92)
	u8 band;
	int power = -100;

	band = mac_param_band;

	if(band == RC_24GHZ){
		switch(lna_gain){
			case 0:
			case 1:
				//Low LNA Gain State
				power = (rssi>>(RSSI_SLOPE_BITSHIFT + PHY_RX_RSSI_SUM_LEN_BITS)) + RSSI_OFFSET_LNA_LOW;
			break;

			case 2:
				//Medium LNA Gain State
				power = (rssi>>(RSSI_SLOPE_BITSHIFT + PHY_RX_RSSI_SUM_LEN_BITS)) + RSSI_OFFSET_LNA_MED;
			break;

			case 3:
				//High LNA Gain State
				power = (rssi>>(RSSI_SLOPE_BITSHIFT + PHY_RX_RSSI_SUM_LEN_BITS)) + RSSI_OFFSET_LNA_HIGH;
			break;

		}
	}
	return power;
}

/**
 * @brief Polls for PHY Rx Start
 *
 * This function polls for PHY receptions and calls the appropriate callback;
 *
 * @param None
 * @return u32
 * 	- status flags about the reception
 */
inline u32 wlan_mac_low_poll_frame_rx(){
	u32 return_status = 0;
	u32 rate, length;
	u32 mac_hw_status = wlan_mac_get_status();

	//is the MAC currently blocking the rx PHY
	//WLAN_MAC_STATUS_MASK_RX_PHY_BLOCKED
	if(mac_hw_status & (WLAN_MAC_STATUS_MASK_PHY_RX_ACTIVE | WLAN_MAC_STATUS_MASK_RX_PHY_BLOCKED)) {

		return_status |= POLL_MAC_STATUS_RECEIVED_PKT; //We received something in this poll

		length = wlan_mac_get_rx_phy_length() - WLAN_PHY_FCS_NBYTES; //Strip off FCS
		rate =  wlan_mac_get_rx_phy_rate();

		switch(rate){
			case WLAN_PHY_RATE_DSSS_1M:
				rate = WLAN_MAC_RATE_1M;
			break;
			case WLAN_PHY_RATE_BPSK12:
				rate = WLAN_MAC_RATE_6M;
			break;
			case WLAN_PHY_RATE_BPSK34:
				rate = WLAN_MAC_RATE_9M;
			break;
			case WLAN_PHY_RATE_QPSK12:
				rate = WLAN_MAC_RATE_12M;
			break;
			case WLAN_PHY_RATE_QPSK34:
				rate = WLAN_MAC_RATE_18M;
			break;
			case WLAN_PHY_RATE_16QAM12:
				rate = WLAN_MAC_RATE_24M;
			break;
			case WLAN_PHY_RATE_16QAM34:
				rate = WLAN_MAC_RATE_36M;
			break;
			case WLAN_PHY_RATE_64QAM23:
				rate = WLAN_MAC_RATE_48M;
			break;
			case WLAN_PHY_RATE_64QAM34:
				rate = WLAN_MAC_RATE_54M;
			break;
		}

		if(wlan_mac_get_rx_phy_sel() == WLAN_RX_PHY_OFDM) {
			//OFDM packet is being received
			return_status |= frame_rx_callback(rx_pkt_buf, rate, length);
		} else {
			//DSSS packet is being received
			length = length-5;
			return_status |= frame_rx_callback(rx_pkt_buf, rate, length);
		}
		//wlan_mac_dcf_hw_unblock_rx_phy();
	}

	return return_status;
}


/**
 * @brief Set Frame Reception Callback
 *
 * Tells the framework which function should be called when
 * the PHY begins processing a frame reception
 *
 * @param function_ptr_t callback
 *  - Pointer to callback function
 * @return None
 *
 */
void wlan_mac_low_set_frame_rx_callback(function_ptr_t callback){
	frame_rx_callback = callback;
}

/**
 * @brief Set Frame Transmission Callback
 *
 * Tells the framework which function should be called when
 * an MPDU is passed down from the upper-level MAC for
 * wireless transmission
 *
 * @param function_ptr_t callback
 *  - Pointer to callback function
 * @return None
 *
 */
void wlan_mac_low_set_frame_tx_callback(function_ptr_t callback){
	frame_tx_callback = callback;
}

/**
 * @brief Notify upper-level MAC of frame reception
 *
 * Sends an IPC message to the upper-level MAC to notify it
 * that a frame has been received and is ready to be processed
 *
 * @param None
 * @return None
 * @note This function assumes it is called in the same context where
 * rx_pkt_buf is still valid.
 */
void wlan_mac_low_frame_ipc_send(){
	wlan_ipc_msg ipc_msg_to_high;
	//IPC_MBOX_GRP_PKT_BUF -> IPC_MBOX_GRP_RX_MPDU_DONE
	ipc_msg_to_high.msg_id = IPC_MBOX_MSG_ID(IPC_MBOX_RX_MPDU_READY);
	ipc_msg_to_high.arg0 = rx_pkt_buf;
	ipc_msg_to_high.num_payload_words = 0;
	ipc_mailbox_write_msg(&ipc_msg_to_high);
}

/**
 * @brief Search for and Lock Empty Packet Buffer (Blocking)
 *
 * This is a blocking function for finding and locking an empty rx packet
 * buffer.
 *
 * @param None
 * @return None
 * @note This function assumes it is called in the same context where
 * rx_pkt_buf is still valid.
 */
inline void wlan_mac_low_lock_empty_rx_pkt_buf(){
	//This function blocks until it safely finds a packet buffer for the PHY RX to stash receptions
	rx_frame_info* rx_mpdu;
	u32 i = 1;

	while(1){
		rx_pkt_buf = (rx_pkt_buf+1) % NUM_RX_PKT_BUFS;
		rx_mpdu = (rx_frame_info*) RX_PKT_BUF_TO_ADDR(rx_pkt_buf);
		if((rx_mpdu->state) == RX_MPDU_STATE_EMPTY){
			if(lock_pkt_buf_rx(rx_pkt_buf) == PKT_BUF_MUTEX_SUCCESS){

				//bzero((void *)(RX_PKT_BUF_TO_ADDR(rx_pkt_buf)), 2048);

				rx_mpdu->state = RX_MPDU_STATE_RX_PENDING;
				wlan_phy_rx_pkt_buf_ofdm(rx_pkt_buf);
				wlan_phy_rx_pkt_buf_dsss(rx_pkt_buf);


				return;
			}
		}
		xil_printf("Searching for empty packet buff %d\n", i);
		i++;
	}
}

/**
 * @brief Get the Current Microsecond Timestamp
 *
 * This function returns the current timestamp of the system
 *
 * @param None
 * @return u64
 * - microsecond timestamp
 */
inline u64 get_usec_timestamp(){
	u32 timestamp_high_u32;
	u32 timestamp_low_u32;
	u64 timestamp_u64;
	timestamp_high_u32 = Xil_In32(WLAN_MAC_REG_TIMESTAMP_MSB);
	timestamp_low_u32 = Xil_In32(WLAN_MAC_REG_TIMESTAMP_LSB);
	timestamp_u64 = (((u64)timestamp_high_u32)<<32) + ((u64)timestamp_low_u32);
	return timestamp_u64;
}

/**
 * @brief Get the Rx Start Microsecond Timestamp
 *
 * This function returns the rx start timestamp of the system
 *
 * @param None
 * @return u64
 * - microsecond timestamp
 */
inline u64 get_rx_start_timestamp() {
	u32 timestamp_high_u32;
	u32 timestamp_low_u32;
	u64 timestamp_u64;
	timestamp_high_u32 = Xil_In32(WLAN_MAC_REG_RX_TIMESTAMP_MSB);
	timestamp_low_u32 = Xil_In32(WLAN_MAC_REG_RX_TIMESTAMP_LSB);
	timestamp_u64 = (((u64)timestamp_high_u32)<<32) + ((u64)timestamp_low_u32);
	return timestamp_u64;
}

/**
 * @brief Unblock the Receive PHY
 *
 * This function unblocks the receive PHY, allowing it to overwrite the currently selected
 * rx packet buffer
 *
 * @param None
 * @return None
 */
void wlan_mac_dcf_hw_unblock_rx_phy() {
	//Posedge on WLAN_MAC_CTRL_MASK_RX_PHY_BLOCK_RESET unblocks PHY (clear then set here to ensure posedge)
	REG_CLEAR_BITS(WLAN_MAC_REG_CONTROL, WLAN_MAC_CTRL_MASK_RX_PHY_BLOCK_RESET);
	REG_SET_BITS(WLAN_MAC_REG_CONTROL, WLAN_MAC_CTRL_MASK_RX_PHY_BLOCK_RESET);
	REG_CLEAR_BITS(WLAN_MAC_REG_CONTROL, WLAN_MAC_CTRL_MASK_RX_PHY_BLOCK_RESET);

	return;
}

/**
 * @brief Finish PHY Reception
 *
 * This function returns the rx start timestamp of the system
 *
 * @param None
 * @return u32
 * - FCS status (RX_MPDU_STATE_FCS_GOOD or RX_MPDU_STATE_FCS_BAD)
 */
inline u32 wlan_mac_dcf_hw_rx_finish(){
	u32 mac_status;
	//Wait for the packet to finish
	do{
		mac_status = wlan_mac_get_status();
	} while(mac_status & WLAN_MAC_STATUS_MASK_PHY_RX_ACTIVE);

	//Check FCS

	if(mac_status & WLAN_MAC_STATUS_MASK_RX_FCS_GOOD) {
		return RX_MPDU_STATE_FCS_GOOD;
	} else {
		wlan_mac_auto_tx_en(0);
		return RX_MPDU_STATE_FCS_BAD;
	}
}




