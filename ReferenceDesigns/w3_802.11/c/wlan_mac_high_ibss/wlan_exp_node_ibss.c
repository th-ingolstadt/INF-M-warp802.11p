/** @file wlan_exp_node_sta.c
 *  @brief Station WARPNet Experiment
 *
 *  This contains code for the 802.11 Station's WARPNet experiment interface.
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

#include "wlan_exp_common.h"
#include "wlan_exp_node.h"
#include "wlan_exp_node_ibss.h"

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
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_bss_info.h"
#include "wlan_mac_ibss_scan_fsm.h"
#include "wlan_mac_ibss.h"
#include "wlan_mac_schedule.h"


/*************************** Constant Definitions ****************************/


/*********************** Global Variable Definitions *************************/
extern dl_list		  association_table;

extern u8             pause_data_queue;
extern u32            mac_param_chan;

extern u8	          allow_beacon_ts_update;

extern bss_info*      my_bss_info;
extern u32            beacon_sched_id;


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
int wlan_exp_node_ibss_processCmd( unsigned int cmdID, const wn_cmdHdr* cmdHdr, const void* cmdArgs, wn_respHdr* respHdr, void* respArgs, void* pktSrc, unsigned int eth_dev_num){
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
    // unsigned int  max_words  = 300;             // Max number of u32 words that can be sent in the packet (~1200 bytes)
                                                   //   If we need more, then we will need to rework this to send multiple response packets
    int           status;

    u32           temp;
    u32           msg_cmd;

    // Note:    
    //   Response header cmd, length, and numArgs fields have already been initialized.
    
    
#ifdef _DEBUG_
	xil_printf("In wlan_exp_node_ap_processCmd():  ID = %d \n", cmdID);
#endif

	switch(cmdID){

		//---------------------------------------------------------------------
		case CMDID_NODE_DISASSOCIATE:
			//There isn't much to "disassociating" from an IBSS. We'll just stop sending beacons and probe responses
			if(my_bss_info != NULL){
				my_bss_info = NULL;
				if(beacon_sched_id != SCHEDULE_FAILURE){
					wlan_mac_remove_schedule(SCHEDULE_FINE, beacon_sched_id);
				}
			}
		break;


		//---------------------------------------------------------------------
		case CMDID_NODE_ASSOCIATE:
			// Associate with the AP
			//
			// Message format:
			//     cmdArgs32[0]        Association flags         (for future use)
			//     cmdArgs32[1]        Association flags mask    (for future use)
			//     cmdArgs32[2:3]      Association MAC Address
			//     cmdArgs32[4]        Association AID
			//
			// Response format:
			//     respArgs32[0]       Status
			//

			// TODO. This command should first explicitly disconnect from any existing
			// IBSS (already done, see CMDID_NODE_DISASSOCIATE). Next, it should create
			// a BSS_INFO and start beaconing just like how main() of the IBSS code does it.
			// Like the STA implementation, the associate command needs a channel as well.

		break;


		//---------------------------------------------------------------------
		case CMDID_NODE_CHANNEL:
			//   - cmdArgs32[0]      - Command
			//   - cmdArgs32[1]      - Channel

			// TODO: Discuss
			// This command strikes me as a little dangerous. Should it really just arbitrarily leap
			// to a new channel? That would mean it would start beaconing and advertise an existing IBSS
			// on a different channel. If other nodes are on the IBSS, that could get super confusing.

			msg_cmd = Xil_Ntohl(cmdArgs32[0]);
			temp    = Xil_Ntohl(cmdArgs32[1]);
			status  = CMD_PARAM_SUCCESS;

			if ( msg_cmd == CMD_PARAM_WRITE_VAL ) {
				// Set the Channel
				//   NOTE:  We modulate temp so that we always have a valid channel
				temp = temp % 12;          // Get a channel number between 0 - 11
				if ( temp == 0 ) temp++;   // Change all values of 0 to 1

				mac_param_chan = temp;
				wlan_mac_high_set_channel( mac_param_chan );

			    xil_printf("Setting Channel = %d\n", mac_param_chan);
			}

			// Send response
            respArgs32[respIndex++] = Xil_Htonl( status );
            respArgs32[respIndex++] = Xil_Htonl( mac_param_chan );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;

		//---------------------------------------------------------------------
		default:
			xil_printf("Unknown node command: 0x%x\n", cmdID);
		break;
	}

	return respSent;
}






#ifdef _DEBUG_

/*****************************************************************************/
/**
* Print Station Status
*
* This function will print a list of station_info structures
*
* @param    stations     - pointer to the station_info list
*           num_stations - number of station_info structures in the list
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void wlan_exp_print_ap_list( ap_info * ap_list, u32 num_ap ){
	u32  i, j;
	char str[4];

	for( i = 0; i < num_ap; i++ ) {
		xil_printf("---------------------------------------------------\n");
		xil_printf(" AP: %02x -- MAC Addr: %02x:%02x:%02x:%02x:%02x:%02x\n", i,
				ap_list[i].bssid[0],ap_list[i].bssid[1],ap_list[i].bssid[2],ap_list[i].bssid[3],ap_list[i].bssid[4],ap_list[i].bssid[5]);
		xil_printf("     - SSID             : %s  (private = %d)\n", ap_list[i].ssid, ap_list[i].private);
		xil_printf("     - Channel          : %d\n", ap_list[i].chan);
		xil_printf("     - Rx Power         : %d\n", ap_list[i].rx_power);
		xil_printf("     - Rates            : ");

		for( j = 0; j < ap_list[i].num_basic_rates; j++ ) {
			wlan_mac_high_tagged_rate_to_readable_rate(ap_list[i].basic_rates[j], str);
			xil_printf("%s, ",str);
		}
        xil_printf("\n");
	}
}

#endif


#endif
