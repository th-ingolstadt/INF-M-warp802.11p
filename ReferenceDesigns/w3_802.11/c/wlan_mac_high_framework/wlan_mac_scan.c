/** @file wlan_mac_scan.c
 *  @brief Active Scan FSM
 *
 *  This contains code for the active scan finite state machine.
 *
 *  @copyright Copyright 2014-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */


// Xilinx SDK includes
#include "xparameters.h"
#include "stdio.h"
#include "stdlib.h"
#include "xtmrctr.h"
#include "xio.h"
#include "string.h"
#include "xintc.h"

// WLAN includes
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_scan.h"

//Default Scan Channels
// u8 default_channel_selections[11] = {1,2,3,4,5,6,7,8,9,10,11}; //Scan only 2.4GHz channels
u8 default_channel_selections[15] = {1,2,3,4,5,6,7,8,9,10,11,36,40,44,48}; //Scan 2.4GHz and 5GHz channels

typedef enum {IDLE, RUNNING, PAUSED} scan_state_t;

volatile 	function_ptr_t		tx_probe_request_callback;
volatile 	scan_parameters_t 	gl_scan_parameters;		//Note: this variable needs to be treated as volatile since it
														//is expected to be modified by other contexts after a call to
														//wlan_mac_scan_get_parameters;

static 		u8                 	channel_save;
static 		s8                 	curr_scan_chan_idx;

static 		u32                	scan_sched_id;
static		u32					probe_sched_id;
static 		scan_state_t       	scan_state;


int wlan_mac_scan_init(){

	tx_probe_request_callback = (function_ptr_t)wlan_null_callback;

	gl_scan_parameters.channel_vec 				= wlan_mac_high_malloc(sizeof(default_channel_selections));
	if(gl_scan_parameters.channel_vec != NULL){
		memcpy(gl_scan_parameters.channel_vec, default_channel_selections, sizeof(default_channel_selections));
		gl_scan_parameters.channel_vec_len = sizeof(default_channel_selections);
	}

	gl_scan_parameters.probe_tx_interval_usec   = 20000;
	gl_scan_parameters.time_per_channel_usec	= 150000;
	gl_scan_parameters.ssid						= strdup("");

	scan_sched_id  = SCHEDULE_ID_RESERVED_MAX;
	probe_sched_id = SCHEDULE_ID_RESERVED_MAX;
	scan_state     = IDLE;

	return 0;
}

void wlan_mac_scan_set_tx_probe_request_callback(function_ptr_t callback){
	tx_probe_request_callback = callback;
}

volatile scan_parameters_t* wlan_mac_scan_get_parameters(){
	return &gl_scan_parameters;
}


void wlan_mac_scan_start(){
	if( (scan_state == IDLE) ){
		channel_save = wlan_mac_high_get_channel();
		curr_scan_chan_idx = -1;
		scan_state = RUNNING;
		wlan_mac_scan_state_transition();
	}
	return;
}

void wlan_mac_scan_pause(){
	//We do not offer a way to resume the scanner. To restart the scanner,
	//call wlan_mac_scan_stop() to transition through the IDLE state and
	//then call wlan_mac_scan_start().
	if( (scan_state == RUNNING) ){
		if(scan_sched_id != SCHEDULE_ID_RESERVED_MAX){
			wlan_mac_remove_schedule(SCHEDULE_FINE, scan_sched_id);
			scan_sched_id  = SCHEDULE_ID_RESERVED_MAX;
		}
		if(probe_sched_id != SCHEDULE_ID_RESERVED_MAX){
			wlan_mac_remove_schedule(SCHEDULE_FINE, probe_sched_id);
			probe_sched_id = SCHEDULE_ID_RESERVED_MAX;
		}
		scan_state = PAUSED;
	}
}

void wlan_mac_scan_stop(){
	if( (scan_state == RUNNING)||(scan_state == PAUSED) ){
		if(scan_sched_id != SCHEDULE_ID_RESERVED_MAX){
			wlan_mac_remove_schedule(SCHEDULE_FINE, scan_sched_id);
			scan_sched_id  = SCHEDULE_ID_RESERVED_MAX;
		}
		if(probe_sched_id != SCHEDULE_ID_RESERVED_MAX){
			wlan_mac_remove_schedule(SCHEDULE_FINE, probe_sched_id);
			probe_sched_id = SCHEDULE_ID_RESERVED_MAX;
		}
		wlan_mac_high_set_channel(channel_save);
		curr_scan_chan_idx = -1;
		scan_state = IDLE;
	}
}

u32 wlan_mac_scan_is_scanning(){
	if( (scan_state == RUNNING)||(scan_state == PAUSED) ){
		return 1;
	} else {
		return 0;
	}
}

void wlan_mac_scan_state_transition(){

	if(probe_sched_id != SCHEDULE_ID_RESERVED_MAX){
		wlan_mac_remove_schedule(SCHEDULE_FINE, probe_sched_id);
		probe_sched_id = SCHEDULE_ID_RESERVED_MAX;
	}

	curr_scan_chan_idx = (curr_scan_chan_idx+1)%(gl_scan_parameters.channel_vec_len);

	wlan_mac_high_set_channel(gl_scan_parameters.channel_vec[(u8)curr_scan_chan_idx]);
	tx_probe_request_callback();
	probe_sched_id = wlan_mac_schedule_event_repeated(SCHEDULE_FINE, gl_scan_parameters.probe_tx_interval_usec, SCHEDULE_REPEAT_FOREVER, (void*)tx_probe_request_callback);
	if(scan_sched_id == SCHEDULE_ID_RESERVED_MAX){
		scan_sched_id = wlan_mac_schedule_event_repeated(SCHEDULE_FINE, gl_scan_parameters.time_per_channel_usec, SCHEDULE_REPEAT_FOREVER, (void*)wlan_mac_scan_state_transition);
	}
}
