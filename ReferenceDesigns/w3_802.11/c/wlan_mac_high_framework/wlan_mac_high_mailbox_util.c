#include "xmbox.h"

#include "wlan_mac_common.h"
#include "wlan_mac_high.h"
#include "wlan_mac_high_mailbox_util.h"
#include "wlan_mac_mailbox_util.h"
#include "wlan_platform_common.h"
#include "wlan_platform_high.h"


#define MAILBOX_RIT                                        0         /* mailbox receive interrupt threshold */
#define MAILBOX_SIT                                        0         /* mailbox send interrupt threshold */

static XMbox*						xmbox_ptr;
static XIntc*                		intc_ptr;
static platform_common_dev_info_t	platform_common_dev_info;
static platform_high_dev_info_t		platform_high_dev_info;

// IPC variables
static wlan_ipc_msg_t               ipc_msg_from_low;                                           ///< IPC message from lower-level
static u32                          ipc_msg_from_low_payload[MAILBOX_BUFFER_MAX_NUM_WORDS];     ///< Buffer space for IPC message from lower-level

void wlan_mac_high_init_mailbox(){
	xmbox_ptr = init_mailbox();

	//Create IPC message to receive into
	ipc_msg_from_low.payload_ptr = &(ipc_msg_from_low_payload[0]);

	platform_common_dev_info = wlan_platform_common_get_dev_info();
	platform_high_dev_info   = wlan_platform_high_get_dev_info();
}

/**
 * @brief WLAN MAC IPC receive
 *
 * IPC receive function that will poll the mailbox for as many messages as are
 * available and then call the CPU high IPC processing function on each message
 *
 * @param  None
 * @return None
 */
void wlan_mac_high_ipc_rx(){
	while (read_mailbox_msg(&ipc_msg_from_low) == IPC_MBOX_SUCCESS) {
		wlan_mac_high_process_ipc_msg(&ipc_msg_from_low, ipc_msg_from_low_payload);
	}
}

/*****************************************************************************/
/**
 * Setup mailbox interrupt
 *
 * @param   intc             - Pointer to the interrupt controller instance
 *
 * @return  int              - Status:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - Command failed
 *****************************************************************************/
int setup_mailbox_interrupt(XIntc* intc){
    int status;

    // Set global interrupt controller pointer
    intc_ptr = intc;

    // Set Send / Receive threshold for interrupts
    XMbox_SetSendThreshold(xmbox_ptr, MAILBOX_SIT);
    XMbox_SetReceiveThreshold(xmbox_ptr, MAILBOX_RIT);

    // Connect interrupt handler
    status = XIntc_Connect(intc_ptr, platform_high_dev_info.mailbox_int_id, (XInterruptHandler)mailbox_int_handler, xmbox_ptr);

    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // Enable interrupt
    XMbox_SetInterruptEnable(xmbox_ptr, XMB_IX_RTA);
    XIntc_Enable(intc_ptr, platform_high_dev_info.mailbox_int_id);

    return XST_SUCCESS;
}


/*****************************************************************************/
/**
 * Mailbox interrupt handler
 *
 * @param   callback_ref     - Callback reference (set by interrupt framework)
 *
 *****************************************************************************/
void mailbox_int_handler(void * callback_ref){
    u32       mask;
    XMbox *   mbox_ptr = (XMbox *)callback_ref;

#ifdef _ISR_PERF_MON_EN_
    wlan_mac_set_dbg_hdr_out(ISR_PERF_MON_GPIO_MASK);
#endif

    // Stop the interrupt controller
    XIntc_Stop(intc_ptr);

    // Get the interrupt status
    mask = XMbox_GetInterruptStatus(mbox_ptr);

    // Clear the interrupt
    //     - Since only the XMB_IX_RTA interrupt was enabled in setup_mailbox_interrupt()
    //       that is the only interrupt that will ever need to be cleared
    XMbox_ClearInterrupt(mbox_ptr, XMB_IX_RTA);

    // If this is a receive interrupt, then call the callback function
    if (mask & XMB_IX_RTA) {
    	wlan_mac_high_ipc_rx();
    }

    // Restart the interrupt conroller
    XIntc_Start(intc_ptr, XIN_REAL_MODE);

#ifdef _ISR_PERF_MON_EN_
    wlan_mac_clear_dbg_hdr_out(ISR_PERF_MON_GPIO_MASK);
#endif
}
