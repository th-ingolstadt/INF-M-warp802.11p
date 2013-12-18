////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_util.c
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
//          Erik Welsh (welsh [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

/***************************** Include Files *********************************/

//Xilinx Includes
#include "stdlib.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xil_exception.h"
#include "xintc.h"
#include "xuartlite.h"
#include "xaxicdma.h"

//WARP Includes
#include "w3_userio.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_ipc_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_fmc_pkt.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_ipc.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_schedule.h"
#include "malloc.h"

#include "wlan_exp_common.h"
#include "wlan_exp_node.h"

/*********************** Global Variable Definitions *************************/

extern int __data_start;
extern int __data_end;
extern int __bss_start;
extern int __bss_end;
extern int _heap_start;
extern int _HEAP_SIZE;


/*************************** Variable Definitions ****************************/

// HW structures
static XGpio       Gpio_timestamp;
static XGpio       Gpio;
XIntc       	   InterruptController;
XUartLite          UartLite;
XAxiCdma           cdma_inst;

// UART interface
u8                 uart_rx_buffer[UART_BUFFER_SIZE];

// 802.11 Transmit packet buffer
u8                 tx_pkt_buf;

// Callback function pointers
function_ptr_t     pb_u_callback;
function_ptr_t     pb_m_callback;
function_ptr_t     pb_d_callback;
function_ptr_t     uart_callback;

// Node information
wlan_mac_hw_info   	hw_info;
u8					dram_present;

// Memory Allocation Debugging
static u32			num_malloc;
static u32			num_free;
static u32			num_realloc;

/******************************** Functions **********************************/

void initialize_heap(){
	u32 data_size;
	volatile u32* identifier = (u32*)INIT_DATA_BASEADDR;
	data_size = 4*(&__data_end - &__data_start);

	//Zero out the heap
	bzero((void*)&_heap_start, (int)&_HEAP_SIZE);

	//Zero out the bss
	bzero((void*)&__bss_start, 4*(&__bss_end - &__bss_start));

	#ifdef INIT_DATA_BASEADDR
	if(*identifier == INIT_DATA_DOTDATA_IDENTIFIER){
		//This program has run before. We should copy the .data out of the INIT_DATA memory.
		if(data_size <= INIT_DATA_DOTDATA_SIZE){
			memcpy((void*)&__data_start, (void*)INIT_DATA_DOTDATA_START, data_size);
		}

	} else {
		//This is the first time this program has been run.
		if(data_size <= INIT_DATA_DOTDATA_SIZE){
			*identifier = INIT_DATA_DOTDATA_IDENTIFIER;
			memcpy((void*)INIT_DATA_DOTDATA_START, (void*)&__data_start, data_size);
		}

	}
	#endif
}



