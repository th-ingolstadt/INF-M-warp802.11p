/** @file wlan_mac_nomac.c
 *  @brief Simple MAC that does nothing but transmit and receive
 *
 *  @copyright Copyright 2014-2015, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

/***************************** Include Files *********************************/

// Xilinx SDK includes
#include "xparameters.h"
#include <stdio.h>
#include <stdlib.h>
#include "xtmrctr.h"
#include "xio.h"
#include <string.h>

// WARP includes
#include "wlan_mac_low.h"
#include "w3_userio.h"
#include "radio_controller.h"

#include "wlan_mac_ipc_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_misc_util.h"
#include "wlan_phy_util.h"
#include "wlan_mac_nomac.h"

#include "wlan_exp.h"
#include "math.h"


/*************************** Constant Definitions ****************************/

#define WARPNET_TYPE_80211_LOW         WARPNET_TYPE_80211_LOW_NOMAC
#define NUM_LEDS                       4

/*********************** Global Variable Definitions *************************/

///////// TOKEN MAC EXTENSION /////////
u8 in_reservation;
u64 reservation_ts_end;
///////// TOKEN MAC EXTENSION /////////


/*************************** Variable Definitions ****************************/
static u8                              eeprom_addr[6];

volatile u8                            red_led_index;
volatile u8                            green_led_index;

/******************************** Functions **********************************/

int main(){

	wlan_mac_hw_info* hw_info;
	xil_printf("\f");
	xil_printf("----- Mango 802.11 Reference Design -----\n");
	xil_printf("----- v1.3 ------------------------------\n");
	xil_printf("----- wlan_mac_nomac --------------------\n");
	xil_printf("Compiled %s %s\n\n", __DATE__, __TIME__);

	xil_printf("Note: this UART is currently printing from CPU_LOW. To view prints from\n");
	xil_printf("and interact with CPU_HIGH, raise the right-most User I/O DIP switch bit.\n");
	xil_printf("This switch can be toggled live while the design is running.\n\n");

	wlan_tx_config_ant_mode(TX_ANTMODE_SISO_ANTA);

	red_led_index = 0;
	green_led_index = 0;
	userio_write_leds_green(USERIO_BASEADDR, (1<<green_led_index));
	userio_write_leds_red(USERIO_BASEADDR, (1<<red_led_index));

	wlan_mac_low_init(WARPNET_TYPE_80211_LOW);

	hw_info = wlan_mac_low_get_hw_info();
	memcpy(eeprom_addr,hw_info->hw_addr_wlan,6);


	wlan_mac_low_set_frame_rx_callback((void*)frame_receive);
	wlan_mac_low_set_frame_tx_callback((void*)frame_transmit);

	///////// TOKEN MAC EXTENSION /////////
	in_reservation = 0;
	wlan_mac_low_set_new_reservation_callback((void*)token_new_reservation);
	wlan_mac_low_set_adjust_reservation_ts_callback((void*)adjust_reservation_ts_end);
	///////// TOKEN MAC EXTENSION /////////

	wlan_mac_low_finish_init();

	REG_SET_BITS(WLAN_MAC_REG_CONTROL, (WLAN_MAC_CTRL_MASK_CCA_IGNORE_PHY_CS | WLAN_MAC_CTRL_MASK_CCA_IGNORE_NAV));

    xil_printf("Initialization Finished\n");

	while(1){

		//Poll PHY RX start
		wlan_mac_low_poll_frame_rx();

		//Poll IPC rx
		wlan_mac_low_poll_ipc_rx();

		///////// TOKEN MAC EXTENSION /////////
		poll_reservation_time();
		///////// TOKEN MAC EXTENSION /////////
	}
	return 0;
}

/**
 * @brief Handles reception of a wireless packet
 *
 * This function is called after a good SIGNAL field is detected by either PHY (OFDM or DSSS)
 * It is the responsibility of this function to wait until a sufficient number of bytes have been received
 * before it can start to process those bytes. When this function is called the eventual checksum status is
 * unknown. In NOMAC, this function doesn't need to do any kind of filtering or operations like transmitting
 * an acknowledgment.
 *
 * @param u8 rx_pkt_buf
 *  -Index of the Rx packet buffer containing the newly recevied packet
 * @param u8 rate
 *  -Index of PHY rate at which pcaket was received
 * @param u16 length
 *  -Number of bytes received by the PHY, including MAC header and FCS
 * @return
 *  - always returns 0 in NOMAC implementation
 */
