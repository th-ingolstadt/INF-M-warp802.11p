/** @file wlan_exp_node.h
 *  @brief Experiment Framework
 *
 *  This contains the code for WLAN Experimental Framework.
 *
 *  @copyright Copyright 2013-2015, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *                See LICENSE.txt included in the design archive or
 *                at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */


/***************************** Include Files *********************************/
#include "wlan_exp_common.h"


/*************************** Constant Definitions ****************************/
#ifndef WLAN_EXP_USER_H_
#define WLAN_EXP_USER_H_



// ****************************************************************************
// Define User Commands
//
// NOTE:  All User Command IDs (CMDID_*) must be a 24 bit unique number
//

//-----------------------------------------------
// User Commands
//
// #define CMDID_USER_<COMMAND_NAME>                          0x000000


//-----------------------------------------------
// AP Specific User Command Parameters
//
// #define CMD_PARAM_USER_<VALUE>                             0x00000000


/*********************** Global Structure Definitions ************************/


/*************************** Function Prototypes *****************************/

// User command processing
int  process_user_cmd(int socket_index, void * from, cmd_resp * command, cmd_resp * response, u32 max_resp_len);


#endif
