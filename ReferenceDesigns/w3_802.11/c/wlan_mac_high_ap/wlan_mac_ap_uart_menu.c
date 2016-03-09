/** @file wlan_mac_ap_uart_menu.c
 *  @brief Access Point UART Menu
 *
 *  This contains code for the 802.11 Access Point's UART menu.
 *
 *  @copyright Copyright 2013-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

/***************************** Include Files *********************************/

// Xilinx SDK includes
#include "xparameters.h"
#include "stdio.h"
#include "stdlib.h"
#include "xtmrctr.h"
#include "xio.h"
#include "string.h"
#include "xintc.h"

// WLAN includes
#include "wlan_mac_time_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_entries.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_ap.h"
#include "ascii_characters.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_bss_info.h"


//
// Use the UART Menu
//     - If WLAN_USE_UART_MENU in wlan_mac_ap.h is commented out, then this function
//       will do nothing.  This might be necessary to save code space.
//


#ifndef WLAN_USE_UART_MENU

void uart_rx(u8 rxByte){ };

#else


/*************************** Constant Definitions ****************************/


/*********************** Global Variable Definitions *************************/

extern wlan_mac_low_config_t                cpu_low_config;
extern tx_params_t                          default_unicast_data_tx_params;

extern bss_info*                            my_bss_info;
extern dl_list                              counts_table;


/*************************** Variable Definitions ****************************/

static volatile u8                          uart_mode            = UART_MODE_MAIN;
static volatile u32                         schedule_ID;
static volatile u8                          print_scheduled      = 0;

static char                                 text_entry[SSID_LEN_MAX + 1];
static u8                                   curr_char            = 0;

static          ltg_pyld_all_assoc_fixed    traffic_blast_pyld;
static          ltg_sched_periodic_params   traffic_blast_sched;
static volatile u32                         traffic_blast_ltg_id = LTG_ID_INVALID;


/*************************** Functions Prototypes ****************************/

void print_main_menu();
void print_ssid_menu();

void print_station_status();
void print_queue_status();
void print_all_observed_counts();

void start_periodic_print();
void stop_periodic_print();


/*************************** Variable Definitions ****************************/


/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * Process each character received by the UART
 *
 * The following functionality is supported:
 *    - Main Menu
 *      - Interactive Menu (prints all station infos)
 *      - Print queue status
 *      - Print all counts
 *      - Print event log size (hidden)
 *      - Print Network List
 *      - Change channel
 *      - Change default TX power
 *      - Change TX MCS value (default unicast and all current associations)
 *      - Print Malloc info (hidden)
 *      - Change SSID
 *    - Interactive Menu
 *      - Reset counts
 *      - Deauthenticate all stations
 *      - Turn on/off "Traffic Blaster" (hidden)
 *
 * The escape key is used to return to the Main Menu.
 *
 *****************************************************************************/
