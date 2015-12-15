/** @file wlan_exp_node_sta.c
 *  @brief Station WLAN Experiment
 *
 *  This contains code for the 802.11 Station's WLAN experiment interface.
 *
 *  @copyright Copyright 2014-2015, Mango Communications. All rights reserved.
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
#include "wlan_exp_node.h"
#include "wlan_exp_node_ibss.h"

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
#include "wlan_mac_event_log.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_bss_info.h"
#include "wlan_mac_scan_fsm.h"
#include "wlan_mac_ibss_join_fsm.h"
#include "wlan_mac_ibss.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_entries.h"


/*************************** Constant Definitions ****************************/

#define WLAN_EXP_IBSS_JOIN_IDLE                  0x00
#define WLAN_EXP_IBSS_JOIN_RUN                   0x01


/*********************** Global Variable Definitions *************************/
extern dl_list        association_table;
extern dl_list        counts_table;

extern u8             pause_data_queue;
extern u32            mac_param_chan;

extern u8             allow_beacon_ts_update;
extern u8             enable_beacon_tx;

extern bss_info*      my_bss_info;
extern u32            beacon_sched_id;


/*************************** Variable Definitions ****************************/

volatile u8           join_success = WLAN_EXP_IBSS_JOIN_IDLE;



/*************************** Functions Prototypes ****************************/

void wlan_exp_ibss_join_success(bss_info* bss_description);


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
 * @param   max_resp_len     - Maximum number of u32 words allowed in response
 *
 * @return  int              - Status of the command:
 *                                 NO_RESP_SENT - No response has been sent
 *                                 RESP_SENT    - A response has been sent
 *
 * @note    See on-line documentation for more information about the Ethernet
 *          packet structure:  www.warpproject.org
 *
 *****************************************************************************/
int wlan_exp_process_node_cmd(u32 cmd_id, int socket_index, void * from, cmd_resp * command, cmd_resp * response, u32 max_resp_len) {

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
    u32                 success;

    u32                 temp, temp2, i;
    u32                 msg_cmd;

    u32                 length;
    u32                 enable;
    u32                 timeout;

    u8                * channel_list;
    u8                  mac_addr[6];

    u64                 end_timestamp;
    u64                 curr_timestamp;

    char              * ssid;

    bss_info          * temp_bss_info;
    bss_info_entry    * temp_bss_info_entry;

    switch(cmd_id){


//-----------------------------------------------------------------------------
// WLAN Exp Node Commands that must be implemented in child classes
//-----------------------------------------------------------------------------


        //---------------------------------------------------------------------
        case CMDID_NODE_DISASSOCIATE:
            // Disassociate from the AP
            //
            // Message format:
            //     cmd_args_32[0:1]      MAC Address (All 0xFF means all station info)
            //
            // Response format:
            //     resp_args_32[0]       Status
            //
            // wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Disassociate\n");

            status  = CMD_PARAM_SUCCESS;

            leave_ibss();

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_CHANNEL:
            // Message format:
            //   - cmd_args_32[0]      - Command
            //   - cmd_args_32[1]      - Channel
            //
            // Setting the channel via command is not supported for IBSS.  To change the channel, you should:
            //     - Disassociate from the current BSS;
            //     - Create a new BBS info;
            //     - Join the new BSS
            // See TBD for more information
            //
            msg_cmd = Xil_Ntohl(cmd_args_32[0]);
            temp    = Xil_Ntohl(cmd_args_32[1]);
            status  = CMD_PARAM_SUCCESS;

            if ( msg_cmd == CMD_PARAM_WRITE_VAL ) {
                status  = CMD_PARAM_ERROR;
                wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Setting Channel via command not supported for IBSS\n");
                wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "  See documentation for how to change channels for IBSS\n");
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);
            resp_args_32[resp_index++] = Xil_Htonl(mac_param_chan);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


//-----------------------------------------------------------------------------
// IBSS Specific Commands
//-----------------------------------------------------------------------------


        //---------------------------------------------------------------------
        case CMDID_NODE_IBSS_CONFIG:
            // CMDID_NODE_IBSS_CONFIG Packet Format:
            //   - cmd_args_32[0]  - flags
            //                         [ 0] - NODE_CONFIG_FLAG_BEACON_TS_UPDATE
            //                         [ 1] - NODE_CONFIG_FLAG_BEACON_TRANSMIT
            //   - cmd_args_32[1]  - mask for flags
            //
            //   - resp_args_32[0] - CMD_PARAM_SUCCESS
            //                     - CMD_PARAM_ERROR

            // Set the return value
            status = CMD_PARAM_SUCCESS;

            // Get flags
            temp  = Xil_Ntohl(cmd_args_32[0]);
            temp2 = Xil_Ntohl(cmd_args_32[1]);

            wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "IBSS: Configure flags = 0x%08x  mask = 0x%08x\n", temp, temp2);

            // Configure based on the flag bit / mask
            if ((temp2 & CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TS_UPDATE) == CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TS_UPDATE) {
                if ((temp & CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TS_UPDATE) == CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TS_UPDATE) {
                    allow_beacon_ts_update = 1;
                } else {
                    allow_beacon_ts_update = 0;
                }
            }

            if ((temp2 & CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TRANSMIT) == CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TRANSMIT) {
                if ((temp & CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TRANSMIT) == CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TRANSMIT) {
                    enable_beacon_tx = 1;
                } else {
                    enable_beacon_tx = 0;
                }
            }

            // Send response of status
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


