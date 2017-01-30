#ifndef WLAN_MAC_HIGH_MAILBOX_UTIL_H_
#define WLAN_MAC_HIGH_MAILBOX_UTIL_H_

#include "xintc.h"

void wlan_mac_high_init_mailbox();
void wlan_mac_high_ipc_rx();

int setup_mailbox_interrupt(XIntc* intc);
void mailbox_int_handler(void * callback_ref);

#endif /* WLAN_MAC_HIGH_MAILBOX_UTIL_H_ */