void uart_rx(u8 rxByte){

	dl_entry*                   curr_station_info_entry;
	station_info*               curr_station_info;
	void*                       ltg_state;
	bss_config_t                bss_config;

	// ----------------------------------------------------
	// Return to the Main Menu
	//    - Stops any prints / LTGs
	if (rxByte == ASCII_ESC) {
		uart_mode = UART_MODE_MAIN;
		stop_periodic_print();
		print_main_menu();
		ltg_sched_remove(LTG_REMOVE_ALL);
		traffic_blast_ltg_id = LTG_ID_INVALID;
		return;
	}

	switch(uart_mode){

		// ------------------------------------------------
		// Main Menu processing
		//
		case UART_MODE_MAIN:
			switch(rxByte){

				// ----------------------------------------
				// '1' - Switch to Interactive Menu
				//
				case ASCII_1:
					uart_mode = UART_MODE_INTERACTIVE;
					start_periodic_print();
				break;

				// ----------------------------------------
				// '2' - Print Queue status
				//
				case ASCII_2:
					print_queue_status();
				break;

				// ----------------------------------------
				// '3' - Print Counts
				//
				case ASCII_3:
					print_all_observed_counts();
				break;

				// ----------------------------------------
				// 'e' - Print event log size
				//
				case ASCII_e:
					event_log_config_logging(EVENT_LOG_LOGGING_DISABLE);
					print_event_log_size();
					event_log_config_logging(EVENT_LOG_LOGGING_ENABLE);
				break;

				// ----------------------------------------
				// 'a' - Print BSS information
				//
				case ASCII_a:
					print_bss_info();
				break;

				// ----------------------------------------
				// 'c' - Channel Down (2.4 GHz only)
				//
				case ASCII_c:
					if (cpu_low_config.channel > 1) {
						deauthenticate_all_stations();
						(cpu_low_config.channel--);

						if(my_bss_info != NULL){
							my_bss_info->chan = cpu_low_config.channel;
						}

						// Send a message to other processor to tell it to switch channels
						wlan_mac_high_set_channel(cpu_low_config.channel);
					}

					xil_printf("(-) Channel: %d\n", cpu_low_config.channel);
				break;

				// ----------------------------------------
				// 'C' - Channel Up (2.4 GHz only)
				//
				case ASCII_C:
					if (cpu_low_config.channel < 11) {
						deauthenticate_all_stations();
						(cpu_low_config.channel++);

						if(my_bss_info != NULL){
							my_bss_info->chan = cpu_low_config.channel;
						}

						// Send a message to other processor to tell it to switch channels
						wlan_mac_high_set_channel(cpu_low_config.channel);
					}

					xil_printf("(+) Channel: %d\n", cpu_low_config.channel);
				break;

				// ----------------------------------------
				// 'g' - Decrease TX power
				//
				case ASCII_g:
					// Decrease the default unicast data TX parameters power
					//     - This is for any new association
					if ((default_unicast_data_tx_params.phy.power) > TX_POWER_MIN_DBM) {
						(default_unicast_data_tx_params.phy.power)--;
					} else {
						(default_unicast_data_tx_params.phy.power) = TX_POWER_MIN_DBM;
					}

					// Decrease the TX power for all associated stations
					curr_station_info_entry = my_bss_info->associated_stations.first;

					while (curr_station_info_entry != NULL) {
						curr_station_info = (station_info*)(curr_station_info_entry->data);
						curr_station_info->tx.phy.power = (default_unicast_data_tx_params.phy.power);
						curr_station_info_entry = dl_entry_next(curr_station_info_entry);
					}

					xil_printf("(-) Default Tx Power: %d dBm\n", (default_unicast_data_tx_params.phy.power));

				break;

				// ----------------------------------------
				// 'G' - Increase TX power
				//
				case ASCII_G:
					// Increase the default unicast data TX parameters power
					//     - This is for any new association
					if ((default_unicast_data_tx_params.phy.power) < TX_POWER_MAX_DBM) {
						(default_unicast_data_tx_params.phy.power)++;
					} else {
						(default_unicast_data_tx_params.phy.power) = TX_POWER_MAX_DBM;
					}

					// Increase the TX power for all associated stations
					curr_station_info_entry = my_bss_info->associated_stations.first;

					while (curr_station_info_entry != NULL) {
						curr_station_info = (station_info*)(curr_station_info_entry->data);
						curr_station_info->tx.phy.power = (default_unicast_data_tx_params.phy.power);
						curr_station_info_entry = dl_entry_next(curr_station_info_entry);
					}

					xil_printf("(+) Default Tx Power: %d dBm\n", (default_unicast_data_tx_params.phy.power));
				break;

				// ----------------------------------------
				// 'r' - Decrease MCS for Unicast TX traffic
				//
				case ASCII_r:
					// Decrease the default unicast data TX parameters MCS
					//     - This is for any new association
					if ((default_unicast_data_tx_params.phy.mcs) > 0) {
						(default_unicast_data_tx_params.phy.mcs)--;
					} else {
						(default_unicast_data_tx_params.phy.mcs) = 0;
					}

					// Decrease the MCS for all associated stations
					curr_station_info_entry = my_bss_info->associated_stations.first;

					while (curr_station_info_entry != NULL) {
						curr_station_info = (station_info*)(curr_station_info_entry->data);
						curr_station_info->tx.phy.mcs = (default_unicast_data_tx_params.phy.mcs);
						curr_station_info_entry = dl_entry_next(curr_station_info_entry);
					}

					xil_printf("(-) Default Unicast MCS Index: %d\n", default_unicast_data_tx_params.phy.mcs);
				break;

				// ----------------------------------------
				// 'R' - Increase MCS for Unicast TX traffic
				//
				case ASCII_R:
					// Increase the default unicast data TX parameters MCS
					//     - This is for any new association
					if ((default_unicast_data_tx_params.phy.mcs) < WLAN_MAC_NUM_MCS) {
						(default_unicast_data_tx_params.phy.mcs)++;
					} else {
						(default_unicast_data_tx_params.phy.mcs) = WLAN_MAC_NUM_MCS;
					}

					// Increase the MCS for all associated stations
					curr_station_info_entry = my_bss_info->associated_stations.first;

					while (curr_station_info_entry != NULL) {
						curr_station_info = (station_info*)(curr_station_info_entry->data);
						curr_station_info->tx.phy.mcs = (default_unicast_data_tx_params.phy.mcs);
						curr_station_info_entry = dl_entry_next(curr_station_info_entry);
					}

					xil_printf("(+) Default Unicast MCS Index: %d\n", default_unicast_data_tx_params.phy.mcs);
				break;

				// ----------------------------------------
				// 'm' - Display Heap / Malloc information
				//
				case ASCII_m:
					wlan_mac_high_display_mallinfo();
				break;

				// ----------------------------------------
				// 's' - Change the SSID
				//
				case ASCII_s:
					uart_mode = UART_MODE_SSID_CHANGE;
					deauthenticate_all_stations();
					curr_char = 0;
					print_ssid_menu();
				break;
			}
		break;


		// ------------------------------------------------
		// Interactive Menu processing
		//
		case UART_MODE_INTERACTIVE:
			switch(rxByte){

				// ----------------------------------------
				// 'r' - Reset station counts
				//
				case ASCII_r:
					reset_station_counts();
				break;

				// ----------------------------------------
				// 'd' - De-authenticate all stations
				//
				case ASCII_d:
					deauthenticate_all_stations();
				break;

				// ----------------------------------------
				// 'b' - Enable / Disable "Traffic Blaster"
				//     The "Traffic Blaster" will create a backlogged LTG with a payload of
				//     1400 bytes that will be sent to all associated nodes.
				//
				case ASCII_b:
					// Check if an LTG has been created and create a new one if not
					if (traffic_blast_ltg_id == LTG_ID_INVALID) {
						// Set up LTG payload
						traffic_blast_pyld.hdr.type = LTG_PYLD_TYPE_ALL_ASSOC_FIXED;
						traffic_blast_pyld.length   = 1400;

						// Set up LTG schedule
						traffic_blast_sched.duration_count = LTG_DURATION_FOREVER;
						traffic_blast_sched.interval_count = 0;

						// Start the LTG
						traffic_blast_ltg_id = ltg_sched_create(LTG_SCHED_TYPE_PERIODIC, &traffic_blast_sched, &traffic_blast_pyld, NULL);

						// Check if there was an error
						if (traffic_blast_ltg_id == LTG_ID_INVALID) {
							xil_printf("Error in creating LTG\n");
							break;
						}
					}

					// Check to see if this LTG ID is currently running.
					//     Note: Given that the "Traffic Blaster" only creates period LTGs, the code can assume
					//         the type of the ltg_state.  In general, the second argument to ltg_sched_get_state
					//         can be used to figure out what type to cast the ltg_state.
					ltg_sched_get_state(traffic_blast_ltg_id, NULL, &ltg_state);

					// Based on the state, start / stop the LTG
					switch (((ltg_sched_periodic_state*)ltg_state)->hdr.enabled) {

						// LTG is not running, so let's start it
						case 0:   ltg_sched_start(traffic_blast_ltg_id);  break;

						// LTG is running, so let's stop it
						case 1:   ltg_sched_stop(traffic_blast_ltg_id);  break;
					}
				break;
			}
		break;


		// ------------------------------------------------
		// Change SSID processing
		//
		case UART_MODE_SSID_CHANGE:
			switch(rxByte){

				// ----------------------------------------
				// Press <Enter> - process new SSID
				//
				case ASCII_CR:
					// Create a '\0' as the final character so SSID is a proper string
					text_entry[curr_char] = 0;

					// Update BSS configuration w/ new SSID
					strcpy(bss_config.ssid, text_entry);
					bss_config.update_mask = BSS_FIELD_MASK_SSID;
					configure_bss(&bss_config);

					// Reset SSID character pointer
					curr_char = 0;

					// Change menu back to Main Menu
					uart_mode = UART_MODE_MAIN;

					// Print info to screen
					//     - Pause for a second since the return to the Main Menu will erase the screen
					xil_printf("\nSetting new SSID: %s\n", my_bss_info->ssid);
					usleep(2000000);
					print_main_menu();
				break;

				// ----------------------------------------
				// Press <Delete> - Remove last character
				//
				case ASCII_DEL:
					if (curr_char > 0) {
						curr_char--;
						xil_printf("\b \b");
					}
				break;

				// ----------------------------------------
				// Process character
				//
				default:
					if (((rxByte <= ASCII_z) && (rxByte >= ASCII_A)) ||
						(rxByte == ASCII_SPACE) || (rxByte == ASCII_DASH)) {
						if (curr_char < SSID_LEN_MAX) {
							xil_printf("%c", rxByte);
							text_entry[curr_char] = rxByte;
							curr_char++;
						}
					}
				break;
			}
		break;


		default:
			uart_mode = UART_MODE_MAIN;
			print_main_menu();
		break;
	}
}



