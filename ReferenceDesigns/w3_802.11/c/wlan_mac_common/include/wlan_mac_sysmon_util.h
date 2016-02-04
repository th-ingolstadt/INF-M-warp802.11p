/** @file wlan_mac_misc_util.h
 *  @brief Common Miscellaneous Definitions
 *
 *  This contains miscellaneous definitions of required by both the upper and
 *  lower-level CPUs.
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

/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_SYSMON_UTIL_H_
#define WLAN_MAC_SYSMON_UTIL_H_


/*********************** Global Structure Definitions ************************/

/*************************** Function Prototypes *****************************/
void               init_sysmon();

u32                get_current_temp();
u32                get_min_temp();
u32                get_max_temp();


#endif
