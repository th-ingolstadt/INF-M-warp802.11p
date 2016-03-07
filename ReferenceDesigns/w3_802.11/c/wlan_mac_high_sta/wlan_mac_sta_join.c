/** @file wlan_mac_sta_join.c
 *  @brief Join FSM
 *
 *  This contains code for the STA join process.
 *
 *  @copyright Copyright 2014-2015, Mango Communications. All rights reserved.
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
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_scan.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_bss_info.h"
#include "wlan_mac_sta_join.h"
#include "wlan_mac_sta.h"


typedef enum {IDLE, SEARCHING, ATTEMPTING} join_state_t;

//External Global Variables:
extern mac_header_80211_common    tx_header_common;
extern u8                         pause_data_queue;
extern u32                        mac_param_chan; ///< This is the "home" channel
extern tx_params_t                default_unicast_mgmt_tx_params;
extern bss_info*				  my_bss_info;
extern u8						  my_aid;

// File Variables
static join_state_t               join_state;
static function_ptr_t             join_success_callback;

volatile join_parameters_t		  gl_join_parameters;

static bss_info*                  attempt_bss_info;

char*							  ssid_save;

static u32                        search_sched_id;
static u32                        attempt_sched_id;

void wlan_mac_sta_join_init(){
	join_state = IDLE;
	join_success_callback = (function_ptr_t)wlan_null_callback;

	bzero((u8*)gl_join_parameters.bssid, 6);
	gl_join_parameters.ssid = NULL;
	gl_join_parameters.channel = 0;
	attempt_bss_info = NULL;
	ssid_save = NULL;

	search_sched_id = SCHEDULE_ID_RESERVED_MAX;
	attempt_sched_id = SCHEDULE_ID_RESERVED_MAX;
}

volatile join_parameters_t* wlan_mac_sta_get_join_parameters(){
	return &gl_join_parameters;
}

void wlan_mac_sta_set_join_success_callback(function_ptr_t callback){
	join_success_callback = callback;
}

int wlan_mac_is_joining(){
	if( join_state == IDLE ){
		return 0;
	} else {
		return 1;
	}
}


/**
 * @brief Join an AP
 *
 * This function begin an attempt to join an AP
 * given the parameters present in the join_parameters_t
 * struct pointed to by the return of wlan_mac_sta_get_join_parameters.
 *
 * A value of NULL for the SSID element in join_parameters_t indicates
 * that the function should halt any ongoing attempts to join an AP.
 */
void wlan_mac_sta_join(){
	u8								zero_addr[6] 	= {0,0,0,0,0,0};
	volatile scan_parameters_t* 	scan_parameters = wlan_mac_scan_get_parameters();

	//Save the existing scan parameters SSID so we can revert after the join has finished
	ssid_save = strdup(scan_parameters->ssid);
	wlan_mac_high_free(scan_parameters->ssid);
	scan_parameters->ssid = strdup(gl_join_parameters.ssid);

	if(wlan_mac_is_joining()){
		// The join state machine is already running. We should remove any ongoing
		// scheduled events and move on because the user has already overwritten
		// gl_join_parameters from underneath us.
		wlan_mac_sta_return_to_idle();
		wlan_mac_sta_join(); //Re-enter this function, this time with an idle state machine.
		return;
	}

	if(gl_join_parameters.ssid == NULL){
		wlan_mac_sta_return_to_idle();
	} else {

		if( wlan_addr_eq(gl_join_parameters.bssid, zero_addr) || (gl_join_parameters.channel == 0) ){
			//BSSID not provided. We need to start an active scan and find the AP
			//before attempting authentication and association.
			join_state = SEARCHING;

			wlan_mac_scan_start();
			search_sched_id = wlan_mac_schedule_event_repeated(SCHEDULE_FINE, BSS_SEARCH_POLL_INTERVAL_USEC, SCHEDULE_REPEAT_FOREVER, (void*)wlan_mac_sta_bss_search_poll);

		} else {
			//BSSID and channel are provided. We can look for / create this bss_info and skip
			//directly to authentication.
			join_state = ATTEMPTING;

			wlan_mac_scan_stop();

			if(my_bss_info != NULL){
				if(wlan_addr_eq(gl_join_parameters.bssid, my_bss_info->bssid)){
					//We're already associated with this BSS
					wlan_mac_sta_return_to_idle();
					return;
				} else {
					//We're associated with a different BSS. We will
					//disassociate and then continue with the new join operation.
					sta_disassociate();
				}

			}


			attempt_bss_info = wlan_mac_high_create_bss_info((u8*)gl_join_parameters.bssid,
															 (char*)gl_join_parameters.ssid,
															 (u8)gl_join_parameters.channel);
			wlan_mac_sta_join_l();
		}

	}
}







