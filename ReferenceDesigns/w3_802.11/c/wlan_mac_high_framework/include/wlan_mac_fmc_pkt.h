////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_fmc_pkt.h
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

#ifndef WLAN_MAC_FMC_PKT_H_
#define WLAN_MAC_FMC_PKT_H_

#define FMC_MBOX_DEVICE_ID			XPAR_MBOX_0_DEVICE_ID

#define MBOX_ALIGN_OFFSET 2

#define FMC_IPC_DELIMITER	0x2452A9C0

#define FMC_IPC_MSG_ID_PKT_TO_W3   0x6BF8
#define FMC_IPC_MSG_ID_PKT_FROM_W3 0xEA6D

typedef struct {
	u32 delimiter;
	u16 msg_id;
	u16	size_bytes;
} wlan_fmc_ipc_msg;


/*
typedef struct {
	u16 msg_id;
	u8	num_payload_words;
	u8	arg0;
	u32* payload_ptr;
} wlan_ipc_msg;
*/


int fmc_ipc_rx();
int wlan_fmc_pkt_eth_send(u8* eth_hdr, u16 length);
int wlan_fmc_pkt_setup_mailbox_interrupt();
void FMCMailboxIntrHandler(void *CallbackRef);

#endif /* WLAN_MAC_FMC_PKT_H_ */
