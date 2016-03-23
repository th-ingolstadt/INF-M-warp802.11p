/** @file wlan_mac_sta_uart_menu.c
 *  @brief Station UART Menu
 *
 *  This contains code for the 802.11 Station's UART menu.
 *
 *  @copyright Copyright 2014-2016, Mango Communications. All rights reserved.
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
#include "wlan_mac_ltg.h"
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_ibss.h"
#include "ascii_characters.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_bss_info.h"


//
// Use the UART Menu
//     - If WLAN_USE_UART_MENU in wlan_mac_ibss.h is commented out, then this function
//       will do nothing.  This might be necessary to save code space.
//


#ifndef WLAN_USE_UART_MENU

void uart_rx(u8 rxByte){ };

#else


/*************************** Constant Definitions ****************************/

//-----------------------------------------------
// UART Menu Modes
#define UART_MODE_MAIN                                     0
#define UART_MODE_INTERACTIVE                              1


/*********************** Global Variable Definitions *************************/

extern wlan_mac_low_config_t                cpu_low_config;
extern tx_params_t                          default_unicast_data_tx_params;

extern bss_info*                            my_bss_info;
extern dl_list                              counts_table;


/*************************** Variable Definitions ****************************/

static volatile u8                          uart_mode            = UART_MODE_MAIN;
static volatile u32                         schedule_id;
static volatile u8                          print_scheduled      = 0;

static          ltg_pyld_all_assoc_fixed    traffic_blast_pyld;
static          ltg_sched_periodic_params   traffic_blast_sched;
static volatile u32                         traffic_blast_ltg_id = LTG_ID_INVALID;


/*************************** Functions Prototypes ****************************/

void print_main_menu();

void print_queue_status();
void print_station_status();
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
 *      - Print Malloc info (hidden)
 *    - Interactive Menu
 *      - Reset counts
 *      - Turn on/off "Traffic Blaster" (hidden)
 *
 * The escape key is used to return to the Main Menu.
 *
 *****************************************************************************/
void uart_rx(u8 rxByte){

	void*                       ltg_state;

	// ----------------------------------------------------
	// Return to the Main Menu
	//    - Stops any prints / LTGs
	if (rxByte == ASCII_ESC) {
		uart_mode = UART_MODE_MAIN;
		stop_periodic_print();
		print_main_menu();
		ltg_sched_remove(LTG_REMOVE_ALL);
		return;
	}

	switch (uart_mode) {

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
				// 'm' - Display Heap / Malloc information
				//
				case ASCII_m:
					wlan_mac_high_display_mallinfo();
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


		default:
			uart_mode = UART_MODE_MAIN;
			print_main_menu();
		break;
	}
}



void print_main_menu(){
	xil_printf("\f");
	xil_printf("********************** Station Menu **********************\n");
	xil_printf("[1]   - Interactive Station Status\n");
	xil_printf("[2]   - Print Queue Status\n");
	xil_printf("[3]   - Print all Observed Counts\n");
	xil_printf("\n");
	xil_printf("[a]   - Display Network List\n");
	xil_printf("**********************************************************\n");
}



void print_station_status() {

	station_info_t* curr_station_info;
	dl_entry*	    curr_entry;

	u64 timestamp;

	if(uart_mode == UART_MODE_INTERACTIVE){
		timestamp = get_system_time_usec();
		xil_printf("\f");

		if(my_bss_info != NULL){
			curr_entry = my_bss_info->station_info_list.first;

			while(curr_entry != NULL){
				curr_station_info = (station_info_t*)(curr_entry->data);
				xil_printf("---------------------------------------------------\n");
				if(curr_station_info->hostname[0] != 0){
					xil_printf(" Hostname: %s\n", curr_station_info->hostname);
				}
				xil_printf(" ID: %02x -- MAC Addr: %02x:%02x:%02x:%02x:%02x:%02x\n", curr_station_info->ID,
						curr_station_info->addr[0],curr_station_info->addr[1],curr_station_info->addr[2],curr_station_info->addr[3],curr_station_info->addr[4],curr_station_info->addr[5]);

				xil_printf("     - Last heard from         %d ms ago\n",((u32)(timestamp - (curr_station_info->latest_activity_timestamp)))/1000);
				xil_printf("     - Last Rx Power:          %d dBm\n",curr_station_info->rx.last_power);
				xil_printf("     - # of queued MPDUs:      %d\n", queue_num_queued(STATION_ID_TO_QUEUE_ID(curr_station_info->ID)));
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
		}
	}
}



void print_queue_status(){
	dl_entry* curr_entry;
	station_info_t* curr_station_info;
	xil_printf("\nQueue Status:\n");
	xil_printf(" FREE || MCAST|");

	if(my_bss_info != NULL){
		curr_entry = my_bss_info->station_info_list.first;
		while(curr_entry != NULL){
			curr_station_info = (station_info_t*)(curr_entry->data);
			xil_printf("%6d|", curr_station_info->ID);
			curr_entry = dl_entry_next(curr_entry);
		}
	}
	xil_printf("\n");

	xil_printf("%6d||%6d|",queue_num_free(),queue_num_queued(MCAST_QID));

	if(my_bss_info != NULL){
		curr_entry = my_bss_info->station_info_list.first;
		while(curr_entry != NULL){
			curr_station_info = (station_info_t*)(curr_entry->data);
			xil_printf("%6d|", queue_num_queued(STATION_ID_TO_QUEUE_ID(curr_station_info->ID)));
			curr_entry = dl_entry_next(curr_entry);
		}
	}
	xil_printf("\n");

}



void print_all_observed_counts(){
	dl_entry     * curr_counts_entry;
	counts_txrx  * curr_counts;

	curr_counts_entry = counts_table.first;

	xil_printf("\nAll Counts:\n");
	while(curr_counts_entry != NULL){
		curr_counts = (counts_txrx*)(curr_counts_entry->data);
		xil_printf("---------------------------------------------------\n");
		xil_printf("%02x:%02x:%02x:%02x:%02x:%02x\n",     curr_counts->addr[0], curr_counts->addr[1], curr_counts->addr[2],
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
	schedule_id = wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, 1000000, SCHEDULE_REPEAT_FOREVER, (void*)print_station_status);
}



void stop_periodic_print(){
	if (print_scheduled) {
		print_scheduled = 0;
		wlan_mac_remove_schedule(SCHEDULE_COARSE, schedule_id);
	}
}



#endif


