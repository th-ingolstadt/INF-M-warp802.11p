////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_ap.c
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

//Xilinx SDK includes
#include "xparameters.h"
#include "stdio.h"
#include "stdlib.h"
#include "xtmrctr.h"
#include "xio.h"
#include "string.h"
#include "xintc.h"

//WARP includes
#include "wlan_mac_ipc_util.h"
#include "wlan_mac_misc_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_util.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_ap.h"
#include "ascii_characters.h"


#ifdef WLAN_USE_UART_MENU

extern u8 interactive_mode;
extern u8 default_unicast_rate;
extern u32 mac_param_chan;

extern station_info associations[MAX_ASSOCIATIONS+1];
extern u32 next_free_assoc_index;



void uart_rx(u8 rxByte){
	wlan_ipc_msg ipc_msg_to_low;
	u32 ipc_msg_to_low_payload[1];
	ipc_config_rf_ifc* config_rf_ifc;
	u32 i;

    static u8 ltg_mode = 0;
	cbr_params cbr_parameters;
    
	if(rxByte == ASCII_ESC){
		interactive_mode = 0;
		print_menu();
		return;
	}

	if(interactive_mode){
		switch(rxByte){
			case ASCII_r:
				//Reset statistics
				reset_station_statistics();
			break;
			case ASCII_d:
				//Deauthenticate all stations
				deauthenticate_stations();
			break;
		}
	} else {
		switch(rxByte){
			case ASCII_1:
				interactive_mode = 1;
				print_station_status();
			break;

			case ASCII_2:
				print_queue_status();
			break;

			case ASCII_e:
				print_event_log();
			break;

			case ASCII_c:

				if(mac_param_chan > 1){
					deauthenticate_stations();
					(mac_param_chan--);

					//Send a message to other processor to tell it to switch channels
					ipc_msg_to_low.msg_id = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_RF_IFC);
					ipc_msg_to_low.num_payload_words = sizeof(ipc_config_rf_ifc)/sizeof(u32);
					ipc_msg_to_low.payload_ptr = &(ipc_msg_to_low_payload[0]);
					init_ipc_config(config_rf_ifc,ipc_msg_to_low_payload,ipc_config_rf_ifc);
					config_rf_ifc->channel = mac_param_chan;
					ipc_mailbox_write_msg(&ipc_msg_to_low);
				} else {

				}
				xil_printf("(-) Channel: %d\n", mac_param_chan);


			break;
			case ASCII_C:
				if(mac_param_chan < 11){
					deauthenticate_stations();
					(mac_param_chan++);

					//Send a message to other processor to tell it to switch channels
					ipc_msg_to_low.msg_id = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_RF_IFC);
					ipc_msg_to_low.num_payload_words = sizeof(ipc_config_rf_ifc)/sizeof(u32);
					ipc_msg_to_low.payload_ptr = &(ipc_msg_to_low_payload[0]);
					init_ipc_config(config_rf_ifc,ipc_msg_to_low_payload,ipc_config_rf_ifc);
					config_rf_ifc->channel = mac_param_chan;
					ipc_mailbox_write_msg(&ipc_msg_to_low);
				} else {

				}
				xil_printf("(+) Channel: %d\n", mac_param_chan);

			break;
			case ASCII_r:
				if(default_unicast_rate > WLAN_MAC_RATE_6M){
					default_unicast_rate--;
				} else {
					default_unicast_rate = WLAN_MAC_RATE_6M;
				}

				for(i=0; i < next_free_assoc_index; i++){
					associations[i].tx_rate = default_unicast_rate;
				}

				xil_printf("(-) Default Unicast Rate: %d Mbps\n", wlan_lib_mac_rate_to_mbps(default_unicast_rate));
			break;
			case ASCII_R:
				if(default_unicast_rate < WLAN_MAC_RATE_54M){
					default_unicast_rate++;
				} else {
					default_unicast_rate = WLAN_MAC_RATE_54M;
				}

				for(i=0; i < next_free_assoc_index; i++){
					associations[i].tx_rate = default_unicast_rate;
				}
				xil_printf("(+) Default Unicast Rate: %d Mbps\n", wlan_lib_mac_rate_to_mbps(default_unicast_rate));
			break;
			case ASCII_l:
				if(ltg_mode == 0){
					#define LTG_INTERVAL 10000
					xil_printf("Enabling LTG mode to AID 1, interval = %d usec\n", LTG_INTERVAL);
					cbr_parameters.interval_usec = LTG_INTERVAL; //Time between calls to the packet generator in usec. 0 represents backlogged... go as fast as you can.
					start_ltg(1, LTG_TYPE_CBR, &cbr_parameters);

					ltg_mode = 1;

				} else {
					stop_ltg(1);
					ltg_mode = 0;
					xil_printf("Disabled LTG mode to AID 1\n");
				}
			break;
        }
	}
}


void print_queue_status(){
	u32 i;
	xil_printf("\nQueue Status:\n");
	xil_printf(" FREE || BCAST|");

	for(i=0; i<next_free_assoc_index; i++){
		xil_printf("%6d|", associations[i].AID);
	}
	xil_printf("\n");

	xil_printf("%6d||%6d|",queue_num_free(),queue_num_queued(0));

	for(i=0; i<next_free_assoc_index; i++){
		xil_printf("%6d|", queue_num_queued(associations[i].AID));
	}
	xil_printf("\n");

}




void print_menu(){
	xil_printf("\f");
	xil_printf("********************** AP Menu **********************\n");
	xil_printf("[1] - Interactive AP Status\n");
	xil_printf("[2] - Print Queue Status\n");
	xil_printf("\n");
	xil_printf("[c/C] - change channel (note: changing channel will\n");
	xil_printf("        purge any associations, forcing stations to\n");
	xil_printf("        join the network again)\n");
	xil_printf("[r/R] - change default unicast rate\n");
	xil_printf("[l]   - toggle local traffic generation to AID 1\n");
	xil_printf("*****************************************************\n");
}



void print_station_status(){
	u32 i;
	u64 timestamp;

	if(interactive_mode){
		timestamp = get_usec_timestamp();
		xil_printf("\f");

		for(i=0; i < next_free_assoc_index; i++){
			xil_printf("---------------------------------------------------\n");
			xil_printf(" AID: %02x -- MAC Addr: %02x:%02x:%02x:%02x:%02x:%02x\n", associations[i].AID,
					associations[i].addr[0],associations[i].addr[1],associations[i].addr[2],associations[i].addr[3],associations[i].addr[4],associations[i].addr[5]);
			xil_printf("     - Last heard from %d ms ago\n",((u32)(timestamp - (associations[i].rx_timestamp)))/1000);
			xil_printf("     - Last Rx Power: %d dBm\n",associations[i].last_rx_power);
			xil_printf("     - # of queued MPDUs: %d\n", queue_num_queued(associations[i].AID));
			xil_printf("     - # Tx MPDUs: %d (%d successful)\n", associations[i].num_tx_total, associations[i].num_tx_success);
			xil_printf("     - # Rx MPDUs: %d (%d bytes)\n", associations[i].num_rx_success, associations[i].num_rx_bytes);
		}
		    xil_printf("---------------------------------------------------\n");
		    xil_printf("\n");
		    xil_printf("[r] - reset statistics\n");
		    xil_printf("[d] - deauthenticate all stations\n");


		//Update display
		wlan_mac_schedule_event(SCHEDULE_COARSE, 1000000, (void*)print_station_status);
	}
}

#endif


