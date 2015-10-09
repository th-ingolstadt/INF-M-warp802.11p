/** @file wlan_exp_node_ap.h
 *  @brief Access Point WLAN Experiment
 *
 *  This contains code for the 802.11 Access Point's WLAN experiment interface.
 *
 *  @copyright Copyright 2013-2015, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */



/***************************** Include Files *********************************/
#include "wlan_exp_common.h"



/*************************** Constant Definitions ****************************/
#ifndef WLAN_EXP_NODE_AP_H_
#define WLAN_EXP_NODE_AP_H_



// ****************************************************************************
// Define Node Commands
//
// NOTE:  All Command IDs (CMDID_*) must be a 24 bit unique number
//

//-----------------------------------------------
// WLAN Exp Node AP Commands
//
#define CMDID_NODE_AP_CONFIG                                         0x100000
#define CMDID_NODE_AP_DTIM_PERIOD                                    0x100001
#define CMDID_NODE_AP_SET_SSID                                       0x100002
#define CMDID_NODE_AP_SET_AUTHENTICATION_ADDR_FILTER                 0x100003
#define CMDID_NODE_AP_BEACON_INTERVAL                                0x100004

#define CMD_PARAM_NODE_AP_CONFIG_FLAG_POWER_SAVING                   0x00000001

#define CMD_PARAM_AP_ASSOCIATE_FLAG_ALLOW_TIMEOUT                    0x00000001
#define CMD_PARAM_AP_ASSOCIATE_FLAG_STATION_INFO_DO_NOT_REMOVE       0x00000002


// ****************************************************************************
// Define Node AP Parameters
//   - NOTE:  To add another parameter, add the define before "NODE_MAX_PARAMETER"
//     and then change the value of "NODE_MAX_PARAMETER" to be the largest value
//     in the list so it is easy to iterate over all parameters
//


/*********************** Global Structure Definitions ************************/



/*************************** Function Prototypes *****************************/

int wlan_exp_node_ap_init(u32 wlan_exp_type, u32 serial_number, u32 *fpga_dna, u32 eth_dev_num, u8 *wlan_exp_hw_addr, u8 *wlan_hw_addr);

int wlan_exp_node_ap_process_cmd(u32 cmd_id, int socket_index, void * from, cmd_resp * command, cmd_resp * response);


#endif /* WLAN_EXP_NODE_H_ */