u32 frame_receive(u8 rx_pkt_buf, phy_rx_details* phy_details){
	//This function is called after a good SIGNAL field is detected by either PHY (OFDM or DSSS)
	//It is the responsibility of this function to wait until a sufficient number of bytes have been received
	// before it can start to process those bytes. When this function is called the eventual checksum status is
	// unknown. The packet contents can be provisionally processed (e.g. prepare an ACK for fast transmission),
	// but post-reception actions must be conditioned on the eventual FCS status (good or bad).
	//
	// Note: The timing of this function is critical for correct operation of the 802.11 DCF. It is not
	// safe to add large delays to this function (e.g. xil_printf or usleep)
	//
	//Two primary job responsibilities of this function:
	// (1): Prepare outgoing ACK packets and instruct the MAC_DCF_HW core whether or not to send ACKs
	// (2): Pass up MPDUs (FCS valid or invalid) to CPU_HIGH

	///////// TOKEN MAC EXTENSION /////////
	u8 unicast_to_me;
	mac_header_80211* rx_header;
	mac_frame_custom_token* rx_token_frame;
	u8 ctrl_tx_gain;
	u32 tx_length;
	u32 return_value = 0;
	u32 mac_hw_status;
	///////// TOKEN MAC EXTENSION /////////

	rx_frame_info* mpdu_info;
	void* pkt_buf_addr = (void *)RX_PKT_BUF_TO_ADDR(rx_pkt_buf);

	mpdu_info = (rx_frame_info*)pkt_buf_addr;

	///////// TOKEN MAC EXTENSION /////////
	rx_header = (mac_header_80211*)((void*)(pkt_buf_addr + PHY_RX_PKT_BUF_MPDU_OFFSET));

	//Wait until the PHY has written enough bytes so that the first address field can be processed
	while(wlan_mac_get_last_byte_index() < MAC_HW_LASTBYTE_TOKEN){
	};

	unicast_to_me = wlan_addr_eq(rx_header->address_1, eeprom_addr);

	if(unicast_to_me && (rx_header->frame_control_1 == MAC_FRAME_CTRL1_SUBTYPE_TOKEN_OFFER)){
		//Received a token offer
		rx_token_frame = (mac_frame_custom_token*)rx_header;

		//Set up a Token Response
		//wlan_mac_tx_ctrl_B_params(pktBuf, antMask, req_zeroNAV, preWait_postRxTimer1, preWait_postRxTimer2, postWait_postTxTimer1)
		wlan_mac_tx_ctrl_B_params(TX_PKT_BUF_TOKEN, 0x1, 0, 1, 0, 0);

		//ACKs are transmitted with a nominal Tx power used for all control packets
		ctrl_tx_gain = wlan_mac_low_dbm_to_gain_target(15);
		wlan_mac_tx_ctrl_B_gains(ctrl_tx_gain, ctrl_tx_gain, ctrl_tx_gain, ctrl_tx_gain);

		//Construct the token response frame in the dedicated Tx pkt buf
		tx_length = wlan_create_token_response_frame((void*)(TX_PKT_BUF_TO_ADDR(TX_PKT_BUF_TOKEN) + PHY_TX_PKT_BUF_MPDU_OFFSET),
													  rx_token_frame->address_ta,
												      eeprom_addr,
												      0,
												      rx_token_frame->res_duration_usec); //TODO: Calculate appropriate duration


		//Write the SIGNAL field for the ACK
		wlan_phy_set_tx_signal(TX_PKT_BUF_TOKEN, WLAN_PHY_RATE_BPSK12, tx_length);

		mpdu_info->state = wlan_mac_dcf_hw_rx_finish();

		if(mpdu_info->state == RX_MPDU_STATE_FCS_GOOD){
			wlan_mac_tx_ctrl_B_start(1);
			wlan_mac_tx_ctrl_B_start(0);
		}

		do{
			mac_hw_status = wlan_mac_get_status();

			if( (mac_hw_status & WLAN_MAC_STATUS_MASK_TX_B_STATE) == WLAN_MAC_STATUS_TX_B_STATE_DO_TX ) {
				break;
			}
		} while(mac_hw_status & WLAN_MAC_STATUS_MASK_TX_B_PENDING);

		//Since this is our reservation period, we are now allowed to transmit
		in_reservation = 1;
		reservation_ts_end = get_usec_timestamp() + ((u64)rx_token_frame->res_duration_usec);
		wlan_mac_low_enable_new_mpdu_tx();
	} else if(unicast_to_me && (rx_header->frame_control_1 == MAC_FRAME_CTRL1_SUBTYPE_TOKEN_RESPONSE)) {
		//Received a token offer
		rx_token_frame = (mac_frame_custom_token*)rx_header;

		if(rx_token_frame->res_duration_usec != 0){
			mpdu_info->state = wlan_mac_dcf_hw_rx_finish();
			if(mpdu_info->state == RX_MPDU_STATE_FCS_GOOD){
				return_value |= POLL_MAC_STATUS_TOKEN_OFFER_ACCEPTED;
			}
		}
	} else {
		mpdu_info->state = wlan_mac_dcf_hw_rx_finish(); //Blocks until reception is complete
	}

	///////// TOKEN MAC EXTENSION /////////


	mpdu_info->flags = 0;
	mpdu_info->phy_details = *phy_details;
	mpdu_info->channel = wlan_mac_low_get_active_channel();
	mpdu_info->timestamp = get_rx_start_timestamp();


	mpdu_info->ant_mode = wlan_phy_rx_get_active_rx_ant();

	mpdu_info->rf_gain = wlan_phy_rx_get_agc_RFG(mpdu_info->ant_mode);
	mpdu_info->bb_gain = wlan_phy_rx_get_agc_BBG(mpdu_info->ant_mode);
	mpdu_info->rx_power = wlan_mac_low_calculate_rx_power(wlan_phy_rx_get_pkt_rssi(mpdu_info->ant_mode), wlan_phy_rx_get_agc_RFG(mpdu_info->ant_mode));

	if(mpdu_info->state == RX_MPDU_STATE_FCS_GOOD){
		green_led_index = (green_led_index + 1) % NUM_LEDS;
		userio_write_leds_green(USERIO_BASEADDR, (1<<green_led_index));
	} else {
		red_led_index = (red_led_index + 1) % NUM_LEDS;
		userio_write_leds_red(USERIO_BASEADDR, (1<<red_led_index));
	}

	//Unlock the pkt buf mutex before passing the packet up
	// If this fails, something has gone horribly wrong
	if(unlock_pkt_buf_rx(rx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
		xil_printf("Error: unable to unlock RX pkt_buf %d\n", rx_pkt_buf);
		wlan_mac_low_send_exception(EXC_MUTEX_RX_FAILURE);
	} else {
		wlan_mac_low_frame_ipc_send();
		//Find a free packet buffer and begin receiving packets there (blocks until free buf is found)
		wlan_mac_low_lock_empty_rx_pkt_buf();
	}

	//Unblock the PHY post-Rx (no harm calling this if the PHY isn't actually blocked)
	wlan_mac_dcf_hw_unblock_rx_phy();

	///////// TOKEN MAC EXTENSION /////////
	return return_value;
	///////// TOKEN MAC EXTENSION /////////
}

/**
 * @brief Handles transmission of a wireless packet
 *
 * This function is called to transmit a new packet via the PHY. While the code does utilize the wlan_mac_dcf_hw core,
 * it bypasses any of the DCF-specific state in order to directly transmit the frame. This function should be called once per packet and will return 
 * immediately following that transmission. It will not perform any DCF-like retransmissions.
 *
 * This function is called once per IPC_MBOX_TX_MPDU_READY message from CPU High. The IPC_MBOX_TX_MPDU_DONE message will be sent
 * back to CPU High when this function returns.
 *
 * @param u8 rx_pkt_buf
 *  -Index of the Tx packet buffer containing the packet to transmit
 * @param u8 rate
 *  -Index of PHY rate at which packet will be transmitted
 * @param u16 length
 *  -Number of bytes in packet, including MAC header and FCS
 * @param wlan_mac_low_tx_details* low_tx_details
 *  -Pointer to array of metadata entries to be created for each PHY transmission of this packet (eventually leading to TX_LOW log entries)
 * @return
 *  -Transmission result
 */
int frame_transmit(u8 pkt_buf, u8 rate, u16 length, wlan_mac_low_tx_details* low_tx_details) {
	//This function manages the MAC_DCF_HW core.

	tx_frame_info* mpdu_info = (tx_frame_info*) (TX_PKT_BUF_TO_ADDR(pkt_buf));
	u64 last_tx_timestamp;
	int curr_tx_pow;
	last_tx_timestamp = (u64)(mpdu_info->delay_accept) + (u64)(mpdu_info->timestamp_create);

	u32 mac_hw_status;

	//Write the SIGNAL field (interpreted by the PHY during Tx waveform generation)
	wlan_phy_set_tx_signal(pkt_buf, rate, length);

	unsigned char mpdu_tx_ant_mask = 0;
	switch(mpdu_info->params.phy.antenna_mode) {
		case TX_ANTMODE_SISO_ANTA:
			mpdu_tx_ant_mask |= 0x1;
		break;
		case TX_ANTMODE_SISO_ANTB:
			mpdu_tx_ant_mask |= 0x2;
		break;
		case TX_ANTMODE_SISO_ANTC:
			mpdu_tx_ant_mask |= 0x4;
		break;
		case TX_ANTMODE_SISO_ANTD:
			mpdu_tx_ant_mask |= 0x8;
		break;
		default:
			mpdu_tx_ant_mask = 0x1;
		break;
	}

	mpdu_info->num_tx_attempts = 1;

	curr_tx_pow = wlan_mac_low_dbm_to_gain_target(mpdu_info->params.phy.power);

	//wlan_mac_tx_ctrl_A_params(pktBuf, antMask, preTx_backoff_slots, preWait_postRxTimer1, preWait_postTxTimer1, postWait_postTxTimer2)
	wlan_mac_tx_ctrl_A_params(pkt_buf, mpdu_tx_ant_mask, 0, 0, 0, 0);

	//Set Tx Gains
	wlan_mac_tx_ctrl_A_gains(curr_tx_pow, curr_tx_pow, curr_tx_pow, curr_tx_pow);

	//Before we mess with any PHY state, we need to make sure it isn't actively
	//transmitting. For example, it may be sending an ACK when we get to this part of the code
	while(wlan_mac_get_status() & WLAN_MAC_STATUS_MASK_TX_PHY_ACTIVE){}

	//Submit the MPDU for transmission - this starts the MAC hardware's MPDU Tx state machine
	wlan_mac_tx_ctrl_A_start(1);
	wlan_mac_tx_ctrl_A_start(0);

	//Wait for the MPDU Tx to finish
	do{
		if(low_tx_details != NULL){
			low_tx_details[0].mpdu_phy_params.rate = mpdu_info->params.phy.rate;
			low_tx_details[0].mpdu_phy_params.power = mpdu_info->params.phy.power;
			low_tx_details[0].mpdu_phy_params.antenna_mode = mpdu_info->params.phy.antenna_mode;
			low_tx_details[0].chan_num = wlan_mac_low_get_active_channel();
			low_tx_details[0].num_slots = 0;
			low_tx_details[0].cw = 0;
		}
		mac_hw_status = wlan_mac_get_status();

		if( mac_hw_status & WLAN_MAC_STATUS_MASK_TX_A_DONE) {
			if( low_tx_details != NULL ){
				low_tx_details[0].tx_start_delta = (u32)(get_tx_start_timestamp() - last_tx_timestamp);
				last_tx_timestamp = get_tx_start_timestamp();
			}

			switch( mac_hw_status & WLAN_MAC_STATUS_MASK_TX_A_RESULT ){
				case WLAN_MAC_STATUS_TX_A_RESULT_NONE:
					return 0;
				break;
			}
		}
	} while( mac_hw_status & WLAN_MAC_STATUS_MASK_TX_A_PENDING );

	return -1;
}

///////// TOKEN MAC EXTENSION /////////
void token_new_reservation(ipc_token_new_reservation* new_reservation){

	u8 mac_cfg_rate;
	u16 mac_cfg_length;
	int curr_tx_pow;
	u32 mac_hw_status;
	u32 rx_status;

	wlan_ipc_msg       ipc_msg_to_high;
	ipc_token_end_reservation ipc_payload;

	ipc_msg_to_high.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_TOKEN_END_RESERVATION);

	if( (sizeof(u32)*(sizeof(ipc_token_end_reservation)/sizeof(u32))) ==  sizeof(ipc_token_end_reservation) ){
		ipc_msg_to_high.num_payload_words = (sizeof(ipc_token_end_reservation)/sizeof(u32));
	} else {
		ipc_msg_to_high.num_payload_words = (sizeof(ipc_token_end_reservation)/sizeof(u32)) + 1;
	}

	ipc_msg_to_high.payload_ptr       = (u32*)(&ipc_payload);

	mac_cfg_rate = WLAN_PHY_RATE_BPSK12;

	if(wlan_addr_eq(new_reservation->addr, eeprom_addr)){
		//This is my reservation
		in_reservation = 1;
		wlan_mac_low_enable_new_mpdu_tx();
		reservation_ts_end = get_usec_timestamp() + ((u64)new_reservation->res_duration);
	} else {
		//This is someone else's reservation
		mac_cfg_length = wlan_create_token_offer_frame((void*)(TX_PKT_BUF_TO_ADDR(TX_PKT_BUF_TOKEN) + PHY_TX_PKT_BUF_MPDU_OFFSET),
										   new_reservation->addr,
										   eeprom_addr,
										   0,
										   new_reservation->res_duration); //TODO: Calculate appropriate duration



		wlan_phy_set_tx_signal(TX_PKT_BUF_TOKEN, mac_cfg_rate, mac_cfg_length); // Write SIGNAL for RTS

		curr_tx_pow = wlan_mac_low_dbm_to_gain_target(15);
		wlan_mac_tx_ctrl_A_gains(curr_tx_pow, curr_tx_pow, curr_tx_pow, curr_tx_pow);

		//wlan_mac_tx_ctrl_A_params(pktBuf, antMask, preTx_backoff_slots, preWait_postRxTimer1, preWait_postTxTimer1, postWait_postTxTimer2)
		//postTxTimer2 is a timeout. We'll use that to wait for a token response
		wlan_mac_tx_ctrl_A_params(TX_PKT_BUF_TOKEN, 0x1, 0, 0, 0, 1);

		//Start the Tx state machine
		wlan_mac_tx_ctrl_A_start(1);
		wlan_mac_tx_ctrl_A_start(0);

		//Wait for the MPDU Tx to finish
		do { //while(tx_status & WLAN_MAC_STATUS_MASK_TX_A_PENDING)

			//Poll the DCF core status register
			mac_hw_status = wlan_mac_get_status();

			if( mac_hw_status & WLAN_MAC_STATUS_MASK_TX_A_DONE ) {
				//Transmission is complete

				//Switch on the result of the transmission attempt
				switch( mac_hw_status & WLAN_MAC_STATUS_MASK_TX_A_RESULT ) {
					case WLAN_MAC_STATUS_TX_A_RESULT_RX_STARTED:
						//Transmission ended, followed by a new reception (hopefully a token response)

						//Handle the new reception
						rx_status = wlan_mac_low_poll_frame_rx();

						//Check if the reception is an ACK addressed to this node, received with a valid checksum
						if( (rx_status & POLL_MAC_STATUS_TOKEN_OFFER_ACCEPTED)) {

							//We are now in a new reservation state for this user
							in_reservation = 1;
							reservation_ts_end = get_usec_timestamp() + ((u64)new_reservation->res_duration);
						} else {
							//Received a packet immediately after transmitting, but it wasn't the offer response we wanted
							//This is equivalent to a timeout. Let CPU_HIGH know that this reservation period is over
							in_reservation = 0;
							ipc_payload.reason = TOKEN_TIMEOUT;
							//ipc_payload.low_tx_details; //TODO
							ipc_mailbox_write_msg(&ipc_msg_to_high);
						}
					break;
					case WLAN_MAC_STATUS_TX_A_RESULT_TIMEOUT:
						in_reservation = 0;
						ipc_payload.reason = TOKEN_TIMEOUT;
						//ipc_payload.low_tx_details; //TODO
						ipc_mailbox_write_msg(&ipc_msg_to_high);
					break;
				}
			} else { //else for if(mac_hw_status & WLAN_MAC_STATUS_MASK_TX_A_DONE)
				//Poll the MAC Rx state to check if a packet was received while our Tx was deferring

				if( mac_hw_status & (WLAN_MAC_STATUS_MASK_RX_PHY_ACTIVE | WLAN_MAC_STATUS_MASK_RX_PHY_BLOCKED_FCS_GOOD | WLAN_MAC_STATUS_MASK_RX_PHY_BLOCKED) ) {
					rx_status = wlan_mac_low_poll_frame_rx();
				}
			}//END if(Tx A state machine done)
		} while( mac_hw_status & WLAN_MAC_STATUS_MASK_TX_A_PENDING );
	}
}

