/** @file wlan_exp_node.c
 *  @brief Experiment Framework
 *
 *  This contains the code for WLAN Experiments Framework.
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

// Xilinx / Standard library includes
#include <xparameters.h>
#include <xil_io.h>
#include <xio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// WARP Includes

// WLAN includes

// WLAN Exp includes
#include "wlan_exp_common.h"
#include "wlan_exp_node.h"
#include "wlan_exp_user.h"



#ifdef USE_WLAN_EXP

/*************************** Constant Definitions ****************************/


/*********************** Global Variable Definitions *************************/

extern wlan_exp_function_ptr_t    wlan_exp_user_process_cmd_callback;


/*************************** Functions Prototypes ****************************/


/*************************** Variable Definitions ****************************/


/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * Process User Commands
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
int user_process_cmd(int socket_index, void * from, cmd_resp * command, cmd_resp * response, u32 max_words) {

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
    u32                      resp_sent      = NO_RESP_SENT;

    cmd_resp_hdr           * cmd_hdr        = command->header;
    u32                    * cmd_args_32    = command->args;
    u32                      cmd_id         = CMD_TO_CMDID(cmd_hdr->cmd);

    cmd_resp_hdr           * resp_hdr       = response->header;
    u32                    * resp_args_32   = response->args;
    u32                      resp_index     = 0;

    // Set up the response header
    resp_hdr->cmd       = cmd_hdr->cmd;
    resp_hdr->length    = 0;
    resp_hdr->num_args  = 0;

    // Variables for functions
    u32                      i;
    int                      status;

    u32                      size;


    // Process the command
    switch(cmd_id){

//-----------------------------------------------------------------------------
// Common User Commands
//-----------------------------------------------------------------------------

        //---------------------------------------------------------------------
        case CMDID_USER_ECHO:
            // Echo received information to the UART terminal
            //
            // NOTE:  Variables are declared above.
            // NOTE:  Please take care of the endianness of the arguments (see comment above)
            //
            // Message format:
            //     cmd_args_32[0]      Size in words of received values (N)
            //     cmd_args_32[1:N]    Values
            //
            // Response format:
            //     resp_args_32[0]     Status
            //

            // Set the return value
            status      = CMD_PARAM_SUCCESS;
            size        = Xil_Ntohl(cmd_args_32[0]);

            // Print ECHO header
            xil_printf("Node ECHO Commands (%d):\n", size);

            // Print all values set in the command:
            //     Byte swap all the value words in the message (in place)
            for (i = 1; i < (size + 1); i++) {
                xil_printf("    [%04d] = 0x%08x\n", Xil_Ntohl(cmd_args_32[i]));
            }

            // Send response of status
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


//-----------------------------------------------------------------------------
// Child Commands
//-----------------------------------------------------------------------------

        //---------------------------------------------------------------------
        default:
            // Call standard function in child class to parse parameters implemented there
            resp_sent = wlan_exp_user_process_cmd_callback(cmd_id, socket_index, from, command, response, max_words);
        break;
    }

    return resp_sent;
}


#endif        // End USE_WLAN_EXP
