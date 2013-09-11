////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_util.c
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

#include "xgpio.h"
#include "stdlib.h"
#include "xil_exception.h"
#include "xintc.h"
#include "xuartlite.h"
#include "xaxicdma.h"

#include "wlan_mac_ipc_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_util.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_eth_util.h"
#include "w3_userio.h"
#include "xparameters.h"
#include "xtmrctr.h"

static XGpio GPIO_timestamp;
static XGpio Gpio;
static XIntc InterruptController;
XUartLite UartLite;
static XTmrCtr TimerCounterInst;
XAxiCdma cdma_inst;

static u32 cpu_low_status;
static u32 cpu_high_status;

#define TX_BUFFER_NUM 2
u8 tx_pkt_buf;

u8 ReceiveBuffer[UART_BUFFER_SIZE];

static u8 eeprom_mac_addr[6];

wlan_ipc_msg ipc_msg_from_low;
u32 ipc_msg_from_low_payload[10];

function_ptr_t eth_rx_callback, mpdu_tx_done_callback, mpdu_rx_callback, pb_u_callback, pb_m_callback, pb_d_callback, uart_callback, ipc_rx_callback, check_queue_callback;

static u8 scheduler_in_use[NUM_SCHEDULERS][SCHEDULER_NUM_EVENTS];
static function_ptr_t scheduler_callbacks[NUM_SCHEDULERS][SCHEDULER_NUM_EVENTS];
static u64 scheduler_timestamps[NUM_SCHEDULERS][SCHEDULER_NUM_EVENTS];
static u8 timer_running[NUM_SCHEDULERS];

