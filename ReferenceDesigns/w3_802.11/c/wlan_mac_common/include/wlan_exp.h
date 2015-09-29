/** @file wlan_exp.h
 *  @brief Experiment Framework (Common)
 *
 *  This contains the code for WLAN Experimental Framework.
 *
 *  @copyright Copyright 2014-2015, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *                See LICENSE.txt included in the design archive or
 *                at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

/***************************** Include Files *********************************/
// Include xil_types so function prototypes can use u8/u16/u32 data types
#include "xil_types.h"
#include "warp_hw_ver.h"


/*************************** Constant Definitions ****************************/
#ifndef WLAN_EXP_H_
#define WLAN_EXP_H_


// **********************************************************************
// WLAN Experiment Framework Enable
//
//     NOTE:  To disable the WLAN Experiment Framework, comment out the USE_WLAN_EXP define.
//
#define USE_WLAN_EXP


// **********************************************************************
// Version Information
//

// Version info:  MAJOR.MINOR.REV
//     [31:24]  - MAJOR (u8)
//     [23:16]  - MINOR (u8)
//     [15:0]   - REVISION (u16)
//
#define WLAN_EXP_VER_MAJOR                                 1
#define WLAN_EXP_VER_MINOR                                 5
#define WLAN_EXP_VER_REV                                   0

#define REQ_WLAN_EXP_HW_VER                               (WLAN_EXP_VER_MAJOR << 24)|(WLAN_EXP_VER_MINOR << 16)|(WLAN_EXP_VER_REV)


// WARP Type Information:
//
//     Define the WARP Type to communicate the type of node to the host.  The WARP type is
// divided into two fields:  Hardware Mask, Design Mask.  The Hardware Mask (upper 8 bits)
// communicates the version of WARP hardware that is running the design.  The Design Mask
// (lower 24 bits) is design specific:
//
//   Design Mask                       Values
//   802.11                            0x010000 - 0x01FFFF
//       - Of the lower four digits, the lowest two describe CPU Low, while the upper two
//         describe CPU High.  For example:  in CPU High AP = 0x01; STA = 0x02, while in
//         CPU Low DCF = 0x01.  This creates the following supported combinations.
//
//     802.11 AP DCF                   0x010101
//     802.11 Station DCF              0x010201
//
#define WARP_TYPE_DESIGN_MASK                              0x00FFFFFF
#define WARP_TYPE_DESIGN_80211                             0x00010000

#define WARP_TYPE_DESIGN_80211_CPU_HIGH_MASK               0x0000FF00
#define WARP_TYPE_DESIGN_80211_CPU_HIGH_AP                 0x00000100
#define WARP_TYPE_DESIGN_80211_CPU_HIGH_STA                0x00000200
#define WARP_TYPE_DESIGN_80211_CPU_HIGH_IBSS               0x00000300

#define WARP_TYPE_DESIGN_80211_CPU_LOW_MASK                0x000000FF
#define WARP_TYPE_DESIGN_80211_CPU_LOW_DCF                 0x00000001
#define WARP_TYPE_DESIGN_80211_CPU_LOW_NOMAC               0x00000002



// WARP Hardware Version Information:
//
#ifdef  WARP_HW_VER_v3
#    define WARP_TYPE_HW_VERSION                           0x00000003
#else
#    define WARP_TYPE_HW_VERSION                           0xFFFFFFFF
#endif




// **********************************************************************
// WARP Command Group defines
//
#define GROUP_WARP                                         0xFF
#define GROUP_NODE                                         0x00
#define GROUP_TRANSPORT                                    0x10


// Global WARP commands
//
#define CMDID_WARP_TYPE                                    0xFFFFFF


/*********************** Global Structure Definitions ************************/


/*************************** Function Prototypes *****************************/


#endif /* WLAN_EXP_H_ */
