/** @file wlan_mac_ap_mac_filter.c
 *  @brief Access Point MAC Filter
 *
 *  This contains code for the 802.11 Access Point's MAC address
 *  filtering subsystem.
 *
 *  @copyright Copyright 2014, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 *  @bug No known bugs
 */

#include "stdio.h"
#include "xil_types.h"
#include "wlan_mac_ap_mac_filter.h"

static u8 mac_filter_mode = FILTER_MODE_ALLOW_ALL;

///For the mode_allow_mask, bits that are 1 are treated as "any" and bits that are 0 are treated as "must equal"
///By default, these two lines of code will filter out all non-Mango MAC addresses
static u8 mode_allow_range_mask[6]      = { 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF };
static u8 mode_allow_range_compare[6]	= { 0x40, 0xD8, 0x55, 0x04, 0x20, 0x00 }; ///< This is the started address of the range of
																				  ///< Mango-owned MAC addresses

#define NUM_WHITELIST_NODES 2
///For the whitelist mode, this array explicitly lists every allowed MAC address. Any address not
///present in this array will be filtered out
static u8 mode_whitelist_compare[2][6] = { { 0x40, 0xD8, 0x55, 0x04, 0x21, 0x4A }, \
										   { 0x40, 0xD8, 0x55, 0x04, 0x21, 0x3A } };

void set_mac_filter_mode(u8 mode){
	mac_filter_mode = mode;
}

u8 mac_filter_is_allowed(u8* addr){
	u8 i;
	u32 sum;

	sum = 0;

	switch(mac_filter_mode){
		case FILTER_MODE_ALLOW_ALL:
			return 1;
		break;
		case FILTER_MODE_ALLOW_RANGE:
			for(i=0; i<6; i++){
				sum += (mode_allow_range_mask[i] | mode_allow_range_compare[i]) == (mode_allow_range_mask[i] | addr[i]);
			}
			if(sum == 6) return 1;
		break;
		case FILTER_MODE_WHITELIST:
			return 0;
		break;
	}
	return 0;
}

