/** @file wlan_mac_misc_util.c
 *  @brief Miscellaneous Utilities
 *
 *  This contains code common to both CPU_LOW and CPU_HIGH that allows them
 *  to interact with the MAC Time and User IO cores.
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

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "xstatus.h"
#include "xparameters.h"

#include "w3_userio.h"
#include "wlan_mac_misc_util.h"


/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/


/*************************** Functions Prototypes ****************************/


/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * @brief Get MAC Time
 *
 * The Reference Design includes a 64-bit counter that increments every microsecond
 * and can be updated (ie the MAC time).  This function returns the value of the
 * counter at the time the function is called and is used throughout the framework
 * as a timestamp.  The MAC time can be updated by the set_mac_time_usec() and
 * apply_mac_time_delta_usec() methods.
 *
 * @param   None
 * @return  u64              - Current number of microseconds of MAC time.
 */
u64 get_mac_time_usec() {
    //The MAC time core register interface is only 32-bit, so the 64-bit time
    // is read from two 32-bit registers and reconstructed here.

    u32 time_high_u32;
    u32 time_low_u32;
    u64 time_u64;

    time_high_u32 = Xil_In32(WLAN_MAC_TIME_REG_MAC_TIME_MSB);
    time_low_u32  = Xil_In32(WLAN_MAC_TIME_REG_MAC_TIME_LSB);

    // Catch very rare race when 32-LSB of 64-bit value wraps between the two 32-bit reads
    if((time_high_u32 & 0x1) != (Xil_In32(WLAN_MAC_TIME_REG_MAC_TIME_MSB) & 0x1)) {
        //32-LSB wrapped - start over
        time_high_u32 = Xil_In32(WLAN_MAC_TIME_REG_MAC_TIME_MSB);
        time_low_u32  = Xil_In32(WLAN_MAC_TIME_REG_MAC_TIME_LSB);
    }

    time_u64 = (((u64)time_high_u32) << 32) + ((u64)time_low_u32);

    return time_u64;
}



/*****************************************************************************/
/**
 * @brief Get System Timestamp (Microsecond Counter)
 *
 * The Reference Design includes a 64-bit counter that increments every microsecond
 * and can not be updated (ie the system time).  This function returns the value of
 * the counter at the time the function is called and is used throughout the framework
 * as a timestamp.  The system time can not be updated and reflects the number of
 * microseconds that has past since the hardware booted.
 *
 * @param   None
 * @return  u64              - Current number of microseconds that have elapsed
 *                             since the hardware has booted.
 */
u64 get_system_time_usec() {
    // The MAC time core register interface is only 32-bit, so the 64-bit time
    // is read from two 32-bit registers and reconstructed here.

    u32 time_high_u32;
    u32 time_low_u32;
    u64 time_u64;

    time_high_u32 = Xil_In32(WLAN_MAC_TIME_REG_SYSTEM_TIME_MSB);
    time_low_u32  = Xil_In32(WLAN_MAC_TIME_REG_SYSTEM_TIME_LSB);

    // Catch very rare race when 32-LSB of 64-bit value wraps between the two 32-bit reads
    if((time_high_u32 & 0x1) != (Xil_In32(WLAN_MAC_TIME_REG_SYSTEM_TIME_MSB) & 0x1) ) {
        // 32-LSB wrapped - start over
        time_high_u32 = Xil_In32(WLAN_MAC_TIME_REG_SYSTEM_TIME_MSB);
        time_low_u32  = Xil_In32(WLAN_MAC_TIME_REG_SYSTEM_TIME_LSB);
    }

    time_u64 = (((u64)time_high_u32)<<32) + ((u64)time_low_u32);

    return time_u64;
}



/*****************************************************************************/
/**
 * @brief Set MAC time
 *
 * The Reference Design includes a 64-bit counter that increments every microsecond
 * and can be updated (ie the MAC time).  This function sets the counter value.
 * Some 802.11 handshakes require updating the MAC time to match a partner node's
 * MAC time value (reception of a beacon, for example)
 *
 * @param   new_time         - u64 number of microseconds for the new MAC time of the node
 * @return  None
 */
