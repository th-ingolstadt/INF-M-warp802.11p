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
#include "wlan_mac_high.h"
#include "wlan_mac_entries.h"
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
#include "wlan_mac_addr_filter.h"
#include "wlan_mac_ap.h"



/*************************** Constant Definitions ****************************/


/*********************** Global Variable Definitions *************************/

extern dl_list		association_table;
extern dl_list      statistics_table;
extern tx_params    default_unicast_data_tx_params;

extern char       * access_point_ssid;

extern u32          mac_param_chan;

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
    int           status;

    u32           temp, temp2, i;
    u32           msg_cmd;
    u32           id;
    u32           flags;

	u8            mac_addr[6];
    u8            mask[6];

	dl_entry	* curr_entry;
	station_info* curr_station_info;

	station_info_entry* station_log_entry;

    // Note:    
    //   Response header cmd, length, and numArgs fields have already been initialized.
    
    
#ifdef _DEBUG_
	xil_printf("In wlan_exp_node_ap_processCmd():  ID = %d \n", cmdID);
#endif

	switch(cmdID){

		//---------------------------------------------------------------------
		case CMDID_NODE_DISASSOCIATE:
            // Disassociate device from node
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
        	id = wlan_exp_get_aid_from_ADDR(&mac_addr[0]);

			status  = CMD_PARAM_SUCCESS;

            if ( id == 0 ) {
				// If we cannot find the MAC address, print a warning and return status error
				xil_printf("WARNING:  Could not find specified node: %02x", mac_addr[0]);
				for ( i = 1; i < ETH_ADDR_LEN; i++ ) { xil_printf(":%02x", mac_addr[i] ); } xil_printf("\n");

				status = CMD_PARAM_ERROR;

            } else {
				// If parameter is not the magic number to disassociate all stations
				if ( id != CMD_PARAM_NODE_CONFIG_ALL_ASSOCIATED ) {
					// Find the station_info entry
					curr_entry = wlan_mac_high_find_station_info_ADDR( get_station_info_list(), &mac_addr[0]);

					if (curr_entry != NULL) {
						curr_station_info = (station_info*)(curr_entry->data);

						// Disable interrupts so no packets interrupt the disassociate
						wlan_mac_high_interrupt_stop();

						// Deauthenticate station
						deauthenticate_station(curr_station_info);

						// Re-enable interrupts
						wlan_mac_high_interrupt_start();

						// Set return parameters and print info to console
						xil_printf("Disassociated node: %02x", mac_addr[0]);
						for ( i = 1; i < ETH_ADDR_LEN; i++ ) { xil_printf(":%02x", mac_addr[i] ); } xil_printf("\n");

					} else {
						// If we cannot find the MAC address, print a warning and return status error
						xil_printf("WARNING:  Could not find specified node: %02x", mac_addr[0]);
						for ( i = 1; i < ETH_ADDR_LEN; i++ ) { xil_printf(":%02x", mac_addr[i] ); } xil_printf("\n");

						status = CMD_PARAM_ERROR;
					}
				} else {
					// Disable interrupts so no packets interrupt the disassociate
					wlan_mac_high_interrupt_stop();

					// Deauthenticate all stations
					deauthenticate_stations();

					// Re-enable interrupts
					wlan_mac_high_interrupt_start();

					// Set return parameters and print info to console
					xil_printf("Disassociated node: %02x", mac_addr[0]);
					for ( i = 1; i < ETH_ADDR_LEN; i++ ) { xil_printf(":%02x", mac_addr[i] ); } xil_printf("\n");
				}
            }

			// Send response
            respArgs32[respIndex++] = Xil_Htonl( status );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;

        
		//---------------------------------------------------------------------
		case CMDID_NODE_ASSOCIATE:
			// Associate with the device
			//
			// Message format:
			//     cmdArgs32[0]        Association flags
			//                             CMD_PARAM_ASSOCIATE_ALLOW_TIMEOUT
			//     cmdArgs32[1]        Association flags mask
			//     cmdArgs32[2:3]      Association MAC Address
			//
			// Response format:
			//     respArgs32[0]       Status
			//
			xil_printf("Associate\n");

			status            = CMD_PARAM_SUCCESS;
            curr_station_info = NULL;

            // Set default value for the flags
            flags             = STATION_INFO_FLAG_DISABLE_ASSOC_CHECK;

			if( association_table.length < MAX_NUM_ASSOC ) {

				// Get MAC Address
				wlan_exp_get_mac_addr(&((u32 *)cmdArgs32)[2], &mac_addr[0]);

				// Get flags
				temp  = Xil_Ntohl(cmdArgs32[0]);
				temp2 = Xil_Ntohl(cmdArgs32[1]);

				xil_printf("FLAGS = 0x%08x  mask = 0x%08x\n", temp, temp2);

				// Configure the LOG based on the flag bit / mask
				if ( ( temp2 & CMD_PARAM_ASSOCIATE_ALLOW_TIMEOUT ) == CMD_PARAM_ASSOCIATE_ALLOW_TIMEOUT ) {
					if ( ( temp & CMD_PARAM_ASSOCIATE_ALLOW_TIMEOUT ) == CMD_PARAM_ASSOCIATE_ALLOW_TIMEOUT ) {
						flags |= STATION_INFO_FLAG_DISABLE_ASSOC_CHECK;
					} else {
						flags &= ~STATION_INFO_FLAG_DISABLE_ASSOC_CHECK;
					}
				}

				// Disable interrupts so no packets interrupt the disassociate
				wlan_mac_high_interrupt_stop();

				// Add association
				curr_station_info = wlan_mac_high_add_association(&association_table, &statistics_table, mac_addr, ADD_ASSOCIATION_ANY_AID);

				// Set the flags
				curr_station_info->flags = flags;

				// Re-enable interrupts
				wlan_mac_high_interrupt_start();

				// Set return parameters and print info to console
				if (curr_station_info != NULL) {
					// Log association state change
					station_log_entry = (station_info_entry*)wlan_exp_log_create_entry( ENTRY_TYPE_STATION_INFO, sizeof(station_info_entry));

					if(station_log_entry != NULL){
						station_log_entry->timestamp = get_usec_timestamp();
						memcpy((u8*)(&(station_log_entry->info)),(u8*)(curr_station_info), sizeof(station_info_base) );
					}

					memcpy(&(curr_station_info->tx), &default_unicast_data_tx_params, sizeof(tx_params));

					// Update the hex display
					ap_write_hex_display(association_table.length);

					xil_printf("Associated with node");
				} else {
					xil_printf("Could not associate with node");
					status = CMD_PARAM_ERROR;
				}
			} else {
				xil_printf("Could not associate with node");
				status = CMD_PARAM_ERROR;
			}

			xil_printf(": %02x", mac_addr[0]);
			for ( i = 1; i < ETH_ADDR_LEN; i++ ) { xil_printf(":%02x", mac_addr[i] ); } xil_printf("\n");

			// Send response
			respArgs32[respIndex++] = Xil_Htonl( status );
			if (curr_station_info != NULL ) {
			    respArgs32[respIndex++] = Xil_Htonl( curr_station_info->AID );
			} else {
			    respArgs32[respIndex++] = Xil_Htonl( 0 );
			}

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

				// Send Channel Switch Announcement
				send_channel_switch_announcement( temp );

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
        case CMDID_NODE_AP_SET_ASSOCIATION_ADDR_FILTER:
            // Allow / Disallow wireless associations
            //
			// Message format:
			//     cmdArgs32[0]   Command:
			//                      - Write       (CMD_PARAM_WRITE_VAL)
			//     cmdArgs32[1]   Number of address filters
        	//     cmdArgs32[2:N] [ (Mask (u64), Compare address (u64)) ]
        	//
			// Response format:
			//     respArgs32[0]  Status
            //
        	msg_cmd = Xil_Ntohl(cmdArgs32[0]);
			temp    = Xil_Ntohl(cmdArgs32[1]);
			status  = CMD_PARAM_SUCCESS;

			switch (msg_cmd) {
				case CMD_PARAM_WRITE_VAL:
                    // Need to disable interrupts during this operation so the filter does not have any holes
					wlan_mac_high_interrupt_stop();

					// Reset the current address filter
					wlan_mac_addr_filter_reset();

					// Add all the address ranges to the filter
                    for( i = 0; i < temp; i++ ) {
                        // Extract the address and the mask
                    	wlan_exp_get_mac_addr(&((u32 *)cmdArgs32)[2 + (4*i)], &mask[0]);
                    	wlan_exp_get_mac_addr(&((u32 *)cmdArgs32)[4 + (4*i)], &mac_addr[0]);

        				xil_printf("Adding Address filter: (%02x", mask[0]);
        				for ( i = 1; i < ETH_ADDR_LEN; i++ ) { xil_printf(":%02x", mask[i] ); }
        				xil_printf(", %02x", mac_addr[0]);
        				for ( i = 1; i < ETH_ADDR_LEN; i++ ) { xil_printf(":%02x", mac_addr[i] ); } xil_printf(")\n");

                    	if ( wlan_mac_addr_filter_add(mask, mac_addr) == -1 ) {
                    		status = CMD_PARAM_ERROR;
                    	}
                    }

					wlan_mac_high_interrupt_start();
			    break;

				default:
					xil_printf("Unknown command for 0x%6x: %d\n", cmdID, msg_cmd);
					status = CMD_PARAM_ERROR;
				break;
			}

			// Send response
            respArgs32[respIndex++] = Xil_Htonl( status );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
        break;


		//---------------------------------------------------------------------
		case CMDID_NODE_AP_SSID:
            // Get / Set AP SSID
            //
			// NOTE:  This method does not force any maximum length on the SSID.  However,
			//   the rest of the framework enforces the convention that the maximum length
			//   of the SSID is SSID_LEN_MAX.
			//
			// Message format:
			//     cmdArgs32[0]        Command:
			//                           - Write       (CMD_PARAM_WRITE_VAL)
			//                           - Read        (CMD_PARAM_READ_VAL)
			//     cmdArgs32[1]        SSID Length (write-only)
			//     cmdArgs32[2:N]      SSID        (write-only)
            //
			// Response format:
			//     respArgs32[0]       Status
        	//     respArgs32[1]       SSID Length
			//     respArgs32[2:N]     SSID (packed array of ascii character values)
			//                             NOTE: The characters are copied with a straight strcpy
			//                               and must be correctly processed on the host side
            //
        	msg_cmd = Xil_Ntohl(cmdArgs32[0]);
			temp    = Xil_Ntohl(cmdArgs32[1]);
			status  = CMD_PARAM_SUCCESS;

			char * ssid;

			switch (msg_cmd) {
				case CMD_PARAM_WRITE_VAL:
					ssid = (char *)&cmdArgs32[2];

					// Deauthenticate all stations since we are changing the SSID
					deauthenticate_stations();

					// Re-allocate memory for the new SSID and copy the characters of the new SSID
					access_point_ssid = wlan_mac_high_realloc(access_point_ssid, (temp + 1));

					if (access_point_ssid != NULL) {
						strcpy(access_point_ssid, ssid);
						xil_printf("Set SSID - AP:  %s\n", access_point_ssid);
					} else {
						xil_printf("Failed to reallocate memory for SSID: %d bytes.  Please reset the AP.\n", (temp + 1));
						status = CMD_PARAM_ERROR;
					}
			    break;

				case CMD_PARAM_READ_VAL:
					xil_printf("Get SSID - AP - %s\n", access_point_ssid);
				break;

				default:
					xil_printf("Unknown command for 0x%6x: %d\n", cmdID, msg_cmd);
					status = CMD_PARAM_ERROR;
				break;
			}

			// Send response
            respArgs32[respIndex++] = Xil_Htonl( status );

            // Return the size and current SSID
			if (access_point_ssid != NULL) {
				temp = strlen(access_point_ssid);

				respArgs32[respIndex++] = Xil_Htonl( temp );

				strcpy( (char *)&respArgs32[respIndex], access_point_ssid );

				respIndex       += ( temp / sizeof(respArgs32) ) + 1;
			} else {
				// Return a zero length string
				respArgs32[respIndex++] = 0;
			}

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


#endif
