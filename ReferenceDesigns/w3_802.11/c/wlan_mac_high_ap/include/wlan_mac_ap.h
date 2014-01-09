/** @file wlan_mac_ap.c
 *  @brief Access Point
 *
 *  This contains code for the 802.11 Access Point.
 *
 *  @copyright Copyright 2013, Mango Communications. All rights reserved.
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
#ifndef WLAN_MAC_AP_H_
#define WLAN_MAC_AP_H_

#include "wlan_mac_dl_list.h"


// **********************************************************************
// Switches for various modes
//

// Enable the WLAN UART Menu
//    NOTE:  To enable the WLAN Exp framework, please modify wlan_exp_common.h
#define WLAN_USE_UART_MENU

// Allow Ethernet transmission of packets received by an associated station
// destined for another associated station
//#define ALLOW_ETH_TX_OF_WIRELESS_TX

// Allow promiscuous statistics kept for unassociated stations. Note: this mode
// has an unbounded memory usage depending on the number of devices surrounding the
// AP.
#define ALLOW_PROMISC_STATISTICS

// **********************************************************************
// UART Menu Modes
//
#define UART_MODE_MAIN                 0
#define UART_MODE_INTERACTIVE          1
#define UART_MODE_SSID_CHANGE          2
#define UART_MODE_LTG_SIZE_CHANGE	   3
#define UART_MODE_LTG_INTERVAL_CHANGE  4

// **********************************************************************
// Common Defines
//
#define MAX_RETRY                      7
#define MAX_PER_FLOW_QUEUE	           150
#define MAX_NUM_ASSOC				   32
#define MAX_NUM_PROMISC_STATS		   50


// **********************************************************************
// Timing Parameters
//

// Time between beacon transmissions
//
#define BEACON_INTERVAL_MS             (10)
#define BEACON_INTERVAL_US             (BEACON_INTERVAL_MS*1000)


// Time between association table check
// Periodically, the association table is culled through and inactive stations are explicitly purged.
//
#define ASSOCIATION_CHECK_INTERVAL_MS  (10000)
#define ASSOCIATION_CHECK_INTERVAL_US  (ASSOCIATION_CHECK_INTERVAL_MS*1000)

// The amount of time since the last time a station was heard from.
// After this interval, a station can be purged from the association table
//
//#define ASSOCIATION_TIMEOUT_S          (600)
#define ASSOCIATION_TIMEOUT_S          (600)
#define ASSOCIATION_TIMEOUT_US         (ASSOCIATION_TIMEOUT_S*1000000)

// When the node is in the state where it temporarily allows associations, this interval
// defines how long the window for new associations is open
//
#define ASSOCIATION_ALLOW_INTERVAL_MS  (30000)
#define ASSOCIATION_ALLOW_INTERVAL_US  (ASSOCIATION_ALLOW_INTERVAL_MS*1000)

// Time between blinking behavior in hex displays
//
#define ANIMATION_RATE_US              (100000)


// **********************************************************************
// Association defines
//
#define ASSOCIATION_ALLOW_NONE          0x0
#define ASSOCIATION_ALLOW_TEMPORARY     0x1
#define ASSOCIATION_ALLOW_PERMANENT     0x3


/*********************** Global Structure Definitions ************************/


/****************************** Helper Macros ********************************/
#define LTG_ID_TO_AID(ltg_id) (ltg_id)
#define AID_TO_LTG_ID(aid)	  (aid)
//#define AID_SCHED_PYLD_GRP_TO_LTG_ID(aid,sched,pyld) ( ((pyld&0xFF)<<24) + ((sched&0xFF)<<16) + (aid&0xFFFF) )

#define LTG_ID_GRP_SCHED_CONSTANT 1
#define LTG_ID_GRP_SCHED_RANDOM	  2

#define LTG_ID_GRP_PYLD_FIXED	  1
#define LTG_ID_GRP_PYLD_RANDOM	  2


/*************************** Function Prototypes *****************************/


int  main();

void ltg_event(u32 id, void* callback_arg);

int  ethernet_receive(dl_list* tx_queue_list, u8* eth_dest, u8* eth_src, u16 tx_length);

void mpdu_rx_process(void* pkt_buf_addr, u8 rate, u16 length);
void bad_fcs_rx_process(void* pkt_buf_addr, u8 rate, u16 length);

void mpdu_transmit_done(tx_frame_info* tx_mpdu);
void check_tx_queue();

void beacon_transmit();

u32  get_associations_status();
void enable_associations();
void disable_associations();
void association_timestamp_check();

void reset_station_statistics();

u32  deauthenticate_station( station_info* station );
void deauthenticate_stations();

station_info* add_association(dl_list* assoc_tbl, dl_list* stat_tbl, u8* addr);
statistics* add_statistics(dl_list* stat_tbl, station_info* station, u8* addr);
int remove_association(dl_list* assoc_tbl, dl_list* stat_tbl, u8* addr);
u8 is_valid_association(dl_list* assoc_tbl, station_info* station);

void animate_hex();
void up_button();

void uart_rx(u8 rxByte);

void print_menu();
void print_ssid_menu();
void print_associations(dl_list* assoc_tbl);
void print_queue_status();
void print_station_status();
void ltg_cleanup(u32 id, void* callback_arg);
void start_periodic_print();
void print_all_observed_statistics();
void stop_periodic_print();
int is_qwerty_row(u8 rxByte);
int qwerty_row_to_number(u8 rxByte);


#endif /* WLAN_MAC_AP_H_ */