//Low-Level functions

void wlan_mac_sta_bss_search_poll(u32 schedule_id){
	dl_list* ssid_match_list = NULL;
	dl_entry* curr_dl_entry = NULL;

	switch(join_state){
		case IDLE:
			xil_printf("JOIN FSM Error: Searching/Idle mismatch\n");
		break;
		case SEARCHING:
			if(wlan_mac_scan_get_num_scans() > 0){
				ssid_match_list = wlan_mac_high_find_bss_info_SSID(gl_join_parameters.ssid);
				if(ssid_match_list->length > 0){
					curr_dl_entry = ssid_match_list->first;
					wlan_mac_sta_return_to_idle();
					attempt_bss_info = (bss_info*)(curr_dl_entry->data);
					join_state = ATTEMPTING;
					wlan_mac_sta_join_l();
				}
			}
		break;
		case ATTEMPTING:
			xil_printf("Join FSM Error: Searching/Attempting mismatch\n");
		break;
	}
}

void wlan_mac_sta_join_l(){
	if(attempt_bss_info == NULL){
		xil_printf("Join FSM Error: low-level join called without setting attempt_bss_info\n");
		return;
	}

	pause_data_queue = 1;
	mac_param_chan = attempt_bss_info->chan;
	wlan_mac_high_set_channel(mac_param_chan);
	wlan_mac_sta_bss_attempt_poll(0);
	attempt_sched_id = wlan_mac_schedule_event_repeated(SCHEDULE_FINE, BSS_ATTEMPT_POLL_INTERVAL_USEC, SCHEDULE_REPEAT_FOREVER, (void*)wlan_mac_sta_bss_attempt_poll);
}


void wlan_mac_sta_return_to_idle(){

	volatile scan_parameters_t* 	scan_parameters = wlan_mac_scan_get_parameters();
	interrupt_state_t 				prev_interrupt_state;


	wlan_mac_scan_stop();
	prev_interrupt_state = wlan_mac_high_interrupt_stop();
	join_state = IDLE;
	if(search_sched_id != SCHEDULE_ID_RESERVED_MAX){
		wlan_mac_remove_schedule(SCHEDULE_FINE, search_sched_id);
		search_sched_id = SCHEDULE_ID_RESERVED_MAX;
	}
	if(attempt_sched_id != SCHEDULE_ID_RESERVED_MAX){
		wlan_mac_remove_schedule(SCHEDULE_FINE, attempt_sched_id);
		attempt_sched_id = SCHEDULE_ID_RESERVED_MAX;
	}
	wlan_mac_high_interrupt_restore_state(prev_interrupt_state);

	attempt_bss_info = NULL;
	// Return the scan SSID parameter back to its previous state
	wlan_mac_high_free(scan_parameters->ssid);
	scan_parameters->ssid = strdup(ssid_save);
	wlan_mac_high_free(ssid_save);
}

