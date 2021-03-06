/** @file wlan_mac_userio_util.h
 *  @brief User I/O Definitions
 *
 *  This contains WARP User I/O definitions for all CPUs
 *
 *  @copyright Copyright 2013-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */


/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_USERIO_UTIL_H_
#define WLAN_MAC_USERIO_UTIL_H_

//-----------------------------------------------
// Base Addresses
//
#define USERIO_BASEADDR                                    XPAR_W3_USERIO_BASEADDR            ///< XParameters rename of base address of User I/O

//-----------------------------------------------
// Extra definitions not provided by driver
#define W3_USERIO_DIPSW_0	0x08
#define W3_USERIO_DIPSW_1	0x04
#define W3_USERIO_DIPSW_2	0x02
#define W3_USERIO_DIPSW_3	0x01

/*********************** Global Structure Definitions ************************/


/*************************** Function Prototypes *****************************/
u8                 hex_to_seven_segment(u8 hex_value);

void               enable_hex_pwm();
void               disable_hex_pwm();
void               set_hex_pwm_period(u16 period);
void               set_hex_pwm_min_max(u16 min, u16 max);

void               write_hex_display(u8 val);
void               write_hex_display_with_pwm(u8 val);

void               set_hex_display_error_status(u8 status);

void               blink_hex_display(u32 num_blinks, u32 blink_time);

void               set_hex_display_right_dp(u8 val);
void               set_hex_display_left_dp(u8 val);


#endif   // END WLAN_MAC_USERIO_UTIL_H_