void wlan_mac_util_init(){
	int Status;
	u32 gpio_read,i;
	u64 timestamp;
	tx_frame_info* tx_mpdu;

	eth_rx_callback = (function_ptr_t)nullCallback;
	mpdu_rx_callback = (function_ptr_t)nullCallback;
	mpdu_tx_done_callback = (function_ptr_t)nullCallback;
	pb_u_callback = (function_ptr_t)nullCallback;
	pb_m_callback = (function_ptr_t)nullCallback;
	pb_d_callback = (function_ptr_t)nullCallback;
	uart_callback = (function_ptr_t)nullCallback;
	ipc_rx_callback = (function_ptr_t)nullCallback;
	check_queue_callback = (function_ptr_t)nullCallback;

	//create IPC message to receive into
	ipc_msg_from_low.payload_ptr = &(ipc_msg_from_low_payload[0]);

	for(i=0;i < NUM_TX_PKT_BUFS; i++){
		tx_mpdu = (tx_frame_info*)TX_PKT_BUF_TO_ADDR(i);
		tx_mpdu->state = TX_MPDU_STATE_EMPTY;
	}

	tx_pkt_buf = 0;
	xil_printf("locking tx_pkt_buf = %d\n", tx_pkt_buf);
	if(lock_pkt_buf_tx(tx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
		warp_printf(PL_ERROR,"Error: unable to lock pkt_buf %d\n",tx_pkt_buf);
	}

	tx_mpdu = (tx_frame_info*)TX_PKT_BUF_TO_ADDR(tx_pkt_buf);
	tx_mpdu->state = TX_MPDU_STATE_TX_PENDING;

	//Initialize the central DMA (CDMA) driver
	XAxiCdma_Config *cdma_cfg_ptr;
	cdma_cfg_ptr = XAxiCdma_LookupConfig(XPAR_AXI_CDMA_0_DEVICE_ID);
	Status = XAxiCdma_CfgInitialize(&cdma_inst, cdma_cfg_ptr, cdma_cfg_ptr->BaseAddress);
	if (Status != XST_SUCCESS) {
		warp_printf(PL_ERROR,"Error initializing CDMA: %d\n", Status);
	}
	XAxiCdma_IntrDisable(&cdma_inst, XAXICDMA_XR_IRQ_ALL_MASK);


	Status = XGpio_Initialize(&Gpio, GPIO_DEVICE_ID);
	gpio_timestamp_initialize();

	if (Status != XST_SUCCESS) {
		warp_printf(PL_ERROR, "Error initializing GPIO\n");
		return;
	}

	Status = XUartLite_Initialize(&UartLite, UARTLITE_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		warp_printf(PL_ERROR, "Error initializing XUartLite\n");
		return;
	}


	gpio_read = XGpio_DiscreteRead(&Gpio, GPIO_INPUT_CHANNEL);
	if(gpio_read&GPIO_MASK_DRAM_INIT_DONE){
		xil_printf("DRAM SODIMM Detected\n");
		queue_dram_present(1);
	} else {
		queue_dram_present(0);

		timestamp = get_usec_timestamp();

		while((get_usec_timestamp() - timestamp) < 100000){
			if((XGpio_DiscreteRead(&Gpio, GPIO_INPUT_CHANNEL)&GPIO_MASK_DRAM_INIT_DONE)){
				xil_printf("DRAM SODIMM Detected\n");
				queue_dram_present(1);
				break;
			}
		}
	}

	queue_init();
	wlan_eth_init();

	//Set direction of GPIO channels
	XGpio_SetDataDirection(&Gpio, GPIO_INPUT_CHANNEL, 0xFFFFFFFF);
	XGpio_SetDataDirection(&Gpio, GPIO_OUTPUT_CHANNEL, 0);


	Status = XTmrCtr_Initialize(&TimerCounterInst, TMRCTR_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("XTmrCtr failed to initialize\n");
		return;
	}

	//Set the handler for Timer
	XTmrCtr_SetHandler(&TimerCounterInst, timer_handler, &TimerCounterInst);

	//Enable interrupt of timer and auto-reload so it continues repeatedly
	XTmrCtr_SetOptions(&TimerCounterInst, TIMER_CNTR_FAST, XTC_DOWN_COUNT_OPTION | XTC_INT_MODE_OPTION);
	XTmrCtr_SetOptions(&TimerCounterInst, TIMER_CNTR_SLOW, XTC_DOWN_COUNT_OPTION | XTC_INT_MODE_OPTION);

	timer_running[TIMER_CNTR_FAST] = 0;
	timer_running[TIMER_CNTR_SLOW] = 0;

}

void XTmrCtr_CustomInterruptHandler(void *InstancePtr){
	//FIXME: Temporarily moved ISR to mac_util

	//xil_printf("Custom Timer ISR\n");

	XTmrCtr *TmrCtrPtr = NULL;
	u8 TmrCtrNumber;
	u32 ControlStatusReg;

	/*
	 * Verify that each of the inputs are valid.
	 */
	Xil_AssertVoid(InstancePtr != NULL);

	/*
	 * Convert the non-typed pointer to an timer/counter instance pointer
	 * such that there is access to the timer/counter
	 */
	TmrCtrPtr = (XTmrCtr *) InstancePtr;

	/*
	 * Loop thru each timer counter in the device and call the callback
	 * function for each timer which has caused an interrupt
	 */
	for (TmrCtrNumber = 0;
		TmrCtrNumber < XTC_DEVICE_TIMER_COUNT; TmrCtrNumber++) {

		ControlStatusReg = XTmrCtr_ReadReg(TmrCtrPtr->BaseAddress,
						   TmrCtrNumber,
						   XTC_TCSR_OFFSET);
		/*
		 * Check if interrupt is enabled
		 */
		if (ControlStatusReg & XTC_CSR_ENABLE_INT_MASK) {

			/*
			 * Check if timer expired and interrupt occured
			 */
			if (ControlStatusReg & XTC_CSR_INT_OCCURED_MASK) {
				/*
				 * Increment statistics for the number of
				 * interrupts and call the callback to handle
				 * any application specific processing
				 */
				TmrCtrPtr->Stats.Interrupts++;
				TmrCtrPtr->Handler(TmrCtrPtr->CallBackRef,
						   TmrCtrNumber);
				/*
				 * Read the new Control/Status Register content.
				 */
				ControlStatusReg =
					XTmrCtr_ReadReg(TmrCtrPtr->BaseAddress,
								TmrCtrNumber,
								XTC_TCSR_OFFSET);
				/*
				 * If in compare mode and a single shot rather
				 * than auto reload mode then disable the timer
				 * and reset it such so that the interrupt can
				 * be acknowledged, this should be only temporary
				 * till the hardware is fixed
				 */
#if 0
				if (((ControlStatusReg &
					XTC_CSR_AUTO_RELOAD_MASK) == 0) &&
					((ControlStatusReg &
					  XTC_CSR_CAPTURE_MODE_MASK)== 0)) {
						/*
						 * Disable the timer counter and
						 * reset it such that the timer
						 * counter is loaded with the
						 * reset value allowing the
						 * interrupt to be acknowledged
						 */
						ControlStatusReg &=
							~XTC_CSR_ENABLE_TMR_MASK;

						XTmrCtr_WriteReg(
							TmrCtrPtr->BaseAddress,
							TmrCtrNumber,
							XTC_TCSR_OFFSET,
							ControlStatusReg |
							XTC_CSR_LOAD_MASK);

						/*
						 * Clear the reset condition,
						 * the reset bit must be
						 * manually cleared by a 2nd write
						 * to the register
						 */
						XTmrCtr_WriteReg(
							TmrCtrPtr->BaseAddress,
							TmrCtrNumber,
							XTC_TCSR_OFFSET,
							ControlStatusReg);
				}
#endif
				/*
				 * Acknowledge the interrupt by clearing the
				 * interrupt bit in the timer control status
				 * register, this is done after calling the
				 * handler so the application could call
				 * IsExpired, the interrupt is cleared by
				 * writing a 1 to the interrupt bit of the
				 * register without changing any of the other
				 * bits
				 */
				XTmrCtr_WriteReg(TmrCtrPtr->BaseAddress,
						 TmrCtrNumber,
						 XTC_TCSR_OFFSET,
						 ControlStatusReg |
						 XTC_CSR_INT_OCCURED_MASK);
			}
		}
	}
}


void process_ipc_msg_from_low(wlan_ipc_msg* msg) {
	u8 rx_pkt_buf;
	rx_frame_info* rx_mpdu;
	tx_frame_info* tx_mpdu;



	switch(IPC_MBOX_MSG_ID_TO_MSG(msg->msg_id)) {
		case IPC_MBOX_RX_MPDU_READY:

			//This message indicates CPU Low has received an MPDU addressed to this node or to the broadcast address
			rx_pkt_buf = msg->arg0;

			//First attempt to lock the indicated Rx pkt buf (CPU Low must unlock it before sending this msg)
			if(lock_pkt_buf_rx(rx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
				warp_printf(PL_ERROR,"Error: unable to lock pkt_buf %d\n",rx_pkt_buf);
			} else {
				rx_mpdu = (rx_frame_info*)RX_PKT_BUF_TO_ADDR(rx_pkt_buf);

				//xil_printf("MB-HIGH: processing buffer %d, mpdu state = %d, length = %d, rate = %d\n",rx_pkt_buf,rx_mpdu->state, rx_mpdu->length,rx_mpdu->rate);
				mpdu_rx_callback((void*)(RX_PKT_BUF_TO_ADDR(rx_pkt_buf)), rx_mpdu->rate, rx_mpdu->length);

				//Free up the rx_pkt_buf
				rx_mpdu->state = RX_MPDU_STATE_EMPTY;

				if(unlock_pkt_buf_rx(rx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
					warp_printf(PL_ERROR, "Error: unable to unlock rx pkt_buf %d\n",rx_pkt_buf);
				}
			}
		break;

		case IPC_MBOX_TX_MPDU_ACCEPT:

			//This message indicates CPU Low has begun the Tx process for the previously submitted MPDU
			// CPU High is now free to begin processing its next Tx frame and submit it to CPU Low
			// CPU Low will not accept a new frame until the previous one is complete

			if(tx_pkt_buf != (msg->arg0)) {
				warp_printf(PL_ERROR, "Received CPU_LOW acceptance of buffer %d, but was expecting buffer %d\n", tx_pkt_buf, msg->arg0);
			}

			tx_pkt_buf = (tx_pkt_buf + 1) % TX_BUFFER_NUM;

			cpu_high_status &= (~CPU_STATUS_WAIT_FOR_IPC_ACCEPT);

			if(lock_pkt_buf_tx(tx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS) {
				warp_printf(PL_ERROR,"Error: unable to lock tx pkt_buf %d\n",tx_pkt_buf);
			} else {
				tx_mpdu = (tx_frame_info*)TX_PKT_BUF_TO_ADDR(tx_pkt_buf);
				tx_mpdu->state = TX_MPDU_STATE_TX_PENDING;
			}

			check_queue_callback();

		break;

		case IPC_MBOX_TX_MPDU_DONE:

			//This message indicates CPU Low has finished the Tx process for the previously submitted-accepted frame
			// CPU High should do any necessary post-processing, then recycle the packet buffer
			tx_mpdu = (tx_frame_info*)TX_PKT_BUF_TO_ADDR(msg->arg0);
			mpdu_tx_done_callback(tx_mpdu);


		break;

		case IPC_MBOX_MAC_ADDR:

			//CPU Low updated the node's MAC address (typically stored in the WARP v3 EEPROM, accessible only to CPU Low)
			memcpy((void*) &(eeprom_mac_addr[0]), (void*) &(ipc_msg_from_low_payload[0]), 6);
		break;

		case IPC_MBOX_CPU_STATUS:
			cpu_low_status = ipc_msg_from_low_payload[0];

			if(cpu_low_status & CPU_STATUS_EXCEPTION){
				warp_printf(PL_ERROR, "An unrecoverable exception has occurred in CPU_LOW, halting...\n");
				warp_printf(PL_ERROR, "Reason code: %d\n", ipc_msg_from_low_payload[1]);
				while(1){}
			}
		break;

		default:
			warp_printf(PL_ERROR, "Unknown IPC message type %d\n",IPC_MBOX_MSG_ID_TO_MSG(msg->msg_id));
		break;
	}


	return;
}

void ipc_rx(){
//	u32 numMsg = 0;
//	xil_printf("Mailbox Rx\n");
	while(ipc_mailbox_read_msg(&ipc_msg_from_low) == IPC_MBOX_SUCCESS){
		process_ipc_msg_from_low(&ipc_msg_from_low);
//		numMsg++;
	}
//	xil_printf("Processed %d msg in one ISR\n",numMsg);
}

void timer_handler(void *CallBackRef, u8 TmrCtrNumber){
	u16 k;
	u8 restart_timer;
	u64 timestamp;

	restart_timer = 0;
	timestamp = get_usec_timestamp();

	switch(TmrCtrNumber){
		case TIMER_CNTR_FAST:
			for(k = 0; k<SCHEDULER_NUM_EVENTS; k++){
				if(scheduler_in_use[SCHEDULE_FINE][k] == 1){
					if(timestamp > scheduler_timestamps[SCHEDULE_FINE][k]){
						restart_timer = 1;
						scheduler_in_use[SCHEDULE_FINE][k] = 0; //Free up schedule element before calling callback in case that function wants to reschedule
						scheduler_callbacks[SCHEDULE_FINE][k]();
					}
				}
			}
			if(restart_timer){
				timer_running[TIMER_CNTR_FAST] = 1;
				XTmrCtr_SetResetValue(&TimerCounterInst, TIMER_CNTR_FAST, FAST_TIMER_DUR_US*(TIMER_FREQ/1000000));
				XTmrCtr_Start(&TimerCounterInst, TIMER_CNTR_FAST);
			} else {
				timer_running[TIMER_CNTR_FAST] = 0;
			}
		break;
		case TIMER_CNTR_SLOW:
		//	xil_printf("slow expiration!\n");
			for(k = 0; k<SCHEDULER_NUM_EVENTS; k++){
				if(scheduler_in_use[SCHEDULE_COARSE][k] == 1){
					if(timestamp > scheduler_timestamps[SCHEDULE_COARSE][k]){
						restart_timer = 1;
						scheduler_in_use[SCHEDULE_COARSE][k] = 0; //Free up schedule element before calling callback in case that function wants to reschedule
						scheduler_callbacks[SCHEDULE_COARSE][k]();
					}
				}
			}
			if(restart_timer){
				timer_running[TIMER_CNTR_SLOW] = 1;
				XTmrCtr_SetResetValue(&TimerCounterInst, TIMER_CNTR_SLOW, SLOW_TIMER_DUR_US*(TIMER_FREQ/1000000));
				XTmrCtr_Start(&TimerCounterInst, TIMER_CNTR_SLOW);
			//	xil_printf("restarting slow timer\n");
			} else {
				timer_running[TIMER_CNTR_SLOW] = 0;
			}
		break;
	}
}

inline int interrupt_start(){
	return XIntc_Start(&InterruptController, XIN_REAL_MODE);
}

inline void interrupt_stop(){
	XIntc_Stop(&InterruptController);
}

int interrupt_init(){
	int Result;
	Result = XIntc_Initialize(&InterruptController, INTC_DEVICE_ID);
	if (Result != XST_SUCCESS) {
		return Result;
	}

	Result = XIntc_Connect(&InterruptController, INTC_GPIO_INTERRUPT_ID, (XInterruptHandler)GpioIsr, &Gpio);
	if (Result != XST_SUCCESS) {
		warp_printf(PL_ERROR,"Failed to connect GPIO to XIntc\n");
		return Result;
	}

	Result = XIntc_Connect(&InterruptController, UARTLITE_INT_IRQ_ID, (XInterruptHandler)XUartLite_InterruptHandler, &UartLite);
	if (Result != XST_SUCCESS) {
		warp_printf(PL_ERROR,"Failed to connect XUartLite to XIntc\n");
		return Result;
	}

	//Connect Timer to Interrupt Controller
	Result = XIntc_Connect(&InterruptController, TMRCTR_INTERRUPT_ID, (XInterruptHandler)XTmrCtr_CustomInterruptHandler, &TimerCounterInst);
	//Result = XIntc_Connect(&InterruptController, TMRCTR_DEVICE_ID, (XInterruptHandler)timer_handler, &TimerCounterInst);

	if (Result != XST_SUCCESS) {
		xil_printf("Failed to connect XTmrCtr to XIntC\n");
		return -1;
	}

	wlan_lib_setup_mailbox_interrupt(&InterruptController);
	wlan_eth_setup_interrupt(&InterruptController);

	Result = XIntc_Start(&InterruptController, XIN_REAL_MODE);
	if (Result != XST_SUCCESS) {
		warp_printf(PL_ERROR,"Failed to start XIntc\n");
		return Result;
	}

	XIntc_Enable(&InterruptController, INTC_GPIO_INTERRUPT_ID);
	XIntc_Enable(&InterruptController, UARTLITE_INT_IRQ_ID);
	XIntc_Enable(&InterruptController, TMRCTR_INTERRUPT_ID);


	Xil_ExceptionInit();

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,(Xil_ExceptionHandler)XIntc_InterruptHandler, &InterruptController);

	/* Enable non-critical exceptions */
	Xil_ExceptionEnable();

	XGpio_InterruptEnable(&Gpio, GPIO_INPUT_INTERRUPT);
	XGpio_InterruptGlobalEnable(&Gpio);

	XUartLite_SetSendHandler(&UartLite, SendHandler, &UartLite);
	XUartLite_SetRecvHandler(&UartLite, RecvHandler, &UartLite);

	XUartLite_EnableInterrupt(&UartLite);

	XUartLite_Recv(&UartLite, ReceiveBuffer, UART_BUFFER_SIZE);

	return 0;
}

void SendHandler(void *CallBackRef, unsigned int EventData){
	xil_printf("send\n");
}

void RecvHandler(void *CallBackRef, unsigned int EventData){
	XUartLite_DisableInterrupt(&UartLite);
	uart_callback(ReceiveBuffer[0]);
	XUartLite_EnableInterrupt(&UartLite);
	XUartLite_Recv(&UartLite, ReceiveBuffer, UART_BUFFER_SIZE);
}

void GpioIsr(void *InstancePtr){
	XGpio *GpioPtr = (XGpio *)InstancePtr;
	u32 gpio_read;

	XGpio_InterruptDisable(GpioPtr, GPIO_INPUT_INTERRUPT);
	gpio_read = XGpio_DiscreteRead(GpioPtr, GPIO_INPUT_CHANNEL);

	if(gpio_read & GPIO_MASK_PB_U) pb_u_callback();
	if(gpio_read & GPIO_MASK_PB_M) pb_m_callback();
	if(gpio_read & GPIO_MASK_PB_D) pb_d_callback();

	(void)XGpio_InterruptClear(GpioPtr, GPIO_INPUT_INTERRUPT);
	XGpio_InterruptEnable(GpioPtr, GPIO_INPUT_INTERRUPT);

	return;
}

void wlan_mac_util_set_ipc_rx_callback(void(*callback)()){
	ipc_rx_callback = (function_ptr_t)callback;

	wlan_lib_setup_mailbox_rx_callback((void*)ipc_rx_callback);
}

void wlan_mac_util_set_pb_u_callback(void(*callback)()){
	pb_u_callback = (function_ptr_t)callback;
}

void wlan_mac_util_set_pb_m_callback(void(*callback)()){
	pb_m_callback = (function_ptr_t)callback;
}
void wlan_mac_util_set_pb_d_callback(void(*callback)()){
	pb_d_callback = (function_ptr_t)callback;
}

void wlan_mac_util_set_eth_rx_callback(void(*callback)()){
	eth_rx_callback = (function_ptr_t)callback;
}

void wlan_mac_util_set_mpdu_tx_done_callback(void(*callback)()){
	mpdu_tx_done_callback = (function_ptr_t)callback;
}

void wlan_mac_util_set_mpdu_rx_callback(void(*callback)()){
	mpdu_rx_callback = (function_ptr_t)callback;
}

void wlan_mac_util_set_uart_rx_callback(void(*callback)()){
	uart_callback = (function_ptr_t)callback;
}

void wlan_mac_util_set_check_queue_callback(void(*callback)()){
	check_queue_callback = (function_ptr_t)callback;
}

void gpio_timestamp_initialize(){
	XGpio_Initialize(&GPIO_timestamp, TIMESTAMP_GPIO_DEVICE_ID);
	XGpio_SetDataDirection(&GPIO_timestamp, TIMESTAMP_GPIO_LSB_CHAN, 0xFFFFFFFF);
	XGpio_SetDataDirection(&GPIO_timestamp, TIMESTAMP_GPIO_MSB_CHAN, 0xFFFFFFFF);
}

u64 get_usec_timestamp(){
	u32 timestamp_high_u32;
	u32 timestamp_low_u32;
	u64 timestamp_u64;
	timestamp_high_u32 = XGpio_DiscreteRead(&GPIO_timestamp,TIMESTAMP_GPIO_MSB_CHAN);
	timestamp_low_u32 = XGpio_DiscreteRead(&GPIO_timestamp,TIMESTAMP_GPIO_LSB_CHAN);
	timestamp_u64 = (((u64)timestamp_high_u32)<<32) + ((u64)timestamp_low_u32);
	return timestamp_u64;
}


void wlan_mac_schedule_event(u8 scheduler_sel, u32 delay, void(*callback)()){
	u32 k;

	u64 timestamp = get_usec_timestamp();

	for (k = 0; k<SCHEDULER_NUM_EVENTS; k++){
		if(scheduler_in_use[scheduler_sel][k] == 0){ //Found an empty schedule element
			scheduler_in_use[scheduler_sel][k] = 1; //We are using this schedule element
			scheduler_callbacks[scheduler_sel][k] = (function_ptr_t)callback;
			scheduler_timestamps[scheduler_sel][k] = timestamp+(u64)delay;

			if(scheduler_sel == SCHEDULE_FINE){
				if(timer_running[TIMER_CNTR_FAST] == 0){
					timer_running[TIMER_CNTR_FAST] = 1;
					XTmrCtr_SetResetValue(&TimerCounterInst, TIMER_CNTR_FAST, FAST_TIMER_DUR_US*(TIMER_FREQ/1000000));
					XTmrCtr_Start(&TimerCounterInst, TIMER_CNTR_FAST);
				}

			} else if(scheduler_sel == SCHEDULE_COARSE) {
				if(timer_running[TIMER_CNTR_SLOW] == 0){
					timer_running[TIMER_CNTR_SLOW] = 1;
					XTmrCtr_SetResetValue(&TimerCounterInst, TIMER_CNTR_SLOW, SLOW_TIMER_DUR_US*(TIMER_FREQ/1000000));
					XTmrCtr_Start(&TimerCounterInst, TIMER_CNTR_SLOW);
				}
			}

			return;
		}
	}
	warp_printf(PL_ERROR,"ERROR: %d schedules already filled\n",SCHEDULER_NUM_EVENTS);
}

void poll_schedule(){
	u32 k;
	u64 timestamp = get_usec_timestamp();

	for(k = 0; k<SCHEDULER_NUM_EVENTS; k++){
		if(scheduler_in_use[SCHEDULE_FINE][k] == 1){
			if(timestamp > scheduler_timestamps[SCHEDULE_FINE][k]){
				scheduler_in_use[SCHEDULE_FINE][k] = 0; //Free up schedule element before calling callback in case that function wants to reschedule
				scheduler_callbacks[SCHEDULE_FINE][k]();
			}
		}
	}

	for(k = 0; k<SCHEDULER_NUM_EVENTS; k++){
		if(scheduler_in_use[SCHEDULE_COARSE][k] == 1){
			if(timestamp > scheduler_timestamps[SCHEDULE_COARSE][k]){
				scheduler_in_use[SCHEDULE_COARSE][k] = 0; //Free up schedule element before calling callback in case that function wants to reschedule
				scheduler_callbacks[SCHEDULE_COARSE][k]();
			}
		}
	}



}

int wlan_mac_poll_tx_queue(u16 queue_sel){
	int return_value = 0;;

	packet_bd_list dequeue;
	packet_bd* tx_queue;

	dequeue = dequeue_from_beginning(queue_sel,1);

	if(dequeue.length == 1){
		return_value = 1;
		tx_queue = dequeue.first;
		mpdu_transmit(tx_queue);
		queue_checkin(&dequeue);
		wlan_eth_dma_update();
	}

	return return_value;
}

void wlan_mac_util_process_tx_done(tx_frame_info* frame,station_info* station){
	(station->num_tx_total)++;
	if((frame->state_verbose) == TX_MPDU_STATE_VERBOSE_SUCCESS){
		(station->num_tx_success)++;
	}
}

u8 wlan_mac_util_get_tx_rate(station_info* station){
	return station->tx_rate;
}

void write_hex_display(u8 val){
	//u8 val: 2 digit decimal value to be printed to hex displays
   userio_write_control(USERIO_BASEADDR, userio_read_control(USERIO_BASEADDR) | (W3_USERIO_HEXDISP_L_MAPMODE | W3_USERIO_HEXDISP_R_MAPMODE));
   userio_write_hexdisp_left(USERIO_BASEADDR, val/10);
   userio_write_hexdisp_right(USERIO_BASEADDR, val%10);
}

void write_hex_display_dots(u8 dots_on){
	u32 left_hex,right_hex;

	left_hex = userio_read_hexdisp_left(USERIO_BASEADDR);
	right_hex = userio_read_hexdisp_right(USERIO_BASEADDR);

	if(dots_on){
		userio_write_hexdisp_left(USERIO_BASEADDR, W3_USERIO_HEXDISP_DP | left_hex);
		userio_write_hexdisp_right(USERIO_BASEADDR, W3_USERIO_HEXDISP_DP | right_hex);
	} else {
		userio_write_hexdisp_left(USERIO_BASEADDR, (~W3_USERIO_HEXDISP_DP) & left_hex);
		userio_write_hexdisp_right(USERIO_BASEADDR, (~W3_USERIO_HEXDISP_DP) & right_hex);
	}

}
int memory_test(){
	//Test DRAM
	u8 i,j;

	u8 test_u8;
	u16 test_u16;
	u32 test_u32;
	u64 test_u64;

	/*
	xil_printf("\nTesting DRAM -- Note: this function does not currently handle the case of a DDR3 SODIMM being\n");
	xil_printf("absent from the board. If this hardware design can't reach the DRAM, this function will hang and\n");
	xil_printf("this print will be the last thing that makes it out to uart. The USE_DRAM #define should be disabled\n");
	xil_printf("if this occurs. In a future release, this function will handle DRAM failure better.\n\n");
	*/

	for(i=0;i<6;i++){
		void* memory_ptr = (void*)DDR3_BASEADDR + (i*100000*1024);

		for(j=0;j<3;j++){
			//Test 1 byte offsets to make sure byte enables are all working

			test_u8 = rand()&0xFF;
			test_u16 = rand()&0xFFFF;
			test_u32 = rand()&0xFFFFFFFF;
			test_u64 = (((u64)rand()&0xFFFFFFFF)<<32) + ((u64)rand()&0xFFFFFFFF);

			*((u8*)memory_ptr) = test_u8;

			if(*((u8*)memory_ptr) != test_u8){
				xil_printf("DRAM Failure: Addr: 0x%08x -- Unable to verify write of u8\n",memory_ptr);
				return -1;
			}
			*((u16*)memory_ptr) = test_u16;
			if(*((u16*)memory_ptr) != test_u16){
				xil_printf("DRAM Failure: Addr: 0x%08x -- Unable to verify write of u16\n",memory_ptr);
				return -1;
			}
			*((u32*)memory_ptr) = test_u32;
			if(*((u32*)memory_ptr) != test_u32){
				xil_printf("DRAM Failure: Addr: 0x%08x -- Unable to verify write of u32\n",memory_ptr);
				return -1;
			}
			*((u64*)memory_ptr) = test_u64;
			if(*((u64*)memory_ptr) != test_u64){
				xil_printf("DRAM Failure: Addr: 0x%08x -- Unable to verify write of u64\n",memory_ptr);
				return -1;
			}

		}

	}

	return 0;
}

int is_tx_buffer_empty(){
	tx_frame_info* tx_mpdu = (tx_frame_info*) TX_PKT_BUF_TO_ADDR(tx_pkt_buf);
	if(tx_mpdu->state == TX_MPDU_STATE_TX_PENDING && ((cpu_high_status & CPU_STATUS_WAIT_FOR_IPC_ACCEPT) == 0)){
		return 1;
	} else {
		return 0;
	}
}

void mpdu_transmit(packet_bd* tx_queue) {

	wlan_ipc_msg ipc_msg_to_low;
	tx_frame_info* tx_mpdu = (tx_frame_info*) TX_PKT_BUF_TO_ADDR(tx_pkt_buf);
	station_info* station = (station_info*)(tx_queue->metadata_ptr);





	if(is_tx_buffer_empty()){

		//For now, this is just a one-shot DMA transfer that effectively blocks
		while(XAxiCdma_IsBusy(&cdma_inst)) {}
		XAxiCdma_SimpleTransfer(&cdma_inst, (u32)(tx_queue->buf_ptr), (u32)TX_PKT_BUF_TO_ADDR(tx_pkt_buf), ((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.length + sizeof(tx_frame_info) + PHY_TX_PKT_BUF_PHY_HDR_SIZE, NULL, NULL);
		while(XAxiCdma_IsBusy(&cdma_inst)) {}


		if(station == NULL){
			//Broadcast transmissions have no station information, so we default to a nominal rate
			tx_mpdu->AID = 0;
			tx_mpdu->rate = WLAN_MAC_RATE_6M;
		} else {
			//Request the rate to use for this station
			tx_mpdu->AID = station->AID;
			tx_mpdu->rate = wlan_mac_util_get_tx_rate(station);
			//tx_mpdu->rate = default_unicast_rate;
		}

		tx_mpdu->state = TX_MPDU_STATE_READY;
		tx_mpdu->retry_count = 0;

		ipc_msg_to_low.msg_id = IPC_MBOX_MSG_ID(IPC_MBOX_TX_MPDU_READY);
		ipc_msg_to_low.arg0 = tx_pkt_buf;
		ipc_msg_to_low.num_payload_words = 0;

		if(unlock_pkt_buf_tx(tx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
			warp_printf(PL_ERROR,"Error: unable to unlock tx pkt_buf %d\n",tx_pkt_buf);
		} else {
			cpu_high_status |= CPU_STATUS_WAIT_FOR_IPC_ACCEPT;

			ipc_mailbox_write_msg(&ipc_msg_to_low);
		}
	} else {
		warp_printf(PL_ERROR, "Bad state in mpdu_transmit. Attempting to transmit but tx_buffer %d is not empty\n",tx_pkt_buf);
	}

	return;
}

int cpu_low_initialized(){
	return ((cpu_low_status & CPU_STATUS_INITIALIZED) != 0);
}

int cpu_low_ready(){
	//xil_printf("cpu_high_status = 0x%08x\n",cpu_high_status);
	return ((cpu_high_status & CPU_STATUS_WAIT_FOR_IPC_ACCEPT) == 0);
}

u8* get_eeprom_mac_addr(){
	return &(eeprom_mac_addr[0]);
}

u8 valid_tagged_rate(u8 rate){
	#define NUM_VALID_RATES 12
	u32 i;
	//These values correspond to the 12 possible valid rates sent in 802.11b/a/g. The faster 802.11n rates will return as
	//invalid when this function is used.
	u8 valid_rates[NUM_VALID_RATES] = {0x02, 0x04, 0x0b, 0x16, 0x0c, 0x12, 0x18, 0x24, 0x30, 0x48, 0x60, 0x6c};

	for(i = 0; i < NUM_VALID_RATES; i++ ){
		if((rate & ~RATE_BASIC) == valid_rates[i]) return 1;
	}

	return 0;
}

void tagged_rate_to_readable_rate(u8 rate, char* str){
	#define NUM_VALID_RATES 12
	//These values correspond to the 12 possible valid rates sent in 802.11b/a/g. The faster 802.11n rates will return as
	//invalid when this function is used.


	switch(rate & ~RATE_BASIC){
		case 0x02:
			strcpy(str,"1");
		break;
		case 0x04:
			strcpy(str,"2");
		break;
		case 0x0b:
			strcpy(str,"5.5");
		break;
		case 0x16:
			strcpy(str,"11");
		break;
		case 0x0c:
			strcpy(str,"6");
		break;
		case 0x12:
			strcpy(str,"9");
		break;
		case 0x18:
			strcpy(str,"12");
		break;
		case 0x24:
			strcpy(str,"18");
		break;
		case 0x30:
			strcpy(str,"24");
		break;
		case 0x48:
			strcpy(str,"36");
		break;
		case 0x60:
			strcpy(str,"48");
		break;
		case 0x6c:
			strcpy(str,"54");
		break;
		default:
			//Unknown rate
			*str = NULL;
		break;
	}


	return;
}



