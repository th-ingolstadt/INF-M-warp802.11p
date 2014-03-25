/** @file wlan_mac_ap.c
 *  @brief Access Point
 *
 *  This contains code for the 802.11 Access Point.
 *
 *  @copyright Copyright 2014, Mango Communications. All rights reserved.
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

//Xilinx SDK includes
#include "xparameters.h"
#include "stdio.h"
#include "stdlib.h"
#include "xtmrctr.h"
#include "xio.h"
#include "string.h"
#include "xintc.h"

//WARP includes
#include "wlan_mac_addr_filter.h"
#include "wlan_mac_ipc_util.h"
#include "wlan_mac_misc_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_entries.h"
#include "wlan_mac_ap.h"
#include "ascii_characters.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_dl_list.h"

// WLAN Exp includes
#include "wlan_exp.h"
#include "wlan_exp_common.h"
#include "wlan_exp_node.h"
#include "wlan_exp_node_ap.h"
#include "wlan_exp_transport.h"


/*************************** Constant Definitions ****************************/

#define  WLAN_EXP_ETH                  WN_ETH_B
#define  WLAN_EXP_TYPE                 WARPNET_TYPE_80211_BASE + WARPNET_TYPE_80211_HIGH_AP

#define  WLAN_CHANNEL                  4
#define  TX_POWER_DBM				   10


/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/

// SSID variables
static char default_AP_SSID[] = "WARP-AP";
char*       access_point_ssid;

// Common TX header for 802.11 packets
mac_header_80211_common tx_header_common;

// Control variables
u8 allow_assoc;
u8 perma_assoc_mode;

// Default Transmission Parameters;
tx_params default_unicast_mgmt_tx_params;
tx_params default_unicast_data_tx_params;
tx_params default_multicast_mgmt_tx_params;
tx_params default_multicast_data_tx_params;

// Association table variable
dl_list		 association_table;
dl_list		 statistics_table;

// Tx queue variables;
u32			 max_queue_size;

// AP channel
u32 		 mac_param_chan;

// MAC address
static u8 eeprom_mac_addr[6];

// Misc
u32 animation_schedule_id;

u8 tim_bitmap[1] = {0x0};
u8 tim_control = 1;


/*************************** Functions Prototypes ****************************/


#ifdef WLAN_USE_UART_MENU

void uart_rx(u8 rxByte);

#else

void uart_rx(u8 rxByte){ };

#endif


/******************************** Functions **********************************/


int main(){

	xil_printf("\f----- wlan_mac_ap -----\n");
	xil_printf("Compiled %s %s\n", __DATE__, __TIME__);

	xil_printf("sizeof(station_info) = %d\n", sizeof(station_info));
	xil_printf("sizeof(station_info_base) = %d\n", sizeof(station_info_base));

	//This function should be executed first. It will zero out memory, and if that
	//memory is used before calling this function, unexpected results may happen.
	wlan_mac_high_heap_init();
	wlan_mac_high_init();

    // Set Global variables
	perma_assoc_mode     = 0;

	default_unicast_data_tx_params.mac.num_tx_max = MAX_NUM_TX;
	default_unicast_data_tx_params.phy.power = TX_POWER_DBM;
	default_unicast_data_tx_params.phy.rate = WLAN_MAC_RATE_18M;
	default_unicast_data_tx_params.phy.antenna_mode = WLAN_TX_ANTMODE_SISO_ANTA;

	default_unicast_mgmt_tx_params.mac.num_tx_max = MAX_NUM_TX;
	default_unicast_mgmt_tx_params.phy.power = TX_POWER_DBM;
	default_unicast_mgmt_tx_params.phy.rate = WLAN_MAC_RATE_6M;
	default_unicast_mgmt_tx_params.phy.antenna_mode = WLAN_TX_ANTMODE_SISO_ANTA;

	default_multicast_data_tx_params.mac.num_tx_max = 1;
	default_multicast_data_tx_params.phy.power = TX_POWER_DBM;
	default_multicast_data_tx_params.phy.rate = WLAN_MAC_RATE_18M;
	default_multicast_data_tx_params.phy.antenna_mode = WLAN_TX_ANTMODE_SISO_ANTA;

	default_multicast_mgmt_tx_params.mac.num_tx_max = 1;
	default_multicast_mgmt_tx_params.phy.power = TX_POWER_DBM;
	default_multicast_mgmt_tx_params.phy.rate = WLAN_MAC_RATE_6M;
	default_multicast_mgmt_tx_params.phy.antenna_mode = WLAN_TX_ANTMODE_SISO_ANTA;


#ifdef USE_WARPNET_WLAN_EXP
	node_info_set_max_assn( MAX_NUM_ASSOC );
	node_info_set_max_stats( MAX_NUM_PROMISC_STATS );
	wlan_exp_configure(WLAN_EXP_TYPE, WLAN_EXP_ETH);
#endif

	dl_list_init(&association_table);
	dl_list_init(&statistics_table);

	max_queue_size = min((queue_total_size()- eth_bd_total_size()) / (association_table.length+1),MAX_PER_FLOW_QUEUE);

	// Initialize callbacks
	wlan_mac_util_set_eth_rx_callback(       (void*)ethernet_receive);
	wlan_mac_high_set_mpdu_tx_done_callback( (void*)mpdu_transmit_done);
	wlan_mac_high_set_mpdu_rx_callback(      (void*)mpdu_rx_process);
	wlan_mac_high_set_pb_u_callback(         (void*)up_button);

	wlan_mac_high_set_uart_rx_callback(      (void*)uart_rx);
	wlan_mac_high_set_mpdu_accept_callback(  (void*)check_tx_queue);
    wlan_mac_ltg_sched_set_callback(         (void*)ltg_event);

    wlan_mac_util_set_eth_encap_mode(ENCAP_MODE_AP);

    // Wait for CPU Low to initialize
	while( wlan_mac_high_is_cpu_low_initialized() == 0 ){
		xil_printf("waiting on CPU_LOW to boot\n");
	};

	// CPU Low will pass HW information to CPU High as part of the boot process
	//   - Get necessary HW information
	memcpy((void*) &(eeprom_mac_addr[0]), (void*) wlan_mac_high_get_eeprom_mac_addr(), 6);

    // Set Header information
	tx_header_common.address_2 = &(eeprom_mac_addr[0]);
	tx_header_common.seq_num   = 0;

    // Initialize hex display
	wlan_mac_high_write_hex_display(0);

	// Set up channel
	mac_param_chan = WLAN_CHANNEL;
	wlan_mac_high_set_channel( mac_param_chan );

	// Set SSID
	access_point_ssid = wlan_mac_high_malloc(strlen(default_AP_SSID)+1);
	strcpy(access_point_ssid,default_AP_SSID);

	// Initialize interrupts
	wlan_mac_high_interrupt_init();

    // Schedule all events
	wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, BEACON_INTERVAL_US, SCHEDULE_REPEAT_FOREVER, (void*)beacon_transmit);
	wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, ASSOCIATION_CHECK_INTERVAL_US, SCHEDULE_REPEAT_FOREVER, (void*)association_timestamp_check);

	animation_schedule_id = wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, ANIMATION_RATE_US, SCHEDULE_REPEAT_FOREVER, (void*)animate_hex);

	enable_associations( ASSOCIATION_ALLOW_PERMANENT );

	// Reset the event log
	event_log_reset();

	// Print AP information to the terminal
    xil_printf("WLAN MAC AP boot complete: \n");
    xil_printf("  SSID    : %s \n", access_point_ssid);
    xil_printf("  Channel : %d \n", mac_param_chan);
	xil_printf("  MAC Addr: %02x-%02x-%02x-%02x-%02x-%02x\n\n",eeprom_mac_addr[0],eeprom_mac_addr[1],eeprom_mac_addr[2],eeprom_mac_addr[3],eeprom_mac_addr[4],eeprom_mac_addr[5]);