void print_main_menu(){
	xil_printf("\f");
	xil_printf("********************** AP Menu **********************\n");
	xil_printf("[1]   - Interactive AP Status\n");
	xil_printf("[2]   - Print Queue Status\n");
	xil_printf("[3]   - Print all Observed Counts\n");
	xil_printf("\n");
	xil_printf("[a]   - Display Network List\n");
	xil_printf("[c/C] - Change channel (note: changing channel will\n");
	xil_printf("        purge any associations, forcing stations to\n");
	xil_printf("        join the network again)\n");
	xil_printf("[g/G] - Change TX power\n");
    xil_printf("[r/R] - Change unicast MCS index (rate)\n");
	xil_printf("[s]   - Change SSID (note: changing SSID will purge\n");
	xil_printf("        any associations)\n");
	xil_printf("*****************************************************\n");
}



void print_ssid_menu(){
	xil_printf("\f");
	xil_printf("Current SSID: %s\n", my_bss_info->ssid);
	xil_printf("To change the current SSID, please type a new string and press enter\n");
	xil_printf(": ");
}



void print_station_status(){

	station_info* curr_station_info;
	dl_entry*	  curr_entry;

	u64 timestamp;

	if(uart_mode == UART_MODE_INTERACTIVE){
		timestamp = get_system_time_usec();
		xil_printf("\f");

		curr_entry = my_bss_info->associated_stations.first;

		while(curr_entry != NULL){
			curr_station_info = (station_info*)(curr_entry->data);
			xil_printf("---------------------------------------------------\n");
			if(curr_station_info->hostname[0] != 0){
				xil_printf(" Hostname: %s\n", curr_station_info->hostname);
			}
			xil_printf(" AID: %02x -- MAC Addr: %02x:%02x:%02x:%02x:%02x:%02x\n", curr_station_info->AID,
					curr_station_info->addr[0],curr_station_info->addr[1],curr_station_info->addr[2],curr_station_info->addr[3],curr_station_info->addr[4],curr_station_info->addr[5]);

			xil_printf("     - Last heard from         %d ms ago\n",((u32)(timestamp - (curr_station_info->latest_activity_timestamp)))/1000);
			xil_printf("     - Last Rx Power:          %d dBm\n",curr_station_info->rx.last_power);
			xil_printf("     - # of queued MPDUs:      %d\n", queue_num_queued(AID_TO_QID(curr_station_info->AID)));
			xil_printf("     - # Tx High Data MPDUs:   %d (%d successful)\n", curr_station_info->counts->data.tx_num_packets_total,
			                                                                  curr_station_info->counts->data.tx_num_packets_success);
			xil_printf("     - # Tx High Data bytes:   %d (%d successful)\n", (u32)(curr_station_info->counts->data.tx_num_bytes_total),
			                                                                  (u32)(curr_station_info->counts->data.tx_num_bytes_success));
			xil_printf("     - # Tx Low Data MPDUs:    %d\n", curr_station_info->counts->data.tx_num_packets_low);
			xil_printf("     - # Tx High Mgmt MPDUs:   %d (%d successful)\n", curr_station_info->counts->mgmt.tx_num_packets_total,
			                                                                  curr_station_info->counts->mgmt.tx_num_packets_success);
			xil_printf("     - # Tx High Mgmt bytes:   %d (%d successful)\n", (u32)(curr_station_info->counts->mgmt.tx_num_bytes_total),
			                                                                  (u32)(curr_station_info->counts->mgmt.tx_num_bytes_success));
			xil_printf("     - # Tx Low Mgmt MPDUs:    %d\n", curr_station_info->counts->mgmt.tx_num_packets_low);
			xil_printf("     - # Rx Data MPDUs:        %d\n", curr_station_info->counts->data.rx_num_packets);
			xil_printf("     - # Rx Data Bytes:        %d\n", curr_station_info->counts->data.rx_num_bytes);
			xil_printf("     - # Rx Mgmt MPDUs:        %d\n", curr_station_info->counts->mgmt.rx_num_packets);
			xil_printf("     - # Rx Mgmt Bytes:        %d\n", curr_station_info->counts->mgmt.rx_num_bytes);

			curr_entry = dl_entry_next(curr_entry);
		}

			xil_printf("---------------------------------------------------\n");
			xil_printf("\n");
			xil_printf("[r] - reset counts\n");
			xil_printf("[d] - deauthenticate all stations\n\n");
	}
}