void wlan_mac_high_init(){
	int            Status;
    u32            i;
	u32            queue_len;
	u64            timestamp;
	u32            log_size;
	tx_frame_info* tx_mpdu;

	// ***************************************************
	// Initialize the utility library
	// ***************************************************
	wlan_lib_init();

	// ***************************************************
    // Initialize callbacks and global state variables
	// ***************************************************
	pb_u_callback           = (function_ptr_t)nullCallback;
	pb_m_callback           = (function_ptr_t)nullCallback;
	pb_d_callback           = (function_ptr_t)nullCallback;
	uart_callback           = (function_ptr_t)nullCallback;

	wlan_lib_mailbox_set_rx_callback((void*)ipc_rx);

	num_malloc = 0;
	num_realloc = 0;
	num_free = 0;


	// ***************************************************
	// Initialize Transmit Packet Buffers
	// ***************************************************
	for(i=0;i < NUM_TX_PKT_BUFS; i++){
		tx_mpdu = (tx_frame_info*)TX_PKT_BUF_TO_ADDR(i);
		tx_mpdu->state = TX_MPDU_STATE_EMPTY;
	}
	tx_pkt_buf = 0;
	warp_printf(PL_VERBOSE, "locking tx_pkt_buf = %d\n", tx_pkt_buf);
	if(lock_pkt_buf_tx(tx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
		warp_printf(PL_ERROR,"Error: unable to lock pkt_buf %d\n",tx_pkt_buf);
	}
	tx_mpdu = (tx_frame_info*)TX_PKT_BUF_TO_ADDR(tx_pkt_buf);
	tx_mpdu->state = TX_MPDU_STATE_TX_PENDING;



	// ***************************************************
	// Initialize CDMA, GPIO, and UART drivers
	// ***************************************************
	//Initialize the central DMA (CDMA) driver
	XAxiCdma_Config *cdma_cfg_ptr;
	cdma_cfg_ptr = XAxiCdma_LookupConfig(XPAR_AXI_CDMA_0_DEVICE_ID);
	Status = XAxiCdma_CfgInitialize(&cdma_inst, cdma_cfg_ptr, cdma_cfg_ptr->BaseAddress);
	if (Status != XST_SUCCESS) {
		warp_printf(PL_ERROR,"Error initializing CDMA: %d\n", Status);
	}
	XAxiCdma_IntrDisable(&cdma_inst, XAXICDMA_XR_IRQ_ALL_MASK);

	//Initialize the GPIO driver
	Status = XGpio_Initialize(&Gpio, GPIO_DEVICE_ID);
	gpio_timestamp_initialize();

	if (Status != XST_SUCCESS) {
		warp_printf(PL_ERROR, "Error initializing GPIO\n");
		return;
	}
	//Set direction of GPIO channels
	XGpio_SetDataDirection(&Gpio, GPIO_INPUT_CHANNEL, 0xFFFFFFFF);
	XGpio_SetDataDirection(&Gpio, GPIO_OUTPUT_CHANNEL, 0);

	//Initialize the UART driver
	Status = XUartLite_Initialize(&UartLite, UARTLITE_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		warp_printf(PL_ERROR, "Error initializing XUartLite\n");
		return;
	}

	//Test to see if DRAM SODIMM is connected to board
	queue_dram_present(0);
	dram_present = 0;
	timestamp = get_usec_timestamp();

	while((get_usec_timestamp() - timestamp) < 100000){
		if((XGpio_DiscreteRead(&Gpio, GPIO_INPUT_CHANNEL)&GPIO_MASK_DRAM_INIT_DONE)){
			xil_printf("DRAM SODIMM Detected\n");
			if(memory_test()==0){
				queue_dram_present(1);
				dram_present = 1;
			} else {
				queue_dram_present(0);
				dram_present = 0;
			}
			break;
		}
	}

	// ***************************************************
	// Initialize various subsystems in the MAC High Framework
	// ***************************************************
	queue_len = queue_init();

	if( dram_present ) {
		//The event_list lives in DRAM immediately following the queue payloads.
		if(MAX_EVENT_LOG == -1){
			log_size = (DDR3_SIZE - queue_len);
		} else {
			log_size = min( (DDR3_SIZE - queue_len), MAX_EVENT_LOG );
		}

		event_log_init( (void*)(DDR3_BASEADDR + queue_len), log_size );

	} else {
		//No DRAM, so the log has nowhere to be stored.
		log_size = 0;
	}

#ifdef USE_WARPNET_WLAN_EXP
	// Communicate the log size to WARPNet
	node_info_set_event_log_size( log_size );
#endif

	wlan_fmc_pkt_init();
	wlan_mac_ipc_init();
	wlan_eth_init();
	wlan_mac_schedule_init();
	wlan_mac_ltg_sched_init();
    
}

int wlan_mac_high_interrupt_init(){
	int Result;

	// ***************************************************
	// Initialize XIntc
	// ***************************************************
	Result = XIntc_Initialize(&InterruptController, INTC_DEVICE_ID);
	if (Result != XST_SUCCESS) {
		return Result;
	}

	// ***************************************************
	// Connect interrupt devices "owned" by wlan_mac_util
	// ***************************************************
	Result = XIntc_Connect(&InterruptController, INTC_GPIO_INTERRUPT_ID, (XInterruptHandler)GpioIsr, &Gpio);
	if (Result != XST_SUCCESS) {
		warp_printf(PL_ERROR,"Failed to connect GPIO to XIntc\n");
		return Result;
	}
	XIntc_Enable(&InterruptController, INTC_GPIO_INTERRUPT_ID);
	XGpio_InterruptEnable(&Gpio, GPIO_INPUT_INTERRUPT);
	XGpio_InterruptGlobalEnable(&Gpio);

	Result = XIntc_Connect(&InterruptController, UARTLITE_INT_IRQ_ID, (XInterruptHandler)XUartLite_InterruptHandler, &UartLite);
	if (Result != XST_SUCCESS) {
		warp_printf(PL_ERROR,"Failed to connect XUartLite to XIntc\n");
		return Result;
	}
	XIntc_Enable(&InterruptController, UARTLITE_INT_IRQ_ID);
	XUartLite_SetRecvHandler(&UartLite, wlan_mac_high_uart_rx_handler, &UartLite);
	XUartLite_EnableInterrupt(&UartLite);

	// ***************************************************
	// Connect interrupt devices in other subsystems
	// ***************************************************
	Result = wlan_mac_schedule_setup_interrupt(&InterruptController);
	if (Result != XST_SUCCESS) {
		xil_printf("Failed to set up scheduler interrupt\n");
		return -1;
	}

	Result = wlan_lib_mailbox_setup_interrupt(&InterruptController);
	if (Result != XST_SUCCESS) {
		xil_printf("Failed to set up wlan_lib mailbox interrupt\n");
		return -1;
	}

	Result = wlan_eth_setup_interrupt(&InterruptController);
	if (Result != XST_SUCCESS) {
		warp_printf(PL_ERROR,"Failed to set up Ethernet interrupt\n");
		return Result;
	}

	wlan_fmc_pkt_mailbox_setup_interrupt(&InterruptController);
	if (Result != XST_SUCCESS) {
		warp_printf(PL_ERROR,"Failed to set up FMC Pkt interrupt\n");
		return Result;
	}

	// ***************************************************
	// Start the interrupt controller
	// ***************************************************
	//wlan_mac_interrupt_start();

	// ***************************************************
	// Enable MicroBlaze exceptions
	// ***************************************************
	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,(Xil_ExceptionHandler)XIntc_InterruptHandler, &InterruptController);
	Xil_ExceptionEnable();

	return 0;
}

