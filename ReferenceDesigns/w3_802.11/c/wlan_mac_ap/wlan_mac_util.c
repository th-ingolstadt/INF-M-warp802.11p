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

#include "wlan_lib.h"
#include "wlan_mac_util.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_eth_util.h"
#include "w3_userio.h"
#include "xparameters.h"

static XGpio GPIO_timestamp;
static XGpio Gpio;
static XIntc InterruptController;
XUartLite UartLite;

u8 ReceiveBuffer[UART_BUFFER_SIZE];

function_ptr_t eth_rx_callback, mpdu_tx_callback, pb_u_callback, pb_m_callback, pb_d_callback, uart_callback, ipc_rx_callback;

//Scheduler
#define SCHEDULER_NUM_EVENTS 6
static u8 scheduler_in_use[SCHEDULER_NUM_EVENTS];
static function_ptr_t scheduler_callbacks[SCHEDULER_NUM_EVENTS];
static u64 scheduler_timestamps[SCHEDULER_NUM_EVENTS];

void wlan_mac_util_init(){
	int Status;
	u32 gpio_read;
	u64 timestamp;

	eth_rx_callback = (function_ptr_t)nullCallback;
	mpdu_tx_callback = (function_ptr_t)nullCallback;
	pb_u_callback = (function_ptr_t)nullCallback;
	pb_m_callback = (function_ptr_t)nullCallback;
	pb_d_callback = (function_ptr_t)nullCallback;
	uart_callback = (function_ptr_t)nullCallback;
	ipc_rx_callback = (function_ptr_t)nullCallback;

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

//	while(1){
//		xil_printf("0x%08x\n",XGpio_DiscreteRead(&Gpio, GPIO_INPUT_CHANNEL));
//	}


//	Status = interrupt_init();
//	if(Status != 0){
//		warp_printf(PL_ERROR, "Error initializing interrupts, status code: %d\n", Status);
//		return;
//	}

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

	wlan_lib_setup_mailbox_interrupt(&InterruptController);
	//wlan_lib_setup_mailbox_rx_callback((void*)ipc_rx_callback);

	Result = XIntc_Start(&InterruptController, XIN_REAL_MODE);
	if (Result != XST_SUCCESS) {
		warp_printf(PL_ERROR,"Failed to start XIntc\n");
		return Result;
	}

	XIntc_Enable(&InterruptController, INTC_GPIO_INTERRUPT_ID);
	XIntc_Enable(&InterruptController, UARTLITE_INT_IRQ_ID);


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

void wlan_mac_util_set_mpdu_tx_callback(void(*callback)()){
	mpdu_tx_callback = (function_ptr_t)callback;
}

void wlan_mac_util_set_uart_rx_callback(void(*callback)()){
	uart_callback = (function_ptr_t)callback;
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


void wlan_mac_schedule_event(u32 delay, void(*callback)()){
	u32 k;

	u64 timestamp = get_usec_timestamp();

	for (k = 0; k<SCHEDULER_NUM_EVENTS; k++){
		if(scheduler_in_use[k] == 0){ //Found an empty schedule element
			scheduler_in_use[k] = 1; //We are using this schedule element
			scheduler_callbacks[k] = (function_ptr_t)callback;
			scheduler_timestamps[k] = timestamp+(u64)delay;
			return;
		}
	}
	warp_printf(PL_ERROR,"ERROR: %d schedules already filled\n",SCHEDULER_NUM_EVENTS);
}

void poll_schedule(){
	u32 k;
	u64 timestamp = get_usec_timestamp();

	for(k = 0; k<SCHEDULER_NUM_EVENTS; k++){
		if(scheduler_in_use[k] == 1){
			if(timestamp > scheduler_timestamps[k]){
				scheduler_in_use[k] = 0; //Free up schedule element before calling callback in case that function wants to reschedule
				scheduler_callbacks[k]();
			}
		}
	}
}

int wlan_mac_poll_tx_queue(u16 queue_sel){
	int return_value = 0;;

	pqueue_list dequeue;
	pqueue* tx_queue;

	dequeue = dequeue_from_beginning(queue_sel,1);

	if(dequeue.length == 1){
		return_value = 1;
		tx_queue = dequeue.first;
		mpdu_tx_callback(tx_queue);
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

