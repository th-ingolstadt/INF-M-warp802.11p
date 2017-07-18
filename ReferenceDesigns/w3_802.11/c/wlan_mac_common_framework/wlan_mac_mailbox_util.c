/** @file wlan_mac_mailbox_util.c
 *  @brief Inter-processor Communication (Mailbox) Framework
 *
 *  This contains code common to both CPU_LOW and CPU_HIGH that allows them
 *  to pass messages to one another.
 *
 *  @copyright Copyright 2013-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/***************************** Include Files *********************************/

#include "stdlib.h"
#include "stdio.h"

#include "xstatus.h"
#include "xparameters.h"
#include "xmbox.h"

#include "wlan_platform_common.h"
#include "wlan_mac_common.h"
#include "wlan_mac_mailbox_util.h"
#include "wlan_cpu_id.h"

/*********************** Global Variable Definitions *************************/

/*************************** Variable Definitions ****************************/


static XMbox ipc_mailbox;

static platform_common_dev_info_t platform_common_dev_info;


/*************************** Functions Prototypes ****************************/


/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * Initialize Mailbox
 *
 * This function will always return XST_SUCCESS and should be used to initialize
 * the mailbox peripheral used for IPC messages.  This function supports both
 * using the mailbox in a polling mode or with interrupts.
 *
 * @return  XMbox*              - pointer to mailbox driver instance
 *****************************************************************************/
XMbox* init_mailbox() {

	platform_common_dev_info = wlan_platform_common_get_dev_info();

    XMbox_Config *mbox_config_ptr;

    // Initialize the IPC mailbox core
    mbox_config_ptr = XMbox_LookupConfig(platform_common_dev_info.mailbox_dev_id);
    XMbox_CfgInitialize(&ipc_mailbox, mbox_config_ptr, mbox_config_ptr->BaseAddress);

    return &ipc_mailbox;
}


#if WLAN_COMPILE_FOR_CPU_HIGH
#include "wlan_mac_high.h"
#endif

/*****************************************************************************/
/**
 * Write IPC message
 *
 * This function will write an IPC message to the mailbox for the other CPU.
 * This function is blocking and each message write is atomic in the sense that
 * it will not be interrupted.
 *
 * @param   msg              - Pointer to IPC message structure to write
 *
 * @return  int              - Status:
 *                                 IPC_MBOX_SUCCESS - Message sent successfully
 *                                 IPC_MBOX_INVALID_MSG - Message invalid
 *****************************************************************************/
int write_mailbox_msg(wlan_ipc_msg_t* msg) {
    // Check that msg points to a valid IPC message
    if (((msg->msg_id) & IPC_MBOX_MSG_ID_DELIM) != IPC_MBOX_MSG_ID_DELIM) {
        return IPC_MBOX_INVALID_MSG;
    }

    // Check that msg isn't too long
    if ((msg->num_payload_words) > MAILBOX_BUFFER_MAX_NUM_WORDS) {
        return IPC_MBOX_INVALID_MSG;
    }

#if WLAN_COMPILE_FOR_CPU_HIGH
    interrupt_state_t     prev_interrupt_state;
    prev_interrupt_state = wlan_mac_high_interrupt_stop();
#endif
    // Write msg header (first 32b word)
    XMbox_WriteBlocking(&ipc_mailbox, (u32*)msg, 4);

    // Write msg payload
    if ((msg->num_payload_words) > 0) {
        XMbox_WriteBlocking(&ipc_mailbox, (u32*)(msg->payload_ptr), (u32)(4 * (msg->num_payload_words)));
    }

#if WLAN_COMPILE_FOR_CPU_HIGH
    wlan_mac_high_interrupt_restore_state(prev_interrupt_state);
#endif
    return IPC_MBOX_SUCCESS;
}



/*****************************************************************************/
/**
 * Send IPC message
 *
 * This function is a wrapper method that will send an IPC message to the other
 * CPU. This function will create a wlan_ipc_msg_t message and send it using
 * write_mailbox_msg().  Therefore, this function is blocking.
 *
 * @param   msg_id           - IPC Message ID (should not contain IPC_MBOX_MSG_ID_DELIM)
 * @param   arg              - Optional u8 argument to the IPC message
 * @param   num_words        - Number of u32 words in the payload
 * @param   payload          - u32 pointer to the payload
 *
 * @return  int              - Status:
 *                                 IPC_MBOX_SUCCESS - Message sent successfully
 *                                 IPC_MBOX_INVALID_MSG - Message invalid
 *****************************************************************************/
int send_msg(u16 msg_id, u8 arg, u8 num_words, u32* payload) {
    wlan_ipc_msg_t ipc_msg;

    // Set message fields
    ipc_msg.msg_id            = IPC_MBOX_MSG_ID(msg_id);
    ipc_msg.num_payload_words = num_words;
    ipc_msg.arg0              = arg;
    ipc_msg.payload_ptr       = payload;

    // Send message
    return write_mailbox_msg(&ipc_msg);
}


/*****************************************************************************/
/**
 * Read IPC message
 *
 * This function will read an IPC message from the mailbox from the other CPU.
 * This function is blocking and will block until entire message is read.
 *
 * In the current 802.11 framework, mailbox messages are only read in polling
 * mode on a single threaded CPU or in an interrupt service routine.  In both
 * cases, this will effectively make the read operation atomic.  If a mailbox
 * read needed to occur in an interruptable context, it would required the
 * interrupt stop / restore code to be added as in write_mailbox_msg().
 *
 * @param   msg              - Pointer to IPC message structure to place read data
 *
 * @return  int              - Status:
 *                                 IPC_MBOX_SUCCESS      - Message sent successfully
 *                                 IPC_MBOX_NO_MSG_AVAIL - No message available
 *                                 IPC_MBOX_INVALID_MSG  - Message invalid
 *****************************************************************************/
int read_mailbox_msg(wlan_ipc_msg_t* msg) {
    u32 bytes_read;
    u32 i;
    u32 trash_bin;
    int status;

    // Check if there is a message to read
    if (XMbox_IsEmpty(&ipc_mailbox)) {
        return IPC_MBOX_INVALID_MSG;
    }

    // Attempt to read one 32b word from the mailbox into the user-supplied msg
    status = XMbox_Read(&ipc_mailbox, (u32*)msg, 4, &bytes_read);

    if ((status != XST_SUCCESS) || (bytes_read != 4)) {
        return IPC_MBOX_NO_MSG_AVAIL;
    }

    // Check if the received word is a valid msg
    if (((msg->msg_id) & IPC_MBOX_MSG_ID_DELIM) != IPC_MBOX_MSG_ID_DELIM) {
        // Flush the mailbox to hopefully get back to a known state
        XMbox_Flush(&ipc_mailbox);
        return IPC_MBOX_INVALID_MSG;
    }

    // Check that msg isn't too long
    if ((msg->num_payload_words) > MAILBOX_BUFFER_MAX_NUM_WORDS) {
        for (i = 0; i < (msg->num_payload_words); i++) {
            // Read out message into trash since there is not enough space available to hold it
            XMbox_ReadBlocking(&ipc_mailbox, &trash_bin, 4);
        }

        return IPC_MBOX_INVALID_MSG;
    }

    // Read message payload
    if ((msg->num_payload_words) > 0) {
        XMbox_ReadBlocking(&ipc_mailbox, (u32*)(msg->payload_ptr), 4 * (msg->num_payload_words));
    }

    return IPC_MBOX_SUCCESS;
}


