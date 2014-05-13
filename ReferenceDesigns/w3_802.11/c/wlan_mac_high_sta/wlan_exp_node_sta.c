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
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_sta.h"



/*************************** Constant Definitions ****************************/


/*********************** Global Variable Definitions *************************/
extern dl_list		  association_table;
extern u8			  ap_addr[6];
extern char*          access_point_ssid;

extern ap_info      * ap_list;
extern u8             num_ap_list;
extern u8             active_scan;
extern u8             pause_queue;

extern int            association_state;

extern u32            mac_param_chan;
extern u32            mac_param_chan_save;

extern u8             access_point_num_basic_rates;
extern u8             access_point_basic_rates[NUM_BASIC_RATES_MAX];

extern u8	          allow_beacon_ts_update;

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
    int           status;

    u32           temp, temp2, i;
    u32           msg_cmd;
    u32           aid;

    u8            mac_addr[6];

    // Note:    
    //   Response header cmd, length, and numArgs fields have already been initialized.
    
    
#ifdef _DEBUG_
	xil_printf("In wlan_exp_node_ap_processCmd():  ID = %d \n", cmdID);
#endif

	switch(cmdID){

		//---------------------------------------------------------------------
		case CMDID_NODE_DISASSOCIATE:
			// Disassociate from the AP
			//
			// Message format:
			//     cmdArgs32[0:1]      MAC Address (All 0xFF means all station info)
			//
			// Response format:
			//     respArgs32[0]       Status
			//
			xil_printf("Disassociate\n");

			// Get MAC Address
			wlan_exp_get_mac_addr(&((u32 *)cmdArgs32)[0], &mac_addr[0]);
			aid = wlan_exp_get_aid_from_ADDR(&mac_addr[0]);

			status  = CMD_PARAM_SUCCESS;

			if ( aid == 0 ) {
				// If we cannot find the MAC address, print a warning and return status error
				xil_printf("WARNING:  Could not find specified node: %02x", mac_addr[0]);
				for ( i = 1; i < ETH_ADDR_LEN; i++ ) { xil_printf(":%02x", mac_addr[i] ); } xil_printf("\n");

				status = CMD_PARAM_ERROR;

			} else {
				// Disable interrupts so no packets interrupt the disassociate
				wlan_mac_high_interrupt_stop();

                // STA disassociate command is the same for an individual AP or ALL
				status = sta_disassociate();

				// Re-enable interrupts
				wlan_mac_high_interrupt_start();

				// Set return parameters and print info to console
				if ( status == 0 ) {
					xil_printf("Disassociated node");
					status = CMD_PARAM_SUCCESS;
				} else {
					xil_printf("Could not disassociate node");
					status = CMD_PARAM_ERROR;
				}

				xil_printf(": %02x", mac_addr[0]);
				for ( i = 1; i < ETH_ADDR_LEN; i++ ) { xil_printf(":%02x", mac_addr[i] ); } xil_printf("\n");
			}

			// Send response
			respArgs32[respIndex++] = Xil_Htonl( status );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
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
			xil_printf("Associate\n");

			// Get MAC Address
			wlan_exp_get_mac_addr(&((u32 *)cmdArgs32)[2], &mac_addr[0]);

			// Get AID
			aid = Xil_Ntohl(cmdArgs32[4]);

			// Disable interrupts so no packets interrupt the associate
			wlan_mac_high_interrupt_stop();

			// Stop any active scans
			stop_active_scan();

			// Set the access_point_ssid to "" so that we won't try to re-join the default
			// network if we are disassociated.
			bzero(access_point_ssid, SSID_LEN_MAX);

			// Add the new association
			status = sta_associate( mac_addr, aid );

			// Re-enable interrupts
			wlan_mac_high_interrupt_start();

			// Set return parameters and print info to console
			if ( status == 0 ) {
				xil_printf("Associated with node");
				status = CMD_PARAM_SUCCESS;
			} else {
				xil_printf("Could not associate with node");
				status = CMD_PARAM_ERROR;
			}

			xil_printf(": %02x", mac_addr[0]);
			for ( i = 1; i < ETH_ADDR_LEN; i++ ) { xil_printf(":%02x", mac_addr[i] ); } xil_printf("\n");

			// Send response
			respArgs32[respIndex++] = Xil_Htonl( status );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;


		//---------------------------------------------------------------------
		case CMDID_NODE_CHANNEL:
			//   - cmdArgs32[0]      - Command
			//   - cmdArgs32[1]      - Channel

			msg_cmd = Xil_Ntohl(cmdArgs32[0]);
			temp    = Xil_Ntohl(cmdArgs32[1]);
			status  = CMD_PARAM_SUCCESS;

			if ( msg_cmd == CMD_PARAM_WRITE_VAL ) {
				// Set the Channel
				//   NOTE:  We modulate temp so that we always have a valid channel
				temp = temp % 12;          // Get a channel number between 0 - 11
				if ( temp == 0 ) temp++;   // Change all values of 0 to 1

				// TODO:  Disassociate from the current AP

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
		case CMDID_NODE_STA_CONFIG:
            // CMDID_NODE_STA_CONFIG Packet Format:
			//   - cmdArgs32[0]  - flags
			//                     [ 0] - Timestamps are updated from beacons = 1
			//                            Timestamps are not updated from beacons = 0
			//   - cmdArgs32[1]  - mask for flags
			//
            //   - respArgs32[0] - CMD_PARAM_SUCCESS
			//                   - CMD_PARAM_ERROR

			// Set the return value
			status = CMD_PARAM_SUCCESS;

			// Get flags
			temp  = Xil_Ntohl(cmdArgs32[0]);
			temp2 = Xil_Ntohl(cmdArgs32[1]);

			xil_printf("STA:  Configure flags = 0x%08x  mask = 0x%08x\n", temp, temp2);

			// Configure the LOG based on the flag bit / mask
			if ( ( temp2 & CMD_PARAM_NODE_STA_BEACON_TS_UPDATE ) == CMD_PARAM_NODE_STA_BEACON_TS_UPDATE ) {
				if ( ( temp & CMD_PARAM_NODE_STA_BEACON_TS_UPDATE ) == CMD_PARAM_NODE_STA_BEACON_TS_UPDATE ) {
					allow_beacon_ts_update = 1;
				} else {
					allow_beacon_ts_update = 0;
				}
			}

			// Send response of status
            respArgs32[respIndex++] = Xil_Htonl( status );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;



#if 0
		//---------------------------------------------------------------------
		case NODE_STA_GET_AP_LIST:
            // NODE_STA_GET_AP_LIST Response Packet Format:
			//
            //   - respArgs32[0] - 31:0  - Number of APs
        	//   - respArgs32[1 .. N]    - AP Info List
			//

			//Send broadcast probe requests across all channels
			if(active_scan ==0){

				wlan_mac_high_free( ap_list );

				// Clean up current state
				ap_list             = NULL;
				num_ap_list         = 0;
				access_point_ssid   = wlan_mac_high_realloc(access_point_ssid, 1);
				*access_point_ssid  = 0;

				// Start scan
				active_scan         = 1;
				pause_queue         = 1;
				mac_param_chan_save = mac_param_chan;

				probe_req_transmit();

				respIndex += get_ap_list( ap_list, num_ap_list, &respArgs32[respIndex], max_words );
			} else {
				xil_printf("WARNING:  STA - Cannot get AP List.  Currently in active scan.\n");

				respArgs32[respIndex++] = 0;
			}

			// Finalize response
			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;
#endif

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
