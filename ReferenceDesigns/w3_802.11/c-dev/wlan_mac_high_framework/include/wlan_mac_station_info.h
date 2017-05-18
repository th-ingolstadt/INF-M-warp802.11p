/** @file wlan_mac_station_info.c
 *  @brief Station Information Metadata Subsystem
 *
 *  This contains code tracking metadata about stations.
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

#ifndef WLAN_MAC_STATION_INFO_H_
#define WLAN_MAC_STATION_INFO_H_

/***************************** Include Files *********************************/

#include "wlan_mac_high_sw_config.h"
#include "wlan_mac_high_types.h"


/*************************** Constant Definitions ****************************/

#define RATE_SELECTION_SCHEME_STATIC                       0
#define ADD_STATION_INFO_ANY_ID                            0                                  ///< Special argument to function that adds station_info structs

//-----------------------------------------------
// Timeout used to remove inactive BSS infos
//     - Part of bss_info_timestamp_check()
//
#define STATION_INFO_TIMEOUT_USEC                           600000000

#define STATION_INFO_FLAG_KEEP                             0x00000001			   ///< Prevent MAC High Framework from deleting this station_infO
#define STATION_INFO_FLAG_DISABLE_ASSOC_CHECK              0x00000002              ///< Mask for flag in station_info -- disable association check
#define STATION_INFO_FLAG_DOZE                             0x00000004              ///< Mask to sleeping stations (if STA supports PS)
#define STATION_INFO_FLAG_HT_CAPABLE                       0x00000008              ///< Station is capable of HT Tx and Rx

#define RX_PROCESS_COUNTS_OPTION_FLAG_IS_DUPLICATE		   0x00000001

#define STATION_INFO_PRINT_OPTION_FLAG_INCLUDE_COUNTS	   0x00000001

/*************************** Function Prototypes *****************************/

void             station_info_init();
void             station_info_init_finish();

station_info_entry_t* station_info_checkout();
void             station_info_checkin(dl_entry* entry);

station_info_t*  station_info_tx_process(void* pkt_buf_addr);
station_info_t*	 station_info_rx_process(void* pkt_buf_addr);
#if WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS
void   					station_info_rx_process_counts(void* pkt_buf_addr, station_info_t* station_info, u32 option_flags);
#endif

void             station_info_print(dl_list* list, u32 option_flags);

#if WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS
void             txrx_counts_zero_all();
void 			 station_info_clear_txrx_counts(station_txrx_counts_t* txrx_counts);
#endif

void             station_info_timestamp_check();

station_info_t*  station_info_create(u8* addr);
void             station_info_reset_all();
void 			 station_info_clear(station_info_t* station_info);

dl_list*  		 station_info_get_list();

station_info_entry_t* station_info_find_by_id(u32 id, dl_list* list);
station_info_entry_t* station_info_find_by_addr(u8* addr, dl_list* list);

station_info_t*  station_info_add(dl_list* app_station_info_list, u8* addr, u16 requested_ID, tx_params_t* tx_params, u8 ht_capable);
int              station_info_remove(dl_list* app_station_info_list, u8* addr);

u8               station_info_is_member(dl_list* app_station_info_list, station_info_t* station_info);
int              station_info_update_rate(station_info_t* station_info, u8 mcs, u8 phy_mode);

#endif
