/** @file wlan_mac_sta.h
 *  @brief Station
 *
 *  This contains code for the 802.11 Station.
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

/***************************** Include Files *********************************/


/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_STA_H_
#define WLAN_MAC_STA_H_

#include "wlan_mac_dl_list.h"


// **********************************************************************
// Enable the WLAN UART Menu
//    NOTE:  To enable the WLAN Exp framework, please modify wlan_exp_common.h
 #define WLAN_USE_UART_MENU



// **********************************************************************
// Common Defines
//
#define SSID_LEN_MAX                   32
#define NUM_BASIC_RATES_MAX            10

#define MAX_RETRY                       7



// **********************************************************************
// UART Menu Modes
//
#define UART_MODE_MAIN                 0
#define UART_MODE_INTERACTIVE          1
#define UART_MODE_AP_LIST              2
#define UART_MODE_LTG_SIZE_CHANGE	   3
#define UART_MODE_LTG_INTERVAL_CHANGE  4



// **********************************************************************
// Timing Parameters
//
#define ASSOCIATION_TIMEOUT_US         100000
#define ASSOCIATION_NUM_TRYS           5

#define AUTHENTICATION_TIMEOUT_US      100000
#define AUTHENTICATION_NUM_TRYS        5

#define NUM_PROBE_REQ                  5

//The amount of time the active scan procedure will dwell on each channel before
//moving to the next channel.
#define ACTIVE_SCAN_DWELL			   100000

//The amount of time between full active scans when looking for a particular SSID
//Note: This value must be larger than the maximum amount of time it takes for
//a single active scan. For an active scan over 11 channels, this value must be larger
//than 11*ACTIVE_SCAN_DWELL.
#define ACTIVE_SCAN_UPDATE_RATE		  5000000



/*********************** Global Structure Definitions ************************/
//
// Information about APs
//
typedef struct{
	u8   bssid[6];
	u8   chan;
	u8   private;
	char ssid[SSID_LEN_MAX];
	u8   num_basic_rates;
	u8   basic_rates[NUM_BASIC_RATES_MAX];
	char rx_power;
} ap_info;



/*************************** Function Prototypes *****************************/

int main();

void ltg_event(u32 id, void* callback_arg);

int ethernet_receive(dl_list* tx_queue_list, u8* eth_dest, u8* eth_src, u16 tx_length);

void mpdu_rx_process(void* pkt_buf_addr, u8 rate, u16 length);
void mpdu_transmit_done(tx_frame_info* tx_mpdu);
void check_tx_queue();

void start_active_scan();
void stop_active_scan();
void probe_req_transmit();

void attempt_authentication();

void reset_station_statistics();

int  get_ap_list( ap_info * ap_list, u32 num_ap, u32 * buffer, u32 max_words );

void print_menu();
void print_ap_list();
void print_station_status(u8 manual_call);
void ltg_cleanup(u32 id, void* callback_arg);
void bad_fcs_rx_process(void* pkt_buf_addr, u8 rate, u16 length);
void uart_rx(u8 rxByte);



#endif /* WLAN_MAC_STA_H_ */