inline int wlan_mac_high_interrupt_start(){
	return XIntc_Start(&InterruptController, XIN_REAL_MODE);
}

inline void wlan_mac_high_interrupt_stop(){
	XIntc_Stop(&InterruptController);
}

wlan_mac_hw_info* wlan_mac_high_get_hw_info(){
	return &hw_info;
}

void wlan_mac_high_print_hw_info( wlan_mac_hw_info * info ) {
	int i;

	xil_printf("WLAN MAC HW INFO:  \n");
	xil_printf("  Type             :  0x%8x\n", info->type);
	xil_printf("  Serial Number    :  %d\n",    info->serial_number);
	xil_printf("  FPGA DNA         :  0x%8x  0x%8x\n", info->fpga_dna[1], info->fpga_dna[0]);
	xil_printf("  WLAN EXP ETH Dev :  %d\n",    info->wn_exp_eth_device);

	xil_printf("  WLAN EXP HW Addr :  %02x",    info->hw_addr_wn[0]);
	for( i = 1; i < WLAN_MAC_ETH_ADDR_LEN; i++ ) {
		xil_printf(":%02x", info->hw_addr_wn[i]);
	}
	xil_printf("\n");

	xil_printf("  WLAN HW Addr     :  %02x",    info->hw_addr_wlan[0]);
	for( i = 1; i < WLAN_MAC_ETH_ADDR_LEN; i++ ) {
		xil_printf(":%02x", info->hw_addr_wlan[i]);
	}
	xil_printf("\n");

	xil_printf("END \n");

}


void wlan_mac_high_uart_rx_handler(void *CallBackRef, unsigned int EventData){
	XUartLite_Recv(&UartLite, uart_rx_buffer, UART_BUFFER_SIZE);
	uart_callback(uart_rx_buffer[0]);
}

station_info* wlan_mac_high_find_station_info_AID(dl_list* list, u32 aid){
	u32 i;
	station_info* curr_station_info = (station_info*)(list->first);

	for( i = 0; i < list->length; i++){
		if(curr_station_info->AID == aid){
			return curr_station_info;
		} else {
			curr_station_info = station_info_next(curr_station_info);
		}
	}
	return NULL;
}

station_info* wlan_mac_high_find_station_info_ADDR(dl_list* list, u8* addr){
	u32 i;
	station_info* curr_station_info = (station_info*)(list->first);

	for( i = 0; i < list->length; i++){
		if(wlan_addr_eq(curr_station_info->addr, addr)){
			return curr_station_info;
		} else {
			curr_station_info = station_info_next(curr_station_info);
		}
	}
	return NULL;
}

