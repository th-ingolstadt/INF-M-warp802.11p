////////////////////////////////////////////////////////////////////////////////
// File   : wlan_lib.c
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

#include "stdlib.h"
#include "stdio.h"

#include "xstatus.h"
#include "xmutex.h"
#include "xmbox.h"

#include "wlan_lib.h"
#include "xparameters.h"


XMbox ipc_mailbox;
XMutex pkt_buf_mutex;

int wlan_lib_init () {

	//Initialize the pkt buffer mutex core
	XMutex_Config *mutex_ConfigPtr;
	mutex_ConfigPtr = XMutex_LookupConfig(PKT_BUF_MUTEX_DEVICE_ID);
	XMutex_CfgInitialize(&pkt_buf_mutex, mutex_ConfigPtr, mutex_ConfigPtr->BaseAddress);

	//Initialize the inter-processor mailbox core
	XMbox_Config *mbox_ConfigPtr;
	mbox_ConfigPtr = XMbox_LookupConfig(MAILBOX_DEVICE_ID);
	XMbox_CfgInitialize(&ipc_mailbox, mbox_ConfigPtr, mbox_ConfigPtr->BaseAddress);

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

/************** Inter-processor Messaging ************/


int ipc_mailbox_write_msg(wlan_ipc_msg* msg) {

	//Check that msg points to a valid IPC message
	if( ((msg->msg_id) & IPC_MBOX_MSG_ID_DELIM) != IPC_MBOX_MSG_ID_DELIM) {
		return IPC_MBOX_INVALID_MSG;
	}

	//Check that msg isn't too long
	if( (msg->num_payload_words) > IPC_MBOX_MAX_MSG_WORDS) {
		return IPC_MBOX_INVALID_MSG;
	}

	//Write msg header (first 32b word)
	XMbox_WriteBlocking(&ipc_mailbox, (u32*)msg, 4);

	if((msg->num_payload_words) > 0) {
		//Write msg payload
		XMbox_WriteBlocking(&ipc_mailbox, (u32*)(msg->payload_ptr), (u32)(4 * (msg->num_payload_words)));
	}

	return IPC_MBOX_SUCCESS;
}

int ipc_mailbox_read_msg(wlan_ipc_msg* msg) {
	u32 bytes_read;
	int status;

	//Mailbox read functions:
	//int XMbox_Read(XMbox *InstancePtr, u32 *BufferPtr, u32 RequestedBytes, u32 *BytesRecvdPtr);
	//void XMbox_ReadBlocking(XMbox *InstancePtr, u32 *BufferPtr, u32 RequestedBytes)

	//Attempt to read one 32b word from the mailbox into the user-supplied msg struct
	status = XMbox_Read(&ipc_mailbox, (u32*)msg, 4, &bytes_read);
	if((status != XST_SUCCESS) || (bytes_read != 4) ) {
		return IPC_MBOX_NO_MSG_AVAIL;
	}

	//Check if the received word is a valid msg
	if( ((msg->msg_id) & IPC_MBOX_MSG_ID_DELIM) != IPC_MBOX_MSG_ID_DELIM) {
		XMbox_Flush(&ipc_mailbox);
		return IPC_MBOX_INVALID_MSG;
	}

	//Check that msg isn't too long
	if( (msg->num_payload_words) > IPC_MBOX_MAX_MSG_WORDS) {
		XMbox_Flush(&ipc_mailbox);
		return IPC_MBOX_INVALID_MSG;
	}

	//Message header must have been valid; wait for all remaining words
	if((msg->num_payload_words) > 0) {
		XMbox_ReadBlocking(&ipc_mailbox, (u32*)(msg->payload_ptr), 4 * (msg->num_payload_words));
	}

	return IPC_MBOX_SUCCESS;
}
