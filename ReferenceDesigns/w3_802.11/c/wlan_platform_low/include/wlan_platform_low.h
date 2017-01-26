/** @file wlan_platform_low.h
 *  @brief Platform abstraction header for CPU Low
 *
 *  This contains code for configuring low-level parameters in the PHY and hardware.
 *
 *  @copyright Copyright 2013-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *                See LICENSE.txt included in the design archive or
 *                at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */


#ifndef WLAN_PLATFORM_LOW_
#define WLAN_PLATFORM_LOW_

// Low platform must implement macros for accessing MAC/PHY registers
//  These macros rename the platform-specific macros from xparameters.h
#include "w3_mac_phy_regs.h"

// Functions the low framework must implement
int wlan_platform_low_init();
void wlan_platform_low_set_samp_rate();
void wlan_platform_low_param_handler(u8 mode, u32* payload);
int wlan_platform_low_set_radio_channel(u32 channel);
void wlan_platform_low_set_rx_ant_mode(u32 ant_mode);

#endif /* WLAN_PLATFORM_LOW_ */