void print_queue_status(){
	dl_entry* curr_entry;
	station_info* curr_station_info;
	xil_printf("\nQueue Status:\n");
	xil_printf(" FREE || MCAST|");

	curr_entry = my_bss_info->associated_stations.first;
	while(curr_entry != NULL){
		curr_station_info = (station_info*)(curr_entry->data);
		xil_printf("%6d|", curr_station_info->AID);
		curr_entry = dl_entry_next(curr_entry);
	}
	xil_printf("\n");

	xil_printf("%6d||%6d|",queue_num_free(),queue_num_queued(MCAST_QID));

	curr_entry = my_bss_info->associated_stations.first;
	while(curr_entry != NULL){
		curr_station_info = (station_info*)(curr_entry->data);
		xil_printf("%6d|", queue_num_queued(AID_TO_QID(curr_station_info->AID)));
		curr_entry = dl_entry_next(curr_entry);
	}
	xil_printf("\n");

}



void print_all_observed_counts(){
	counts_txrx  * curr_counts;
	dl_entry     * curr_counts_entry;

	curr_counts_entry = counts_table.first;

	xil_printf("\nAll Counts:\n");
	while(curr_counts_entry != NULL){
		curr_counts = (counts_txrx*)(curr_counts_entry->data);

		xil_printf("---------------------------------------------------\n");
		xil_printf("%02x:%02x:%02x:%02x:%02x:%02x\n",          curr_counts->addr[0], curr_counts->addr[1], curr_counts->addr[2],
		                                                       curr_counts->addr[3], curr_counts->addr[4], curr_counts->addr[5]);
		xil_printf("     - Last timestamp:         %d usec\n", (u32)curr_counts->latest_txrx_timestamp);
		xil_printf("     - Associated?             %d\n", curr_counts->is_associated);
		xil_printf("     - # Tx High Data MPDUs:   %d (%d successful)\n", curr_counts->data.tx_num_packets_total, curr_counts->data.tx_num_packets_success);
		xil_printf("     - # Tx High Data bytes:   %d (%d successful)\n", (u32)(curr_counts->data.tx_num_bytes_total), (u32)(curr_counts->data.tx_num_bytes_success));
		xil_printf("     - # Tx Low Data MPDUs:    %d\n", curr_counts->data.tx_num_packets_low);
		xil_printf("     - # Tx High Mgmt MPDUs:   %d (%d successful)\n", curr_counts->mgmt.tx_num_packets_total, curr_counts->mgmt.tx_num_packets_success);
		xil_printf("     - # Tx High Mgmt bytes:   %d (%d successful)\n", (u32)(curr_counts->mgmt.tx_num_bytes_total), (u32)(curr_counts->mgmt.tx_num_bytes_success));
		xil_printf("     - # Tx Low Mgmt MPDUs:    %d\n", curr_counts->mgmt.tx_num_packets_low);
		xil_printf("     - # Rx Data MPDUs:        %d\n", curr_counts->data.rx_num_packets);
		xil_printf("     - # Rx Data Bytes:        %d\n", curr_counts->data.rx_num_bytes);
		xil_printf("     - # Rx Mgmt MPDUs:        %d\n", curr_counts->mgmt.rx_num_packets);
		xil_printf("     - # Rx Mgmt Bytes:        %d\n", curr_counts->mgmt.rx_num_bytes);

		curr_counts_entry = dl_entry_next(curr_counts_entry);
	}
}



void start_periodic_print(){
	stop_periodic_print();
	print_station_status();
	print_scheduled = 1;
	schedule_ID = wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, 1000000, SCHEDULE_REPEAT_FOREVER, (void*)print_station_status);
}



void stop_periodic_print(){
	if (print_scheduled) {
		print_scheduled = 0;
		wlan_mac_remove_schedule(SCHEDULE_COARSE, schedule_ID);
	}
}


#endif


