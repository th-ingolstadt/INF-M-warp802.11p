/** @file wlan_mac_ltg.h
 *  @brief Local Traffic Generator
 *
 *  This contains code for scheduling local traffic directly from the
 *  board.
 *
 *  @copyright Copyright 2013-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

#ifndef WLAN_MAC_LTG_H_
#define WLAN_MAC_LTG_H_

//LTG Schedules define the times when LTG event callbacks are called.
#define LTG_SCHED_TYPE_PERIODIC			1
#define LTG_SCHED_TYPE_UNIFORM_RAND	 	2

//LTG Payloads define how payloads are constructed once the LTG event callbacks
//are called. For example, the LTG_SCHED_TYPE_PERIODIC schedule that employs the
//LTG_PYLD_TYPE_FIXED would result in a constant bit rate (CBR) traffic
//profile
#define LTG_PYLD_TYPE_FIXED				1
#define LTG_PYLD_TYPE_UNIFORM_RAND		2
#define LTG_PYLD_TYPE_ALL_ASSOC_FIXED	3


#define LTG_REMOVE_ALL                  0xFFFFFFFF
#define LTG_START_ALL                   0xFFFFFFFF
#define LTG_STOP_ALL                    0xFFFFFFFF


//Note: This definition simply reflects the use of the fast timer for LTG polling. To increase LTG
//polling rate at the cost of more overhead in checking LTGs, increase the speed of the fast timer.
#define LTG_POLL_INTERVAL              FAST_TIMER_DUR_US

#define LTG_ID_INVALID	               0xFFFFFFFF

//External function to LTG -- user code interacts with the LTG via these functions
int wlan_mac_ltg_sched_init();
void wlan_mac_ltg_sched_set_callback(void(*callback)());
u32 ltg_sched_create(u32 type, void* params, void* callback_arg, void(*callback)());
int ltg_sched_remove(u32 id);
int ltg_sched_remove_all();
int ltg_sched_start(u32 id);
int ltg_sched_start_all();
int ltg_sched_stop(u32 id);
int ltg_sched_stop_all();
int ltg_sched_get_state(u32 id, u32* type, void** state);
int ltg_sched_get_params(u32 id, void** params);
int ltg_sched_get_callback_arg(u32 id, void** callback_arg);
int wlan_create_ltg_frame(void* pkt_buf, mac_header_80211_common* common, u8 tx_flags, u32 ltg_id);
dl_entry* ltg_sched_find_tg_schedule(u32 id);

// WLAN Exp function to LTG -- users may call these directly or modify if needed
void * ltg_sched_deserialize(u32 * src, u32 * ret_type, u32 * ret_size);
void * ltg_payload_deserialize(u32 * src, u32 * ret_type, u32 * ret_size);

#endif /* WLAN_MAC_LTG_H_ */
