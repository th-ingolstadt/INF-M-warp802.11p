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

// Library includes
#include "string.h"
#include "stdlib.h"

// WLAN includes
#include "wlan_mac_event_log.h"
#include "wlan_mac_bss_info.h"
#include "wlan_mac_ibss.h"


/*************************** Constant Definitions ****************************/


/*********************** Global Variable Definitions *************************/
extern dl_list        counts_table;

extern bss_info*      my_bss_info;


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

    // u32               * cmd_args_32    = command->args;

    cmd_resp_hdr      * resp_hdr       = response->header;
    u32               * resp_args_32   = response->args;
    u32                 resp_index     = 0;

    //
    // NOTE: Response header cmd, length, and num_args fields have already been initialized.
    //

    switch(cmd_id){

//-----------------------------------------------------------------------------
// WLAN Exp Node Commands that must be implemented in child classes
//-----------------------------------------------------------------------------


        //---------------------------------------------------------------------
        case CMDID_NODE_DISASSOCIATE: {
            // Disassociate from the IBSS
            //
            // Message format:
            //     cmd_args_32[0:1]      MAC Address (All 0xFF means all station info)
            //
            // Response format:
            //     resp_args_32[0]       Status
            //
            u32    status         = CMD_PARAM_SUCCESS;

            wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Disassociate\n");

            leave_ibss();

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


//-----------------------------------------------------------------------------
// IBSS Specific Commands
//-----------------------------------------------------------------------------


        //---------------------------------------------------------------------
        default: {
            wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown node command: 0x%x\n", cmd_id);
        }
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

    return XST_SUCCESS;
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
