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

#include "xil_types.h"

// Low platform must implement macros for accessing MAC/PHY registers
//  These macros rename the platform-specific macros from xparameters.h
#include "w3_mac_phy_regs.h"
#include "w3_phy_util.h"

//Forward declarations
enum phy_samp_rate_t;

typedef enum userio_disp_status_t{
	USERIO_DISP_STATUS_GOOD_FCS_EVENT       = 4,
	USERIO_DISP_STATUS_BAD_FCS_EVENT        = 5,
	USERIO_DISP_STATUS_CPU_ERROR    		= 255
} userio_disp_low_status_t;

// Functions the low framework must implement
int wlan_platform_low_init();
void wlan_platform_low_userio_disp_status(userio_disp_low_status_t status, ...);

int wlan_platform_low_set_samp_rate(enum phy_samp_rate_t phy_samp_rate);
void wlan_platform_low_param_handler(u8 mode, u32* payload);
int  wlan_platform_low_set_radio_channel(u32 channel);
void wlan_platform_low_set_rx_ant_mode(u32 ant_mode);
int  wlan_platform_get_rx_pkt_pwr(u8 antenna);
int  wlan_platform_set_pkt_det_min_power(int min_power);
void wlan_platform_set_phy_cs_thresh(int power_thresh);
int wlan_platform_get_rx_pkt_gain(u8 ant);
int wlan_platform_set_radio_tx_power(s8 power);

#endif /* WLAN_PLATFORM_LOW_ */
