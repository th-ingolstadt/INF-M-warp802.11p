/** @file wlan_mac_common.c
 *  @brief Common Code
 *
 *  This contains code common to both CPU_LOW and CPU_HIGH.
 *
 *  @copyright Copyright 2013-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *                See LICENSE.txt included in the design archive or
 *                at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/***************************** Include Files *********************************/

#include "stdlib.h"
#include "string.h"

#include "xil_io.h"
#include "xstatus.h"
#include "xparameters.h"

#include "wlan_platform_common.h"
#include "wlan_mac_common.h"



/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/

static wlan_mac_hw_info_t         mac_hw_info;


/*************************** Functions Prototypes ****************************/


/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * Null Callback
 *
 * This function will always return XST_SUCCESS and should be used to initialize
 * callbacks.  All input parameters will be ignored.
 *
 * @param   param            - Void pointer for parameters
 *
 * @return  int              - Status:
 *                                 XST_SUCCESS - Command completed successfully
 *****************************************************************************/
int wlan_null_callback(void* param) {
    return XST_SUCCESS;
};



/*****************************************************************************/
/**
 * Verify channel is supported
 *
 * @param   channel          - Channel to verify
 *
 * @return  int              - Channel supported?
 *                                 XST_SUCCESS - Channel supported
 *                                 XST_FAILURE - Channel not supported
 *****************************************************************************/
int wlan_verify_channel(u32 channel) {
    int return_value;

    // The 802.11 reference design allows a subset of 2.4 and 5 GHz channels
    //     Channel number follows 802.11 conventions:
    //         https://en.wikipedia.org/wiki/List_of_WLAN_channels
    //
#if 1
    switch (channel) {
        // 2.4GHz channels
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        // 5GHz channels
        case 36:
        case 40:
        case 44:
        case 48:
            return_value = XST_SUCCESS;
        break;
        default:
            return_value = XST_FAILURE;
        break;
    }
#else
    switch (channel) {
            // 2.4GHz channels
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
            // 5GHz channels
            case 36: // 5180 MHz
            case 38: // 5190 MHz
            case 40: // 5200 MHz
            case 44: // 5220 MHz
            case 46: // 5230 MHz
            case 48: // 5240 MHz
            case 52: // 5260 MHz
            case 54: // 5270 MHz
            case 56: // 5280 MHz
            case 60: // 5300 MHz
            case 62: // 5310 MHz
            case 64: // 5320 MHz
            case 100: // 5500 MHz
            case 102: // 5510 MHz
            case 104: // 5520 MHz
            case 108: // 5540 MHz
            case 110: // 5550 MHz
            case 112: // 5560 MHz
            case 116: // 5580 MHz
            case 118: // 5590 MHz
            case 120: // 5600 MHz
            case 124: // 5620 MHz
            case 126: // 5630 MHz
            case 128: // 5640 MHz
            case 132: // 5660 MHz
            case 134: // 5670 MHz
            case 136: // 5680 MHz
            case 140: // 5700 MHz
            case 142: // 5710 MHz
            case 144: // 5720 MHz
            case 149: // 5745 MHz
            case 151: // 5755 MHz
            case 153: // 5765 MHz
            case 157: // 5785 MHz
            case 159: // 5795 MHz
            case 161: // 5805 MHz
            case 165: // 5825 MHz
            case 172: // 5860 MHz
            case 174: // 5870 MHz
            case 175: // 5875 MHz
            case 176: // 5880 MHz
            case 177: // 5885 MHz
            case 178: // 5890 MHz
                return_value = XST_SUCCESS;
            break;
            default:
            	xil_printf("ERROR (wlan_verify_channel): Channel %d invalid\n", channel);
                return_value = XST_FAILURE;
            break;
        }
#endif

    return return_value;
}


/*****************************************************************************/
/**
 * Initialize the MAC Hardware Info
 *
 * This function will initialize the MAC hardware information structure for
 * the CPU based on information contained in the EEPROM and the wlan_exp_type
 * provided.  This function should only be called after the EEPROM has been
 * initialized.
 *
 * @param   None
 *
 *****************************************************************************/
void init_mac_hw_info() {
	mac_hw_info = wlan_platform_get_hw_info();
}

time_hr_min_sec_t wlan_mac_time_to_hr_min_sec(u64 time) {
	time_hr_min_sec_t time_hr_min_sec;
	u64 time_sec;
	u32 remainder;

	time_sec = time / 1e6;
	remainder = time_sec % 3600;

	time_hr_min_sec.hr = time_sec / 3600;
	time_hr_min_sec.min = remainder / 60;
	time_hr_min_sec.sec = remainder % 60;

	return time_hr_min_sec;
}


/*****************************************************************************/
/**
 * Get the MAC Hardware Info
 *
 * Return the MAC hardware information structure.  This should only be used
 * after the structure is initialized.
 *
 * @return  wlan_mac_hw_info_t *  - Pointer to HW info structure
 *
 *****************************************************************************/
wlan_mac_hw_info_t* get_mac_hw_info() { return &mac_hw_info; }
u8* get_mac_hw_addr_wlan() { return mac_hw_info.hw_addr_wlan; }
u8* get_mac_hw_addr_wlan_exp() { return mac_hw_info.hw_addr_wlan_exp; }

