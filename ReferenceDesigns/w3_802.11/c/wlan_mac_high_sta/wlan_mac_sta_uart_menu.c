/** @file wlan_mac_sta_uart_menu.c
 *  @brief Station UART Menu
 *
 *  This contains code for the 802.11 Station's UART menu.
 *
 *  @copyright Copyright 2014, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 *  @bug No known bugs
 */

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
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_sta.h"
#include "ascii_characters.h"
#include "wlan_mac_schedule.h"


#ifdef WLAN_USE_UART_MENU


// SSID variables
extern char*  access_point_ssid;

// Control variables
extern u8  default_unicast_rate;
extern s8  default_tx_power;
extern int association_state;                      // Section 10.3 of 802.11-2012
extern u8  uart_mode;
extern u8  active_scan;
static u32 schedule_ID;
static u8 print_scheduled = 0;

// Access point information
extern ap_info* ap_list;
extern u8       num_ap_list;

extern u8       access_point_num_basic_rates;
extern u8       access_point_basic_rates[NUM_BASIC_RATES_MAX];

extern u8 pause_queue;


// Association Table variables
extern dl_list		  association_table;
extern dl_list 		  statistics_table;
extern u8			  ap_addr[6];

// AP channel
extern u32 mac_param_chan;
extern u32 mac_param_chan_save;

// LTG variables
static u8 curr_traffic_type;
#define TRAFFIC_TYPE_PERIODIC_FIXED		1
#define TRAFFIC_TYPE_PERIODIC_RAND		2
#define TRAFFIC_TYPE_RAND_FIXED			3
#define TRAFFIC_TYPE_RAND_RAND			4

u32 num_slots = SLOT_CONFIG_RAND;