//-----------------------------------------------------------------------------
// Common STA / IBSS Command
//-----------------------------------------------------------------------------


        //---------------------------------------------------------------------
        case CMDID_NODE_SCAN_PARAM:
            // Set the active scan parameters
            //
            // Message format:
            //     cmd_args_32[0]    Command:
            //                           - Write       (NODE_WRITE_VAL)
            //     cmd_args_32[1]    Time per channel (in microseconds)
            //                         (or CMD_PARAM_NODE_TIME_RSVD_VAL if not setting the parameter)
            //     cmd_args_32[2]    Idle time per loop (in microseconds)
            //                         (or CMD_PARAM_NODE_TIME_RSVD_VAL if not setting the parameter)
            //     cmd_args_32[3]    Length of channel list
            //                         (or CMD_PARAM_RSVD if not setting channel list)
            //     cmd_args_32[4:N]  Channel
            //
            // Response format:
            //     resp_args_32[0]   Status
            //
            status  = CMD_PARAM_SUCCESS;
            msg_cmd = Xil_Ntohl(cmd_args_32[0]);

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Set Scan Parameters\n");
                    // Set the timing parameters
                    temp  = Xil_Ntohl(cmd_args_32[1]);       // Time per channel
                    temp2 = Xil_Ntohl(cmd_args_32[2]);       // Idle time per loop

                    if ((temp != CMD_PARAM_NODE_TIME_RSVD_VAL) && (temp2 != CMD_PARAM_NODE_TIME_RSVD_VAL)) {
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Time per channel   = %d us\n", temp);
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Idle time per loop = %d us\n", temp2);
                        wlan_mac_set_scan_timings(temp, temp2);
                    }

                    // Set the scan channels
                    length = Xil_Ntohl(cmd_args_32[3]);

                    if (length != CMD_PARAM_RSVD){
                        channel_list = wlan_mac_high_malloc(length);

                        for (i = 0; i < length; i++) {
                            channel_list[i] = Xil_Ntohl(cmd_args_32[4 + i]);
                        }

                        if (wlan_mac_set_scan_channels(channel_list, length) != 0) {
                            status = CMD_PARAM_ERROR;
                        }

                        if (status == CMD_PARAM_SUCCESS) {
                            wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Channels = ");
                            for (i = 0; i < length; i++) {
                                wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "%d ",channel_list[i]);
                            }
                            wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");
                        }

                        wlan_mac_high_free(channel_list);
                    }
                break;

                default:
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response of status
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_SCAN:
            // Enable / Disable active scan
            //
            // Message format:
            //     cmd_args_32[0]   Enable / Disable active scan
            //     cmd_args_32[1:2] BSSID (or CMD_PARAM_RSVD_MAC_ADDR if BSSID not set)
            //     cmd_args_32[3]   SSID Length
            //     cmd_args_32[4:N] SSID (packed array of ascii character values)
            //                          NOTE: The characters are copied with a straight strcpy
            //                              and must be correctly processed on the host side
            //
            // Response format:
            //     resp_args_32[0]  Status
            //
            status  = CMD_PARAM_SUCCESS;
            enable  = Xil_Ntohl(cmd_args_32[0]);

            if (enable == CMD_PARAM_NODE_SCAN_ENABLE) {
                // Enable active scan
                wlan_exp_get_mac_addr(&cmd_args_32[1], &mac_addr[0]);

                ssid = (char *)&cmd_args_32[4];

                wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Active scan enabled for SSID '%s'  BSSID: ", ssid);
                wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, &mac_addr[0]); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");

                wlan_mac_scan_enable(&mac_addr[0], ssid);
            } else {
                // Disable active scan
                wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Active scan disabled.\n");
                wlan_mac_scan_disable();
            }

            // Send response of status
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
       break;


        //---------------------------------------------------------------------
        case CMDID_NODE_JOIN:
            // Join the given BSS
            //
            // Message format:
            //     cmd_args_32[0]   Timeout (ignored for IBSS)
            //     cmd_args_32[1]   BSS info entry length
            //     cmd_args_32[2:N] BSS info entry buffer (packed bytes)
            //
            // Response format:
            //     resp_args_32[0]  Status
            //     resp_args_32[1]  Success (CMD_PARAM_NODE_JOIN_SUCCEEDED)
            //                      Failure (CMD_PARAM_NODE_JOIN_FAILED)
            //
            status  = CMD_PARAM_SUCCESS;
            success = CMD_PARAM_NODE_JOIN_SUCCEEDED;

            wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Joining the BSS\n");

            temp_bss_info_entry = (bss_info_entry *)&cmd_args_32[2];

            temp_bss_info       = wlan_mac_high_create_bss_info(temp_bss_info_entry->info.bssid,
                                                                temp_bss_info_entry->info.ssid,
                                                                temp_bss_info_entry->info.chan);

            if (temp_bss_info != NULL) {
                // Copy all the parameters
                //   NOTE:  Even though this copies some things twice, this is done so that this function does not
                //          need to be modified if the parameters in the bss_info change.
                //
                memcpy((void *)(temp_bss_info), (void *)(&temp_bss_info_entry->info), sizeof(bss_info_base));
                temp_bss_info->latest_activity_timestamp = get_system_time_usec();

                // Enforce ownership over this bss_info so that the framework cannot purge it
                temp_bss_info->state = BSS_STATE_OWNED;

                // Overwrite capabilities in case user didn't set this properly. The capabilities
                // are immutable anyway.
                temp_bss_info->capabilities = (CAPABILITIES_SHORT_TIMESLOT | CAPABILITIES_IBSS);

                // Join the BSS
                wlan_mac_ibss_join(temp_bss_info);

            } else {
                status  = CMD_PARAM_ERROR;
            }

            // Send response of status
            resp_args_32[resp_index++] = Xil_Htonl(status);
            resp_args_32[resp_index++] = Xil_Htonl(success);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_SCAN_AND_JOIN:
            // Scan for the given network and join if present
            //
            // Message format:
            //     cmd_args_32[0]   Timeout for scan (in seconds)
            //     cmd_args_32[1:2] BSSID (or CMD_PARAM_RSVD_MAC_ADDR if BSSID not set)
            //     cmd_args_32[3]   SSID Length
            //     cmd_args_32[4:N] SSID (packed array of ascii character values)
            //                          NOTE: The characters are copied with a straight strcpy
            //                              and must be correctly processed on the host side
            //
            // Response format:
            //     resp_args_32[0]  Status
            //     resp_args_32[1]  Success (CMD_PARAM_NODE_JOIN_SUCCEEDED)
            //                      Failure (CMD_PARAM_NODE_JOIN_FAILED)
            //
            status  = CMD_PARAM_SUCCESS;
            success = CMD_PARAM_NODE_JOIN_SUCCEEDED;
            timeout = Xil_Ntohl(cmd_args_32[0]);

            // Get SSID
            ssid = (char *)&cmd_args_32[4];

            // Scan and join the SSID
            //   NOTE:  The scan and join method returns immediately.  Therefore, we have to wait until
            //          we have successfully joined the network or we have timed out.
            wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Scan and join SSID '%s' ... ", ssid);

            if (timeout > 1000000) {
                wlan_exp_printf(WLAN_EXP_PRINT_WARNING, print_type_node, "Scan timeout of %d seconds is very large.\n", timeout);
            }

            join_success     = WLAN_EXP_IBSS_JOIN_RUN;
            curr_timestamp   = get_system_time_usec();
            end_timestamp    = curr_timestamp + (timeout * 1000000);         // Convert to microseconds for the usec timer

            wlan_mac_ibss_scan_and_join(ssid, timeout);

            while(join_success == WLAN_EXP_IBSS_JOIN_RUN) {
                if (curr_timestamp > end_timestamp) {
                    success = CMD_PARAM_NODE_JOIN_FAILED;
                    break;
                }
                // Sleep for 0.1 seconds before next check
                usleep(100000);
                curr_timestamp = get_system_time_usec();
            }

            // Indicate on the UART if we were successful in joining the network
            if (success == CMD_PARAM_NODE_JOIN_SUCCEEDED) {
                wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "SUCCEEDED\n");
            } else {
                wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "FAILED\n");
            }

            // Send response of status
            resp_args_32[resp_index++] = Xil_Htonl(status);
            resp_args_32[resp_index++] = Xil_Htonl(success);

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
 * This will initialize the WLAN Exp IBSS specific items
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
 * @note    Function name must not collide with wlan_exp_node_init
 *
 ******************************************************************************/