#ifdef WLAN_USE_UART_MENU
	xil_printf("\nAt any time, press the Esc key in your terminal to access the AP menu\n");
#endif

#ifdef USE_WARPNET_WLAN_EXP
	// Set AP processing callbacks
	node_set_process_callback( (void *)wlan_exp_node_ap_processCmd );
#endif

	wlan_mac_high_interrupt_start();

	while(1){
		//The design is entirely interrupt based. When no events need to be processed, the processor
		//will spin in this loop until an interrupt happens
#ifdef USE_WARPNET_WLAN_EXP
//		wlan_mac_high_interrupt_stop();
		transport_poll( WLAN_EXP_ETH );
//		wlan_mac_high_interrupt_start();
#endif
	}
	return -1;
}

void check_tx_queue(){
	u32 i,k;
	#define NUM_QUEUE_GROUPS 2
	typedef enum {MGMT_QGRP, DATA_QGRP} queue_group_t;

	static queue_group_t next_queue_group = MGMT_QGRP;
	queue_group_t curr_queue_group;

	static dl_entry* next_station_info_entry = NULL;
	dl_entry* curr_station_info_entry;

	station_info* curr_station_info;

	if( wlan_mac_high_is_cpu_low_ready() ){

		for(k = 0; k < NUM_QUEUE_GROUPS; k++){

			curr_queue_group = next_queue_group;

			switch(curr_queue_group){
				case MGMT_QGRP:
					next_queue_group = DATA_QGRP;
					if(wlan_mac_queue_poll(MANAGEMENT_QID)){
						return;
					}
				break;
				case DATA_QGRP:
					next_queue_group = MGMT_QGRP;
					curr_station_info_entry = next_station_info_entry;

						for(i = 0; i < (association_table.length + 1) ; i++){
							//Loop through all associated stations' queues + the broadcast queue
							if(curr_station_info_entry == NULL){
								//Check the broadcast queue
								next_station_info_entry = association_table.first;
								if(wlan_mac_queue_poll(MCAST_QID)){
									return;
								} else {
									curr_station_info_entry = next_station_info_entry;
								}
							} else {
								curr_station_info = (station_info*)(curr_station_info_entry->data);
								if( wlan_mac_high_is_valid_association(&association_table, curr_station_info) ){
									if(curr_station_info_entry == association_table.last){
										//We've reached the end of the table, so we wrap around to the beginning
										next_station_info_entry = NULL;
									} else {
										next_station_info_entry = dl_entry_next(curr_station_info_entry);
									}

									if(wlan_mac_queue_poll(AID_TO_QID(curr_station_info->AID))){
										return;
									} else {
										curr_station_info_entry = next_station_info_entry;
									}
								} else {
									//This curr_station_info is invalid. Perhaps it was removed from
									//the association table before check_tx_queue was called. We will
									//start the round robin checking back at broadcast.
									//xil_printf("isn't getting here\n");
									next_station_info_entry = NULL;
									return;
								}
							}
						}
				break;
			}
		}
	}
}

void purge_all_data_tx_queue(){
	u32 i;
	dl_entry*	  curr_station_info_entry;
	station_info* curr_station_info;

	// Purge all data transmit queues
	purge_queue(MCAST_QID);                                    		// Broadcast Queue
	curr_station_info_entry = association_table.first;

	for(i=0; i < association_table.length; i++){
		curr_station_info = (station_info*)(curr_station_info_entry->data);
		purge_queue(AID_TO_QID(curr_station_info->AID));       		// Each unicast queue
		curr_station_info_entry = dl_entry_next(curr_station_info_entry);
	}
}


