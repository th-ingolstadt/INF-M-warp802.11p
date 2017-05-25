/** @file wlan_mac_queue.h
 *  @brief Transmit Queue Framework
 *
 *  This contains code for accessing the transmit queue.
 *
 *  @copyright Copyright 2013-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_QUEUE_H_
#define WLAN_MAC_QUEUE_H_

/***************************** Include Files *********************************/

#include "wlan_mac_high_sw_config.h"
#include "xil_types.h"
#include "wlan_common_types.h"

//Forward declarations
struct station_info_t;


//-----------------------------------------------
// Queue defines
//
#define QUEUE_BUFFER_SIZE                                  0x1000    // 4KB

typedef struct{
	struct station_info_t*	station_info;
	dl_entry*			  	tx_queue_entry;
	tx_frame_info_t       	tx_frame_info;
	u8                    	phy_hdr_pad[PHY_TX_PKT_BUF_PHY_HDR_SIZE];
	u8                    	frame[MAX_PKT_SIZE_B];
} tx_queue_buffer_t;


/*************************** Function Prototypes *****************************/

void                queue_init();

void                enqueue_after_tail(u16 queue_sel, dl_entry* tqe);
dl_entry* 			dequeue_from_head(u16 queue_sel);
int          		dequeue_transmit_checkin(u16 queue_sel);
void 	    		queue_set_state_change_callback(function_ptr_t callback);

dl_entry* 			queue_checkout();
void                queue_checkin(dl_entry* tqe);

int                 queue_checkout_list(dl_list* new_list, u16 num_tqe);
int                 queue_checkin_list(dl_list * list);

u32          		queue_num_free();
u32          		queue_num_queued(u16 queue_sel);
u32                 queue_total_size();

void                purge_queue(u16 queue_sel);

#endif /* WLAN_MAC_QUEUE_H_ */