int wlan_exp_node_ibss_init(u32 wlan_exp_type, u32 serial_number, u32 *fpga_dna, u32 eth_dev_num, u8 *wlan_exp_hw_addr, u8 *wlan_hw_addr) {

    xil_printf("Configuring IBSS ...\n");

    wlan_mac_ibss_set_join_success_callback((void *)wlan_exp_ibss_join_success);

    return XST_SUCCESS;
}




/*****************************************************************************/
/**
 * Used by join_success_callback in wlan_mac_ibss_join_fsm.c
 *
 * @param   bss_description  - Pointer to BSS info of the BSS that was just joined
 *
 * @return  None
 *
 *****************************************************************************/
void wlan_exp_ibss_join_success(bss_info* bss_description) {

    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Successfully joined:  %s\n", bss_description->ssid);

    // Set global variable back to idle to indicate successful join
    join_success = WLAN_EXP_IBSS_JOIN_IDLE;

}




/*****************************************************************************/
/**
 * Used by wlan_exp_cmd_add_association_callback in wlan_exp_node.c
 *
 * @param   mac_addr         - Pointer to MAC address association that will be added
 *
 * @return  None
 *
 *****************************************************************************/
void wlan_exp_ibss_tx_cmd_add_association(u8* mac_addr) {

    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Adding association for:  ");
    wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, mac_addr); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");

    wlan_mac_high_add_association(&my_bss_info->associated_stations, &counts_table, mac_addr, ADD_ASSOCIATION_ANY_AID);
}


#endif