void poll_reservation_time(){
	wlan_ipc_msg       ipc_msg_to_high;
	ipc_token_end_reservation ipc_payload;

	ipc_msg_to_high.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_TOKEN_END_RESERVATION);

	if( (sizeof(u32)*(sizeof(ipc_token_end_reservation)/sizeof(u32))) ==  sizeof(ipc_token_end_reservation) ){
		ipc_msg_to_high.num_payload_words = (sizeof(ipc_token_end_reservation)/sizeof(u32));
	} else {
		ipc_msg_to_high.num_payload_words = (sizeof(ipc_token_end_reservation)/sizeof(u32)) + 1;
	}

	ipc_msg_to_high.payload_ptr       = (u32*)(&ipc_payload);

	if(in_reservation && (get_usec_timestamp() >= reservation_ts_end)){
		in_reservation = 0;
		wlan_mac_low_disable_new_mpdu_tx();
		ipc_payload.reason = TOKEN_DURATION_COMPLETE;
		//ipc_payload.low_tx_details; //TODO
		ipc_mailbox_write_msg(&ipc_msg_to_high);
	}
}

void adjust_reservation_ts_end(s64 adjustment){
	reservation_ts_end += adjustment;
}

int wlan_create_token_offer_frame(void* pkt_buf_addr, u8* address_ra, u8* address_ta, u16 duration, u32 res_duration) {
	mac_frame_custom_token* token;
	token = (mac_frame_custom_token*)(pkt_buf_addr);

	token->frame_control_1 = MAC_FRAME_CTRL1_SUBTYPE_TOKEN_OFFER;
	token->frame_control_2 = 0;
	token->duration_id = duration;
	memcpy(token->address_ra, address_ra, 6);
	memcpy(token->address_ta, address_ta, 6);
	token->res_duration_usec = res_duration;

	//Include FCS in packet size (MAC accounts for FCS, even though the PHY calculates it)
	return (sizeof(mac_frame_custom_token)+WLAN_PHY_FCS_NBYTES);
}

int wlan_create_token_response_frame(void* pkt_buf_addr, u8* address_ra, u8* address_ta, u16 duration, u32 res_duration) {
	mac_frame_custom_token* token;
	token = (mac_frame_custom_token*)(pkt_buf_addr);

	token->frame_control_1 = MAC_FRAME_CTRL1_SUBTYPE_TOKEN_RESPONSE;
	token->frame_control_2 = 0;
	token->duration_id = duration;
	memcpy(token->address_ra, address_ra, 6);
	memcpy(token->address_ta, address_ta, 6);
	token->res_duration_usec = res_duration;

	//Include FCS in packet size (MAC accounts for FCS, even though the PHY calculates it)
	return (sizeof(mac_frame_custom_token)+WLAN_PHY_FCS_NBYTES);
}



///////// TOKEN MAC EXTENSION /////////

