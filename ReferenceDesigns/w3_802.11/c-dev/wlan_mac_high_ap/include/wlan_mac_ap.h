/** @file wlan_mac_ap.h
 *  @brief Access Point
 *
 *  This contains code for the 802.11 Access Point.
 *
 *  @copyright Copyright 2013-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_AP_H_
#define WLAN_MAC_AP_H_

/***************************** Include Files *********************************/

#include "xil_types.h"
#include "wlan_common_types.h"

// Forward declarations
struct station_info_t;
struct rx_common_entry;
struct network_info_t;
enum scan_state_t;
struct bss_config_t;
struct tx_queue_buffer_t;
struct tx_frame_info_t;
//-----------------------------------------------
// Enable the WLAN UART Menu
#define WLAN_USE_UART_MENU


//-----------------------------------------------
// Allow Ethernet transmission of packets received by an associated station
// destined for another associated station
//#define ALLOW_ETH_TX_OF_WIRELESS_TX


//-----------------------------------------------
// Common Defines
#define MAX_TX_QUEUE_LEN                                   150       /// Maximum number of entries in any Tx queue

#define MAX_NUM_ASSOC                                      10        /// Maximum number of associations allowed
#define MAX_NUM_AUTH                                       10        /// Maximum number of authentications allowed


//-----------------------------------------------
// Tx queue IDs
#define MCAST_QID                                          0
#define MANAGEMENT_QID                                     1
#define STATION_ID_TO_QUEUE_ID(x)                        ((x) + 1)   /// Map association ID to Tx queue ID; min AID is 1
#define QID_TO_AID(x)                                    ((x) - 1)


//-----------------------------------------------
// Timing Parameters

// Period for checking association table for stale associations
#define ASSOCIATION_CHECK_INTERVAL_MS                     (1000)
#define ASSOCIATION_CHECK_INTERVAL_US                     (ASSOCIATION_CHECK_INTERVAL_MS * 1000)

// Timeout for last reception for an association; timed-out associations are subject to de-association
#define ASSOCIATION_TIMEOUT_S                             (300)
#define ASSOCIATION_TIMEOUT_US                            (ASSOCIATION_TIMEOUT_S * 1000000)

// Interval to allow associations after entering ASSOCIATION_ALLOW_TEMPORARY mode
#define ASSOCIATION_ALLOW_INTERVAL_MS                     (30000)
#define ASSOCIATION_ALLOW_INTERVAL_US                     (ASSOCIATION_ALLOW_INTERVAL_MS * 1000)

// Blinking period for hex displays, when used to show association mode
#define ANIMATION_RATE_US                                 (100000)


/*********************** Global Structure Definitions ************************/

/*************************** Function Prototypes *****************************/
int  main();

void ltg_event(u32 id, void* callback_arg);

int  ethernet_receive(dl_entry* curr_tx_queue_element, u8* eth_dest, u8* eth_src, u16 tx_length);
u32 					mpdu_rx_process(void* pkt_buf_addr, struct station_info_t* station_info, struct rx_common_entry* rx_event_log_entry);
struct network_info_t* 		active_network_info_getter();
void 					process_scan_state_change(enum scan_state_t scan_state);

void queue_state_change(u32 QID, u8 queue_len);
void update_tim_tag_aid(u8 aid, u8 bit_val_in);
void update_tim_tag_all(u32 sched_id);

void poll_tx_queues();
void purge_all_data_tx_queue();

void enable_associations();
void disable_associations();
void remove_inactive_station_infos();
void association_timestamp_adjust(s64 timestamp_diff);

u32  deauthenticate_station( struct station_info_t* station_info );
void deauthenticate_all_stations();
void handle_cpu_low_reboot(u32 type);
u32  configure_bss(struct bss_config_t* bss_config, u32 update_mask);
void mpdu_dequeue(struct tx_queue_buffer_t* tx_queue_buffer, struct tx_frame_info_t* tx_frame_info);

void button_0_press();
void button_0_release();

void uart_rx(u8 rxByte);

#endif /* WLAN_MAC_AP_H_ */