void mpdu_transmit_done(tx_frame_info* tx_mpdu, wlan_mac_low_tx_details* tx_low_details){
	u32 i;
	tx_high_entry* tx_high_event_log_entry;
	tx_low_entry*  tx_low_event_log_entry;
	station_info* station;
	dl_entry*	  entry;
	u8 			  pkt_type;

	frame_statistics_txrx* frame_stats = NULL;

	void* mpdu = (u8*)tx_mpdu + PHY_TX_PKT_BUF_MPDU_OFFSET;
	u8* mpdu_ptr_u8 = (u8*)mpdu;
	mac_header_80211* tx_80211_header;
	tx_80211_header = (mac_header_80211*)((void *)mpdu_ptr_u8);
	u32 ts_old = 0;
	u32 payload_log_len;
	u32 total_payload_len = max(tx_mpdu->length + sizeof(mac_header_80211) , MAX_MAC_PAYLOAD_LOG_LEN);

	pkt_type = wlan_mac_high_pkt_type(mpdu,tx_mpdu->length);

	for(i = 0; i < tx_mpdu->num_tx; i++){

		tx_low_event_log_entry = get_next_empty_tx_low_entry();
		if(tx_low_event_log_entry != NULL){
			tx_low_event_log_entry->mac_payload_log_len = sizeof(mac_header_80211);
			wlan_mac_high_cdma_start_transfer((&((tx_low_entry*)tx_low_event_log_entry)->mac_payload), tx_80211_header, sizeof(mac_header_80211));
			tx_low_event_log_entry->transmission_count        = i+1;
			tx_low_event_log_entry->timestamp_send            = (u64)(  tx_mpdu->timestamp_create + (u64)(tx_mpdu->delay_accept) + (u64)(tx_low_details[i].tx_start_delta) + ts_old);
			tx_low_event_log_entry->chan_num                  = tx_low_details[i].chan_num;
			tx_low_event_log_entry->num_slots				  = tx_low_details[i].num_slots;
			memcpy((&((tx_low_entry*)tx_low_event_log_entry)->phy_params), &(tx_low_details[i].phy_params), sizeof(phy_tx_params));
			tx_low_event_log_entry->length                    = tx_mpdu->length;
			tx_low_event_log_entry->pkt_type				  = wlan_mac_high_pkt_type(mpdu,tx_mpdu->length);
			wlan_mac_high_cdma_finish_transfer();

			if(i==0){
				//This is the first transmission
				((mac_header_80211*)(tx_low_event_log_entry->mac_payload))->frame_control_2 &= ~MAC_FRAME_CTRL2_FLAG_RETRY;
			} else {
				//This is all subsequent transmissions
				((mac_header_80211*)(tx_low_event_log_entry->mac_payload))->frame_control_2 |= MAC_FRAME_CTRL2_FLAG_RETRY;
			}

		}

		ts_old = tx_low_details[i].tx_start_delta;
	}




	payload_log_len = max( 1 + ( ( ( total_payload_len ) - 1) / 4 ) , MAX_MAC_PAYLOAD_LOG_LEN );
	tx_high_event_log_entry = get_next_empty_tx_high_entry(payload_log_len);

	if(tx_high_event_log_entry != NULL){
		tx_high_event_log_entry->mac_payload_log_len = total_payload_len;
		wlan_mac_high_cdma_start_transfer((&((tx_high_entry*)tx_high_event_log_entry)->mac_payload), tx_80211_header, total_payload_len);

		tx_high_event_log_entry->result                   = tx_mpdu->state_verbose;
		tx_high_event_log_entry->power              	  = tx_mpdu->params.phy.power;
		tx_high_event_log_entry->length                   = tx_mpdu->length;
		tx_high_event_log_entry->rate                     = tx_mpdu->params.phy.rate;
		tx_high_event_log_entry->chan_num				  = mac_param_chan;
		tx_high_event_log_entry->pkt_type				  = pkt_type;
		tx_high_event_log_entry->num_tx                   = tx_mpdu->num_tx;
		tx_high_event_log_entry->timestamp_create         = tx_mpdu->timestamp_create;
		tx_high_event_log_entry->delay_accept             = tx_mpdu->delay_accept;
		tx_high_event_log_entry->delay_done               = tx_mpdu->delay_done;
		tx_high_event_log_entry->ant_mode				  = tx_mpdu->params.phy.antenna_mode;
	}

	if(tx_mpdu->AID != 0){
		entry = wlan_mac_high_find_station_info_AID(&association_table, tx_mpdu->AID);
		if(entry != NULL){
			station = (station_info*)(entry->data);

			switch(pkt_type){
				case PKT_TYPE_DATA_ENCAP_ETH:
				case PKT_TYPE_DATA_ENCAP_LTG:
					frame_stats = &(station->stats->data);
				break;

				case PKT_TYPE_MGMT:
					frame_stats = &(station->stats->mgmt);
				break;
			}


			//Update Transmission Stats
			if(frame_stats != NULL){



				(frame_stats->tx_num_packets_total)++;
				(frame_stats->tx_num_bytes_total) += tx_mpdu->length;

				(frame_stats->tx_num_packets_low) += (tx_mpdu->num_tx);

				if((tx_mpdu->state_verbose) == TX_MPDU_STATE_VERBOSE_SUCCESS){
					(frame_stats->tx_num_packets_success)++;
					(frame_stats->tx_num_bytes_success) += tx_mpdu->length;
				}

			}

		}
	}

	if (tx_high_event_log_entry != NULL) {
        //wn_transmit_log_entry((void *)tx_high_event_log_entry);
	}
}




void up_button(){

	switch ( get_associations_status() ) {

        case ASSOCIATION_ALLOW_NONE:
    		// AP is currently not allowing any associations to take place
        	animation_schedule_id = wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, ANIMATION_RATE_US, SCHEDULE_REPEAT_FOREVER, (void*)animate_hex);
    		enable_associations( ASSOCIATION_ALLOW_TEMPORARY );
    		wlan_mac_schedule_event(SCHEDULE_COARSE,ASSOCIATION_ALLOW_INTERVAL_US, (void*)disable_associations);
        break;

        case ASSOCIATION_ALLOW_TEMPORARY:
    		// AP is currently allowing associations, but only for the small allow window.
    		//   Go into permanent allow association mode.
    		enable_associations( ASSOCIATION_ALLOW_PERMANENT );
    		xil_printf("Allowing associations indefinitely\n");
        break;

        case ASSOCIATION_ALLOW_PERMANENT:
    		// AP is permanently allowing associations. Toggle everything off.
    		enable_associations( ASSOCIATION_ALLOW_TEMPORARY );
    		disable_associations();
        break;
	}

}



void ltg_event(u32 id, void* callback_arg){
	dl_list checkout;
	dl_entry* tx_queue_entry;
	tx_queue_buffer* tx_queue;
	u32 tx_length;
	u32 payload_length = 0;
	u8* mpdu_ptr_u8;
	llc_header* llc_hdr;
	dl_entry*	  station_info_entry;
	station_info* station;
	u8* addr_da = ((ltg_pyld_hdr*)callback_arg)->addr_da;

	switch(((ltg_pyld_hdr*)callback_arg)->type){
		case LTG_PYLD_TYPE_FIXED:
			payload_length = ((ltg_pyld_fixed*)callback_arg)->length;
		break;
		case LTG_PYLD_TYPE_UNIFORM_RAND:
			payload_length = (rand()%(((ltg_pyld_uniform_rand*)(callback_arg))->max_length - ((ltg_pyld_uniform_rand*)(callback_arg))->min_length))+((ltg_pyld_uniform_rand*)(callback_arg))->min_length;
		break;
		default:
		break;

	}

	station_info_entry = wlan_mac_high_find_station_info_ADDR(&association_table, addr_da);

	if(station_info_entry != NULL){
		station = (station_info*)(station_info_entry->data);

		if(queue_num_queued(AID_TO_QID(station->AID)) < max_queue_size){
			//Send a Data packet to this station
			//Checkout 1 element from the queue;
			queue_checkout(&checkout,1);

			if(checkout.length == 1){ //There was at least 1 free queue element
				tx_queue_entry = checkout.first;

				tx_queue = ((tx_queue_buffer*)(tx_queue_entry->data));

				wlan_mac_high_setup_tx_header( &tx_header_common, station->addr, eeprom_mac_addr );



				mpdu_ptr_u8 = (u8*)(tx_queue->frame);
				tx_length = wlan_create_data_frame((void*)mpdu_ptr_u8, &tx_header_common, MAC_FRAME_CTRL2_FLAG_FROM_DS);

				mpdu_ptr_u8 += sizeof(mac_header_80211);
				llc_hdr = (llc_header*)(mpdu_ptr_u8);

				//Prepare the MPDU LLC header
				llc_hdr->dsap = LLC_SNAP;
				llc_hdr->ssap = LLC_SNAP;
				llc_hdr->control_field = LLC_CNTRL_UNNUMBERED;
				bzero((void *)(llc_hdr->org_code), 3); //Org Code 0x000000: Encapsulated Ethernet
				llc_hdr->type = LLC_TYPE_CUSTOM;

				tx_length += sizeof(llc_header);
				tx_length += payload_length;

				wlan_mac_high_setup_tx_frame_info ( tx_queue_entry, tx_length, (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO) );

				tx_queue->metadata.metadata_type = QUEUE_METADATA_TYPE_STATION_INFO;
				tx_queue->metadata.metadata_ptr = (u32)station;
				tx_queue->frame_info.AID = station->AID;


				enqueue_after_end(AID_TO_QID(station->AID), &checkout);
				check_tx_queue();
			}
		}
	}
}



