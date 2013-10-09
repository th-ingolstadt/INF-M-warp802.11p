////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_ap.c
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

//Xilinx SDK includes
#include "xparameters.h"
#include "stdio.h"
#include "stdlib.h"
#include "xtmrctr.h"
#include "xio.h"
#include "string.h"
#include "xintc.h"

//WARP includes
#include "wlan_mac_ipc_util.h"
#include "wlan_mac_misc_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_util.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_sta.h"
#include "ascii_characters.h"


#ifdef WLAN_USE_UART_MENU


// SSID variables
extern char*  access_point_ssid;

// Control variables
extern u8  default_unicast_rate;
extern int association_state;                      // Section 10.3 of 802.11-2012
extern u8  uart_mode;
extern u8  active_scan;

// Access point information
extern ap_info* ap_list;
extern u8       num_ap_list;

extern u8       access_point_num_basic_rates;
extern u8       access_point_basic_rates[NUM_BASIC_RATES_MAX];

// Association Table variables
//   The last entry in associations[MAX_ASSOCIATIONS][] is swap space
extern station_info access_point;

// AP channel
extern u32 mac_param_chan;




void uart_rx(u8 rxByte){
	#define MAX_NUM_AP_CHARS 4
	static char numerical_entry[MAX_NUM_AP_CHARS+1];
	static u8 curr_decade = 0;
	static u8 ltg_mode = 0;

	u16 ap_sel;
	wlan_ipc_msg ipc_msg_to_low;
	u32 ipc_msg_to_low_payload[1];
	ipc_config_rf_ifc* config_rf_ifc;
	cbr_params cbr_parameters;

	if(rxByte == ASCII_ESC){
		uart_mode = UART_MODE_MAIN;
		print_menu();
		return;
	}

	switch(uart_mode){
		case UART_MODE_MAIN:
			switch(rxByte){
				case ASCII_1:
					uart_mode = UART_MODE_INTERACTIVE;
					print_station_status();
				break;

				case ASCII_a:
					//Send bcast probe requests across all channels
					if(active_scan ==0){
						num_ap_list = 0;
						//xil_printf("- Free 0x%08x\n",ap_list);
						free(ap_list);
						ap_list = NULL;
						active_scan = 1;
						access_point_ssid = realloc(access_point_ssid, 1);
						*access_point_ssid = 0;
						//xil_printf("+++ starting active scan\n");
						probe_req_transmit();
					}
				break;

				case ASCII_r:
					if(default_unicast_rate > WLAN_MAC_RATE_6M){
						default_unicast_rate--;
					} else {
						default_unicast_rate = WLAN_MAC_RATE_6M;
					}


					access_point.tx_rate = default_unicast_rate;


					xil_printf("(-) Default Unicast Rate: %d Mbps\n", wlan_lib_mac_rate_to_mbps(default_unicast_rate));
				break;
				case ASCII_R:
					if(default_unicast_rate < WLAN_MAC_RATE_54M){
						default_unicast_rate++;
					} else {
						default_unicast_rate = WLAN_MAC_RATE_54M;
					}

					access_point.tx_rate = default_unicast_rate;

					xil_printf("(+) Default Unicast Rate: %d Mbps\n", wlan_lib_mac_rate_to_mbps(default_unicast_rate));
				break;
				case ASCII_l:
					if(ltg_mode == 0){
						#define LTG_INTERVAL 10000
						xil_printf("Enabling LTG mode to AP, interval = %d usec\n", LTG_INTERVAL);
						cbr_parameters.interval_usec = LTG_INTERVAL; //Time between calls to the packet generator in usec. 0 represents backlogged... go as fast as you can.
						start_ltg(0, LTG_TYPE_CBR, &cbr_parameters);

						ltg_mode = 1;

					} else {
						stop_ltg(0);
						ltg_mode = 0;
						xil_printf("Disabled LTG mode to AID 1\n");
					}
				break;
			}
		break;
		case UART_MODE_INTERACTIVE:
			switch(rxByte){
				case ASCII_r:
					//Reset statistics
					reset_station_statistics();
				break;
			}
		break;
		case UART_MODE_AP_LIST:
			switch(rxByte){
				case ASCII_CR:

					numerical_entry[curr_decade] = 0;
					curr_decade = 0;

					ap_sel = str2num(numerical_entry);

					if( (ap_sel >= 0) && (ap_sel <= (num_ap_list-1))){

						if( ap_list[ap_sel].private == 0) {
							uart_mode = UART_MODE_MAIN;
							mac_param_chan = ap_list[ap_sel].chan;

							//Send a message to other processor to tell it to switch channels
							ipc_msg_to_low.msg_id = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_RF_IFC);
							ipc_msg_to_low.num_payload_words = sizeof(ipc_config_rf_ifc)/sizeof(u32);
							ipc_msg_to_low.payload_ptr = &(ipc_msg_to_low_payload[0]);
							init_ipc_config(config_rf_ifc,ipc_msg_to_low_payload,ipc_config_rf_ifc);
							config_rf_ifc->channel = mac_param_chan;
							ipc_mailbox_write_msg(&ipc_msg_to_low);


							xil_printf("\nAttempting to join %s\n", ap_list[ap_sel].ssid);
							memcpy(access_point.addr, ap_list[ap_sel].bssid, 6);

							access_point_ssid = realloc(access_point_ssid, strlen(ap_list[ap_sel].ssid)+1);
							//xil_printf("allocated %d bytes in 0x%08x\n", strlen(ap_list[ap_sel].ssid), access_point_ssid);
							strcpy(access_point_ssid,ap_list[ap_sel].ssid);

							access_point_num_basic_rates = ap_list[ap_sel].num_basic_rates;
							memcpy(access_point_basic_rates, ap_list[ap_sel].basic_rates,access_point_num_basic_rates);

							association_state = 1;
							attempt_authentication();

						} else {
							xil_printf("\nInvalid selection, please choose an AP that is not private: ");
						}


					} else {

						xil_printf("\nInvalid selection, please choose a number between [0,%d]: ", num_ap_list-1);

					}



				break;
				case ASCII_DEL:
					if(curr_decade > 0){
						curr_decade--;
						xil_printf("\b \b");
					}

				break;
				default:
					if( (rxByte <= ASCII_9) && (rxByte >= ASCII_0) ){
						//the user entered a character

						if(curr_decade < MAX_NUM_AP_CHARS){
							xil_printf("%c", rxByte);
							numerical_entry[curr_decade] = rxByte;
							curr_decade++;
						}



					}

				break;

			}
		break;

	}


}