void wlan_mac_sta_bss_attempt_poll(u32 arg){
	// Depending on who is calling this function, the argument means different things
	// When called by the framework, the argument is automatically filled in with the schedule ID
	// There are two other contexts in which this function is called:
	//	(1) STA will call this when it receives an authentication packet that elevates the state
	//		to BSS_AUTHENTICATED. In this context, the argument is meaningless (explicitly 0 valued)
	//	(2) STA will call this when it receives an association response that elevates the state to
	//		BSS_ASSOCIATED. In this context, the argument will be the AID provided by the AP sending
	//		the association response.
	bss_config_t bss_config;

	switch(join_state){
		case IDLE:
			xil_printf("JOIN FSM Error: Attempting/Idle mismatch\n");
		break;
		case SEARCHING:
			xil_printf("JOIN FSM Error: Attempting/Searching mismatch\n");
		break;
		case ATTEMPTING:

			if(attempt_bss_info->last_join_attempt_result == DENIED){
				wlan_mac_sta_return_to_idle();
				//TODO: call join() again to move on to the next matching SSID
				return;
			}

			switch(attempt_bss_info->state){
				case BSS_STATE_UNAUTHENTICATED:
					wlan_mac_sta_scan_auth_transmit();
				break;

				case BSS_STATE_AUTHENTICATED:
					wlan_mac_sta_scan_assoc_req_transmit();
				break;

				case BSS_STATE_ASSOCIATED:
					my_aid = arg;
					memcpy(bss_config.bssid, attempt_bss_info->bssid, 6);
					strcpy(bss_config.ssid, attempt_bss_info->ssid);
					bss_config.chan = attempt_bss_info->chan;
					bss_config.ht_capable = 1;
					bss_config.beacon_interval = attempt_bss_info->beacon_interval;
					bss_config.update_mask = (BSS_FIELD_MASK_BSSID  		 |
											  BSS_FIELD_MASK_CHAN   		 |
											  BSS_FIELD_MASK_SSID			 |
											  BSS_FIELD_MASK_BEACON_INTERVAL |
											  BSS_FIELD_MASK_HT_CAPABLE);

					if(configure_bss(&bss_config) == 0){
						//Important: return_to_idle will NULL out attempt_bss_info,
						//so it should not be called before actually setting the
						//association state
						join_success_callback(attempt_bss_info);		//FIXME: Is this callback needed any longer? the wlan_exp join process is now non-blocking
					}
					wlan_mac_sta_return_to_idle();
				break;

				default:
					xil_printf("Error: STA attempt poll: Unknown state %d for BSS info %s\n",attempt_bss_info->state, attempt_bss_info->ssid);
				break;
			}
		break;
	}

}

void wlan_mac_sta_scan_auth_transmit(){
	u16                 tx_length;
	tx_queue_element*	curr_tx_queue_element;
	tx_queue_buffer* 	curr_tx_queue_buffer;

	if(join_state == ATTEMPTING){

		// Send authentication request
		curr_tx_queue_element = queue_checkout();

		if(curr_tx_queue_element != NULL){
			curr_tx_queue_buffer = (tx_queue_buffer*)(curr_tx_queue_element->data);

			// Setup the TX header
			wlan_mac_high_setup_tx_header( &tx_header_common, attempt_bss_info->bssid, attempt_bss_info->bssid );

			// Fill in the data
			tx_length = wlan_create_auth_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, AUTH_ALGO_OPEN_SYSTEM, AUTH_SEQ_REQ, STATUS_SUCCESS);

			// Setup the TX frame info
			wlan_mac_high_setup_tx_frame_info (&tx_header_common, curr_tx_queue_element, tx_length, (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO), MANAGEMENT_QID );

			// Set the information in the TX queue buffer
			curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
			curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)(&default_unicast_mgmt_tx_params);
			curr_tx_queue_buffer->frame_info.AID         = 0;

			// Put the packet in the queue
			enqueue_after_tail(MANAGEMENT_QID, curr_tx_queue_element);

			// Poll the TX queues to possibly send the packet
			poll_tx_queues();

		}
	}
}

void wlan_mac_sta_scan_assoc_req_transmit(){
	u16                 tx_length;
	tx_queue_element*	curr_tx_queue_element;
	tx_queue_buffer* 	curr_tx_queue_buffer;

	if(join_state == ATTEMPTING){

		// Send authentication request
		curr_tx_queue_element = queue_checkout();

		if(curr_tx_queue_element != NULL){
			curr_tx_queue_buffer = (tx_queue_buffer*)(curr_tx_queue_element->data);

			// Setup the TX header
			wlan_mac_high_setup_tx_header( &tx_header_common, attempt_bss_info->bssid, attempt_bss_info->bssid );

			// Fill in the data
			tx_length = wlan_create_association_req_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, attempt_bss_info);

			// Setup the TX frame info
			wlan_mac_high_setup_tx_frame_info (&tx_header_common, curr_tx_queue_element, tx_length, (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO), MANAGEMENT_QID );

			// Set the information in the TX queue buffer
			curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
			curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)(&default_unicast_mgmt_tx_params);
			curr_tx_queue_buffer->frame_info.AID         = 0;

			// Put the packet in the queue
			enqueue_after_tail(MANAGEMENT_QID, curr_tx_queue_element);

			// Poll the TX queues to possibly send the packet
			poll_tx_queues();
		}
	}
}