int ethernet_receive(dl_list* tx_queue_list, u8* eth_dest, u8* eth_src, u16 tx_length){
	//Receives the pre-encapsulated Ethernet frames
	station_info* station;
	dl_entry* entry;

	dl_entry* tx_queue_entry = tx_queue_list->first;
	tx_queue_buffer* tx_queue = (tx_queue_buffer*)(tx_queue_entry->data);

	wlan_mac_high_setup_tx_header( &tx_header_common, (u8*)(&(eth_dest[0])), (u8*)(&(eth_src[0])) );

	wlan_create_data_frame((void*)(tx_queue->frame), &tx_header_common, MAC_FRAME_CTRL2_FLAG_FROM_DS);

	if(wlan_addr_mcast(eth_dest)){
		if(queue_num_queued(MCAST_QID) < max_queue_size){
			wlan_mac_high_setup_tx_frame_info ( tx_queue_entry, tx_length, 0 );

			tx_queue->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
			tx_queue->metadata.metadata_ptr = (u32)&(default_multicast_data_tx_params);

			enqueue_after_end(MCAST_QID, tx_queue_list);
			check_tx_queue();
		} else {
			return 0;
		}

	} else {
		//Check associations
		//Is this packet meant for a station we are associated with?
		entry = wlan_mac_high_find_station_info_ADDR(&association_table, eth_dest);
		if( entry != NULL ) {
			station = (station_info*)(entry->data);
			if(queue_num_queued(AID_TO_QID(station->AID)) < max_queue_size){
				wlan_mac_high_setup_tx_frame_info ( tx_queue_entry, tx_length, (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO) );

				tx_queue->metadata.metadata_type = QUEUE_METADATA_TYPE_STATION_INFO;
				tx_queue->metadata.metadata_ptr = (u32)station;
				tx_queue->frame_info.AID = station->AID;


				enqueue_after_end(AID_TO_QID(station->AID), tx_queue_list);
				check_tx_queue();
			} else {
				return 0;
			}
		} else {
			//Checkin this packet_bd so that it can be checked out again
			return 0;
		}


	}

	return 1;

}



void beacon_transmit() {
 	u16 tx_length;
 	dl_list checkout;
 	dl_entry*	tx_queue_entry;

 	tx_queue_buffer* tx_queue;

 	//Checkout 1 element from the queue;
 	queue_checkout(&checkout,1);

 	if(checkout.length == 1){ //There was at least 1 free queue element
 		tx_queue_entry = checkout.first;

 		tx_queue = (tx_queue_buffer*)(tx_queue_entry->data);

 		wlan_mac_high_setup_tx_header( &tx_header_common, (u8 *)bcast_addr, eeprom_mac_addr );
        tx_length = wlan_create_beacon_frame((void*)(tx_queue->frame),&tx_header_common, BEACON_INTERVAL_MS, strlen(access_point_ssid), (u8*)access_point_ssid, mac_param_chan,1,tim_control,tim_bitmap);

 		wlan_mac_high_setup_tx_frame_info ( tx_queue_entry, tx_length, TX_MPDU_FLAGS_FILL_TIMESTAMP );

 		tx_queue->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
 		tx_queue->metadata.metadata_ptr = (u32)(&default_multicast_mgmt_tx_params);

 		enqueue_after_end(MANAGEMENT_QID, &checkout);



 		check_tx_queue();
 	}

 	return;
}


void association_timestamp_check() {

	u32 i;
	u64 time_since_last_rx;
	dl_list checkout;
	dl_entry* tx_queue_entry;
	tx_queue_buffer* tx_queue;


	u32 tx_length;
	station_info* curr_station_info;
	dl_entry* next_station_info_entry;
	dl_entry* curr_station_info_entry;

	next_station_info_entry = association_table.first;

	for(i=0; i < association_table.length; i++) {
		curr_station_info_entry = next_station_info_entry;
		next_station_info_entry = dl_entry_next(curr_station_info_entry);

		curr_station_info = (station_info*)(curr_station_info_entry->data);

		time_since_last_rx = (get_usec_timestamp() - curr_station_info->rx.last_timestamp);
		if((time_since_last_rx > ASSOCIATION_TIMEOUT_US) && ((curr_station_info->flags & STATION_INFO_FLAG_DISABLE_ASSOC_CHECK) == 0)){
			//Send De-authentication

		 	//Checkout 1 element from the queue;
		 	queue_checkout(&checkout,1);

		 	if(checkout.length == 1){ //There was at least 1 free queue element
		 		tx_queue_entry = checkout.first;

		 		tx_queue = (tx_queue_buffer*)(tx_queue_entry->data);

		 		wlan_mac_high_setup_tx_header( &tx_header_common, curr_station_info->addr, eeprom_mac_addr );

		 		tx_length = wlan_create_deauth_frame((void*)(tx_queue->frame), &tx_header_common, DEAUTH_REASON_INACTIVITY);

		 		wlan_mac_high_setup_tx_frame_info ( tx_queue_entry, tx_length, (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO) );

		 		tx_queue->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
		 		tx_queue->metadata.metadata_ptr = (u32)(&default_unicast_mgmt_tx_params);

		 		enqueue_after_end(MANAGEMENT_QID, &checkout);
		 		check_tx_queue();

		 		//Purge any packets in the queue meant for this node
		 		purge_queue(AID_TO_QID(curr_station_info->AID));

				//Remove this STA from association list
				xil_printf("\n\nDisassociation due to inactivity:\n");
				wlan_mac_high_remove_association( &association_table, &statistics_table, curr_station_info->addr );
			}
		}
	}
	return;
}




