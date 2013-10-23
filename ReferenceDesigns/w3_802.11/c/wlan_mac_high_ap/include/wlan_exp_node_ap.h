////////////////////////////////////////////////////////////////////////////////
// File   :	wlan_exp_node.h
// Authors:	Chris Hunter (chunter [at] mangocomm.com)
//			Patrick Murphy (murphpo [at] mangocomm.com)
//          Erik Welsh (welsh [at] mangocomm.com)
// License:	Copyright 2013, Mango Communications. All rights reserved.
//			Distributed under the WARP license  (http://warpproject.org/license)
////////////////////////////////////////////////////////////////////////////////


/***************************** Include Files *********************************/
#include "wlan_exp_common.h"



/*************************** Constant Definitions ****************************/
#ifndef WLAN_EXP_NODE_AP_H_
#define WLAN_EXP_NODE_AP_H_



// ****************************************************************************
// Define WLAN Exp Node AP Commands
//
#define NODE_AP_ALLOW_ASSOCIATIONS     100
#define NODE_AP_DISALLOW_ASSOCIATIONS  101
#define NODE_AP_GET_SSID               102
#define NODE_AP_SET_SSID               103


// ****************************************************************************
// Define Node AP Parameters
//   - NOTE:  To add another parameter, add the define before "NODE_MAX_PARAMETER"
//     and then change the value of "NODE_MAX_PARAMETER" to be the largest value
//     in the list so it is easy to iterate over all parameters
//


/*********************** Global Structure Definitions ************************/



/*************************** Function Prototypes *****************************/

int wlan_exp_node_ap_processCmd( unsigned int cmdID, const wn_cmdHdr* cmdHdr, const void* cmdArgs, wn_respHdr* respHdr, void* respArgs, void* pktSrc, unsigned int eth_dev_num);


#endif /* WLAN_EXP_NODE_H_ */
