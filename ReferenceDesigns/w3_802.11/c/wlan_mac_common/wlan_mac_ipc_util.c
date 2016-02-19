/** @file wlan_mac_ipc_util.c
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

/***************************** Include Files *********************************/

#include "stdlib.h"
#include "stdio.h"

#include "xstatus.h"
#include "xmutex.h"
#include "xparameters.h"

#include "wlan_mac_common.h"
#include "wlan_mac_ipc_util.h"
#include "wlan_mac_misc_util.h"
#include "wlan_mac_802_11_defs.h"

/*********************** Global Variable Definitions *************************/

/*************************** Variable Definitions ****************************/

static XMutex                pkt_buf_mutex;


/*************************** Functions Prototypes ****************************/


/******************************** Functions **********************************/

int wlan_lib_init () {
	u32 i;

	//Initialize the pkt buffer mutex core
	XMutex_Config *mutex_ConfigPtr;
	mutex_ConfigPtr = XMutex_LookupConfig(PKT_BUF_MUTEX_DEVICE_ID);
	XMutex_CfgInitialize(&pkt_buf_mutex, mutex_ConfigPtr, mutex_ConfigPtr->BaseAddress);

	//Unlock all mutexes this CPU might own at boot
	// Most unlocks will fail harmlessly, but this helps cleanup state on soft reset
	for(i=0; i < NUM_TX_PKT_BUFS; i++) {
		unlock_pkt_buf_tx(i);
	}
	for(i=0; i < NUM_RX_PKT_BUFS; i++) {
		unlock_pkt_buf_rx(i);
	}

	return 0;
}



/************** Pkt Buffer Mutex Management ************/
int lock_pkt_buf_tx(u8 pkt_buf_ind) {
	int status;

	//Check inputs
	if(pkt_buf_ind >= NUM_TX_PKT_BUFS)
		return PKT_BUF_MUTEX_FAIL_INVALID_BUF;

	status = XMutex_Trylock(&pkt_buf_mutex, (pkt_buf_ind + PKT_BUF_MUTEX_TX_BASE));

	if(status == XST_SUCCESS)
		return PKT_BUF_MUTEX_SUCCESS;
	else
		return PKT_BUF_MUTEX_FAIL_ALREADY_LOCKED;
}

int lock_pkt_buf_rx(u8 pkt_buf_ind) {
	int status;

	//Check inputs
	if(pkt_buf_ind >= NUM_RX_PKT_BUFS)
		return PKT_BUF_MUTEX_FAIL_INVALID_BUF;


	status = XMutex_Trylock(&pkt_buf_mutex, (pkt_buf_ind + PKT_BUF_MUTEX_RX_BASE));

	if(status == XST_SUCCESS)
		return PKT_BUF_MUTEX_SUCCESS;
	else
		return PKT_BUF_MUTEX_FAIL_ALREADY_LOCKED;
}

int unlock_pkt_buf_tx(u8 pkt_buf_ind) {
	int status;

	//Check inputs
	if(pkt_buf_ind >= NUM_TX_PKT_BUFS)
		return PKT_BUF_MUTEX_FAIL_INVALID_BUF;

	status = XMutex_Unlock(&pkt_buf_mutex, (pkt_buf_ind + PKT_BUF_MUTEX_TX_BASE));

	if(status == XST_SUCCESS)
		return PKT_BUF_MUTEX_SUCCESS;
	else
		return PKT_BUF_MUTEX_FAIL_NOT_LOCK_OWNER;
}

int unlock_pkt_buf_rx(u8 pkt_buf_ind) {
	int status;

	//Check inputs
	if(pkt_buf_ind >= NUM_RX_PKT_BUFS)
		return PKT_BUF_MUTEX_FAIL_INVALID_BUF;

	status = XMutex_Unlock(&pkt_buf_mutex, (pkt_buf_ind + PKT_BUF_MUTEX_RX_BASE));

	if(status == XST_SUCCESS)
		return PKT_BUF_MUTEX_SUCCESS;
	else
		return PKT_BUF_MUTEX_FAIL_NOT_LOCK_OWNER;
}

int status_pkt_buf_tx(u8 pkt_buf_ind, u32* Locked, u32 *Owner){

	//Check inputs
	if(pkt_buf_ind >= NUM_TX_PKT_BUFS)
		return PKT_BUF_MUTEX_FAIL_INVALID_BUF;

	XMutex_GetStatus(&pkt_buf_mutex, (pkt_buf_ind + PKT_BUF_MUTEX_TX_BASE), Locked, Owner);

	return PKT_BUF_MUTEX_SUCCESS;
}

int status_pkt_buf_rx(u8 pkt_buf_ind, u32* Locked, u32 *Owner){

	//Check inputs
	if(pkt_buf_ind >= NUM_RX_PKT_BUFS)
		return PKT_BUF_MUTEX_FAIL_INVALID_BUF;

	XMutex_GetStatus(&pkt_buf_mutex, (pkt_buf_ind + PKT_BUF_MUTEX_RX_BASE), Locked, Owner);

	return PKT_BUF_MUTEX_SUCCESS;
}