void print_menu(){
	xil_printf("\f");
	xil_printf("********************** Station Menu **********************\n");
	xil_printf("[1] - Interactive Station Status\n");
	xil_printf("[2] - Print Queue Status\n");
	xil_printf("\n");
	xil_printf("[a] - 	active scan and display nearby APs\n");
	xil_printf("[r/R] - change default unicast rate\n");
	xil_printf("[l]	  - toggle local traffic generation to AP\n");

}

void print_station_status(){

	u64 timestamp;
	if(uart_mode == UART_MODE_INTERACTIVE){
		timestamp = get_usec_timestamp();
		xil_printf("\f");

		xil_printf("---------------------------------------------------\n");
		xil_printf(" AID: %02x -- MAC Addr: %02x:%02x:%02x:%02x:%02x:%02x\n", access_point.AID,
			access_point.addr[0],access_point.addr[1],access_point.addr[2],access_point.addr[3],access_point.addr[4],access_point.addr[5]);
			if(access_point.AID > 0){
				xil_printf("     - Last heard from %d ms ago\n",((u32)(timestamp - (access_point.rx_timestamp)))/1000);
				xil_printf("     - Last Rx Power: %d dBm\n",access_point.last_rx_power);
				xil_printf("     - # of queued MPDUs: %d\n", queue_num_queued(access_point.AID));
				xil_printf("     - # Tx MPDUs: %d (%d successful)\n", access_point.num_tx_total, access_point.num_tx_success);
				xil_printf("     - # Rx MPDUs: %d (%d bytes)\n", access_point.num_rx_success, access_point.num_rx_bytes);
			}
		xil_printf("---------------------------------------------------\n");
		xil_printf("\n");
		xil_printf("[r] - reset statistics\n");

		//Update display
		wlan_mac_schedule_event(SCHEDULE_COARSE, 1000000, (void*)print_station_status);
	}
}




#endif


