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

// Xilinx / Standard library includes
#include <xparameters.h>
#include <xil_io.h>

// WLAN include files
#include "wlan_mac_time_util.h"


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



