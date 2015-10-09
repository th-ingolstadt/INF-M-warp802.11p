/** @file wlan_exp_node_ap.c
 *  @brief Access Point WLAN Experiment
 *
 *  This contains code for the 802.11 Access Point's WLAN experiment interface.
 *
 *  @copyright Copyright 2013-2015, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

/***************************** Include Files *********************************/

#include "wlan_exp_common.h"
#include "wlan_exp.h"
#include "wlan_mac_high.h"
#include "wlan_mac_entries.h"
#include "wlan_exp_node.h"
#include "wlan_exp_node_ap.h"


#ifdef USE_WLAN_EXP

// Xilinx includes
#include <xparameters.h>
#include <xil_io.h>
#include <xio.h>
#include "xintc.h"


// Library includes
#include "string.h"
#include "stdlib.h"

// WLAN includes
#include "wlan_mac_ipc_util.h"
#include "wlan_mac_misc_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_addr_filter.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_ap.h"
#include "wlan_mac_bss_info.h"



/*************************** Constant Definitions ****************************/


/*********************** Global Variable Definitions *************************/


extern dl_list      counts_table;
extern tx_params    default_unicast_data_tx_params;
extern u32          mac_param_chan;
extern bss_info*    my_bss_info;
extern ps_conf      power_save_configuration;
extern u32          beacon_schedule_id;

/*************************** Variable Definitions ****************************/


/*************************** Functions Prototypes ****************************/


/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * Process Node Commands
 *
 * This function is part of the Ethernet processing system and will process the
 * various node related commands.
 *
 * @param   socket_index     - Index of the socket on which to send message
 * @param   from             - Pointer to socket address structure (struct sockaddr *) where command is from
 * @param   command          - Pointer to Command
 * @param   response         - Pointer to Response
 *
 * @return  int              - Status of the command:
 *                                 NO_RESP_SENT - No response has been sent
 *                                 RESP_SENT    - A response has been sent
 *
 * @note    See on-line documentation for more information about the Ethernet
 *          packet structure:  www.warpproject.org
 *
 *****************************************************************************/
