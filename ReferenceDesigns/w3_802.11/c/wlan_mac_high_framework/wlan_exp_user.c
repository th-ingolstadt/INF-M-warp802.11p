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
    //     NOTE:  Some of the standard variables below have been commented out.  This was to remove
    //         compiler warnings for "unused variables" since the default implemention is empty.  As
    //         you add commands, you should un-comment the standard variables.
    //
    u32                      resp_sent      = NO_RESP_SENT;

    cmd_resp_hdr           * cmd_hdr        = command->header;
    // u32                    * cmd_args_32    = command->args;
    u32                      cmd_id         = CMD_TO_CMDID(cmd_hdr->cmd);

    cmd_resp_hdr           * resp_hdr       = response->header;
    // u32                    * resp_args_32   = response->args;
    // u32                      resp_index     = 0;

    // Initialize the response header
    resp_hdr->cmd       = cmd_hdr->cmd;
    resp_hdr->length    = 0;
    resp_hdr->num_args  = 0;

    // Variables for functions
    // int                 status;
    // u32                 arg_0;


    // Process the command
    switch(cmd_id){

//-----------------------------------------------------------------------------
// Common User Commands
//-----------------------------------------------------------------------------

        // Template framework for a Command
        //
        // NOTE:  The WLAN Exp framework assumes that the Over-the-Wire format of the data is
        //     big endian.  However, the node processes data using little endian.  Therefore,
        //     any data received from the host must be properly endian-swapped and similarly,
        //     any data sent to the host must be properly endian-swapped.  The built-in Xilinx
        //     functions:  Xil_Ntohl() (Network to Host) and Xil_Htonl() (Host to Network) are
        //     used for this.
        //
        // //---------------------------------------------------------------------
        // case CMDID_USER_<COMMAND_NAME>:
        //     // Command Description
        //     //
        //     // Message format:
        //     //     cmd_args_32[0:N]    Document command arguments from the host
        //     //
        //     // Response format:
        //     //     resp_args_32[0:M]   Document response arguments from the node
        //     // NOTE:  Variables are declared above.
        //     // NOTE:  Please take care of the endianness of the arguments (see comment above)
        //     //
        //     //
        //
        //     // Set the return value
        //     status      = CMD_PARAM_SUCCESS;
        //     arg_0       = Xil_Ntohl(cmd_args_32[0]);              // Swap endianness of command argument
        //
        //     // Do something with argument(s)
        //     xil_printf("Command argument 0: 0x%08x\n", arg_0);
        //
        //     // Send response
        //     //   NOTE:  It is good practice to send a status as the first argument of the response.
        //     //       This way it is easy to determine if the other data in the response is valid.
        //     //
        //     resp_args_32[resp_index++] = Xil_Htonl(status);       // Swap endianness of response arguments
        //
        //     resp_hdr->length  += (resp_index * sizeof(resp_args_32));
        //     resp_hdr->num_args = resp_index;
        // break;


//-----------------------------------------------------------------------------
// Child Commands (Callback is implemented in each child project, eg. AP, STA, IBSS)
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