void uart_rx(u8 rxByte){

	station_info* access_point = ((station_info*)(association_table.first));

	#define MAX_NUM_AP_CHARS 4
	static char numerical_entry[MAX_NUM_AP_CHARS+1];
	static u8 curr_decade = 0;

	#define MAX_NUM_CHARS 31
	static char text_entry[MAX_NUM_CHARS+1];
	static u8 curr_char = 0;

	u16 ap_sel;
	wlan_ipc_msg ipc_msg_to_low;
	u32 ipc_msg_to_low_payload[1];
	ipc_config_rf_ifc* config_rf_ifc;

	void* ltg_sched_state;
	u32 ltg_type;

	void* ltg_callback_arg;
	ltg_sched_periodic_params periodic_params;
	ltg_sched_uniform_rand_params rand_params;

	if(rxByte == ASCII_ESC){
		uart_mode = UART_MODE_MAIN;

		stop_active_scan();

		if(print_scheduled){
			wlan_mac_remove_schedule(SCHEDULE_COARSE, schedule_ID);
		}
		print_menu();

		ltg_sched_remove(LTG_REMOVE_ALL);

		return;
	}

	switch(uart_mode){
		case UART_MODE_MAIN:
			switch(rxByte){
				case ASCII_1:
					uart_mode = UART_MODE_INTERACTIVE;
					print_station_status(1);
				break;

				case ASCII_2:
					print_all_observed_statistics();
				break;

				case ASCII_a:
					//Send bcast probe requests across all channels
					if(active_scan == 0){
						num_ap_list = 0;
						//xil_printf("- Free 0x%08x\n",ap_list);
						wlan_mac_high_free(ap_list);
						ap_list = NULL;
						active_scan = 1;
						access_point_ssid = wlan_mac_high_realloc(access_point_ssid, 1);
						*access_point_ssid = 0;
						//xil_printf("+++ starting active scan\n");
						pause_queue = 1;
						mac_param_chan_save = mac_param_chan;
						probe_req_transmit();
					}
				break;

				case ASCII_r:
					if(default_unicast_rate > WLAN_MAC_RATE_6M){
						default_unicast_rate--;
					} else {
						default_unicast_rate = WLAN_MAC_RATE_6M;
					}

					if(association_table.length > 0) access_point->tx.phy.rate = default_unicast_rate;


					xil_printf("(-) Default Unicast Rate: %d Mbps\n", wlan_lib_mac_rate_to_mbps(default_unicast_rate));
				break;
				case ASCII_R:
					if(default_unicast_rate < WLAN_MAC_RATE_54M){
						default_unicast_rate++;
					} else {
						default_unicast_rate = WLAN_MAC_RATE_54M;
					}

					if(association_table.length > 0) access_point->tx.phy.rate = default_unicast_rate;

					xil_printf("(+) Default Unicast Rate: %d Mbps\n", wlan_lib_mac_rate_to_mbps(default_unicast_rate));
				break;
				case ASCII_g:
					if(default_tx_power > TX_POWER_MIN_DBM){
						default_tx_power--;
					} else {
						default_tx_power = TX_POWER_MIN_DBM;
					}

					xil_printf("(-) Default Tx Power: %d dBm\n", default_tx_power);

				break;
				case ASCII_G:
					if(default_tx_power < TX_POWER_MAX_DBM){
						default_tx_power++;
					} else {
						default_tx_power = TX_POWER_MAX_DBM;
					}

					xil_printf("(+) Default Tx Power: %d dBm\n", default_tx_power);

				break;
			}
		break;
		case UART_MODE_INTERACTIVE:
			switch(rxByte){
				case ASCII_r:
					//Reset statistics
					reset_station_statistics();
				break;
				case ASCII_1:
					if(access_point != NULL){
						uart_mode = UART_MODE_LTG_SIZE_CHANGE;
						curr_char = 0;
						curr_traffic_type = TRAFFIC_TYPE_PERIODIC_FIXED;

						if(ltg_sched_get_state(0,&ltg_type,&ltg_sched_state) == 0){
							//A scheduler of ID = 0 has been previously configured
							if(((ltg_sched_state_hdr*)ltg_sched_state)->enabled == 1){
								//This LTG is currently running. We'll turn it off.
								ltg_sched_stop(0);
								uart_mode = UART_MODE_INTERACTIVE;
								print_station_status(1);
								return;
							}
						}
						xil_printf("\n\n Configuring Local Traffic Generator (LTG) for AP\n");
						xil_printf("\nEnter packet payload size (in bytes): ");
					}
				break;
				case ASCII_Q:
					if(access_point != NULL){
						uart_mode = UART_MODE_LTG_SIZE_CHANGE;
						curr_char = 0;
						curr_traffic_type = TRAFFIC_TYPE_RAND_RAND;

						if(ltg_sched_get_state(0,&ltg_type,&ltg_sched_state) == 0){
							//A scheduler of ID = 0 has been previously configured
							if(((ltg_sched_state_hdr*)ltg_sched_state)->enabled == 1){
								//This LTG is currently running. We'll turn it off.
								ltg_sched_stop(0);
								uart_mode = UART_MODE_INTERACTIVE;
								print_station_status(1);
								return;
							}
						}
						xil_printf("\n\n Configuring Random Local Traffic Generator (LTG) for AP\n");
						xil_printf("\nEnter maximum payload size (in bytes): ");
					}
				break;
			}
		break;
		case UART_MODE_LTG_SIZE_CHANGE:
			switch(rxByte){
				case ASCII_CR:
					text_entry[curr_char] = 0;
					curr_char = 0;

					if(ltg_sched_get_callback_arg(0,&ltg_callback_arg) == 0){
						//This LTG has already been configured. We need to free the old callback argument so we can create a new one.
						ltg_sched_stop(0);
						wlan_mac_high_free(ltg_callback_arg);
					}
					switch(curr_traffic_type){
							case TRAFFIC_TYPE_PERIODIC_FIXED:
								ltg_callback_arg = wlan_mac_high_malloc(sizeof(ltg_pyld_fixed));
								if(ltg_callback_arg != NULL){
									((ltg_pyld_fixed*)ltg_callback_arg)->hdr.type = LTG_PYLD_TYPE_FIXED;
									((ltg_pyld_fixed*)ltg_callback_arg)->length = str2num(text_entry);
									memcpy(((ltg_pyld_hdr*)ltg_callback_arg)->addr_da, access_point->addr, 6);

									//Note: This call to configure is incomplete. At this stage in the uart menu, the periodic_params argument hasn't been updated. This is
									//simply an artifact of the sequential nature of UART entry. We won't start the scheduler until we call configure again with that updated
									//entry.
									ltg_sched_configure(0, LTG_SCHED_TYPE_PERIODIC, &periodic_params, ltg_callback_arg, &ltg_cleanup);

									uart_mode = UART_MODE_LTG_INTERVAL_CHANGE;
									xil_printf("\nEnter packet Tx interval (in microseconds): ");
								} else {
									xil_printf("Error allocating memory for ltg_callback_arg\n");
									uart_mode = UART_MODE_INTERACTIVE;
									print_station_status(1);
								}

							break;
							case TRAFFIC_TYPE_RAND_RAND:
								ltg_callback_arg = wlan_mac_high_malloc(sizeof(ltg_pyld_uniform_rand));
								if(ltg_callback_arg != NULL){
									((ltg_pyld_uniform_rand*)ltg_callback_arg)->hdr.type = LTG_PYLD_TYPE_UNIFORM_RAND;
									((ltg_pyld_uniform_rand*)ltg_callback_arg)->min_length = 0;
									((ltg_pyld_uniform_rand*)ltg_callback_arg)->max_length = str2num(text_entry);
									memcpy(((ltg_pyld_hdr*)ltg_callback_arg)->addr_da, access_point->addr, 6);

									//Note: This call to configure is incomplete. At this stage in the uart menu, the periodic_params argument hasn't been updated. This is
									//simply an artifact of the sequential nature of UART entry. We won't start the scheduler until we call configure again with that updated
									//entry.
									ltg_sched_configure(0, LTG_SCHED_TYPE_UNIFORM_RAND, &rand_params, ltg_callback_arg, &ltg_cleanup);

									uart_mode = UART_MODE_LTG_INTERVAL_CHANGE;
									xil_printf("\nEnter maximum packet Tx interval (in microseconds): ");
								} else {
									xil_printf("Error allocating memory for ltg_callback_arg\n");
									uart_mode = UART_MODE_INTERACTIVE;
									print_station_status(1);
								}

							break;
						}

				break;
				case ASCII_DEL:
					if(curr_char > 0){
						curr_char--;
						xil_printf("\b \b");
					}
				break;
				default:
					if( (rxByte <= ASCII_9) && (rxByte >= ASCII_0) ){
						//the user entered a character
						if(curr_char < MAX_NUM_CHARS){
							xil_printf("%c", rxByte);
							text_entry[curr_char] = rxByte;
							curr_char++;
						}
					}
				break;
			}
		break;
		case UART_MODE_LTG_INTERVAL_CHANGE:
			switch(rxByte){
				case ASCII_CR:
					text_entry[curr_char] = 0;
					curr_char = 0;

					if(ltg_sched_get_callback_arg(0,&ltg_callback_arg) != 0){
						xil_printf("Error: expected to find an already configured LTG ID %d\n", 0);
						return;
					}

					switch(curr_traffic_type){
						case TRAFFIC_TYPE_PERIODIC_FIXED:
							if(ltg_callback_arg != NULL){
								periodic_params.interval_usec = str2num(text_entry);
								ltg_sched_configure(0, LTG_SCHED_TYPE_PERIODIC, &periodic_params, ltg_callback_arg, &ltg_cleanup);
								ltg_sched_start(0);
							} else {
								xil_printf("Error: ltg_callback_arg was NULL\n");
							}

						break;
						case TRAFFIC_TYPE_RAND_RAND:
							if(ltg_callback_arg != NULL){
								rand_params.min_interval = 0;
								rand_params.max_interval = str2num(text_entry);
								ltg_sched_configure(0, LTG_SCHED_TYPE_UNIFORM_RAND, &rand_params, ltg_callback_arg, &ltg_cleanup);
								ltg_sched_start(0);
							} else {
								xil_printf("Error: ltg_callback_arg was NULL\n");
							}

						break;
					}
					uart_mode = UART_MODE_INTERACTIVE;
					print_station_status(1);

				break;
				case ASCII_DEL:
					if(curr_char > 0){
						curr_char--;
						xil_printf("\b \b");
					}
				break;
				default:
					if( (rxByte <= ASCII_9) && (rxByte >= ASCII_0) ){
						//the user entered a character
						if(curr_char < MAX_NUM_CHARS){
							xil_printf("%c", rxByte);
							text_entry[curr_char] = rxByte;
							curr_char++;
						}
					}
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
							memcpy(ap_addr, ap_list[ap_sel].bssid, 6);

							access_point_ssid = wlan_mac_high_realloc(access_point_ssid, strlen(ap_list[ap_sel].ssid)+1);
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

void print_ltg_interval_menu(){

	xil_printf("\nEnter packet Tx interval (in microseconds): ");

}


void print_menu(){
	xil_printf("\f");
	xil_printf("********************** Station Menu **********************\n");
	xil_printf("[1] - Interactive Station Status\n");
	xil_printf("[2] - Print all Observed Statistics\n");
	xil_printf("\n");
	xil_printf("[a] - 	active scan and display nearby APs\n");
	xil_printf("[r/R] - change default unicast rate\n");
}

void print_station_status(u8 manual_call){

	u64 timestamp;
	void* ltg_sched_state;
	void* ltg_sched_parameters;
	void* ltg_pyld_callback_arg;
	dl_entry* access_point_entry = association_table.first;
	station_info* access_point = ((station_info*)(access_point_entry->data));

	u32 ltg_type;

	if(uart_mode == UART_MODE_INTERACTIVE){
		timestamp = get_usec_timestamp();
		xil_printf("\f");
		xil_printf("---------------------------------------------------\n");

			if(association_table.length > 0){
				xil_printf(" AID: %02x -- MAC Addr: %02x:%02x:%02x:%02x:%02x:%02x\n", access_point->AID,
							access_point->addr[0],access_point->addr[1],access_point->addr[2],access_point->addr[3],access_point->addr[4],access_point->addr[5]);
				if(ltg_sched_get_state(0,&ltg_type,&ltg_sched_state) == 0){

					ltg_sched_get_params(0, &ltg_type, &ltg_sched_parameters);
					ltg_sched_get_callback_arg(0,&ltg_pyld_callback_arg);

					if(((ltg_sched_state_hdr*)ltg_sched_state)->enabled == 1){
						switch(ltg_type){
							case LTG_SCHED_TYPE_PERIODIC:
								xil_printf("  Periodic LTG Schedule Enabled\n");
								xil_printf("  Packet Tx Interval: %d microseconds\n", ((ltg_sched_periodic_params*)(ltg_sched_parameters))->interval_usec);
							break;
							case LTG_SCHED_TYPE_UNIFORM_RAND:
								xil_printf("  Uniform Random LTG Schedule Enabled\n");
								xil_printf("  Packet Tx Interval: Uniform over range of [%d,%d] microseconds\n", ((ltg_sched_uniform_rand_params*)(ltg_sched_parameters))->min_interval,((ltg_sched_uniform_rand_params*)(ltg_sched_parameters))->max_interval);
							break;
						}

						switch(((ltg_pyld_hdr*)(ltg_sched_state))->type){
							case LTG_PYLD_TYPE_FIXED:
								xil_printf("  Fixed Packet Length: %d bytes\n", ((ltg_pyld_fixed*)(ltg_pyld_callback_arg))->length);
							break;
							case LTG_PYLD_TYPE_UNIFORM_RAND:
								xil_printf("  Random Packet Length: Uniform over [%d,%d] bytes\n", ((ltg_pyld_uniform_rand*)(ltg_pyld_callback_arg))->min_length,((ltg_pyld_uniform_rand*)(ltg_pyld_callback_arg))->max_length);
							break;
						}

					}
				}
				xil_printf("     - Last heard from    %d ms ago\n",((u32)(timestamp - (access_point->rx.last_timestamp)))/1000);
				xil_printf("     - Last Rx Power:     %d dBm\n",access_point->rx.last_power);
				xil_printf("     - # of queued MPDUs: %d\n", queue_num_queued(UNICAST_QID));
				xil_printf("     - # Tx High MPDUs:   %d (%d successful)\n", access_point->stats->num_high_tx_total, access_point->stats->num_high_tx_success);
				xil_printf("     - # Tx Low:          %d\n", access_point->stats->num_low_tx);
				xil_printf("     - # DATA: Rx MPDUs:  %d (%d bytes)\n", access_point->stats->data_num_rx_success, access_point->stats->data_num_rx_bytes);
				xil_printf("     - # MGMT: Rx MPDUs:  %d (%d bytes)\n", access_point->stats->mgmt_num_rx_success, access_point->stats->mgmt_num_rx_bytes);
			}
		xil_printf("---------------------------------------------------\n");
		xil_printf("\n");
		xil_printf("[r] - reset statistics\n\n");
		xil_printf(" The interactive STA menu supports sending arbitrary traffic\n");
		xil_printf(" to an associated AP. To use this feature, press the number 1\n");
		xil_printf(" Pressing Esc at any time will halt all local traffic\n");
		xil_printf(" generation and return you to the main menu.");

		//Update display
		schedule_ID = wlan_mac_schedule_event(SCHEDULE_COARSE, 1000000, (void*)print_station_status);

	}
}

void ltg_cleanup(u32 id, void* callback_arg){
	wlan_mac_high_free(callback_arg);
}

void print_all_observed_statistics(){
	u32 i;
	dl_entry*	curr_statistics_entry;
	statistics_txrx* curr_statistics;

	curr_statistics_entry = statistics_table.first;

	xil_printf("\nAll Statistics:\n");
	for(i=0; i<statistics_table.length; i++){
		curr_statistics = (statistics_txrx*)(curr_statistics_entry->data);
		xil_printf("---------------------------------------------------\n");
		xil_printf("%02x:%02x:%02x:%02x:%02x:%02x\n", curr_statistics->addr[0],curr_statistics->addr[1],curr_statistics->addr[2],curr_statistics->addr[3],curr_statistics->addr[4],curr_statistics->addr[5]);
		xil_printf("     - Last timestamp: %d usec\n", (u32)curr_statistics->last_timestamp);
		xil_printf("     - Associated?       %d\n", curr_statistics->is_associated);
		xil_printf("     - # Tx High MPDUs:  %d (%d successful)\n", curr_statistics->num_high_tx_total, curr_statistics->num_high_tx_success);
		xil_printf("     - # Tx Low:         %d\n", curr_statistics->num_low_tx);
		xil_printf("     - # DATA: Rx MPDUs: %d (%d bytes)\n", curr_statistics->data_num_rx_success, curr_statistics->data_num_rx_bytes);
		xil_printf("     - # MGMT: Rx MPDUs: %d (%d bytes)\n", curr_statistics->mgmt_num_rx_success, curr_statistics->mgmt_num_rx_bytes);
		curr_statistics_entry = dl_entry_next(curr_statistics_entry);
	}
}




#endif