void set_mac_time_usec(u64 new_time) {

    Xil_Out32(WLAN_MAC_TIME_REG_NEW_MAC_TIME_MSB, (u32)(new_time >> 32));
    Xil_Out32(WLAN_MAC_TIME_REG_NEW_MAC_TIME_LSB, (u32)(new_time & 0xFFFFFFFF));

    Xil_Out32(WLAN_MAC_TIME_REG_CONTROL, (Xil_In32(WLAN_MAC_TIME_REG_CONTROL) & ~WLAN_MAC_TIME_CTRL_REG_UPDATE_MAC_TIME));
    Xil_Out32(WLAN_MAC_TIME_REG_CONTROL, (Xil_In32(WLAN_MAC_TIME_REG_CONTROL) | WLAN_MAC_TIME_CTRL_REG_UPDATE_MAC_TIME));
    Xil_Out32(WLAN_MAC_TIME_REG_CONTROL, (Xil_In32(WLAN_MAC_TIME_REG_CONTROL) & ~WLAN_MAC_TIME_CTRL_REG_UPDATE_MAC_TIME));
}



/*****************************************************************************/
/**
 * @brief Apply time delta to MAC time
 *
 * The Reference Design includes a 64-bit counter that increments every microsecond
 * and can be updated (ie the MAC time).  This function updates the counter value
 * by time_delta microseconds (note that the time delta is an s64 and can be positive
 * or negative).  Some 802.11 handshakes require updating the MAC time to match a
 * partner node's MAC time value (reception of a beacon, for example)
 *
 * @param   time_delta       - s64 number of microseconds to change the MAC time of the node
 * @return  None
 */
void apply_mac_time_delta_usec(s64 time_delta) {

    u64                new_mac_time;

    // Compute the new MAC time based on the current MAC time and the time delta
    new_mac_time = get_mac_time_usec() + time_delta;

    // Update the time in the MAC Time HW core
    set_mac_time_usec(new_mac_time);
}



/*****************************************************************************/
/**
 * @brief Sleep delay (in microseconds)
 *
 * Function will delay execution for the specified amount of time.
 *
 * NOTE:  This function is based on the system timestamp so it will not be affected
 *     by updates to the MAC time.
 *
 * @param   delay            - Time to sleep in microseconds (u64)
 * @return  None
 */
void usleep(u64 delay) {
    u64 timestamp = get_system_time_usec();
    while (get_system_time_usec() < (timestamp + delay)) {}
    return;
}



/*****************************************************************************/
/**
 * @brief Convert a string to a number
 *
 * @param   str              - String to convert (char *)
 * @return  int              - Integer value of the string
 *
 * @note    For now this only works with non-negative values
 */
int str2num(char* str) {
    u32 i;
    u8  decade_index;
    int multiplier;
    int return_value  = 0;
    u8  string_length = strlen(str);

    for(decade_index = 0; decade_index < string_length; decade_index++){
        multiplier = 1;
        for(i = 0; i < (string_length - 1 - decade_index) ; i++){
            multiplier = multiplier*10;
        }
        return_value += multiplier*(u8)(str[decade_index] - 48);
    }

    return return_value;
}



/*****************************************************************************/
/**
 * @brief Mapping of hexadecimal values to the 7-segment display
 *
 * @param   hex_value        - Hexadecimal value to be converted (between 0 and 15)
 * @return  u8               - LED map value of the 7-segment display
 */
u8 hex_to_seven_segment(u8 hex_value) {
    switch(hex_value) {
        case(0x0) : return 0x3F;
        case(0x1) : return 0x06;
        case(0x2) : return 0x5B;
        case(0x3) : return 0x4F;
        case(0x4) : return 0x66;
        case(0x5) : return 0x6D;
        case(0x6) : return 0x7D;
        case(0x7) : return 0x07;
        case(0x8) : return 0x7F;
        case(0x9) : return 0x6F;

        case(0xA) : return 0x77;
        case(0xB) : return 0x7C;
        case(0xC) : return 0x39;
        case(0xD) : return 0x5E;
        case(0xE) : return 0x79;
        case(0xF) : return 0x71;
        default   : return 0x00;
    }
}



/*****************************************************************************/
/**
 * @brief Enable the PWM functionality of the hex display
 *
 * This function will tell the User I/O to enable the PWM to pulse the hex display.
 *
 * @param   None
 * @return  None
 */
void enable_hex_pwm() {
    userio_set_pwm_ramp_en(USERIO_BASEADDR, 1);
}



/*****************************************************************************/
/**
 * @brief Disable the PWM functionality of the hex display
 *
 * This function will tell the User I/O to disable the PWM to pulse the hex display.
 *
 * @param   None
 * @return  None
 */
void disable_hex_pwm() {
    userio_set_pwm_ramp_en(USERIO_BASEADDR, 0);
}



/*****************************************************************************/
/**
 * @brief Set the PWM period for the Hex display
 *
 * This function will set the period used to pulse the hex display
 *
 * @param   period           - Period of the PWM ramp (u16)
 * @return  None
 */
