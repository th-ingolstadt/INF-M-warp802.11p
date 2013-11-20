////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_fmc_pkt.c
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
//          Erik Welsh (welsh [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

/***************************** Include Files *********************************/

#include "stdlib.h"
#include "xintc.h"
#include "xil_exception.h"
#include "wlan_mac_util.h"
#include "xmbox.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_fmc_pkt.h"


#define FMC_MBOX_RIT	0	/* mailbox receive interrupt threshold */
#define FMC_MBOX_SIT	0	/* mailbox send interrupt threshold */
#define FMC_MBOX_INTR_ID		XPAR_MB_HIGH_INTC_MAILBOX_FMC_INTERRUPT_0_INTR

extern XIntc       InterruptController;
extern function_ptr_t	   fmc_ipc_rx_callback;
extern function_ptr_t      eth_rx_callback;

XMbox fmc_ipc_mailbox;

// IPC variables
#define FMC_IPC_BUFFER_SIZE 1600
wlan_fmc_ipc_msg   ipc_msg_from_fmc;

//TODO: shouldn't need local memory for the payload... put it directly in DRAM queue
//u8                 ipc_msg_from_fmc_payload[FMC_IPC_BUFFER_SIZE];


#define FMC_TIMEOUT_USEC 1000000
int fmc_ipc_rx(){
	int status;
	u32 bytes_read;
	u32 pkt_bytes_read;
	u16 num_words;
	u64 timestamp = get_usec_timestamp();
	packet_bd_list checkout;
	packet_bd*	tx_queue;
	void* buf_addr;
	u8 packet_is_queued = 0;
	u32 i;

	u8 eth_dest[6];
	u8 eth_src[6];
	u8* mpdu_start_ptr;
	u8* eth_start_ptr;
	u32 eth_rx_len, eth_rx_buf;
	packet_bd_list tx_queue_list;
	u32 mpdu_tx_len;

	//while((XMbox_IsEmpty(&fmc_ipc_mailbox)==0)  &&   (get_usec_timestamp() < (timestamp+FMC_TIMEOUT_USEC))){
	while((XMbox_IsEmpty(&fmc_ipc_mailbox)==0)){
		//xil_printf("XMbox_IsEmpty(&fmc_ipc_mailbox) == %d\n", XMbox_IsEmpty(&fmc_ipc_mailbox));

		if(get_usec_timestamp() > (timestamp+FMC_TIMEOUT_USEC)){
			xil_printf("Timeout!\n");
			//XMbox_Flush(&fmc_ipc_mailbox);
			return 0; //Stuff is still in the mailbox. Returning 0 tells the ISR to not clear the interrupt.
		}

		//Attempt to read one 32b word from the mailbox
		status = wlan_XMbox_Read(&fmc_ipc_mailbox, (u32*)&ipc_msg_from_fmc, 4, &bytes_read);
		if((status != XST_SUCCESS) || (bytes_read != 4) ) {
			//Failed to read a full word. Flush the mailbox and quit out of the ISR.
			XMbox_Flush(&fmc_ipc_mailbox);
			return 1;
		}

		//Check if the received word is a valid msg
		//if( ((msg->msg_id) & IPC_MBOX_MSG_ID_DELIM) != IPC_MBOX_MSG_ID_DELIM) {
		if(ipc_msg_from_fmc.delimiter == FMC_IPC_DELIMITER) {
			//This is the start of a valid FMC IPC message
			//Read the next 4 bytes into t he ipc_msg_from_fmc struct

			status = wlan_XMbox_Read(&fmc_ipc_mailbox, (u32*)((u8*)&ipc_msg_from_fmc + 4), 4, &bytes_read);
			if((status != XST_SUCCESS) || (bytes_read != 4) ) {
				//Failed to read a full word. Flush the mailbox and quit out of the ISR.
				XMbox_Flush(&fmc_ipc_mailbox);
				return 1;
			}

			switch(ipc_msg_from_fmc.msg_id){
			case FMC_IPC_MSG_ID_PKT_TO_W3:
				//xil_printf("\nFMC_IPC_MSG_ID_PKT_TO_W3\n");
				//xil_printf("    Length: %d\n", ipc_msg_from_fmc.size_bytes);

				if(ipc_msg_from_fmc.size_bytes < FMC_IPC_BUFFER_SIZE){
					queue_checkout(&checkout, 1);

					if(checkout.length == 1){
						tx_queue = checkout.first;
						buf_addr = (void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame + sizeof(mac_header_80211) + sizeof(llc_header) - sizeof(ethernet_header) - MBOX_ALIGN_OFFSET;

                        if( ( ipc_msg_from_fmc.size_bytes + MBOX_ALIGN_OFFSET ) & 0x3){
							num_words = (ipc_msg_from_fmc.size_bytes+4+MBOX_ALIGN_OFFSET)>>2; //Division by 4
						} else {
							num_words = (ipc_msg_from_fmc.size_bytes+MBOX_ALIGN_OFFSET)>>2; //Division by 4
						}

						pkt_bytes_read = 0;
						//xil_printf("\n");
						while(pkt_bytes_read < (num_words<<2)){
							if(get_usec_timestamp() > (timestamp+FMC_TIMEOUT_USEC)){
								xil_printf("Timeout in packet read!\n");
								queue_checkin(&checkout);
								return 0; //Stuff is still in the mailbox. Returning 0 tells the ISR to not clear the interrupt.
							}

							status = wlan_XMbox_Read(&fmc_ipc_mailbox, (u32*)((u8*)buf_addr + pkt_bytes_read), (num_words<<2)-pkt_bytes_read, &bytes_read);

							pkt_bytes_read += bytes_read;

							//xil_printf("+%d (%d / %d) \n", bytes_read, pkt_bytes_read, num_words<<2 );
						}

#if 0
						xil_printf("pkt_bytes_read = %d\n", pkt_bytes_read);

						for(i=0; i < 0x2c;i++){
							xil_printf("%x ", ((u8*)buf_addr)[i]);
						}

						xil_printf("\n\n");
#endif

						eth_rx_len = ipc_msg_from_fmc.size_bytes;
						eth_rx_buf = (u32)((u8 *)buf_addr + MBOX_ALIGN_OFFSET);

						//After encapsulation, byte[0] of the MPDU will be at byte[0] of the queue entry frame buffer
						mpdu_start_ptr = (void*)((tx_packet_buffer*)(tx_queue->buf_ptr))->frame;
						eth_start_ptr = (u8*)eth_rx_buf;

						packet_bd_list_init(&tx_queue_list);
						packet_bd_insertEnd(&tx_queue_list, tx_queue);

						mpdu_tx_len = wlan_eth_encap(mpdu_start_ptr, eth_dest, eth_src, eth_start_ptr, eth_rx_len);

#if 0
						xil_printf("Encapsulated %d bytes\n", mpdu_tx_len);
#endif

						if(mpdu_tx_len>0){
#if 0
							xil_printf("     eth_dest: %02x-%02x-%02x-%02x-%02x-%02x\n", eth_dest[0],eth_dest[1],eth_dest[2],eth_dest[3],eth_dest[4],eth_dest[5]);
							xil_printf("     eth_src:  %02x-%02x-%02x-%02x-%02x-%02x\n", eth_src[0],eth_src[1],eth_src[2],eth_src[3],eth_src[4],eth_src[5]);
#endif
							packet_is_queued = eth_rx_callback(&tx_queue_list, eth_dest, eth_src, mpdu_tx_len);
						}

						if(packet_is_queued == 0){
						//	xil_printf("   ...checking in\n");
							queue_checkin(&checkout);
						}


					}


				}


			break;
			default:
				xil_printf("Unknown FMC IPC message \n" );
				xil_printf("    Delimiter: %x \n", ipc_msg_from_fmc.delimiter);
				xil_printf("    MSG ID   : %x \n", ipc_msg_from_fmc.msg_id);
				xil_printf("    BYTES    : %x \n", ipc_msg_from_fmc.size_bytes);


			break;

			}



		} else {
			xil_printf("Incorrect Delimiter: 0x%x\n", ipc_msg_from_fmc.delimiter);
		}



	} //while( non-empty mailbox & total time spent less than timeout)
	return 1;
}


int wlan_fmc_pkt_eth_send(u8* eth_hdr, u16 length){
	int return_value = 0;
	wlan_fmc_ipc_msg   ipc_msg_to_fmc;
	u16 num_words;

#if 0
	xil_printf("Sending FMC packet: 0x%x bytes \n", length);
#endif

	ipc_msg_to_fmc.delimiter = FMC_IPC_DELIMITER;
	ipc_msg_to_fmc.msg_id = FMC_IPC_MSG_ID_PKT_FROM_W3;
	ipc_msg_to_fmc.size_bytes = length;

	if( (length + MBOX_ALIGN_OFFSET) & 0x3){
		num_words = (length+4+MBOX_ALIGN_OFFSET)>>2; //Division by 4
	} else {
		num_words = (length+MBOX_ALIGN_OFFSET)>>2; //Division by 4
	}

#if 0
	xil_printf("Length = %d     num_words = %d \n", length, num_words);
#endif

	wlan_XMbox_WriteBlocking(&fmc_ipc_mailbox, (u32*)(&ipc_msg_to_fmc), 8);

	//This is a little fast and loose, but we know its safe to reach before the
	//eth_hdr argument to fix the memory alignment issue with the mailbox because
	//this whole function is only ever called in the context that there is other
	//802.11 wireless stuff before eth_hdr anyway.
	wlan_XMbox_WriteBlocking(&fmc_ipc_mailbox, (u32*)((u8*)eth_hdr - MBOX_ALIGN_OFFSET), (4*num_words) );

	return return_value;
}

int wlan_XMbox_Read(XMbox *InstancePtr, u32 *BufferPtr, u32 RequestedBytes, u32 *BytesRecvdPtr){
	u32 NumBytes;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(!((u32) BufferPtr & 0x3));
	Xil_AssertNonvoid(RequestedBytes != 0);
	Xil_AssertNonvoid((RequestedBytes %4) == 0);
	Xil_AssertNonvoid(BytesRecvdPtr != NULL);

	NumBytes = 0;

	if (InstancePtr->Config.UseFSL == 0) {
		/* For memory mapped IO */
		if (XMbox_IsEmptyHw(InstancePtr->Config.BaseAddress))
			return XST_NO_DATA;

		/*
		 * Read the Mailbox until empty or the length requested is
		 * satisfied
		 */
		do {
			//*BufferPtr++ = XMbox_ReadMBox(InstancePtr->Config.BaseAddress);
			*BufferPtr = XMbox_ReadMBox(InstancePtr->Config.BaseAddress);
			if(*(u32*)BufferPtr == FMC_IPC_DELIMITER){
				xil_printf("Read found a delimiter at NumBytes = %d\n", NumBytes);
			}
			BufferPtr++;
			NumBytes += 4;
		} while ((NumBytes != RequestedBytes) &&
			 !(XMbox_IsEmptyHw(InstancePtr->Config.BaseAddress)));

		*BytesRecvdPtr = NumBytes;
	} else {

		/* FSL based Access */
		if (XMbox_FSLIsEmpty(InstancePtr->Config.RecvID))
			return XST_NO_DATA;

		/*
		 * Read the Mailbox until empty or the length requested is
		 * satisfied
		 */
		do {
			*BufferPtr++ =
				XMbox_FSLReadMBox(InstancePtr->Config.RecvID);
			NumBytes += 4;
		} while ((NumBytes != RequestedBytes) &&
			 !(XMbox_FSLIsEmpty(InstancePtr->Config.RecvID)));

		*BytesRecvdPtr = NumBytes;
	}

	return XST_SUCCESS;
}

void wlan_XMbox_WriteBlocking(XMbox *InstancePtr, u32 *BufferPtr, u32 RequestedBytes){
	u32 NumBytes;

	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(!((u32) BufferPtr & 0x3));
	Xil_AssertVoid(RequestedBytes != 0);
	Xil_AssertVoid((RequestedBytes %4) == 0);

	NumBytes = 0;

	if (InstancePtr->Config.UseFSL == 0) {
		/* For memory mapped IO */
		/* Block while the mailbox FIFO becomes free to transfer
		 * at-least one word
		 */
		do {
			while (XMbox_IsFullHw(InstancePtr->Config.BaseAddress)){
				xil_printf("mbox write paused at byte write %d: FIFO is full\n",NumBytes);
			}

			XMbox_WriteMBox(InstancePtr->Config.BaseAddress,
					 *BufferPtr++);
			NumBytes += 4;
		} while (NumBytes != RequestedBytes);
	} else {

		/* FSL based Access */
		/* Block while the mailbox FIFO becomes free to transfer
		 * at-least one word
		 */
		do {
			while (XMbox_FSLIsFull(InstancePtr->Config.SendID));

			XMbox_FSLWriteMBox(InstancePtr->Config.SendID,
					    *BufferPtr++);
			NumBytes += 4;
		} while (NumBytes != RequestedBytes);
	}
}


int wlan_fmc_pkt_init () {
	//Initialize the inter-processor mailbox core
	XMbox_Config *mbox_ConfigPtr;
	mbox_ConfigPtr = XMbox_LookupConfig(FMC_MBOX_DEVICE_ID);
	XMbox_CfgInitialize(&fmc_ipc_mailbox, mbox_ConfigPtr, mbox_ConfigPtr->BaseAddress);

	return 0;
}

int wlan_fmc_pkt_setup_mailbox_interrupt(){
	int Status;

	XMbox_SetSendThreshold(&fmc_ipc_mailbox, FMC_MBOX_SIT);
	XMbox_SetReceiveThreshold(&fmc_ipc_mailbox, FMC_MBOX_RIT);

	Status = XIntc_Connect(&InterruptController, FMC_MBOX_INTR_ID, (XInterruptHandler)FMCMailboxIntrHandler, (void *)&fmc_ipc_mailbox);

	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	//XMbox_SetInterruptEnable(&ipc_mailbox, XMB_IX_STA | XMB_IX_RTA | XMB_IX_ERR);
	XMbox_SetInterruptEnable(&fmc_ipc_mailbox, XMB_IX_RTA);

	XIntc_Enable(&InterruptController, FMC_MBOX_INTR_ID);

	return 0;
}

void FMCMailboxIntrHandler(void *CallbackRef){

	u8 is_empty;

	XMbox_SetReceiveThreshold(&fmc_ipc_mailbox, 0xFFFFFFFF);

	//xil_printf("FMCMailboxIntrHandler\n");

	u32 Mask;
	XMbox *MboxInstPtr = (XMbox *)CallbackRef;

	XIntc_Stop(&InterruptController);

	Mask = XMbox_GetInterruptStatus(MboxInstPtr);

	//XMbox_ClearInterrupt(MboxInstPtr, XMB_IX_RTA);

	if (Mask & XMB_IX_RTA) {
		is_empty = fmc_ipc_rx();
	}

	if(is_empty == 1) XMbox_ClearInterrupt(MboxInstPtr, XMB_IX_RTA);

	XMbox_SetReceiveThreshold(&fmc_ipc_mailbox, FMC_MBOX_RIT);

	XIntc_Start(&InterruptController, XIN_REAL_MODE);
}