void mpdu_rx_process(void* pkt_buf_addr, u8 rate, u16 length) {
	void * mpdu = pkt_buf_addr + PHY_RX_PKT_BUF_MPDU_OFFSET;
	u8* mpdu_ptr_u8 = (u8*)mpdu;
	u16 tx_length;
	u8 send_response;
	mac_header_80211* rx_80211_header;
	rx_80211_header = (mac_header_80211*)((void *)mpdu_ptr_u8);
	u16 rx_seq;
	dl_list checkout;
	dl_entry*	tx_queue_entry;
	tx_queue_buffer* tx_queue;
	u32 payload_log_len;
	u32 total_payload_len = max(length + sizeof(mac_header_80211) , MAX_MAC_PAYLOAD_LOG_LEN);


	dl_entry*	associated_station_entry;
	station_info* associated_station = NULL;
	statistics_txrx* station_stats = NULL;
	u8 eth_send;
	u8 allow_auth = 0;


	rx_common_entry* rx_event_log_entry;

	rx_frame_info* mpdu_info = (rx_frame_info*)pkt_buf_addr;

	typedef enum {PAYLOAD_FIRST, CHAN_EST_FIRST} copy_order_t;
	copy_order_t copy_order;

	mpdu_info->additional_info = (u32)NULL;

	//*************
	// Event logging
	//*************

	//Determine length required for p
	payload_log_len = max( 1 + ( ( ( total_payload_len ) - 1) / 4 ) , MAX_MAC_PAYLOAD_LOG_LEN );

	if(rate != WLAN_MAC_RATE_1M){
		rx_event_log_entry = (rx_common_entry*)get_next_empty_rx_ofdm_entry(payload_log_len);
	} else {
		rx_event_log_entry = (rx_common_entry*)get_next_empty_rx_dsss_entry(payload_log_len);
	}

	if(rx_event_log_entry != NULL){

		//For maximum pipelining, we'll break up the two major log copy operations (packet payload + [optional] channel estimates)
		//We will start the CDMA operation for whichever of those copies is shorter, then fill in the rest of the log entry
		//while that copy is under way, and then start the CDMA operation for the larger (which will first block on the shorter if
		//it is still going).

		if(rate == WLAN_MAC_RATE_1M){
			//This is a DSSS packet that has no channel estimates
			copy_order = PAYLOAD_FIRST;
		} else {
			//This is an OFDM packet that contains channel estimates
#ifdef WLAN_MAC_ENTRIES_LOG_CHAN_EST
			if( sizeof(mpdu_info->channel_est) < ( total_payload_len ) ){
				copy_order = CHAN_EST_FIRST;
			} else {
				copy_order = PAYLOAD_FIRST;
			}
#else
			copy_order = PAYLOAD_FIRST;
#endif
		}


		switch(copy_order){
			case PAYLOAD_FIRST:
				//wlan_mac_high_cdma_start_transfer(&(rx_event_log_entry->mac_hdr), rx_80211_header, sizeof(mac_header_80211));
				if( rate != WLAN_MAC_RATE_1M ){
					((rx_ofdm_entry*)rx_event_log_entry)->mac_payload_log_len = total_payload_len;
					wlan_mac_high_cdma_start_transfer((((rx_ofdm_entry*)rx_event_log_entry)->mac_payload), rx_80211_header, total_payload_len);
				} else {
					((rx_dsss_entry*)rx_event_log_entry)->mac_payload_log_len = total_payload_len;
					wlan_mac_high_cdma_start_transfer((((rx_dsss_entry*)rx_event_log_entry)->mac_payload), rx_80211_header, total_payload_len);
				}
			break;

			case CHAN_EST_FIRST:
#ifdef WLAN_MAC_ENTRIES_LOG_CHAN_EST
				if(rate != WLAN_MAC_RATE_1M) wlan_mac_high_cdma_start_transfer(((rx_ofdm_entry*)rx_event_log_entry)->channel_est, mpdu_info->channel_est, sizeof(mpdu_info->channel_est));
#endif
			break;
		}

		rx_event_log_entry->fcs_status = (mpdu_info->state == RX_MPDU_STATE_FCS_GOOD) ? RX_ENTRY_FCS_GOOD : RX_ENTRY_FCS_BAD;
		rx_event_log_entry->timestamp  =  mpdu_info->timestamp;
		rx_event_log_entry->power      = mpdu_info->rx_power;
		rx_event_log_entry->rf_gain    = mpdu_info->rf_gain;
		rx_event_log_entry->bb_gain    = mpdu_info->bb_gain;
		rx_event_log_entry->length     = mpdu_info->length;
		rx_event_log_entry->rate       = mpdu_info->rate;
		rx_event_log_entry->pkt_type   = wlan_mac_high_pkt_type(mpdu,length);
		rx_event_log_entry->chan_num   = mac_param_chan;
		rx_event_log_entry->ant_mode   = mpdu_info->ant_mode;

		switch(copy_order){
			case CHAN_EST_FIRST:
				//wlan_mac_high_cdma_start_transfer(&(rx_event_log_entry->mac_hdr), rx_80211_header, sizeof(mac_header_80211));
				if( rate != WLAN_MAC_RATE_1M ){
					((rx_ofdm_entry*)rx_event_log_entry)->mac_payload_log_len = total_payload_len;
					wlan_mac_high_cdma_start_transfer((((rx_ofdm_entry*)rx_event_log_entry)->mac_payload), rx_80211_header, total_payload_len);
				} else {
					((rx_dsss_entry*)rx_event_log_entry)->mac_payload_log_len = total_payload_len;
					wlan_mac_high_cdma_start_transfer((((rx_dsss_entry*)rx_event_log_entry)->mac_payload), rx_80211_header, total_payload_len);
				}
			break;

			case PAYLOAD_FIRST:
#ifdef WLAN_MAC_ENTRIES_LOG_CHAN_EST
				if(rate != WLAN_MAC_RATE_1M) wlan_mac_high_cdma_start_transfer(((rx_ofdm_entry*)rx_event_log_entry)->channel_est, mpdu_info->channel_est, sizeof(mpdu_info->channel_est));
#endif
			break;
		}

	}

	if(mpdu_info->state == RX_MPDU_STATE_FCS_GOOD){
		//GOOD FCS
		associated_station_entry = wlan_mac_high_find_station_info_ADDR(&association_table, (rx_80211_header->address_2));

		if( associated_station_entry != NULL ){
			associated_station = (station_info*)(associated_station_entry->data);
			mpdu_info->additional_info = (u32)associated_station;
			station_stats = associated_station->stats;
			rx_seq = ((rx_80211_header->sequence_control)>>4)&0xFFF;

			associated_station->rx.last_timestamp = get_usec_timestamp();
			associated_station->rx.last_power = mpdu_info->rx_power;
			associated_station->rx.last_rate = mpdu_info->rate;

			//Check if duplicate
			if( (associated_station->rx.last_seq != 0)  && (associated_station->rx.last_seq == rx_seq) ) {
				//Received seq num matched previously received seq num for this STA; ignore the MPDU and finish function
				goto mpdu_rx_process_end;

			} else {
				associated_station->rx.last_seq = rx_seq;
			}
		} else {
			station_stats = wlan_mac_high_add_statistics(&statistics_table, NULL, rx_80211_header->address_2);
		}

		if(station_stats != NULL){
			station_stats->last_rx_timestamp = get_usec_timestamp();
			if((rx_80211_header->frame_control_1 & 0xF) == MAC_FRAME_CTRL1_TYPE_DATA){
				((station_stats)->data.rx_num_packets)++;
				((station_stats)->data.rx_num_bytes) += mpdu_info->length;
			} else if((rx_80211_header->frame_control_1 & 0xF) == MAC_FRAME_CTRL1_TYPE_MGMT) {
				((station_stats)->mgmt.rx_num_packets)++;
				((station_stats)->mgmt.rx_num_bytes) += mpdu_info->length;
			}
		}

		switch(rx_80211_header->frame_control_1) {
			case (MAC_FRAME_CTRL1_SUBTYPE_DATA): //Data Packet
				if(associated_station != NULL){
					if((rx_80211_header->frame_control_2) & MAC_FRAME_CTRL2_FLAG_TO_DS) {
						//MPDU is flagged as destined to the DS
						eth_send = 1;
						if(wlan_addr_mcast(rx_80211_header->address_3)){
								queue_checkout(&checkout,1);

								if(checkout.length == 1){ //There was at least 1 free queue element
									tx_queue_entry = checkout.first;
									tx_queue = (tx_queue_buffer*)(tx_queue_entry->data);

									wlan_mac_high_setup_tx_header( &tx_header_common, rx_80211_header->address_3, rx_80211_header->address_2);
									mpdu_ptr_u8 = tx_queue->frame;
									tx_length = wlan_create_data_frame((void*)mpdu_ptr_u8, &tx_header_common, MAC_FRAME_CTRL2_FLAG_FROM_DS);
									mpdu_ptr_u8 += sizeof(mac_header_80211);
									memcpy(mpdu_ptr_u8, (void*)rx_80211_header + sizeof(mac_header_80211), mpdu_info->length - sizeof(mac_header_80211));
									wlan_mac_high_setup_tx_frame_info ( tx_queue_entry, mpdu_info->length, 0 );

									tx_queue->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
									tx_queue->metadata.metadata_ptr = (u32)(&default_multicast_data_tx_params);

									enqueue_after_end(MCAST_QID, &checkout);
									check_tx_queue();
								}

						} else {
							associated_station_entry = wlan_mac_high_find_station_info_ADDR(&association_table, rx_80211_header->address_3);

							if(associated_station_entry != NULL){
								associated_station = (station_info*)(associated_station_entry->data);
								queue_checkout(&checkout,1);

								if(checkout.length == 1){ //There was at least 1 free queue element
									tx_queue_entry = checkout.first;
									tx_queue = (tx_queue_buffer*)(tx_queue_entry->data);

									wlan_mac_high_setup_tx_header( &tx_header_common, rx_80211_header->address_3, rx_80211_header->address_2);
									mpdu_ptr_u8 = tx_queue->frame;
									tx_length = wlan_create_data_frame((void*)mpdu_ptr_u8, &tx_header_common, MAC_FRAME_CTRL2_FLAG_FROM_DS);
									mpdu_ptr_u8 += sizeof(mac_header_80211);
									memcpy(mpdu_ptr_u8, (void*)rx_80211_header + sizeof(mac_header_80211), mpdu_info->length - sizeof(mac_header_80211));
									wlan_mac_high_setup_tx_frame_info ( tx_queue_entry, mpdu_info->length , (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO) );

									tx_queue->metadata.metadata_type = QUEUE_METADATA_TYPE_STATION_INFO;
									tx_queue->metadata.metadata_ptr = (u32)(associated_station);
									tx_queue->frame_info.AID = associated_station->AID;

									enqueue_after_end(AID_TO_QID(associated_station->AID),  &checkout);

									check_tx_queue();
									#ifndef ALLOW_ETH_TX_OF_WIRELESS_TX
									eth_send = 0;
									#endif
								}
							}
						}

						if(eth_send){
							wlan_mpdu_eth_send(mpdu,length);
						}

					}
				} else {
					//TODO: Formally adopt conventions from 10.3 in 802.11-2012 for STA state transitions
					if(wlan_addr_eq(rx_80211_header->address_1, eeprom_mac_addr)){

						//Received a data frame from a STA that claims to be associated with this AP but is not in the AP association table
						// Discard the MPDU and reply with a de-authentication frame to trigger re-association at the STA

						warp_printf(PL_WARNING, "Data from non-associated station: [%x %x %x %x %x %x], issuing de-authentication\n", rx_80211_header->address_2[0],rx_80211_header->address_2[1],rx_80211_header->address_2[2],rx_80211_header->address_2[3],rx_80211_header->address_2[4],rx_80211_header->address_2[5]);
						warp_printf(PL_WARNING, "Address 3: [%x %x %x %x %x %x]\n", rx_80211_header->address_3[0],rx_80211_header->address_3[1],rx_80211_header->address_3[2],rx_80211_header->address_3[3],rx_80211_header->address_3[4],rx_80211_header->address_3[5]);

						//Send De-authentication
						//Checkout 1 element from the queue;
							queue_checkout(&checkout,1);
							if(checkout.length == 1){ //There was at least 1 free queue element
								tx_queue_entry = checkout.first;
								tx_queue = (tx_queue_buffer*)(tx_queue_entry->data);

								wlan_mac_high_setup_tx_header( &tx_header_common, rx_80211_header->address_2, eeprom_mac_addr );
								tx_length = wlan_create_deauth_frame((void*)(((tx_queue_buffer*)(tx_queue_entry->data))->frame), &tx_header_common, DEAUTH_REASON_NONASSOCIATED_STA);
								wlan_mac_high_setup_tx_frame_info ( tx_queue_entry, tx_length, (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO) );

								tx_queue->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
								tx_queue->metadata.metadata_ptr = (u32)(&default_unicast_mgmt_tx_params);

								enqueue_after_end(MANAGEMENT_QID, &checkout);
								check_tx_queue();
							}

					}
				}//END if(associated_station != NULL)

			break;

			case (MAC_FRAME_CTRL1_SUBTYPE_PROBE_REQ): //Probe Request Packet
				if(wlan_addr_eq(rx_80211_header->address_3, bcast_addr)) {
					//BSS Id: Broadcast
					mpdu_ptr_u8 += sizeof(mac_header_80211);
					while(((u32)mpdu_ptr_u8 -  (u32)mpdu)<= length){ //Loop through tagged parameters
						switch(mpdu_ptr_u8[0]){ //What kind of tag is this?
							case TAG_SSID_PARAMS: //SSID parameter set
								if((mpdu_ptr_u8[1]==0) || (memcmp(mpdu_ptr_u8+2, (u8*)access_point_ssid,mpdu_ptr_u8[1])==0)) {
									//Broadcast SSID or my SSID - send unicast probe response
									send_response = 1;
								}
							break;
							case TAG_SUPPORTED_RATES: //Supported rates
							break;
							case TAG_EXT_SUPPORTED_RATES: //Extended supported rates
							break;
							case TAG_DS_PARAMS: //DS Parameter set (e.g. channel)
							break;
						}
						mpdu_ptr_u8 += mpdu_ptr_u8[1]+2; //Move up to the next tag
					}

					//xil_printf("Probe Req Rx %d %d\n", send_response, allow_assoc);

					if(send_response && allow_assoc) {

						//Checkout 1 element from the queue;
						queue_checkout(&checkout,1);

						if(checkout.length == 1){ //There was at least 1 free queue element
							tx_queue_entry = checkout.first;
							tx_queue = (tx_queue_buffer*)(tx_queue_entry->data);

							wlan_mac_high_setup_tx_header( &tx_header_common, rx_80211_header->address_2, eeprom_mac_addr );

							tx_length = wlan_create_probe_resp_frame((void*)(tx_queue->frame), &tx_header_common, BEACON_INTERVAL_MS, strlen(access_point_ssid), (u8*)access_point_ssid, mac_param_chan);

							wlan_mac_high_setup_tx_frame_info ( tx_queue_entry, tx_length, (TX_MPDU_FLAGS_FILL_TIMESTAMP | TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO) );

							tx_queue->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
							tx_queue->metadata.metadata_ptr = (u32)(&default_unicast_mgmt_tx_params);

							enqueue_after_end(MANAGEMENT_QID, &checkout);
							check_tx_queue();
							//xil_printf("Probe Response Tx\n");
						}
						// Finish function
						goto mpdu_rx_process_end;
					}
				}
			break;

			case (MAC_FRAME_CTRL1_SUBTYPE_AUTH): //Authentication Packet
				if(wlan_addr_eq(rx_80211_header->address_3, eeprom_mac_addr) && wlan_mac_addr_filter_is_allowed(rx_80211_header->address_2)) {
					mpdu_ptr_u8 += sizeof(mac_header_80211);
					switch(((authentication_frame*)mpdu_ptr_u8)->auth_algorithm ){
						case AUTH_ALGO_OPEN_SYSTEM:
							allow_auth = 1;
						break;
						default:
							allow_auth = 0;
						break;
					}
				}

				if(allow_auth){
					if(((authentication_frame*)mpdu_ptr_u8)->auth_sequence == AUTH_SEQ_REQ){//This is an auth packet from a requester
						//Checkout 1 element from the queue;
						queue_checkout(&checkout,1);

						if(checkout.length == 1){ //There was at least 1 free queue element
							tx_queue_entry = checkout.first;
							tx_queue = (tx_queue_buffer*)(tx_queue_entry->data);

							wlan_mac_high_setup_tx_header( &tx_header_common, rx_80211_header->address_2, eeprom_mac_addr );

							tx_length = wlan_create_auth_frame((void*)(tx_queue->frame), &tx_header_common, AUTH_ALGO_OPEN_SYSTEM, AUTH_SEQ_RESP, STATUS_SUCCESS);

							wlan_mac_high_setup_tx_frame_info ( tx_queue_entry, tx_length, (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO) );

							tx_queue->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
							tx_queue->metadata.metadata_ptr = (u32)(&default_unicast_mgmt_tx_params);

							enqueue_after_end(MANAGEMENT_QID, &checkout);
							check_tx_queue();
						}
						goto mpdu_rx_process_end;
					}
				} else {
					if(((authentication_frame*)mpdu_ptr_u8)->auth_sequence == AUTH_SEQ_REQ){
						//Checkout 1 element from the queue;
						queue_checkout(&checkout,1);

						if(checkout.length == 1){ //There was at least 1 free queue element
							tx_queue_entry = checkout.first;
							tx_queue = (tx_queue_buffer*)(tx_queue_entry->data);

							wlan_mac_high_setup_tx_header( &tx_header_common, rx_80211_header->address_2, eeprom_mac_addr );

							tx_length = wlan_create_auth_frame((void*)(tx_queue->frame), &tx_header_common, AUTH_ALGO_OPEN_SYSTEM, AUTH_SEQ_RESP, STATUS_AUTH_REJECT_UNSPECIFIED);

							wlan_mac_high_setup_tx_frame_info ( tx_queue_entry, tx_length, (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO) );

							tx_queue->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
							tx_queue->metadata.metadata_ptr = (u32)(&default_unicast_mgmt_tx_params);

							enqueue_after_end(MANAGEMENT_QID, &checkout);
							check_tx_queue();
						}
					}
					// Finish function
					goto mpdu_rx_process_end;
				}
			break;

			case (MAC_FRAME_CTRL1_SUBTYPE_REASSOC_REQ): //Re-association Request
			case (MAC_FRAME_CTRL1_SUBTYPE_ASSOC_REQ): //Association Request

				if(wlan_addr_eq(rx_80211_header->address_3, eeprom_mac_addr)) {


					if(association_table.length < MAX_NUM_ASSOC) associated_station = wlan_mac_high_add_association(&association_table, &statistics_table, rx_80211_header->address_2, ADD_ASSOCIATION_ANY_AID);

					if(associated_station != NULL) {

						memcpy(&(associated_station->tx),&default_unicast_data_tx_params, sizeof(tx_params));

						//Checkout 1 element from the queue;
						queue_checkout(&checkout,1);

						if(checkout.length == 1){ //There was at least 1 free queue element
							tx_queue_entry = checkout.first;
							tx_queue = (tx_queue_buffer*)(tx_queue_entry->data);

							wlan_mac_high_setup_tx_header( &tx_header_common, rx_80211_header->address_2, eeprom_mac_addr );

							tx_length = wlan_create_association_response_frame((void*)(tx_queue->frame), &tx_header_common, STATUS_SUCCESS, associated_station->AID);

							wlan_mac_high_setup_tx_frame_info ( tx_queue_entry, tx_length, (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO) );

							tx_queue->metadata.metadata_type = QUEUE_METADATA_TYPE_STATION_INFO;
							tx_queue->metadata.metadata_ptr = (u32)associated_station;
							tx_queue->frame_info.AID = associated_station->AID;

							enqueue_after_end(AID_TO_QID(associated_station->AID), &checkout);
							check_tx_queue();
						}
						// Finish function
						goto mpdu_rx_process_end;
					} else {
						//Checkout 1 element from the queue;
						queue_checkout(&checkout,1);

						if(checkout.length == 1){ //There was at least 1 free queue element
							tx_queue_entry = checkout.first;
							tx_queue = (tx_queue_buffer*)(tx_queue_entry->data);

							wlan_mac_high_setup_tx_header( &tx_header_common, rx_80211_header->address_2, eeprom_mac_addr );

							tx_length = wlan_create_association_response_frame((void*)(tx_queue->frame), &tx_header_common, STATUS_REJECT_TOO_MANY_ASSOCIATIONS, 0);

							wlan_mac_high_setup_tx_frame_info ( tx_queue_entry, tx_length, (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO) );

							tx_queue->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
							tx_queue->metadata.metadata_ptr = (u32)(&default_unicast_mgmt_tx_params);

							enqueue_after_end(MANAGEMENT_QID, &checkout);
							check_tx_queue();
						}
					}

				}
			break;

			case (MAC_FRAME_CTRL1_SUBTYPE_DISASSOC): //Disassociation

					wlan_mac_high_remove_association(&association_table, &statistics_table, rx_80211_header->address_2);

			break;

			default:
				//This should be left as a verbose print. It occurs often when communicating with mobile devices since they tend to send
				//null data frames (type: DATA, subtype: 0x4) for power management reasons.
				warp_printf(PL_VERBOSE, "Received unknown frame control type/subtype %x\n",rx_80211_header->frame_control_1);
			break;
		}
		goto mpdu_rx_process_end;

	} else {
		//Bad FCS
		goto mpdu_rx_process_end;
	}
	mpdu_rx_process_end:
	if (rx_event_log_entry != NULL) {
		//wn_transmit_log_entry((void *)rx_event_log_entry);
	}
}

