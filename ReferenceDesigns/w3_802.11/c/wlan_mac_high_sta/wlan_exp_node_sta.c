////////////////////////////////////////////////////////////////////////////////
// File   :	wlan_exp_node_ap.c
// Authors:	Chris Hunter (chunter [at] mangocomm.com)
//			Patrick Murphy (murphpo [at] mangocomm.com)
//          Erik Welsh (welsh [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////


/***************************** Include Files *********************************/

#include "wlan_exp_common.h"
#include "wlan_exp_node.h"
#include "wlan_exp_node_sta.h"

#ifdef USE_WARPNET_WLAN_EXP

// Xilinx includes
#include <xparameters.h>
#include <xil_io.h>
#include <Xio.h>
#include "xintc.h"


// Library includes
#include "string.h"
#include "stdlib.h"

//WARP includes
#include "wlan_mac_ipc_util.h"
#include "wlan_mac_misc_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_util.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_sta.h"



/*************************** Constant Definitions ****************************/


/*********************** Global Variable Definitions *************************/

extern station_info access_point;


/*************************** Variable Definitions ****************************/


/*************************** Functions Prototypes ****************************/



/******************************** Functions **********************************/


/*****************************************************************************/
/**
* Node Commands
*
* This function is part of the callback system for the Ethernet transport
* and will be executed when a valid node commands is recevied.
*
* @param    Command Header         - WARPNet Command Header
*           Command Arguments      - WARPNet Command Arguments
*           Response Header        - WARPNet Response Header
*           Response Arguments     - WARPNet Response Arguments
*           Packet Source          - Ethernet Packet Source
*           Ethernet Device Number - Indicates which Ethernet device packet came from
*
* @return	None.
*
* @note		See on-line documentation for more information about the ethernet
*           packet structure for WARPNet:  www.warpproject.org
*
******************************************************************************/
int wlan_exp_node_sta_processCmd( unsigned int cmdID, const wn_cmdHdr* cmdHdr, const void* cmdArgs, wn_respHdr* respHdr, void* respArgs, void* pktSrc, unsigned int eth_dev_num){
	//IMPORTANT ENDIAN NOTES:
	// -cmdHdr is safe to access directly (pre-swapped if needed)
	// -cmdArgs is *not* pre-swapped, since the framework doesn't know what it is
	// -respHdr will be swapped by the framework; user code should fill it normally
	// -respArgs will *not* be swapped by the framework, since only user code knows what it is
	//    Any data added to respArgs by the code below must be endian-safe (swapped on AXI hardware)

	const u32   * cmdArgs32  = cmdArgs;
	u32         * respArgs32 = respArgs;

	unsigned int  respIndex  = 0;                  // This function is called w/ same state as node_processCmd
	unsigned int  respSent   = NO_RESP_SENT;       // Initialize return value to NO_RESP_SENT
    unsigned int  max_words  = 300;                // Max number of u32 words that can be sent in the packet (~1200 bytes)
                                                   //   If we need more, then we will need to rework this to send multiple response packets
    unsigned int  temp, i;
    unsigned int  num_tables;
    unsigned int  table_freq;


    // Note:    
    //   Response header cmd, length, and numArgs fields have already been initialized.
    
    
#ifdef _DEBUG_
	xil_printf("In wlan_exp_node_ap_processCmd():  ID = %d \n", cmdID);
#endif

	switch(cmdID){

		case NODE_GET_ASSN_STATUS:
            // NODE_GET_ASSN_STATUS Packet Format:
            //   - Note:  All u32 parameters in cmdArgs32 are byte swapped so use Xil_Ntohl()
            //
            //   - cmdArgs32[0] - 31:16 - Number of tables to transmit per minute ( 0 => stop )
			//                  - 15:0  - Number of tables to transmit            ( 0 => infinite )
			//

			temp        = Xil_Ntohl(cmdArgs32[0]);
			table_freq  = ( temp >> 16 );
        	num_tables  = temp & 0xFFFF;


        	// Transmit association tables
        	if ( table_freq != 0 ) {

				// Transmit num_tables association tables at the table_freq rate
				if ( num_tables != 0 ) {

//					for( i = 0; i < num_tables; i++ ) {

						respIndex += get_station_status( &access_point, 1, &respArgs32[respIndex], max_words );
//					}


				// Continuously transmit association tables
				} else {
					// TODO:

				}

			// Stop transmitting association tables
        	} else {

        		// Send stop indicator
                respArgs32[respIndex++] = Xil_Htonl( 0xFFFFFFFF );
        	}

            // NODE_GET_ASSN_TBL Response Packet Format:
            //   - Note:  All u32 parameters in cmdArgs32 are byte swapped so use Xil_Ntohl()
            //
            //   - respArgs32[0] - 31:0  - Number of association table entries
        	//   - respArgs32[1 .. N]    - Association table entries
			//

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;

		break;

        
		case NODE_DISASSOCIATE:
            // NODE_DISASSOCIATE Packet Format:
            //   - Note:  All u32 parameters in cmdArgs32 are byte swapped so use Xil_Ntohl()
            //
            //   - cmdArgs32[0] - AID
			//                  - 0xFFFF - Disassociate all
			//
			//   - Returns AID that was disassociated
            //
			xil_printf("Node Disassociate - STA\n");

		break;


		case NODE_RESET_STATS:
			xil_printf("Reseting Statistics - STA\n");

			reset_station_statistics();
		break;


		default:
			xil_printf("Unknown node command: %d\n", cmdID);
		break;
	}

	return respSent;
}



