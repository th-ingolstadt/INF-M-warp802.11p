/** @file wlan_mac_ibss.h
 *  @brief Independent BSS
 *
 *  This contains code for the 802.11 IBSS node (ad hoc).
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/***************************** Include Files *********************************/

/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_IBSS_H_
#define WLAN_MAC_IBSS_H_

#include "xil_types.h"

//Forward declarations
struct bss_config_t;
struct station_info_t;
struct rx_common_entry;
struct network_info_t;
enum scan_state_t;
struct tx_queue_buffer_t;
struct tx_frame_info_t;

//-----------------------------------------------
// Enable the WLAN UART Menu
#define WLAN_USE_UART_MENU


//-----------------------------------------------
// Common Defines
#define NUM_BASIC_RATES_MAX                                10

#define MAX_TX_QUEUE_LEN                                   150       /// Maximum number of entries in any Tx queue
#define MAX_NUM_PEERS                                      15        /// Maximum number of peers allowed


//-----------------------------------------------
// Tx queue IDs
#define MCAST_QID                                          0
#define BEACON_QID                                         1
#define MANAGEMENT_QID                                     2
#define STATION_ID_TO_QUEUE_ID(x)                                    ((x) + 2)   ///map association ID to Tx queue ID; min AID is 1


//-----------------------------------------------
// Timing parameters

// Period for checking association table for stale associations
#define ASSOCIATION_CHECK_INTERVAL_MS                     (1000)
#define ASSOCIATION_CHECK_INTERVAL_US                     (ASSOCIATION_CHECK_INTERVAL_MS * 1000)

// Timeout for last reception for an association; timed-out associations are subject to de-association
#define ASSOCIATION_TIMEOUT_S                             (300)
#define ASSOCIATION_TIMEOUT_US                            (ASSOCIATION_TIMEOUT_S * 1000000)


/*********************** Global Structure Definitions ************************/


/*************************** Function Prototypes *****************************/
int  main();
void handle_cpu_low_reboot();
u32  configure_bss(struct bss_config_t* bss_config, u32 update_mask);

void remove_inactive_station_infos();

void ltg_event(u32 id, void* callback_arg);
void association_timestamp_adjust(s64 timestamp_diff);

int  ethernet_receive(dl_entry* curr_tx_queue_element, u8* eth_dest, u8* eth_src, u16 tx_length);

u32 mpdu_rx_process(void* pkt_buf_addr, struct station_info_t* station_info, struct rx_common_entry* rx_event_log_entry);
struct network_info_t* active_network_info_getter();
void process_scan_state_change(enum scan_state_t scan_state);
void poll_tx_queues();
void purge_all_data_tx_queue();


void uart_rx(u8 rxByte);



#endif /* WLAN_MAC_STA_H_ */