u32  get_associations_status() {
	// Get the status of associations for the AP
	//   - 00 -> Associations not allowed
	//   - 01 -> Associations allowed for a window
	//   - 11 -> Associations allowed permanently

	return ( perma_assoc_mode * 2 ) + allow_assoc;
}

void enable_associations( u32 permanent_association ){
	// Send a message to other processor to tell it to enable associations
#ifdef _DEBUG_
	xil_printf("Allowing new associations\n");
#endif

	// Set the DSSS value in CPU Low
	wlan_mac_high_set_dsss( 1 );

    // Set the global variable
	allow_assoc = 1;

	// Set the global variable for permanently allowing associations
	switch ( permanent_association ) {

        case ASSOCIATION_ALLOW_PERMANENT:
        	perma_assoc_mode = 1;
        break;

        case ASSOCIATION_ALLOW_TEMPORARY:
        	perma_assoc_mode = 0;
        break;
	}
}



void disable_associations(){
	// Send a message to other processor to tell it to disable associations
	if(perma_assoc_mode == 0){

#ifdef _DEBUG_
		xil_printf("Not allowing new associations\n");
#endif

		// Set the DSSS value in CPU Low
		wlan_mac_high_set_dsss( 0 );

        // Set the global variables
		allow_assoc      = 0;

		// Stop the animation on the hex displays from continuing
		wlan_mac_remove_schedule(SCHEDULE_COARSE, animation_schedule_id);

		// Set the hex display
		wlan_mac_high_write_hex_display(association_table.length);
		wlan_mac_high_write_hex_display_dots(0);
	}
}