void set_hex_pwm_period(u16 period) {
    userio_set_pwm_period(USERIO_BASEADDR, period);
}



/*****************************************************************************/
/**
 * @brief Set the Min / Max timing for the PWM ramp
 *
 * This function will set the Min / Max timing parameters for the PWM pulse of
 * the hex display.  The values should be less that the period set by
 * set_hex_pwm_period(), but that condition is not enforced.
 *
 * @param   min              - Timing parameter for when the PWM reaches minimum value (u16)
 * @param   max              - Timing parameter for when the PWM reaches maximum value (u16)
 * @return  None
 *
 * @note   This function will disable the PWM.  Therefore, the PWM must be enabled
 *     after calling this function.
 */
void set_hex_pwm_min_max(u16 min, u16 max) {
    // Ramp must be disabled when changing ramp params
    userio_set_pwm_ramp_en(USERIO_BASEADDR, 0);
    userio_set_pwm_ramp_min(USERIO_BASEADDR, min);
    userio_set_pwm_ramp_max(USERIO_BASEADDR, max);
}



/*****************************************************************************/
/**
 * @brief Write a Decimal Value to the Hex Display
 *
 * This function will write a decimal value to the board's two-digit hex displays.
 * The display is right justified; WLAN Exp will indicate its connection state
 * using the right decimal point.
 *
 * @param   val              - Value to be displayed (between 0 and 99)
 * @return  None
 */
void write_hex_display(u8 val) {
    u32 right_dp;
    u8  left_val;
    u8  right_val;

    // Need to retain the value of the right decimal point
    right_dp = userio_read_hexdisp_right(USERIO_BASEADDR) & W3_USERIO_HEXDISP_DP;

    userio_write_control(USERIO_BASEADDR, (userio_read_control(USERIO_BASEADDR) & (~(W3_USERIO_HEXDISP_L_MAPMODE | W3_USERIO_HEXDISP_R_MAPMODE))));

    if ( val < 10 ) {
        left_val  = 0;
        right_val = hex_to_seven_segment(val);
    } else {
        left_val  = hex_to_seven_segment(((val / 10) % 10));
        right_val = hex_to_seven_segment((val % 10));
    }

    userio_write_hexdisp_left(USERIO_BASEADDR, left_val);
    userio_write_hexdisp_right(USERIO_BASEADDR, (right_val | right_dp));
}



/*****************************************************************************/
/**
 * @brief Write a Decimal Value to the Hex Display with PWM pulsing
 *
 * This function will write a decimal value to the board's two-digit hex displays.
 * The display is right justified and will pulse using the pwms; WLAN Exp will
 * indicate its connection state using the right decimal point.
 *
 * @param   val              - Value to be displayed (between 0 and 99)
 * @return  None
 */
void write_hex_display_with_pwm(u8 val) {
    u32 hw_control;
    u32 temp_control;
    u32 right_dp;
    u8  left_val;
    u8  right_val;
    u32 pwm_val;

    // Need to retain the value of the right decimal point
    right_dp = userio_read_hexdisp_right(USERIO_BASEADDR) & W3_USERIO_HEXDISP_DP;

    if ( val < 10 ) {
        left_val  = 0;
        right_val = hex_to_seven_segment(val);
    } else {
        left_val  = hex_to_seven_segment(((val/10)%10));
        right_val = hex_to_seven_segment((val%10));
    }

    // Store the original value of what is under HW control
    hw_control   = userio_read_control(USERIO_BASEADDR);

    // Need to zero out all of the HW control of the hex displays; Change to raw hex mode
    temp_control = (hw_control & (~(W3_USERIO_HEXDISP_L_MAPMODE | W3_USERIO_HEXDISP_R_MAPMODE | W3_USERIO_CTRLSRC_HEXDISP_R | W3_USERIO_CTRLSRC_HEXDISP_L)));

    // Set the hex display mode to raw bits
    userio_write_control(USERIO_BASEADDR, temp_control);

    // Write the display
    userio_write_hexdisp_left(USERIO_BASEADDR, left_val);
    userio_write_hexdisp_right(USERIO_BASEADDR, (right_val | right_dp));

    pwm_val   = (right_val << 8) + left_val;

    // Set the HW / SW control of the user io (raw mode w/ the new display value)
    userio_write_control(USERIO_BASEADDR, (temp_control | pwm_val));

    // Set the pins that are using PWM mode
    userio_set_hw_ctrl_mode_pwm(USERIO_BASEADDR, pwm_val);
}



