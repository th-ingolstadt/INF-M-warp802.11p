/** @file wlan_mac_ltg.h
 *  @brief Local Traffic Generator
 *
 *  This contains code for scheduling local traffic directly from the
 *  board.
 *
 *  @copyright Copyright 2014, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

#ifndef WLAN_MAC_LTG_H_
#define WLAN_MAC_LTG_H_

#include "wlan_mac_dl_list.h"

//LTG Schedules define the times when LTG event callbacks are called.
#define LTG_SCHED_TYPE_PERIODIC			1
#define LTG_SCHED_TYPE_UNIFORM_RAND	 	2

//LTG Payloads define how payloads are constructed once the LTG event callbacks
//are called. For example, the LTG_SCHED_TYPE_PERIODIC schedule that employs the
//LTG_PYLD_TYPE_FIXED would result in a constant bit rate (CBR) traffic
//profile
#define LTG_PYLD_TYPE_FIXED				1
#define LTG_PYLD_TYPE_UNIFORM_RAND		2

#define LTG_REMOVE_ALL 0xFFFFFFFF


//In spirit, tg_schedule is derived from dl_node. Since C
//lacks a formal notion of inheritance, we adopt a popular
//alternative idiom for inheritance where the dl_node
//is the first entry in the new structure. Since structures
//will never be padded before their first entry, it is safe
//to cast back and forth between the tg_schedule and dl_node.
typedef struct tg_schedule tg_schedule;
struct tg_schedule{
	dl_node node;
	u32 id;
	u32 type;
	u64 timestamp;
	void* params;
	void* callback_arg;
	function_ptr_t cleanup_callback;
	void* state;
};

//Helper macros for traversing the doubly-linked list
#define tg_schedule_next(x) ( (tg_schedule*)dl_node_next(&(x->node)) )
#define tg_schedule_prev(x) ( (tg_schedule*)dl_node_prev(&(x->node)) )

//LTG Schedules

typedef struct {
	u8 enabled;
	u8 reserved[3];
} ltg_sched_state_hdr;

typedef struct {
	u32 interval_usec;
} ltg_sched_periodic_params;

typedef struct {
	ltg_sched_state_hdr hdr;
	u32 time_to_next_usec;
} ltg_sched_periodic_state;

typedef struct {
	u32 min_interval;
	u32 max_interval;
} ltg_sched_uniform_rand_params;

typedef struct {
	ltg_sched_state_hdr hdr;
	u32 time_to_next_usec;
} ltg_sched_uniform_rand_state;

//LTG Payload Profiles

typedef struct {
	u32 type;
} ltg_pyld_hdr;

typedef struct {
	ltg_pyld_hdr hdr;
	u16 length;
	u8 reserved[2];
} ltg_pyld_fixed_length;

typedef struct {
	ltg_pyld_hdr hdr;
	u16 length;
	u8 reserved[2];
} ltg_pyld_fixed;

typedef struct {
	ltg_pyld_hdr hdr;
	u16 min_length;
	u16 max_length;
} ltg_pyld_uniform_rand;

//External function to LTG -- user code interacts with the LTG via these functions
int wlan_mac_ltg_sched_init();
void wlan_mac_ltg_sched_set_callback(void(*callback)());
int ltg_sched_configure(u32 id, u32 type, void* params, void* callback_arg, void(*callback)());
int ltg_sched_remove(u32 id);
int ltg_sched_start(u32 id);
int ltg_sched_stop(u32 id);
int ltg_sched_start_l(tg_schedule* curr_tg);
int ltg_sched_stop_l(tg_schedule* curr_tg);
int ltg_sched_get_state(u32 id, u32* type, void** state);
int ltg_sched_get_params(u32 id, u32* type, void** params);
int ltg_sched_get_callback_arg(u32 id, void** callback_arg);

//Internal functions to LTG -- users should not need to call these directly
void ltg_sched_check();
tg_schedule* ltg_sched_create();
void ltg_sched_destroy(tg_schedule* tg);
void ltg_sched_destroy_params(tg_schedule *tg);
tg_schedule* ltg_sched_find_tg_schedule(u32 id);

#endif /* WLAN_MAC_LTG_H_ */
