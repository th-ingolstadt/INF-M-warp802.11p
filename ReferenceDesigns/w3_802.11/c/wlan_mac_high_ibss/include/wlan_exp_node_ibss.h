/** @file wlan_exp_node_sta.h
 *  @brief Station WLAN Experiment
 *
 *  This contains code for the 802.11 Station's WLAN experiment interface.
 *
 *  @copyright Copyright 2014-2016, Mango Communications. All rights reserved.
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



/*************************** Constant Definitions ****************************/
#ifndef WLAN_EXP_NODE_STA_H_
#define WLAN_EXP_NODE_STA_H_



// ****************************************************************************
// Define WLAN Exp Node Station Commands
//
#define CMDID_NODE_IBSS_CONFIG                             0x300000


#define CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TS_UPDATE        0x00000001
#define CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TRANSMIT         0x00000002


// ****************************************************************************
// Define Node Station Parameters
//   - NOTE:  To add another parameter, add the define before "NODE_MAX_PARAMETER"
//     and then change the value of "NODE_MAX_PARAMETER" to be the largest value
//     in the list so it is easy to iterate over all parameters
//


/*********************** Global Structure Definitions ************************/



/*************************** Function Prototypes *****************************/

int  wlan_exp_process_node_cmd(u32 cmd_id, int socket_index, void * from, cmd_resp * command, cmd_resp * response, u32 max_resp_len);

void wlan_exp_ibss_tx_cmd_add_association(u8 *mac_addr);

#endif /* WLAN_EXP_NODE_H_ */
