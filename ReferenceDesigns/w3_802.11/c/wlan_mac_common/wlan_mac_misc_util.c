/** @file wlan_mac_misc_util.c
 *  @brief Miscellaneous Utilities
 *
 *  This contains code common to both CPU_LOW and CPU_HIGH that allows them
 *  to interact with the MAC Time and User IO cores.
 *
 *  @copyright Copyright 2013-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

/***************************** Include Files *********************************/

#include "string.h"

#include "xil_io.h"

#include "wlan_mac_misc_util.h"


/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/


/*************************** Functions Prototypes ****************************/


/******************************** Functions **********************************/


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