int wlan_exp_node_ap_process_cmd(u32 cmd_id, int socket_index, void * from, cmd_resp * command, cmd_resp * response) {

    //
    // IMPORTANT ENDIAN NOTES:
    //     - command
    //         - header - Already endian swapped by the framework (safe to access directly)
    //         - args   - Must be endian swapped as necessary by code (framework does not know the contents of the command)
    //     - response
    //         - header - Will be endian swapped by the framework (safe to write directly)
    //         - args   - Must be endian swapped as necessary by code (framework does not know the contents of the response)
    //

    // Standard variables
    u32                 resp_sent      = NO_RESP_SENT;

    u32               * cmd_args_32    = command->args;

    cmd_resp_hdr      * resp_hdr       = response->header;
    u32               * resp_args_32   = response->args;
    u32                 resp_index     = 0;

    //
    // NOTE: Response header cmd, length, and num_args fields have already been initialized.
    //


    // Variables for functions
    int                 status;

    u32                 temp, temp2, i;
    u32                 msg_cmd;
    u32                 id;
    u32                 flags;
    u32                 beacon_time;

    u8                  mac_addr[6];
    u8                  mask[6];

    dl_entry          * curr_entry;
    station_info      * curr_station_info;
    interrupt_state_t   prev_interrupt_state;

    switch(cmd_id){


//-----------------------------------------------------------------------------
// WLAN Exp Node Commands that must be implemented in child classes
//-----------------------------------------------------------------------------


        //---------------------------------------------------------------------
        case CMDID_NODE_DISASSOCIATE:
            // Disassociate device from node
            //
            // Message format:
            //     cmd_args_32[0:1]      MAC Address (All 0xFF means all station info)
            //
            // Response format:
            //     resp_args_32[0]       Status
            //
            wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Disassociate\n");

            // Get MAC Address
            wlan_exp_get_mac_addr(&((u32 *)cmd_args_32)[0], &mac_addr[0]);
            id = wlan_exp_get_id_in_associated_stations(&mac_addr[0]);

            status  = CMD_PARAM_SUCCESS;

            if (id == WLAN_EXP_AID_NONE) {
                // If we cannot find the MAC address, return status error
                wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Could not find specified node: ");
                wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, &mac_addr[0]); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");

                status = CMD_PARAM_ERROR;

            } else {
                // If parameter is not the magic number to disassociate all stations
                if (id != WLAN_EXP_AID_ALL) {
                    // Find the station_info entry
                    curr_entry = wlan_mac_high_find_station_info_ADDR( get_station_info_list(), &mac_addr[0]);

                    if (curr_entry != NULL) {
                        curr_station_info = (station_info*)(curr_entry->data);

                        // Disable interrupts so no packets interrupt the disassociate
                        prev_interrupt_state = wlan_mac_high_interrupt_stop();

                        // Deauthenticate station
                        deauthenticate_station(curr_station_info);

                        // Re-enable interrupts
                        wlan_mac_high_interrupt_restore_state(prev_interrupt_state);

                        // Set return parameters and print info to console
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Disassociated node: ");
                        wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, &mac_addr[0]); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");

                    } else {
                        // If we cannot find the MAC address, return status error
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Could not find specified node: ");
                        wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, &mac_addr[0]); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");

                        status = CMD_PARAM_ERROR;
                    }
                } else {
                    // Disable interrupts so no packets interrupt the disassociate
                    prev_interrupt_state = wlan_mac_high_interrupt_stop();

                    // Deauthenticate all stations
                    deauthenticate_stations();

                    // Re-enable interrupts
                    wlan_mac_high_interrupt_restore_state(prev_interrupt_state);

                    // Set return parameters and print info to console
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Disassociated node: ");
                    wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, &mac_addr[0]); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");
                }
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;

        
        //---------------------------------------------------------------------
        case CMDID_NODE_CHANNEL:
            //   - cmd_args_32[0]      - Command
            //   - cmd_args_32[1]      - Channel

            msg_cmd = Xil_Ntohl(cmd_args_32[0]);
            temp    = Xil_Ntohl(cmd_args_32[1]);
            status  = CMD_PARAM_SUCCESS;

            if (msg_cmd == CMD_PARAM_WRITE_VAL) {
                // Set the Channel
                if (wlan_lib_channel_verify(temp) == 0){
                    //
                    // Send Channel Switch Announcement
                    //   NOTE:  We are not sending this at this time b/c it does not look like commercial
                    //       devices honor this message; The WARP nodes do not currently honor this message
                    //       and there are some timing issues that need to be sorted out.
                    //
                    // send_channel_switch_announcement(temp);

                    mac_param_chan = temp;

                    if(my_bss_info != NULL){
                        // The AP uses the value in my_bss_info->chan when constructing beacons and probe response,
                        // not mac_param_chan. In this Reference Design, mac_param_chan and my_bss_info are kept
                        // in lockstep. Keeping them as separate variables, however, allows for flexibility in an
                        // application where the AP wants to temporarily move to a different channel but not shift
                        // the whole BSS to that new channel. In this case, mac_param_chan would change independently
                        // of the channel within the bss_info.
                        my_bss_info->chan = mac_param_chan;
                    }

                    wlan_mac_high_set_channel(mac_param_chan);

                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Set Channel = %d\n", mac_param_chan);

                } else {
                    status  = CMD_PARAM_ERROR;
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node,
                                    "Channel %d is not supported by the node. Staying on Channel %d\n", temp, mac_param_chan);
                }
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);
            resp_args_32[resp_index++] = Xil_Htonl(mac_param_chan);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


