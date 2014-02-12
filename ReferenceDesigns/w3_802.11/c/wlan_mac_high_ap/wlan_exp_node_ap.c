/** @file wlan_exp_node_ap.c
 *  @brief Access Point WARPNet Experiment
 *
 *  This contains code for the 802.11 Access Point's WARPNet experiment interface.
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
#include "wlan_exp.h"
#include "wlan_exp_node.h"
#include "wlan_exp_node_ap.h"


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
#include "wlan_mac_ltg.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_ap.h"



/*************************** Constant Definitions ****************************/


/*********************** Global Variable Definitions *************************/

extern dl_list		association_table;

extern char       * access_point_ssid;

extern u32          mac_param_chan;

extern u8           default_unicast_rate;

extern u8           bcast_addr[6];

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
int wlan_exp_node_ap_processCmd( unsigned int cmdID, const wn_cmdHdr* cmdHdr, const void* cmdArgs, wn_respHdr* respHdr, void* respArgs, void* pktSrc, unsigned int eth_dev_num){
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
    // unsigned int  max_words  = 300;                // Max number of u32 words that can be sent in the packet (~1200 bytes)
                                                   //   If we need more, then we will need to rework this to send multiple response packets

    unsigned int  temp, temp2, i;
    unsigned int  num_tables;
    unsigned int  table_freq;

	station_info* curr_station_info;


    // Note:    
    //   Response header cmd, length, and numArgs fields have already been initialized.
    
    
#ifdef _DEBUG_
	xil_printf("In wlan_exp_node_ap_processCmd():  ID = %d \n", cmdID);
#endif

	switch(cmdID){

		//---------------------------------------------------------------------
		case NODE_ASSN_GET_STATUS:
            // NODE_ASSN_GET_STATUS Packet Format:
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

// FIX!!!						respIndex += get_station_status( &associations[0], next_free_assoc_index, &respArgs32[respIndex], max_words );
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


		//---------------------------------------------------------------------
		// TODO:  THIS FUNCTION IS NOT COMPLETE
		case NODE_ASSN_SET_TABLE:
			xil_printf("AP - Set association table not supported\n");
		break;


		//---------------------------------------------------------------------
		case NODE_DISASSOCIATE:
            // NODE_DISASSOCIATE Packet Format:
            //   - Note:  All u32 parameters in cmdArgs32 are byte swapped so use Xil_Ntohl()
            //
            //   - cmdArgs32[0] - AID
			//                  - 0xFFFF - Disassociate all
			//
			//   - Returns      0      - AID not found
            //                  AID    - AID that was disassociated
			//                  0xFFFF - All AIDs)
            //
			temp = Xil_Ntohl(cmdArgs32[0]);

			// Check argument to see if we are diassociating one or all AIDs
			if ( temp == 0xFFFF ) {
				deauthenticate_stations();                     // Deauthenticate all stations
			} else {
                temp = deauthenticate_station( wlan_mac_high_find_station_info_AID( &association_table, temp ) );
			}

			// Print message to the UART to show which node was disassociated
			xil_printf("Node Disassociate - AP  - 0x%x\n", temp);

			// Send response of current channel
            respArgs32[respIndex++] = Xil_Htonl( temp );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;

        
	    //---------------------------------------------------------------------
		case NODE_TX_RATE:
			// Get node TX rate
			temp = Xil_Ntohl(cmdArgs32[0]);

			// If parameter is not the magic number, then set the TX rate
			if ( temp != NODE_TX_RATE_RSVD_VAL ) {

				default_unicast_rate = temp;

				if( default_unicast_rate < WLAN_MAC_RATE_6M ){
					default_unicast_rate = WLAN_MAC_RATE_6M;
				}

				if(default_unicast_rate > WLAN_MAC_RATE_54M){
					default_unicast_rate = WLAN_MAC_RATE_54M;
				}

				curr_station_info = (station_info*)(association_table.first);
				for(i=0; i < association_table.length; i++){
					curr_station_info->tx.rate = default_unicast_rate;
					curr_station_info = (station_info*)((curr_station_info->node).next);
				}

			    xil_printf("Setting TX rate = %d Mbps\n", wlan_lib_mac_rate_to_mbps(default_unicast_rate) );
			}

			// Send response of current rate
            respArgs32[respIndex++] = Xil_Htonl( default_unicast_rate );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;

		//---------------------------------------------------------------------
		case NODE_CHANNEL:
			// Get channel parameter
			temp = Xil_Ntohl(cmdArgs32[0]);

			// If parameter is not the magic number, then set the mac channel
			//   NOTE:  We modulate temp so that we always have a valid channel
			if ( temp != NODE_CHANNEL_RSVD_VAL ) {
				temp = temp % 12;          // Get a channel number between 0 - 11
				if ( temp == 0 ) temp++;   // Change all values of 0 to 1

				// NOTE:  This function must be implemented in all child classes
				// deauthenticate_stations(); // First deauthenticate all stations //TODO: not sure this should be here for WARPnet

				mac_param_chan = temp;
				wlan_mac_high_set_channel( mac_param_chan );

			    xil_printf("Setting Channel = %d\n", mac_param_chan);
			}

			// Send response of current channel
            respArgs32[respIndex++] = Xil_Htonl( mac_param_chan );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;


		//---------------------------------------------------------------------
        case NODE_AP_ALLOW_ASSOCIATIONS:
            // NODE_AP_ALLOW_ASSOCIATIONS Packet Format:
            //   - Note:  All u32 parameters in cmdArgs32 are byte swapped so use Xil_Ntohl()
            //
            //   - cmdArgs32[0] - 0xFFFF - Permanently turn on associations
			//                  - Others - Temporarily turn on associations
			//
			//   - Returns the status of the associations
            //
			xil_printf("Associations     Allowed - AP - ");

			temp = Xil_Ntohl(cmdArgs32[0]);

			if ( temp == 0xFFFF ) {
	    		enable_associations( ASSOCIATION_ALLOW_PERMANENT );
				xil_printf("Permanent \n");
			} else {
	    		enable_associations( ASSOCIATION_ALLOW_TEMPORARY );
				xil_printf("Temporary \n");
			}

            respArgs32[respIndex++] = Xil_Htonl( get_associations_status() );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
        break;


		//---------------------------------------------------------------------
		case NODE_AP_DISALLOW_ASSOCIATIONS:
            // NODE_AP_DISALLOW_ASSOCIATIONS Packet Format:
			//
			//   - Returns the status of the associations
            //
			disable_associations();

			temp = get_associations_status();

			if ( temp == ASSOCIATION_ALLOW_NONE ) {
				xil_printf("Associations NOT Allowed - AP\n");
			} else {
				xil_printf("Associations     Allowed - AP - Failed to Disable Associations\n");
			}

            respArgs32[respIndex++] = Xil_Htonl( temp );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;


		//---------------------------------------------------------------------
		case NODE_AP_GET_SSID:
            // NODE_AP_GET_SSID Packet Format:
			//
            //   - respArgs32[0] - Number of characters the new SSID
            //   - respArgs32[1] - Packed array of ascii character values
			//                       NOTE: The characters are copied with a straight strcpy and must
			//                         be correctly processed on the host side
            //
			xil_printf("Get SSID - AP - %s\n", access_point_ssid);

			// Get the number of characters in the SSID
			temp = strlen(access_point_ssid);

			// Send response of current channel
            respArgs32[respIndex++] = Xil_Htonl( temp );

			strcpy( (char *)&respArgs32[respIndex], access_point_ssid );

			respIndex       += ( temp / sizeof(respArgs32) ) + 1;

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;


		//---------------------------------------------------------------------
		case NODE_AP_SET_SSID:
            // NODE_AP_SET_SSID Packet Format:
			//
            //   - cmdArgs32[0] - Number of characters in the new SSID
            //   - cmdArgs32[1] - Packed array of ascii character values
			//                      NOTE: The characters are assumed to be in the correct order
			//
			//   - Returns nothing
            //
			temp        = Xil_Ntohl(cmdArgs32[0]);
			char * ssid = (char *)&cmdArgs32[1];

			// Deauthenticate all stations since we are changing the SSID
			deauthenticate_stations();

			// Re-allocate memory for the new SSID and copy the characters of the new SSID
			access_point_ssid = wlan_mac_high_realloc(access_point_ssid, (temp + 1));
			strcpy(access_point_ssid, ssid);

			xil_printf("Set SSID - AP:  %s\n", access_point_ssid);
		break;


	    //---------------------------------------------------------------------
		case NODE_CONFIG_DEMO:
			// Configure the Demo
			//
			// Message format:
			//     cmdArgs32[0]   Flags
			//     cmdArgs32[1]   LTG interval (usec)
			//
			temp       = Xil_Ntohl(cmdArgs32[0]);
			temp2      = Xil_Ntohl(cmdArgs32[1]);

			xil_printf("Configuring Demo:  flags = 0x%08x  interval = %d\n", temp, temp2);

			// Pass the parameters directly to the config_demo function
			//wlan_ap_config_demo(temp, temp2); //TODO: Remove WARPnet Command

			// Send response
			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
        break;


        //---------------------------------------------------------------------
		default:
			xil_printf("Unknown node command: %d\n", cmdID);
		break;
	}

	return respSent;
}



/*****************************************************************************/
/**
* WLAN Exp mapping of MAC Addr to AID
*
* This function contains the mapping of MAC address to AID within a node.
*
* @param    MAC Address
*
* @return	AID associated with that MAC address
*
* @note		None.
*
******************************************************************************/
u32  wlan_exp_get_aid_from_ADDR(u8 * mac_addr) {
	u32            id;
	station_info * info;

	if ( wlan_addr_eq(mac_addr, bcast_addr) ) {
		id = 0xFFFFFFFF;
	} else {
		info = wlan_mac_high_find_station_info_ADDR(&association_table, mac_addr);
		if (info != NULL) {
            id = info->AID;
		} else {
			xil_printf("ERROR:  Could not find MAC address = %02x:%02x:%02x:%02x:%02x:%02x\n",
                       mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
			id = 0;
		}
	}

	return id;
}


#endif
