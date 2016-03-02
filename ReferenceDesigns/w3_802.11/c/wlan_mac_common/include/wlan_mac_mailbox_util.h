/** @file wlan_mac_ipc_util.h
 *  @brief Mailbox Framework
 *
 *  This contains code common to both CPU_LOW and CPU_HIGH that allows them
 *  to pass messages to one another through the mailbox.
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

#include "xparameters.h"


/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_MAILBOX_UTIL_H_
#define WLAN_MAC_MAILBOX_UTIL_H_


//-----------------------------------------------
// Hardware defines
//
#define MAILBOX_DEVICE_ID                                  XPAR_MBOX_0_DEVICE_ID

// FIXME: v1.5 hardware has 512-word mailbox
#define MAILBOX_BUFFER_MAX_NUM_WORDS                       100


//-----------------------------------------------
// Interrupt support
//    Determine if the interrupt controller is present to allow mailbox driver
//    to support both interrupt and polled modes of operation.
//
#ifdef XPAR_INTC_0_DEVICE_ID

// Set define to indicate INTC is present
#define MAILBOX_INTC_PRESENT                               1

// Include Interrupt Controller header file
#include "xintc.h"
#endif


//-----------------------------------------------
// Mailbox Message ID delimiter
//     - Appended to each message ID to allow for error checking of
//       the start of a message
//
#define IPC_MBOX_MSG_ID_DELIM                              0xF000


//-----------------------------------------------
// IPC Message IDs
//
#define IPC_MBOX_RX_MPDU_READY                             0
#define IPC_MBOX_TX_MPDU_READY                             1
#define IPC_MBOX_TXRX_BEACON_CONFIGURE                     2
#define IPC_MBOX_TX_MPDU_DONE                              3
#define IPC_MBOX_HW_INFO                                   4
#define IPC_MBOX_CPU_STATUS                                5
#define IPC_MBOX_CONFIG_LOW                                6
#define IPC_MBOX_CONFIG_CHANNEL                            7
#define IPC_MBOX_CONFIG_PHY_RX                             8
#define IPC_MBOX_CONFIG_PHY_TX                             9
#define IPC_MBOX_TX_BEACON_DONE                            10
#define IPC_MBOX_CONFIG_RX_ANT_MODE                        12
#define IPC_MBOX_CONFIG_TX_CTRL_POW                        13
#define IPC_MBOX_CONFIG_RX_FILTER                          14
#define IPC_MBOX_MEM_READ_WRITE                            15
#define IPC_MBOX_LOW_PARAM                                 16
#define IPC_MBOX_LOW_RANDOM_SEED                           17


//-----------------------------------------------
// Macros to Add / Remove delimiter to message ID
//
#define IPC_MBOX_MSG_ID(id)                               (IPC_MBOX_MSG_ID_DELIM | ((id) & 0xFFF))
#define IPC_MBOX_MSG_ID_TO_MSG(id)                        ((id) & 0xFFF)


//-----------------------------------------------
// Mailbox status codes
//
#define IPC_MBOX_SUCCESS                                   0
#define IPC_MBOX_INVALID_MSG                              -1
#define IPC_MBOX_NO_MSG_AVAIL                             -2


//-----------------------------------------------
// IPC_MBOX_MEM_READ_WRITE arg0 defines
//
#define IPC_REG_READ_MODE                                  0
#define IPC_REG_WRITE_MODE                                 1



/*********************** Global Structure Definitions ************************/

//-----------------------------------------------
// IPC Message structure
//     - msg_id              - Any of the message IDs defined above
//     - num_payload_words   - Number of u32 words in the payload
//     - arg0                - Used to pass a single u8 argument as part of the message
//     - payload_ptr         - Pointer to payload (can be array of u32 or structure defined below)
//
typedef struct {
    u16       msg_id;
    u8        num_payload_words;
    u8        arg0;
    u32*      payload_ptr;
} wlan_ipc_msg_t;


//-----------------------------------------------
// IPC_MBOX_MEM_READ_WRITE payload structure
//     - Must be u32 aligned
//
typedef struct{
    u32       baseaddr;
    u32       num_words;
} ipc_reg_read_write_t;


//-----------------------------------------------
// IPC_MBOX_CONFIG_PHY_TX payload structure
//     - Must be u32 aligned
//
typedef struct{
    u8        reserved[4];
} ipc_config_phy_tx_t;


//-----------------------------------------------
// IPC_MBOX_CONFIG_PHY_TX payload structure
//     - Must be u32 aligned
//
typedef struct{
    u8        enable_dsss;
    u8        reserved[3];
} ipc_config_phy_rx_t;


//-----------------------------------------------
// Macro to initialize IPC config payload structures
//
#define init_ipc_config(x,y,z) {                                        \
                                    x = (z*) y;                         \
                                    memset((void*)x, 0xFF, sizeof(z));  \
                               }



/*************************** Function Prototypes *****************************/

int           init_mailbox();

#if MAILBOX_INTC_PRESENT
int           setup_mailbox_interrupt(XIntc * intc);
void          set_mailbox_rx_callback(function_ptr_t callback);
void          mailbox_int_handler(void * callback_ref);
#endif

inline int    read_mailbox_isempty();
int           read_mailbox_msg(wlan_ipc_msg_t * msg);
int           write_mailbox_msg(wlan_ipc_msg_t * msg);
int           send_msg(u16 msg_id, u8 arg, u8 num_words, u32 * payload);

#endif /* WLAN_MAC_MAILBOX_UTIL_H_ */