/*****************************************************************************/
/**
 * @brief Set Error Status for Node
 *
 * Function will set the hex display to be "Ex", where x is the value of the
 * status error
 *
 * @param   status           - Number from 0 - 0xF to indicate status error
 * @return  None
 */
void set_hex_display_error_status(u8 status) {
    u32 right_dp;

    // Need to retain the value of the right decimal point
    right_dp = userio_read_hexdisp_right( USERIO_BASEADDR ) & W3_USERIO_HEXDISP_DP;

    userio_write_control(USERIO_BASEADDR, (userio_read_control(USERIO_BASEADDR) & (~(W3_USERIO_HEXDISP_L_MAPMODE | W3_USERIO_HEXDISP_R_MAPMODE))));

    userio_write_hexdisp_left(USERIO_BASEADDR,  hex_to_seven_segment(0xE));
    userio_write_hexdisp_right(USERIO_BASEADDR, (hex_to_seven_segment(status % 16) | right_dp));
}



/*****************************************************************************/
/**
 * @brief Blink LEDs
 *
 * For WARP v3 Hardware, this function will blink the hex display.
 *
 * @param   num_blinks       - Number of blinks (0 means blink forever)
 * @param   blink_time       - Time in microseconds between blinks
 *
 * @return  None
 */
void blink_hex_display(u32 num_blinks, u32 blink_time) {
    u32          i;
    u32          hw_control;
    u32          temp_control;
    u8           right_val;
    u8           left_val;

    // Get left / right values
    left_val  = userio_read_hexdisp_left(USERIO_BASEADDR);
    right_val = userio_read_hexdisp_right(USERIO_BASEADDR);

    // Store the original value of what is under HW control
    hw_control   = userio_read_control(USERIO_BASEADDR);

    // Need to zero out all of the HW control of the hex displays; Change to raw hex mode
    temp_control = (hw_control & (~(W3_USERIO_HEXDISP_L_MAPMODE | W3_USERIO_HEXDISP_R_MAPMODE | W3_USERIO_CTRLSRC_HEXDISP_R | W3_USERIO_CTRLSRC_HEXDISP_L)));

    // Set the hex display mode to raw bits
    userio_write_control(USERIO_BASEADDR, temp_control);

    // Use usleep to blink the display
    if ( num_blinks > 0 ) {
        // Perform standard blink
        for(i = 0; i < num_blinks; i++) {
            userio_write_hexdisp_left(USERIO_BASEADDR,  (((i % 2) == 0) ? left_val  : 0x00));
            userio_write_hexdisp_right(USERIO_BASEADDR, (((i % 2) == 0) ? right_val : 0x00));
            usleep(blink_time);
        }
    } else {
        // Perform an infinite blink
        i = 0;
        while(1) {
            userio_write_hexdisp_left(USERIO_BASEADDR,  (((i % 2) == 0) ? left_val  : 0x00));
            userio_write_hexdisp_right(USERIO_BASEADDR, (((i % 2) == 0) ? right_val : 0x00));
            usleep(blink_time);
            i++;
        }
    }

    // Set control back to original value
    userio_write_control( USERIO_BASEADDR, hw_control );
}



/*****************************************************************************/
/**
 * @brief Set Right Decimal Point on the Hex Display
 *
 * This function will set the right decimal point on the hex display
 *
 * @param   val              - 0 - Clear decimal point; 1 - Set decimal point
 * @return  None
 */
void set_hex_display_right_dp(u8 val) {
    if (val) {
        userio_write_hexdisp_right(USERIO_BASEADDR, (userio_read_hexdisp_right(USERIO_BASEADDR) | W3_USERIO_HEXDISP_DP));
    } else {
        userio_write_hexdisp_right(USERIO_BASEADDR, (userio_read_hexdisp_right(USERIO_BASEADDR) & ~W3_USERIO_HEXDISP_DP));
    }
}



/*****************************************************************************/
/**
 * @brief Set Left Decimal Point on the Hex Display
 *
 * This function will set the left decimal point on the hex display
 *
 * @param   val              - 0 - Clear decimal point; 1 - Set decimal point
 * @return  None
 */
void set_hex_display_left_dp(u8 val) {
    if (val) {
        userio_write_hexdisp_left(USERIO_BASEADDR, (userio_read_hexdisp_left(USERIO_BASEADDR) | W3_USERIO_HEXDISP_DP));
    } else {
        userio_write_hexdisp_left(USERIO_BASEADDR, (userio_read_hexdisp_left(USERIO_BASEADDR) & ~W3_USERIO_HEXDISP_DP));
    }
}








