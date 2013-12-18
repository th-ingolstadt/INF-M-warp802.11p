////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_ap_uart_menu.c
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
#include "wlan_mac_ipc.h"
#include "wlan_mac_misc_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_events.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_ap.h"
#include "ascii_characters.h"
#include "wlan_mac_schedule.h"


#ifdef WLAN_USE_UART_MENU

static u8 uart_mode = UART_MODE_MAIN;
extern u8 default_unicast_rate;
extern u32 mac_param_chan;

extern dl_list association_table;
extern dl_list statistics_table;

extern char* access_point_ssid;
static u8 curr_aid;
static u8 curr_traffic_type;
static u32 cpu_high_status;
static u32 schedule_ID;
static u8 print_scheduled = 0;

#define TRAFFIC_TYPE_PERIODIC_FIXED		1
#define TRAFFIC_TYPE_PERIODIC_RAND		2
#define TRAFFIC_TYPE_RAND_FIXED			3
#define TRAFFIC_TYPE_RAND_RAND			4

u32 num_slots = SLOT_CONFIG_RAND;

void uart_rx(u8 rxByte){
	void* ltg_sched_state;
	u32 ltg_type;
	u32 i;
	station_info* curr_station_info;

	void* ltg_callback_arg;
	ltg_sched_periodic_params periodic_params;
	ltg_sched_uniform_rand_params rand_params;

	#define MAX_NUM_CHARS 31
	static char text_entry[MAX_NUM_CHARS+1];
	static u8 curr_char = 0;
    
	if(rxByte == ASCII_ESC){
		uart_mode = UART_MODE_MAIN;
		stop_periodic_print();
		print_menu();
		ltg_sched_remove(LTG_REMOVE_ALL);
		return;
	}

	switch(uart_mode){
		case UART_MODE_MAIN:
			switch(rxByte){
				case ASCII_1:
					uart_mode = UART_MODE_INTERACTIVE;
					start_periodic_print();
				break;

				case ASCII_2:
					print_queue_status();
				break;

				case ASCII_3:
					print_all_observed_statistics();
				break;

				case ASCII_e:
					print_event_log( 0xFFFF );
					print_event_log_size();
				break;

				case ASCII_c:
					if(mac_param_chan > 1){
						deauthenticate_stations();
						(mac_param_chan--);

						//Send a message to other processor to tell it to switch channels
						set_mac_channel( mac_param_chan );
					} else {

					}
					xil_printf("(-) Channel: %d\n", mac_param_chan);


				break;
				case ASCII_C:
					if(mac_param_chan < 11){
						deauthenticate_stations();
						(mac_param_chan++);

						//Send a message to other processor to tell it to switch channels
						set_mac_channel( mac_param_chan );
					} else {

					}
					xil_printf("(+) Channel: %d\n", mac_param_chan);

				break;
				case ASCII_r:
					if(default_unicast_rate > WLAN_MAC_RATE_6M){
						default_unicast_rate--;
					} else {
						default_unicast_rate = WLAN_MAC_RATE_6M;
					}

					curr_station_info = (station_info*)(association_table.first);
					for(i=0; i < association_table.length; i++){
						curr_station_info->tx.rate = default_unicast_rate;
						curr_station_info = (station_info*)((curr_station_info->node).next);
					}

					xil_printf("(-) Default Unicast Rate: %d Mbps\n", wlan_lib_mac_rate_to_mbps(default_unicast_rate));
				break;
				case ASCII_R:
					if(default_unicast_rate < WLAN_MAC_RATE_54M){
						default_unicast_rate++;
					} else {
						default_unicast_rate = WLAN_MAC_RATE_54M;
					}

					curr_station_info = (station_info*)(association_table.first);
					for(i=0; i < association_table.length; i++){
						curr_station_info->tx.rate = default_unicast_rate;
						curr_station_info = (station_info*)((curr_station_info->node).next);
					}
					xil_printf("(+) Default Unicast Rate: %d Mbps\n", wlan_lib_mac_rate_to_mbps(default_unicast_rate));
				break;
				case ASCII_s:
					uart_mode = UART_MODE_SSID_CHANGE;
					deauthenticate_stations();
					curr_char = 0;
					print_ssid_menu();
				break;
				case ASCII_h:
					xil_printf("cpu_high_status = 0x%08x\n", cpu_high_status);
				break;

				case ASCII_n:
					if(num_slots == 0 || num_slots == SLOT_CONFIG_RAND){
						num_slots = SLOT_CONFIG_RAND;
						xil_printf("num_slots = SLOT_CONFIG_RAND\n");
					} else {
						num_slots--;
						xil_printf("num_slots = %d\n", num_slots);
					}


					set_backoff_slot_value(num_slots);
				break;

				case ASCII_N:
					if(num_slots == SLOT_CONFIG_RAND){
						num_slots = 0;
					} else {
						num_slots++;
					}

					xil_printf("num_slots = %d\n", num_slots);

					set_backoff_slot_value(num_slots);
				break;
				case ASCII_m:
					wlan_display_mallinfo();
				break;
			}
		break;

		case UART_MODE_INTERACTIVE:
			switch(rxByte){
				case ASCII_r:
					//Reset statistics
					reset_station_statistics();
				break;
				case ASCII_d:
					//Deauthenticate all stations
					deauthenticate_stations();
				break;
				default:
					if( (rxByte <= ASCII_9) && (rxByte >= ASCII_0) ){
						//This if range covers the numbers [1,...,9,0] on a keyboard.

						curr_aid = rxByte - 48;
						curr_traffic_type = TRAFFIC_TYPE_PERIODIC_FIXED;

						curr_station_info = wlan_mac_high_find_station_info_AID(&association_table, curr_aid);

						if(curr_station_info != NULL){
							uart_mode = UART_MODE_LTG_SIZE_CHANGE;
							curr_char = 0;

							if(ltg_sched_get_state(AID_TO_LTG_ID(curr_aid),&ltg_type,&ltg_sched_state) == 0){
								//A scheduler of ID = AID_TO_LTG_ID(curr_aid) has been previously configured
								if(((ltg_sched_state_hdr*)ltg_sched_state)->enabled == 1){
									//This LTG is currently running. We'll turn it off.
									ltg_sched_stop(AID_TO_LTG_ID(curr_aid));
									uart_mode = UART_MODE_INTERACTIVE;

									start_periodic_print();
									return;
								}
							}
							xil_printf("\n\n Configuring Local Traffic Generator (LTG) for AID %d\n", curr_aid);
							xil_printf("\nEnter packet payload size (in bytes): ");
						}

					}
					if( is_qwerty_row(rxByte) ){
						//This if range covers the range [q,p] on a keyboard (the letters directly below [1,...,9,0])
						//This is a "hidden" UART feature to show how other kinds of LTG traffic sources can be set up.
						//Specifically, this will set up a random interval, random payload length traffic source.

						curr_aid = qwerty_row_to_number(rxByte);
						curr_traffic_type = TRAFFIC_TYPE_RAND_RAND;

						curr_station_info = wlan_mac_high_find_station_info_AID(&association_table, curr_aid);

						if(curr_station_info != NULL){
							uart_mode = UART_MODE_LTG_SIZE_CHANGE;
							curr_char = 0;

							if(ltg_sched_get_state(AID_TO_LTG_ID(curr_aid),&ltg_type,&ltg_sched_state) == 0){
								//A scheduler of ID = AID_TO_LTG_ID(curr_aid) has been previously configured
								if(((ltg_sched_state_hdr*)ltg_sched_state)->enabled == 1){
									//This LTG is currently running. We'll turn it off.
									ltg_sched_stop(AID_TO_LTG_ID(curr_aid));
									uart_mode = UART_MODE_INTERACTIVE;

									start_periodic_print();
									return;
								}
							}
							xil_printf("\n\n Configuring Random Local Traffic Generator (LTG) for AID %d\n", curr_aid);
							xil_printf("\nEnter maximum payload size (in bytes): ");
						}

					}

				break;
			}
		break;

		case UART_MODE_LTG_SIZE_CHANGE:
			switch(rxByte){
				case ASCII_CR:
					text_entry[curr_char] = 0;
					curr_char = 0;

					if(ltg_sched_get_callback_arg(AID_TO_LTG_ID(curr_aid),&ltg_callback_arg) == 0){
						//This LTG has already been configured. We need to free the old callback argument so we can create a new one.
						ltg_sched_stop(AID_TO_LTG_ID(curr_aid));
						wlan_free(ltg_callback_arg);
					}
					switch(curr_traffic_type){
						case TRAFFIC_TYPE_PERIODIC_FIXED:
							ltg_callback_arg = wlan_malloc(sizeof(ltg_pyld_fixed));
							if(ltg_callback_arg != NULL){
								((ltg_pyld_fixed*)ltg_callback_arg)->hdr.type = LTG_PYLD_TYPE_FIXED;
								((ltg_pyld_fixed*)ltg_callback_arg)->length = str2num(text_entry);

								//Note: This call to configure is incomplete. At this stage in the uart menu, the periodic_params argument hasn't been updated. This is
								//simply an artifact of the sequential nature of UART entry. We won't start the scheduler until we call configure again with that updated
								//entry.
								ltg_sched_configure(AID_TO_LTG_ID(curr_aid), LTG_SCHED_TYPE_PERIODIC, &periodic_params, ltg_callback_arg, &ltg_cleanup);

								uart_mode = UART_MODE_LTG_INTERVAL_CHANGE;
								xil_printf("\nEnter packet Tx interval (in microseconds): ");
							} else {
								xil_printf("Error allocating memory for ltg_callback_arg\n");
								uart_mode = UART_MODE_INTERACTIVE;
								start_periodic_print();
							}

						break;
						case TRAFFIC_TYPE_RAND_RAND:
							ltg_callback_arg = wlan_malloc(sizeof(ltg_pyld_uniform_rand));
							if(ltg_callback_arg != NULL){
								((ltg_pyld_uniform_rand*)ltg_callback_arg)->hdr.type = LTG_PYLD_TYPE_UNIFORM_RAND;
								((ltg_pyld_uniform_rand*)ltg_callback_arg)->min_length = 0;
								((ltg_pyld_uniform_rand*)ltg_callback_arg)->max_length = str2num(text_entry);

								//Note: This call to configure is incomplete. At this stage in the uart menu, the periodic_params argument hasn't been updated. This is
								//simply an artifact of the sequential nature of UART entry. We won't start the scheduler until we call configure again with that updated
								//entry.
								ltg_sched_configure(AID_TO_LTG_ID(curr_aid), LTG_SCHED_TYPE_UNIFORM_RAND, &rand_params, ltg_callback_arg, &ltg_cleanup);

								uart_mode = UART_MODE_LTG_INTERVAL_CHANGE;
								xil_printf("\nEnter maximum packet Tx interval (in microseconds): ");
							} else {
								xil_printf("Error allocating memory for ltg_callback_arg\n");
								uart_mode = UART_MODE_INTERACTIVE;
								start_periodic_print();
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

						if(ltg_sched_get_callback_arg(AID_TO_LTG_ID(curr_aid),&ltg_callback_arg) != 0){
							xil_printf("Error: expected to find an already configured LTG ID %d\n", AID_TO_LTG_ID(curr_aid));
							return;
						}
						switch(curr_traffic_type){
							case TRAFFIC_TYPE_PERIODIC_FIXED:
								if(ltg_callback_arg != NULL){
									periodic_params.interval_usec = str2num(text_entry);
									ltg_sched_configure(AID_TO_LTG_ID(curr_aid), LTG_SCHED_TYPE_PERIODIC, &periodic_params, ltg_callback_arg, &ltg_cleanup);
									ltg_sched_start(AID_TO_LTG_ID(curr_aid));
								} else {
									xil_printf("Error: ltg_callback_arg was NULL\n");
								}

							break;
							case TRAFFIC_TYPE_RAND_RAND:
								if(ltg_callback_arg != NULL){
									rand_params.min_interval = 0;
									rand_params.max_interval = str2num(text_entry);
									ltg_sched_configure(AID_TO_LTG_ID(curr_aid), LTG_SCHED_TYPE_UNIFORM_RAND, &rand_params, ltg_callback_arg, &ltg_cleanup);
									ltg_sched_start(AID_TO_LTG_ID(curr_aid));
								} else {
									xil_printf("Error: ltg_callback_arg was NULL\n");
								}

							break;
						}

						uart_mode = UART_MODE_INTERACTIVE;
						start_periodic_print();

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

		case UART_MODE_SSID_CHANGE:
			switch(rxByte){
				case ASCII_CR:


					text_entry[curr_char] = 0;
					curr_char = 0;
					uart_mode = UART_MODE_MAIN;

					access_point_ssid = wlan_realloc(access_point_ssid, strlen(text_entry)+1);
					strcpy(access_point_ssid,text_entry);
					xil_printf("\nSetting new SSID: %s\n", access_point_ssid);

				break;
				case ASCII_DEL:
					if(curr_char > 0){
						curr_char--;
						xil_printf("\b \b");
					}

				break;
				default:
					if( (rxByte <= ASCII_z) && (rxByte >= ASCII_A) ){
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
	}
}

void print_ssid_menu(){
	xil_printf("\f");
	xil_printf("Current SSID: %s\n", access_point_ssid);
	xil_printf("To change the current SSID, please type a new string and press enter\n");
	xil_printf(": ");
}


void print_queue_status(){
	u32 i;
	station_info* curr_station_info;
	xil_printf("\nQueue Status:\n");
	xil_printf(" FREE || BCAST|");

	curr_station_info = (station_info*)association_table.first;
	for(i=0; i < association_table.length; i++){
		xil_printf("%6d|", curr_station_info->AID);
		curr_station_info = (station_info*)((curr_station_info->node).next);
	}
	xil_printf("\n");

	xil_printf("%6d||%6d|",queue_num_free(),queue_num_queued(0));

	curr_station_info = (station_info*)association_table.first;
	for(i=0; i < association_table.length; i++){
		xil_printf("%6d|", queue_num_queued(curr_station_info->AID));
		curr_station_info = (station_info*)((curr_station_info->node).next);
	}
	xil_printf("\n");

}

void print_menu(){
	xil_printf("\f");
	xil_printf("********************** AP Menu **********************\n");
	xil_printf("[1] - Interactive AP Status\n");
	xil_printf("[2] - Print Queue Status\n");
	xil_printf("[3] - Print all Observed Statistics\n");
	xil_printf("\n");
	xil_printf("[c/C] - change channel (note: changing channel will\n");
	xil_printf("        purge any associations, forcing stations to\n");
	xil_printf("        join the network again)\n");
	xil_printf("[r/R] - change default unicast rate\n");
	xil_printf("[s]   - change SSID (note: changing SSID will purge)\n");
	xil_printf("        any associations)\n");
	xil_printf("*****************************************************\n");
}





void print_station_status(){
	u32 i;
	station_info* curr_station_info;
	u64 timestamp;
	void* ltg_sched_state;
	void* ltg_sched_parameters;
	void* ltg_pyld_callback_arg;

	u32 ltg_type;

	if(uart_mode == UART_MODE_INTERACTIVE){
		timestamp = get_usec_timestamp();
		xil_printf("\f");
		//xil_printf("next_free_assoc_index = %d\n", next_free_assoc_index);

		curr_station_info = (station_info*)association_table.first;
		for(i=0; i < association_table.length; i++){
			xil_printf("---------------------------------------------------\n");
			xil_printf(" AID: %02x -- MAC Addr: %02x:%02x:%02x:%02x:%02x:%02x\n", curr_station_info->AID,
					curr_station_info->addr[0],curr_station_info->addr[1],curr_station_info->addr[2],curr_station_info->addr[3],curr_station_info->addr[4],curr_station_info->addr[5]);

			if(ltg_sched_get_state(AID_TO_LTG_ID(curr_station_info->AID),&ltg_type,&ltg_sched_state) == 0){

				ltg_sched_get_params(AID_TO_LTG_ID(curr_station_info->AID), &ltg_type, &ltg_sched_parameters);
				ltg_sched_get_callback_arg(AID_TO_LTG_ID(curr_station_info->AID),&ltg_pyld_callback_arg);

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
							xil_printf("  Fixed Packet Length: %d bytes\n", ((ltg_pyld_fixed_length*)(ltg_pyld_callback_arg))->length);
						break;
						case LTG_PYLD_TYPE_UNIFORM_RAND:
							xil_printf("  Random Packet Length: Uniform over [%d,%d] bytes\n", ((ltg_pyld_uniform_rand*)(ltg_pyld_callback_arg))->min_length,((ltg_pyld_uniform_rand*)(ltg_pyld_callback_arg))->max_length);
						break;
					}

				}
			}

			xil_printf("     - Last heard from %d ms ago\n",((u32)(timestamp - (curr_station_info->rx.last_timestamp)))/1000);
			xil_printf("     - Last Rx Power: %d dBm\n",curr_station_info->rx.last_power);
			xil_printf("     - # of queued MPDUs: %d\n", queue_num_queued(curr_station_info->AID));
			xil_printf("     - # Tx MPDUs: %d (%d successful)\n", curr_station_info->stats->num_tx_total, curr_station_info->stats->num_tx_success);
			xil_printf("     - # Tx Retry: %d\n", curr_station_info->stats->num_retry);
			xil_printf("     - # Rx MPDUs: %d (%d bytes)\n", curr_station_info->stats->num_rx_success, curr_station_info->stats->num_rx_bytes);

			curr_station_info = (station_info*)((curr_station_info->node).next);

		}
			xil_printf("---------------------------------------------------\n");
			xil_printf("\n");
			xil_printf("[r] - reset statistics\n");
			xil_printf("[d] - deauthenticate all stations\n\n");
			xil_printf(" The interactive AP menu supports sending arbitrary traffic\n");
			xil_printf(" to any associated station. To use this feature, press any number\n");
			xil_printf(" on the keyboard that corresponds to an associated station's AID\n");
			xil_printf(" and follow the prompts. Pressing Esc at any time will halt all\n");
			xil_printf(" local traffic generation and return you to the main menu.");
	}


}

void print_all_observed_statistics(){
	u32 i;
	statistics* curr_statistics;

	curr_statistics = (statistics*)(statistics_table.first);
	xil_printf("\nAll Statistics:\n");
	for(i=0; i<statistics_table.length; i++){
		xil_printf("---------------------------------------------------\n");
		xil_printf("%02x:%02x:%02x:%02x:%02x:%02x\n", curr_statistics->addr[0],curr_statistics->addr[1],curr_statistics->addr[2],curr_statistics->addr[3],curr_statistics->addr[4],curr_statistics->addr[5]);
		xil_printf("     - # Tx MPDUs: %d (%d successful)\n", curr_statistics->num_tx_total, curr_statistics->num_tx_success);
		xil_printf("     - # Tx Retry: %d\n", curr_statistics->num_retry);
		xil_printf("     - # Rx MPDUs: %d (%d bytes)\n", curr_statistics->num_rx_success, curr_statistics->num_rx_bytes);
		curr_statistics = statistics_next(curr_statistics);
	}


}

void start_periodic_print(){
	stop_periodic_print();
	print_station_status();
	print_scheduled = 1;
	schedule_ID = wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, 1000000, SCHEDULE_REPEAT_FOREVER, (void*)print_station_status);
}

void stop_periodic_print(){
	if(print_scheduled){
		print_scheduled = 0;
		wlan_mac_remove_schedule(SCHEDULE_COARSE, schedule_ID);
	}
}


void ltg_cleanup(u32 id, void* callback_arg){
	wlan_free(callback_arg);
}

int is_qwerty_row(u8 rxByte){
	int return_value = 0;

	switch(rxByte){
		case ASCII_Q:
		case ASCII_W:
		case ASCII_E:
		case ASCII_R:
		case ASCII_T:
		case ASCII_Y:
		case ASCII_U:
		case ASCII_I:
		case ASCII_O:
		case ASCII_P:
			return_value = 1;
		break;
	}

	return return_value;
}

int qwerty_row_to_number(u8 rxByte){
	int return_value = -1;

	switch(rxByte){
		case ASCII_Q:
			return_value = 1;
		break;
		case ASCII_W:
			return_value = 2;
		break;
		case ASCII_E:
			return_value = 3;
		break;
		case ASCII_R:
			return_value = 4;
		break;
		case ASCII_T:
			return_value = 5;
		break;
		case ASCII_Y:
			return_value = 6;
		break;
		case ASCII_U:
			return_value = 7;
		break;
		case ASCII_I:
			return_value = 8;
		break;
		case ASCII_O:
			return_value = 9;
		break;
		case ASCII_P:
			return_value = 0;
		break;
	}

	return return_value;
}

#endif


