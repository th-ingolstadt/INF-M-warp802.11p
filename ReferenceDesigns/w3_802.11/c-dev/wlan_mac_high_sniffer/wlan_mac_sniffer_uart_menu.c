/** @file wlan_mac_sta_uart_menu.c
 *  @brief Station UART Menu
 *
 *  This contains code for the 802.11 Station's UART menu.
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
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
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_sniffer.h"
#include "ascii_characters.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_network_info.h"
#include "wlan_mac_station_info.h"
#include "wlan_platform_common.h"
#include "wlan_mac_dl_list.h"


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
#define UART_MODE_MAIN        0
#define UART_MODE_INTERACTIVE 1
#define UART_MODE_SETTINGS	  2

#define UART_STATE_MENU 	0
#define UART_STATE_INPUT	1


/*********************** Global Variable Definitions *************************/
extern network_info_t*                      active_network_info;

/*************************** Variable Definitions ****************************/

static volatile u8 uart_mode = UART_MODE_MAIN;
static volatile u32 schedule_id;
static volatile u8 print_scheduled = 0;

#define UART_INPUT_MAX 255
static volatile u8 uart_state = UART_STATE_MENU;
static volatile u8 uart_input_buf[UART_INPUT_MAX];
static volatile void(*uart_input_cb)(const char*);

/*************************** Functions Prototypes ****************************/

void print_main_menu();
void print_settings_menu();

void print_queue_status();
void print_station_status();

void uart_set_ssid(const char*);
void uart_set_channel(const char*);
void uart_set_beaconinterval(const char*);


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

	// ----------------------------------------------------
	// Return to the Main Menu
	//    - Stops any prints / LTGs
	if (rxByte == ASCII_ESC) {
		uart_mode = UART_MODE_MAIN;
		print_main_menu();
		return;
	}

	switch (uart_mode) {

		// ------------------------------------------------
		// Main Menu processing
		//
		case UART_MODE_MAIN:
			switch(rxByte){

//				// ----------------------------------------
//				// '1' - Switch to Interactive Menu
//				//
//				case ASCII_1:
//					uart_mode = UART_MODE_INTERACTIVE;
//					start_periodic_print();
//				break;
//
//				// ----------------------------------------
//				// '2' - Print Queue status
//				//
//				case ASCII_2:
//					print_queue_status();
//				break;
//
//				// ----------------------------------------
//				// '3' - Print Station Infos with Counts
//				//
//				case ASCII_3:
//					station_info_print(NULL , STATION_INFO_PRINT_OPTION_FLAG_INCLUDE_COUNTS);
//				break;

				case ASCII_4:
					uart_mode = UART_MODE_SETTINGS;
					print_settings_menu();
				break;

				// ----------------------------------------
				// 'e' - Print event log size
				//
				case ASCII_e:
#if WLAN_SW_CONFIG_ENABLE_LOGGING
					event_log_config_logging(EVENT_LOG_LOGGING_DISABLE);
					print_event_log_size();
					event_log_config_logging(EVENT_LOG_LOGGING_ENABLE);
#endif //WLAN_SW_CONFIG_ENABLE_LOGGING
				break;

				// ----------------------------------------
				// 'a' - Print BSS information
				//
				case ASCII_a:
					print_network_info();
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
#if WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS
					txrx_counts_zero_all();
#endif
				break;
			}
		break;

		case UART_MODE_SETTINGS:
			switch(uart_state) {
			case UART_STATE_MENU:
				switch(rxByte) {
//				case ASCII_1:
//					memset(uart_input_buf, 0, UART_INPUT_MAX);
//					uart_input_cb = &uart_set_ssid;
//					uart_state = UART_STATE_INPUT;
//					xil_printf("> ");
//				break;
				case ASCII_2:
					memset(uart_input_buf, 0, UART_INPUT_MAX);
					uart_input_cb = &uart_set_channel;
					uart_state = UART_STATE_INPUT;
					xil_printf("> ");
				break;
//				case ASCII_3:
//					memset(uart_input_buf, 0, UART_INPUT_MAX);
//					uart_input_cb = &uart_set_beaconinterval;
//					uart_state = UART_STATE_INPUT;
//					xil_printf("> ");
//				break;
				default:
					xil_printf("unknown command %c\n", rxByte);
				}
			break;
			case UART_STATE_INPUT:
				xil_printf("%c", rxByte);
				switch(rxByte) {
				case ASCII_CR:
					uart_input_buf[UART_INPUT_MAX - 1] = '\0';
					uart_input_cb(uart_input_buf);
					uart_state = UART_STATE_MENU;
					break;
				default:
					uart_input_buf[strnlen(uart_input_buf, UART_INPUT_MAX)] = rxByte;
				}
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
	xil_printf("[4]   - Settings Menu\n");
	xil_printf("\n");
	xil_printf("[a]   - Display Network List\n");
	xil_printf("**********************************************************\n");
}

void print_settings_menu(){
	xil_printf("\f");
	xil_printf("********************** Settings Menu *********************\n");
	xil_printf("**********************************************************\n");
	// xil_printf("[1]   - Change SSID: %s\n", "tbd");
	xil_printf("[2]   - Change Channel: %d\n", 10);
	// xil_printf("[3]   - Change Beacon Interval: %d\n", 10);
	xil_printf("**********************************************************\n");
}

void uart_set_channel(const char* input) {
	xil_printf("-> Changing channel to %s\n", input);
	int channel = atoi(input);
	if (wlan_verify_channel(channel) == XST_SUCCESS) {
		// disable interrupts
		wlan_mac_high_interrupt_stop();

		wlan_mac_high_set_radio_channel(channel);

		// restart interrupts
		wlan_mac_high_interrupt_restore_state(INTERRUPTS_ENABLED);
	} else {
		xil_printf("-> Invalid channel %s\n", input);
	}
}

#endif


