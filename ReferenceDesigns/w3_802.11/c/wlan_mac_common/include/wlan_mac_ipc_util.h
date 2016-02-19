/** @file wlan_mac_ipc_util.h
 *  @brief Inter-processor Communication Framework
 *
 *  This contains code common to both CPU_LOW and CPU_HIGH that allows them
 *  to pass messages to one another.
 *
 *  @copyright Copyright 2013-2015, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

#ifndef WLAN_MAC_IPC_UTIL_H_

#include "wlan_mac_common.h"
#include "wlan_mac_misc_util.h"

#define WLAN_MAC_IPC_UTIL_H_

#define PKT_BUF_MUTEX_DEVICE_ID		XPAR_MUTEX_0_DEVICE_ID

#define PKT_BUF_MUTEX_SUCCESS 				0
#define PKT_BUF_MUTEX_FAIL_INVALID_BUF		-1
#define PKT_BUF_MUTEX_FAIL_ALREADY_LOCKED	-2
#define PKT_BUF_MUTEX_FAIL_NOT_LOCK_OWNER	-3

#define PKT_BUF_MUTEX_TX_BASE	0
#define PKT_BUF_MUTEX_RX_BASE	16


int wlan_lib_init ();

int lock_pkt_buf_tx(u8 pkt_buf_ind);
int lock_pkt_buf_rx(u8 pkt_buf_ind);
int unlock_pkt_buf_tx(u8 pkt_buf_ind);
int unlock_pkt_buf_rx(u8 pkt_buf_ind);
int status_pkt_buf_tx(u8 pkt_buf_ind, u32* Locked, u32 *Owner);
int status_pkt_buf_rx(u8 pkt_buf_ind, u32* Locked, u32 *Owner);


#endif /* WLAN_MAC_IPC_UTIL_H_ */