statistics* wlan_mac_high_find_statistics_ADDR(dl_list* list, u8* addr){
	u32 i;
	statistics* curr_statistics = (statistics*)(list->first);

	for( i = 0; i < list->length; i++){
		if(wlan_addr_eq(curr_statistics->addr, addr)){
			return curr_statistics;
		} else {
			curr_statistics = statistics_next(curr_statistics);
		}
	}
	return NULL;
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

void wlan_mac_util_set_pb_u_callback(void(*callback)()){
	pb_u_callback = (function_ptr_t)callback;
}

void wlan_mac_util_set_pb_m_callback(void(*callback)()){
	pb_m_callback = (function_ptr_t)callback;
}
void wlan_mac_util_set_pb_d_callback(void(*callback)()){
	pb_d_callback = (function_ptr_t)callback;
}

void wlan_mac_util_set_uart_rx_callback(void(*callback)()){
	uart_callback = (function_ptr_t)callback;
}

void gpio_timestamp_initialize(){
	XGpio_Initialize(&Gpio_timestamp, TIMESTAMP_GPIO_DEVICE_ID);
	XGpio_SetDataDirection(&Gpio_timestamp, TIMESTAMP_GPIO_LSB_CHAN, 0xFFFFFFFF);
	XGpio_SetDataDirection(&Gpio_timestamp, TIMESTAMP_GPIO_MSB_CHAN, 0xFFFFFFFF);
}

u64 get_usec_timestamp(){
	u32 timestamp_high_u32;
	u32 timestamp_low_u32;
	u64 timestamp_u64;

	timestamp_high_u32 = XGpio_DiscreteRead(&Gpio_timestamp,TIMESTAMP_GPIO_MSB_CHAN);
	timestamp_low_u32  = XGpio_DiscreteRead(&Gpio_timestamp,TIMESTAMP_GPIO_LSB_CHAN);
	timestamp_u64      = (((u64)timestamp_high_u32)<<32) + ((u64)timestamp_low_u32);

	return timestamp_u64;
}

int wlan_mac_poll_tx_queue(u16 queue_sel){
	int return_value = 0;;

	dl_list dequeue;
	packet_bd* tx_queue;

	dequeue_from_beginning(&dequeue, queue_sel,1);

	//xil_printf("wlan_mac_poll_tx_queue(%d)\n", queue_sel);

	if(dequeue.length == 1){
		return_value = 1;
		tx_queue = (packet_bd*)(dequeue.first);

		mpdu_transmit(tx_queue);
		queue_checkin(&dequeue);
		wlan_eth_dma_update();
	}

	return return_value;
}

void wlan_mac_util_process_tx_done(tx_frame_info* frame,station_info* station){
	(station->stats->num_tx_total)++;
	(station->stats->num_retry) += (frame->retry_count);
	if((frame->state_verbose) == TX_MPDU_STATE_VERBOSE_SUCCESS){
		(station->stats->num_tx_success)++;
		//If this transmission was successful, then we have implicitly received an
		//ACK for it. So, we should update the last RX timestamp
		(station->rx.last_timestamp = get_usec_timestamp());
	}
}

void wlan_display_mallinfo(){
	struct mallinfo mi;
	mi = mallinfo();

	xil_printf("\n");
	xil_printf("--- Malloc Info ---\n");
	xil_printf("Summary:\n");
	xil_printf("   num_malloc:              %d\n", num_malloc);
	xil_printf("   num_realloc:             %d\n", num_realloc);
	xil_printf("   num_free:                %d\n", num_free);
	xil_printf("   num_malloc-num_free:     %d\n", (int)num_malloc - (int)num_free);
	xil_printf("   System:                  %d bytes\n", mi.arena);
	xil_printf("   Total Allocated Space:   %d bytes\n", mi.uordblks);
	xil_printf("   Total Free Space:        %d bytes\n", mi.fordblks);
	xil_printf("Details:\n");
	xil_printf("   arena:                   %d\n", mi.arena);
	xil_printf("   ordblks:                 %d\n", mi.ordblks);
	xil_printf("   smblks:                  %d\n", mi.smblks);
	xil_printf("   hblks:                   %d\n", mi.hblks);
	xil_printf("   hblkhd:                  %d\n", mi.hblkhd);
	xil_printf("   usmblks:                 %d\n", mi.usmblks);
	xil_printf("   fsmblks:                 %d\n", mi.fsmblks);
	xil_printf("   uordblks:                %d\n", mi.uordblks);
	xil_printf("   fordblks:                %d\n", mi.fordblks);
	xil_printf("   keepcost:                %d\n", mi.keepcost);
}

void* wlan_malloc(u32 size){
	//This is just a simple wrapper around malloc to aid in debugging memory leak issues
	void* return_value;
	return_value = malloc(size);

	//xil_printf("MALLOC 0x%08x %d bytes\n", return_value, size);

	if(return_value == NULL){
		xil_printf("malloc error. Try increasing heap size in linker script.\n");
		wlan_display_mallinfo();
	} else {
		num_malloc++;
	}
	return return_value;
}

void* wlan_calloc(u32 size){
	//This is just a simple wrapper around calloc to aid in debugging memory leak issues
	void* return_value;
	return_value = wlan_malloc(size);

	if(return_value == NULL){
	} else {
		memset(return_value, 0 , size);
	}
	return return_value;
}


void* wlan_realloc(void* addr, u32 size){
	//This is just a simple wrapper around realloc to aid in debugging memory leak issues
	void* return_value;
	return_value = realloc(addr, size);

	//xil_printf("REALLOC 0x%08x %d bytes\n", return_value, size);

	if(return_value == NULL){
		xil_printf("realloc error. Try increasing heap size in linker script.\n");
		wlan_display_mallinfo();
	} else {
		num_realloc++;
	}

	return return_value;
}

void wlan_free(void* addr){
	//This is just a simple wrapper around free to aid in debugging memory leak issues

	//xil_printf("FREE 0x%08x\n", addr);

	free(addr);
	num_free++;
}

u8 wlan_mac_util_get_tx_rate(station_info* station){

	u8 return_value;

	if(((station->tx.rate) >= WLAN_MAC_RATE_6M) && ((station->tx.rate) <= WLAN_MAC_RATE_54M)){
		return_value = station->tx.rate;
	} else {
		xil_printf("Station 0x%08x has invalid rate selection (%d), defaulting to WLAN_MAC_RATE_6M\n",station,station->tx.rate);
		return_value = WLAN_MAC_RATE_6M;
	}

	return return_value;
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

	if( ( tx_mpdu->state == TX_MPDU_STATE_TX_PENDING ) && ( is_cpu_low_ready() ) ){
		return 1;
	} else {
		return 0;
	}
}

int wlan_mac_cdma_start_transfer(void* dest, void* src, u32 size){
	//This is a wrapper function around the central DMA simple transfer call. It's arguments
	//are intended to be similar to memcpy. Note: This function does not block on the transfer.

	int return_value;

	while(XAxiCdma_IsBusy(&cdma_inst)) {}
	return_value = XAxiCdma_SimpleTransfer(&cdma_inst, (u32)src, (u32)dest, size, NULL, NULL);

	return return_value;
}

void wlan_mac_cdma_finish_transfer(){
	while(XAxiCdma_IsBusy(&cdma_inst)) {}
	return;
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
			set_cpu_low_not_ready();

			ipc_mailbox_write_msg(&ipc_msg_to_low);
		}
	} else {
		warp_printf(PL_ERROR, "Bad state in mpdu_transmit. Attempting to transmit but tx_buffer %d is not empty\n",tx_pkt_buf);
	}

	return;
}