//-----------------------------------------------------------------------------
// AP Specific Commands
//-----------------------------------------------------------------------------


        //---------------------------------------------------------------------
        case CMDID_NODE_AP_CONFIG:
            // Set AP configuration flags
            //
            // Message format:
            //     cmd_args_32[0]   Flags
            //                          [ 0] - NODE_AP_CONFIG_FLAG_POWER_SAVING
            //     cmd_args_32[1]   Mask for flags
            //
            // Response format:
            //     resp_args_32[0]  Status (CMD_PARAM_SUCCESS/CMD_PARAM_ERROR)
            //

            // Set the return value
            status = CMD_PARAM_SUCCESS;

            // Get flags
            temp  = Xil_Ntohl(cmd_args_32[0]);
            temp2 = Xil_Ntohl(cmd_args_32[1]);

            wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "AP: Configure flags = 0x%08x  mask = 0x%08x\n", temp, temp2);

            // Configure based on the flag bit / mask
            if ((temp2 & CMD_PARAM_NODE_AP_CONFIG_FLAG_POWER_SAVING) == CMD_PARAM_NODE_AP_CONFIG_FLAG_POWER_SAVING) {
                if ((temp & CMD_PARAM_NODE_AP_CONFIG_FLAG_POWER_SAVING) == CMD_PARAM_NODE_AP_CONFIG_FLAG_POWER_SAVING) {
                    power_save_configuration.enable = 1;
                } else {
                    power_save_configuration.enable = 0;
                }
            }

            // Send response of status
            resp_args_32[resp_index++] = Xil_Htonl( status );

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_AP_DTIM_PERIOD:
            // Command to get / set the number of beacon intervals between DTIM beacons
            //
            // Message format:
            //     cmd_args_32[0]   Command:
            //                          - Write       (CMD_PARAM_WRITE_VAL)
            //                          - Read        (CMD_PARAM_READ_VAL)
            //     cmd_args_32[1]   Number of beacon intervals between DTIM beacons (0 - 255)
            //
            // Response format:
            //     resp_args_32[0]  Status (CMD_PARAM_SUCCESS/CMD_PARAM_ERROR)
            //     resp_args_32[1]  Number of beacon intervals between DTIM beacons (0 - 255)
            //
            msg_cmd = Xil_Ntohl(cmd_args_32[0]);
            temp    = Xil_Ntohl(cmd_args_32[1]);
            status  = CMD_PARAM_SUCCESS;

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    power_save_configuration.dtim_period = (temp & 0xFF);
                break;

                case CMD_PARAM_READ_VAL:
                    temp = power_save_configuration.dtim_period;
                break;

                default:
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);
            resp_args_32[resp_index++] = Xil_Htonl(temp);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_AP_SET_AUTHENTICATION_ADDR_FILTER:
            // Allow / Disallow wireless authentications
            //
            // Message format:
            //     cmd_args_32[0]   Command:
            //                          - Write       (CMD_PARAM_WRITE_VAL)
            //     cmd_args_32[1]   Number of address filters
            //     cmd_args_32[2:N] [Compare address (u64), (Mask (u64)]
            //
            // Response format:
            //     resp_args_32[0]  Status
            //
            msg_cmd = Xil_Ntohl(cmd_args_32[0]);
            temp    = Xil_Ntohl(cmd_args_32[1]);
            status  = CMD_PARAM_SUCCESS;

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    // Need to disable interrupts during this operation so the filter does not have any holes
                    prev_interrupt_state = wlan_mac_high_interrupt_stop();

                    // Reset the current address filter
                    wlan_mac_addr_filter_reset();

                    // Add all the address ranges to the filter
                    for (i = 0; i < temp; i++) {
                        // Extract the address and the mask
                        wlan_exp_get_mac_addr(&((u32 *)cmd_args_32)[2 + (4*i)], &mac_addr[0]);
                        wlan_exp_get_mac_addr(&((u32 *)cmd_args_32)[4 + (4*i)], &mask[0]);

                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Adding Address filter: (");
                        wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, mac_addr); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, ", ");
                        wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, mask); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");

                        if (wlan_mac_addr_filter_add(mask, mac_addr) == -1) {
                            status = CMD_PARAM_ERROR;
                        }
                    }

                    wlan_mac_high_interrupt_restore_state(prev_interrupt_state);
                break;

                default:
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_AP_SET_SSID:
            // Set AP SSID
            //
            // NOTE:  This method does not force any maximum length on the SSID.  However,
            //   the rest of the framework enforces the convention that the maximum length
            //   of the SSID is SSID_LEN_MAX.
            //
            // Message format:
            //     cmd_args_32[0]        Command:
            //                               - Write       (CMD_PARAM_WRITE_VAL)
            //                               - Read        (CMD_PARAM_READ_VAL)
            //     cmd_args_32[1]        SSID Length (write-only)
            //     cmd_args_32[2:N]      SSID        (write-only)
            //
            // Response format:
            //     resp_args_32[0]       Status
            //     resp_args_32[1]       SSID Length
            //     resp_args_32[2:N]     SSID (packed array of ascii character values)
            //                               NOTE: The characters are copied with a straight strcpy
            //                                   and must be correctly processed on the host side
            //
            msg_cmd = Xil_Ntohl(cmd_args_32[0]);
            temp    = Xil_Ntohl(cmd_args_32[1]);
            status  = CMD_PARAM_SUCCESS;

            char * ssid;

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    ssid = (char *)&cmd_args_32[2];

                    // Deauthenticate all stations since we are changing the SSID
                    deauthenticate_stations();
                    strcpy(my_bss_info->ssid, ssid);
                break;

                case CMD_PARAM_READ_VAL:
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "AP: SSID = %s\n", my_bss_info->ssid);
                break;

                default:
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);

            // Return the size and current SSID
            if (my_bss_info->ssid != NULL) {
                temp = strlen(my_bss_info->ssid);

                resp_args_32[resp_index++] = Xil_Htonl(temp);

                strcpy((char *)&resp_args_32[resp_index], my_bss_info->ssid);

                resp_index += (temp / sizeof(resp_args_32)) + 1;
            } else {
                // Return a zero length string
                resp_args_32[resp_index++] = 0;
            }

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_AP_BEACON_INTERVAL:
            // Command to get / set the time interval between beacons
            //
            // Message format:
            //     cmd_args_32[0]   Command:
            //                          - Write       (CMD_PARAM_WRITE_VAL)
            //                          - Read        (CMD_PARAM_READ_VAL)
            //     cmd_args_32[1]   Number of Time Units (TU) between beacons [1, 65535]
            //
            // Response format:
            //     resp_args_32[0]  Status (CMD_PARAM_SUCCESS/CMD_PARAM_ERROR)
            //     resp_args_32[1]  Number of Time Units (TU) between beacons [1, 65535]
            //
            msg_cmd = Xil_Ntohl(cmd_args_32[0]);
            temp    = Xil_Ntohl(cmd_args_32[1]);
            status  = CMD_PARAM_SUCCESS;

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    beacon_time                  = (temp & 0xFFFF) * BSS_MICROSECONDS_IN_A_TU;
                    my_bss_info->beacon_interval = (temp & 0xFFFF);

                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Beacon interval: %d microseconds\n", beacon_time);

                    // Start / Restart the beacon event with the new beacon interval
                    if (beacon_schedule_id != SCHEDULE_FAILURE) {
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Restarting beacon\n");
                        wlan_mac_remove_schedule(SCHEDULE_COARSE, beacon_schedule_id);
                        beacon_schedule_id = wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, beacon_time, SCHEDULE_REPEAT_FOREVER, (void*)beacon_transmit);
                    } else {
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Starting beacon\n");
                        beacon_schedule_id = wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, beacon_time, SCHEDULE_REPEAT_FOREVER, (void*)beacon_transmit);
                    }
                break;

                case CMD_PARAM_READ_VAL:
                    temp = my_bss_info->beacon_interval;
                break;

                default:
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);
            resp_args_32[resp_index++] = Xil_Htonl(temp);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