void animate_hex(){
	static u8 i = 0;
	//wlan_mac_high_write_hex_display(next_free_assoc_index,i%2);
	wlan_mac_high_write_hex_display_dots(i%2);
	i++;
}


/*****************************************************************************/
/**
* Reset Station Statistics
*
* Reset all statistics being kept for all stations
*
* @param    None.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void reset_station_statistics(){
	wlan_mac_high_reset_statistics(&statistics_table);
}


u32  deauthenticate_station( station_info* station ) {

	dl_list checkout;
	dl_entry*     tx_queue_entry;
	tx_queue_buffer* tx_queue;
	u32            tx_length;
	u32            aid;

	if(station == NULL){
		return 0;
	}

	// Get the AID
	aid = station->AID;

	// Checkout 1 element from the queue
	queue_checkout(&checkout,1);

	if(checkout.length == 1){ //There was at least 1 free queue element
		tx_queue_entry = checkout.first;
		tx_queue = (tx_queue_buffer*)(tx_queue_entry->data);

		purge_queue(AID_TO_QID(aid));

		// Create deauthentication packet
		wlan_mac_high_setup_tx_header( &tx_header_common, station->addr, eeprom_mac_addr );

		tx_length = wlan_create_deauth_frame((void*)(tx_queue->frame), &tx_header_common, DEAUTH_REASON_INACTIVITY);

		wlan_mac_high_setup_tx_frame_info ( tx_queue_entry, tx_length, (TX_MPDU_FLAGS_FILL_DURATION | TX_MPDU_FLAGS_REQ_TO) );

		tx_queue->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
		tx_queue->metadata.metadata_ptr = (u32)(&default_unicast_mgmt_tx_params);

		enqueue_after_end(MANAGEMENT_QID, &checkout);
		check_tx_queue();

		// Remove this STA from association list
		wlan_mac_high_remove_association( &association_table, &statistics_table, station->addr );
	}

	wlan_mac_high_write_hex_display(association_table.length);

	return aid;
}



/*****************************************************************************/
/**
* Deauthenticate all stations in the Association Table
*
* Loop through all associations in the table and deauthenticate the stations
*
* @param    None.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void deauthenticate_stations(){
	u32 i;
	station_info* curr_station_info;
	dl_entry* next_station_info_entry;
	dl_entry* curr_station_info_entry;

	next_station_info_entry = association_table.first;
	for (i = 0; i < association_table.length ; i++){
		curr_station_info_entry = next_station_info_entry;
		next_station_info_entry = dl_entry_next(curr_station_info_entry);
		curr_station_info = (station_info*)(curr_station_info_entry->data);
		deauthenticate_station(curr_station_info);
	}
}

dl_list * get_statistics(){
	return &statistics_table;
}

dl_list * get_station_info_list(){
	return &association_table;
}
