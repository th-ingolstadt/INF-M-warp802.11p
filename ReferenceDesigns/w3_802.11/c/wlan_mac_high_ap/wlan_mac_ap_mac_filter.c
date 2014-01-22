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

///For the filter_range_mask, bits that are 1 are treated as "any" and bits that are 0 are treated as "must equal"
///For the filter_range_compare, locations of zeroed bits in the mask must match filter_range_compare for incoming addresses

//Any Addresses
static u8 filter_range_mask[6]    = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static u8 filter_range_compare[6]   = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

//No Range of Addresses
//static u8 filter_range_mask[6]      = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//static u8 filter_range_compare[6]   = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

//Mango-only Addresses
//static u8 filter_range_mask[6]      = { 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF };
//static u8 filter_range_compare[6]	  = { 0x40, 0xD8, 0x55, 0x04, 0x20, 0x00 };


#define NUM_WHITELIST_NODES 2
/// whitelist_compare contains all addresses that are explicitly allowed on the network
static u8 whitelist_compare[NUM_WHITELIST_NODES][6] = { { 0x00, 0x1D, 0x4F, 0xCA, 0xEC, 0x8B  }, \
										                { 0x40, 0xD8, 0x55, 0x04, 0x21, 0x3A } };



u8 mac_filter_is_allowed(u8* addr){
	u32 i,j;
	u32 sum;

	//First, check if the incoming address is within the allowable range of addresses
	sum = 0;
	for(i=0; i<6; i++){
		sum += (filter_range_mask[i] | filter_range_compare[i]) == (filter_range_mask[i] | addr[i]);
	}
	if(sum == 6) return 1;

	//Next, check if the incoming address is one of the individual whitelisted addresses
	for(j=0; j<NUM_WHITELIST_NODES; j++){
		sum = 0;
		for(i=0; i<6; i++){
			sum += (whitelist_compare[j][i]) == (addr[i]);
		}
		if(sum == 6) return 1;
	}

	//If the code made it this far, we aren't allowing this address to join the network.
	return 0;

}

