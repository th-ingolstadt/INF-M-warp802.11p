//Xilinx SDK includes
#include "xparameters.h"
#include "stdio.h"
#include "stdlib.h"
#include "xtmrctr.h"
#include "xio.h"
#include "string.h"
#include "xintc.h"

//WARP includes
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_sta_scan_fsm.h"
#include "wlan_mac_sta.h"

#define NUM_SCAN_CHANNELS 23
static u8 channels[NUM_SCAN_CHANNELS] = {1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,52,56,60,64,149,153,157,161}; //NA allowed channels
static u32 idle_timeout_usec = 1000000;
static u32 dwell_timeout_usec = 200000;
static s8 curr_scan_chan_idx;
static char scan_ssid[SSID_LEN_MAX + 1];
static u8 scan_bssid[6];
static u32 scan_sched_id = SCHEDULE_FAILURE;
static u8 channel_save;

extern u8 pause_data_queue;
extern u32 mac_param_chan; ///< This is the "home" channel
extern mac_header_80211_common tx_header_common;
extern tx_params default_multicast_mgmt_tx_params;

typedef enum {SCAN_DISABLED, SCAN_ENABLED} scan_state_t;
static scan_state_t scan_state = SCAN_DISABLED;

void wlan_mac_sta_scan_enable(u8* bssid, char* ssid_str){
	memcpy(scan_bssid, bssid, 6);
	strcpy(scan_ssid, ssid_str);
	if(scan_state == SCAN_DISABLED){
		channel_save = mac_param_chan;
		wlan_mac_sta_scan_state_transition();
	}
}

void wlan_mac_sta_scan_disable(){
	wlan_mac_high_interrupt_stop();
	if(scan_state == SCAN_ENABLED){
		if(scan_sched_id != SCHEDULE_FAILURE){
			wlan_mac_remove_schedule(SCHEDULE_COARSE, scan_sched_id);
			scan_state = SCAN_DISABLED;
			mac_param_chan = channel_save;
			//xil_printf("Returning to Chan: %d\n", mac_param_chan);
			wlan_mac_high_set_channel(mac_param_chan);
			pause_data_queue = 0;
			curr_scan_chan_idx = -1;
		} else {
			xil_printf("ERROR: Active scan currently enabled, but no schedule ID found\n");
		}
	}
	wlan_mac_high_interrupt_start();
}

void wlan_mac_sta_scan_state_transition(){
	switch(scan_state){
		case SCAN_DISABLED:
			pause_data_queue = 1;
			scan_state = SCAN_ENABLED;
			curr_scan_chan_idx = -1;
			wlan_mac_sta_scan_state_transition();
		break;
		case SCAN_ENABLED:
			curr_scan_chan_idx++;
			if(curr_scan_chan_idx < NUM_SCAN_CHANNELS){
				mac_param_chan = channels[(u8)curr_scan_chan_idx];
				//xil_printf("%d: Chan %d\n", curr_scan_chan_idx, mac_param_chan);
				wlan_mac_high_set_channel(mac_param_chan);
				wlan_mac_sta_scan_probe_req_transmit();
				scan_sched_id = wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, dwell_timeout_usec, 1, (void*)wlan_mac_sta_scan_state_transition);
			} else {
				mac_param_chan = channel_save;
				//xil_printf("Returning to Chan: %d\n", mac_param_chan);
				wlan_mac_high_set_channel(mac_param_chan);
				pause_data_queue = 0;
				curr_scan_chan_idx = -1;
				scan_sched_id = wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, idle_timeout_usec, 1, (void*)wlan_mac_sta_scan_state_transition);
			}
		break;
	}
}

void wlan_mac_sta_scan_probe_req_transmit(){
	u16                 tx_length;
	tx_queue_element*	curr_tx_queue_element;
	tx_queue_buffer* 	curr_tx_queue_buffer;

	// Send probe request
	curr_tx_queue_element = queue_checkout();

	if(curr_tx_queue_element != NULL){
		curr_tx_queue_buffer = (tx_queue_buffer*)(curr_tx_queue_element->data);

		// Setup the TX header
		wlan_mac_high_setup_tx_header( &tx_header_common, (u8 *)bcast_addr, (u8 *)scan_bssid );

		// Fill in the data
		tx_length = wlan_create_probe_req_frame((void*)(curr_tx_queue_buffer->frame),&tx_header_common, strlen(scan_ssid), (u8*)scan_ssid, mac_param_chan);

		// Setup the TX frame info
		wlan_mac_high_setup_tx_frame_info ( &tx_header_common, curr_tx_queue_element, tx_length, 0, MANAGEMENT_QID );

		(tx_header_common.seq_num)++; //increment the sequence number

		// Set the information in the TX queue buffer
		curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
		curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)(&default_multicast_mgmt_tx_params);
		curr_tx_queue_buffer->frame_info.AID         = 0;

		// Put the packet in the queue
		enqueue_after_tail(MANAGEMENT_QID, curr_tx_queue_element);

	    // Poll the TX queues to possibly send the packet
		poll_tx_queues();
	}
}