u8* get_eeprom_mac_addr(){
	return (u8 *) &(hw_info.hw_addr_wlan);
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






/*****************************************************************************/
/**
* Setup TX packet
*
* Configure a TX packet to be enqueued
*
* @param    mac_channel  - Value of MAC channel
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void setup_tx_header( mac_header_80211_common * header, u8 * addr_1, u8 * addr_3 ) {

	// Set up Addresses in common header
	header->address_1 = addr_1;
    header->address_3 = addr_3;
}


void setup_tx_queue( packet_bd * tx_queue, void * metadata, u32 tx_length, u8 retry, u8 flags  ) {

    // Set up metadata
	tx_queue->metadata_ptr     = metadata;

	// Set up frame info data
    ((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.length    = tx_length;
	((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.retry_max = retry;
	((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.flags     = flags;
}

int str2num(char* str){
	//For now this only works with non-negative values
	int return_value = 0;
	u8 decade_index;
	int multiplier;
	u8 string_length = strlen(str);
	u32 i;

	for(decade_index = 0; decade_index < string_length; decade_index++){
		multiplier = 1;
		for(i = 0; i < (string_length - 1 - decade_index) ; i++){
			multiplier = multiplier*10;
		}
		return_value += multiplier*(u8)(str[decade_index] - 48);
	}

	return return_value;
}

void usleep(u64 delay){
	u64 timestamp = get_usec_timestamp();
	while(get_usec_timestamp() < (timestamp+delay)){}
	return;
}