//-----------------------------------------------------------------------------
// Association Commands
//-----------------------------------------------------------------------------


        //---------------------------------------------------------------------
        case CMDID_NODE_ASSOCIATE:
            // Associate with the device
            //
            // Message format:
            //     cmd_args_32[0]        Association flags
            //                               CMD_PARAM_AP_ASSOCIATE_FLAG_ALLOW_TIMEOUT
            //                               CMD_PARAM_AP_ASSOCIATE_FLAG_STATION_INFO_DO_NOT_REMOVE
            //     cmd_args_32[1]        Association flags mask
            //     cmd_args_32[2:3]      Association MAC Address
            //
            // Response format:
            //     resp_args_32[0]       Status
            //
            wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "AP: Associate\n");

            status            = CMD_PARAM_SUCCESS;
            curr_station_info = NULL;

            // Set default value for the flags
            flags             = STATION_INFO_FLAG_DISABLE_ASSOC_CHECK;

            if (my_bss_info->associated_stations.length < wlan_mac_high_get_max_associations()) {

                // Get MAC Address
                wlan_exp_get_mac_addr(&((u32 *)cmd_args_32)[2], &mac_addr[0]);

                // Get flags
                temp  = Xil_Ntohl(cmd_args_32[0]);
                temp2 = Xil_Ntohl(cmd_args_32[1]);

                wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Associate flags = 0x%08x  mask = 0x%08x\n", temp, temp2);

                // Configure based on the flag bit / mask
                if ((temp2 & CMD_PARAM_AP_ASSOCIATE_FLAG_ALLOW_TIMEOUT) == CMD_PARAM_AP_ASSOCIATE_FLAG_ALLOW_TIMEOUT) {
                    if ((temp & CMD_PARAM_AP_ASSOCIATE_FLAG_ALLOW_TIMEOUT) == CMD_PARAM_AP_ASSOCIATE_FLAG_ALLOW_TIMEOUT) {
                        flags |= STATION_INFO_FLAG_DISABLE_ASSOC_CHECK;
                    } else {
                        flags &= ~STATION_INFO_FLAG_DISABLE_ASSOC_CHECK;
                    }
                }

                if ((temp2 & CMD_PARAM_AP_ASSOCIATE_FLAG_STATION_INFO_DO_NOT_REMOVE) == CMD_PARAM_AP_ASSOCIATE_FLAG_STATION_INFO_DO_NOT_REMOVE) {
                    if ((temp & CMD_PARAM_AP_ASSOCIATE_FLAG_STATION_INFO_DO_NOT_REMOVE) == CMD_PARAM_AP_ASSOCIATE_FLAG_STATION_INFO_DO_NOT_REMOVE) {
                        flags |= STATION_INFO_DO_NOT_REMOVE;
                    } else {
                        flags &= ~STATION_INFO_DO_NOT_REMOVE;
                    }
                }

                // Disable interrupts so no packets interrupt the disassociate
                prev_interrupt_state = wlan_mac_high_interrupt_stop();

                // Add association
                curr_station_info = wlan_mac_high_add_association(&my_bss_info->associated_stations, &counts_table, &mac_addr[0], ADD_ASSOCIATION_ANY_AID);

                // Set the flags
                curr_station_info->flags = flags;

                // Re-enable interrupts
                wlan_mac_high_interrupt_restore_state(prev_interrupt_state);

                // Set return parameters and print info to console
                if (curr_station_info != NULL) {
                    // Log association state change
                    add_station_info_to_log(curr_station_info, STATION_INFO_ENTRY_NO_CHANGE, WLAN_EXP_STREAM_ASSOC_CHANGE);

                    memcpy(&(curr_station_info->tx), &default_unicast_data_tx_params, sizeof(tx_params));

                    // Update the hex display
                    ap_write_hex_display(my_bss_info->associated_stations.length);

                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Associated with node: ");
                } else {
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Could not associate with node: ");
                    status = CMD_PARAM_ERROR;
                }
            } else {
                wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Could not associate with node: ");
                status = CMD_PARAM_ERROR;
            }

            wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, &mac_addr[0]); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);

            if (curr_station_info != NULL) {
                resp_args_32[resp_index++] = Xil_Htonl(curr_station_info->AID);
            } else {
                resp_args_32[resp_index++] = Xil_Htonl(0);
            }

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        default:
            wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown node command: 0x%x\n", cmd_id);
        break;
    }

    return resp_sent;
}



/*****************************************************************************/
/**
 * This will initialize the WLAN Exp AP specific items
 *
 * @param   wlan_exp_type    - WLAN Exp type of the node
 * @param   serial_number    - Serial number of the node
 * @param   fpga_dna         - FPGA DNA of the node
 * @param   eth_dev_num      - Ethernet device to use for WLAN Exp
 * @param   wlan_exp_hw_addr - WLAN Exp hardware address
 * @param   wlan_hw_addr     - WLAN hardware address
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 ******************************************************************************/
int wlan_exp_node_ap_init(u32 wlan_exp_type, u32 serial_number, u32 *fpga_dna, u32 eth_dev_num, u8 *wlan_exp_hw_addr, u8 *wlan_hw_addr) {

    xil_printf("Configuring AP ...\n");

    return XST_SUCCESS;
}



#endif
