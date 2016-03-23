/** @file wlan_mac_ibss.h
 *  @brief Independent BSS
 *
 *  This contains code for the 802.11 IBSS node (ad hoc).
 *
 *  @copyright Copyright 2014-2015, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

/***************************** Include Files *********************************/

#include "wlan_mac_scan.h"

/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_IBSS_H_
#define WLAN_MAC_IBSS_H_


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

//-----------------------------------------------
// WLAN Exp defines
#define WLAN_EXP_STREAM_ASSOC_CHANGE                       WLAN_EXP_NO_TRANSMIT


/*********************** Global Structure Definitions ************************/


/*************************** Function Prototypes *****************************/
int  main();

u32  configure_bss(bss_config_t* bss_config);

void remove_inactive_station_infos();

void ltg_event(u32 id, void* callback_arg);
void association_timestamp_adjust(s64 timestamp_diff);

int  ethernet_receive(tx_queue_element* curr_tx_queue_element, u8* eth_dest, u8* eth_src, u16 tx_length);

void mpdu_rx_process(void* pkt_buf_addr);
void mpdu_transmit_done(tx_frame_info* tx_mpdu, wlan_mac_low_tx_details_t* tx_low_details, u16 num_tx_low_details);
void beacon_transmit_done( tx_frame_info* tx_mpdu, wlan_mac_low_tx_details_t* tx_low_details );
void process_scan_state_change(scan_state_t scan_state);
void poll_tx_queues();
void purge_all_data_tx_queue();

void reset_station_counts();

dl_list * get_counts();

void uart_rx(u8 rxByte);

void ibss_update_hex_display(u8 val);


#endif /* WLAN_MAC_STA_H_ */
