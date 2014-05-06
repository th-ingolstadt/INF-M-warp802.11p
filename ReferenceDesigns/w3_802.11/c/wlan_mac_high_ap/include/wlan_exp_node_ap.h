/** @file wlan_exp_node_ap.h
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
#define CMDID_NODE_AP_SET_ASSOCIATION_ADDR_FILTER          0x010000
#define CMDID_NODE_AP_SSID                                 0x010001


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