#if 0

//Send bcast probe requests across all channels
if(active_scan ==0){
	num_ap_list = 0;
	//xil_printf("- Free 0x%08x\n",ap_list);
	free(ap_list);
	ap_list = NULL;
	active_scan = 1;
	access_point_ssid = realloc(access_point_ssid, 1);
	*access_point_ssid = 0;
	//xil_printf("+++ starting active scan\n");
	probe_req_transmit();
}





if(default_unicast_rate > WLAN_MAC_RATE_6M){
	default_unicast_rate--;
} else {
	default_unicast_rate = WLAN_MAC_RATE_6M;
}


access_point.tx_rate = default_unicast_rate;






if(ltg_mode == 0){
	#define LTG_INTERVAL 10000
	xil_printf("Enabling LTG mode to AP, interval = %d usec\n", LTG_INTERVAL);
	cbr_parameters.interval_usec = LTG_INTERVAL; //Time between calls to the packet generator in usec. 0 represents backlogged... go as fast as you can.
	start_ltg(0, LTG_TYPE_CBR, &cbr_parameters);

	ltg_mode = 1;

} else {
	stop_ltg(0);
	ltg_mode = 0;
	xil_printf("Disabled LTG mode to AID 1\n");
}









numerical_entry[curr_decade] = 0;
curr_decade = 0;

ap_sel = str2num(numerical_entry);

if( (ap_sel >= 0) && (ap_sel <= (num_ap_list-1))){

	if( ap_list[ap_sel].private == 0) {
		uart_mode = UART_MODE_MAIN;
		mac_param_chan = ap_list[ap_sel].chan;

		//Send a message to other processor to tell it to switch channels
		ipc_msg_to_low.msg_id = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_RF_IFC);
		ipc_msg_to_low.num_payload_words = sizeof(ipc_config_rf_ifc)/sizeof(u32);
		ipc_msg_to_low.payload_ptr = &(ipc_msg_to_low_payload[0]);
		init_ipc_config(config_rf_ifc,ipc_msg_to_low_payload,ipc_config_rf_ifc);
		config_rf_ifc->channel = mac_param_chan;
		ipc_mailbox_write_msg(&ipc_msg_to_low);


		xil_printf("\nAttempting to join %s\n", ap_list[ap_sel].ssid);
		memcpy(access_point.addr, ap_list[ap_sel].bssid, 6);

		access_point_ssid = realloc(access_point_ssid, strlen(ap_list[ap_sel].ssid)+1);
		//xil_printf("allocated %d bytes in 0x%08x\n", strlen(ap_list[ap_sel].ssid), access_point_ssid);
		strcpy(access_point_ssid,ap_list[ap_sel].ssid);

		access_point_num_basic_rates = ap_list[ap_sel].num_basic_rates;
		memcpy(access_point_basic_rates, ap_list[ap_sel].basic_rates,access_point_num_basic_rates);

		association_state = 1;
		attempt_authentication();

	} else {
		xil_printf("\nInvalid selection, please choose an AP that is not private: ");
	}


} else {

	xil_printf("\nInvalid selection, please choose a number between [0,%d]: ", num_ap_list-1);

}


#endif





#endif
