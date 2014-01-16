/** @file wlan_mac_high.c
 *  @brief Top-level WLAN MAC High Framework
 *  
 *  This contains the top-level code for accessing the WLAN MAC High Framework.
 *  
 *  @copyright Copyright 2014, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 *  @bug No known bugs.
 */

/***************************** Include Files *********************************/

//Xilinx Includes
#include "stdlib.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xil_exception.h"
#include "xintc.h"
#include "xuartlite.h"
#include "xaxicdma.h"
#include "malloc.h"

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
#include "wlan_mac_ltg.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_schedule.h"
#include "wlan_exp_common.h"
#include "wlan_exp_node.h"

/*********************** Global Variable Definitions *************************/

extern int __data_start; ///< Start address of the data secion
extern int __data_end;	 ///< End address of the data section
extern int __bss_start;	 ///< Start address of the bss section
extern int __bss_end;	 ///< End address of the bss section
extern int _heap_start;	 ///< Start address of the heap
extern int _HEAP_SIZE;	 ///< Size of the heap


/*************************** Variable Definitions ****************************/

// HW structures
static XGpio       Gpio_timestamp;			///< GPIO instance used for 64-bit usec timestamp
static XGpio       Gpio;					///< General-purpose GPIO instance
XIntc       	   InterruptController;		///< Interrupt Controller instance
XUartLite          UartLite;				///< UART Device instance
XAxiCdma           cdma_inst;				///< Central DMA instance

// UART interface
u8                 uart_rx_buffer[UART_BUFFER_SIZE];	///< Buffer for received byte from UART

// 802.11 Transmit packet buffer
u8                 tx_pkt_buf;				///< @brief Current transmit buffer (ping/pong)
											///< @see TX_BUFFER_NUM
// Callback function pointers
function_ptr_t     pb_u_callback;			///< User callback for "up" pushbutton
function_ptr_t     pb_m_callback;			///< User callback for "middle" pushbutton
function_ptr_t     pb_d_callback;			///< User callback for "down" pushbutton
function_ptr_t     uart_callback;			///< User callback for UART reception
function_ptr_t     mpdu_tx_done_callback;	///< User callback for lower-level message that MPDU transmission is complete
function_ptr_t     mpdu_rx_callback;		///< User callback for lower-level message that MPDU reception is ready for processing
function_ptr_t     fcs_bad_rx_callback;		///< User callback for lower-level message that a bad FCS event has occured
function_ptr_t     mpdu_tx_accept_callback; ///< User callback for lower-level message that MPDU has been accepted for transmission

// Node information
wlan_mac_hw_info   	hw_info;				///< Information about hardware
u8					dram_present;			///< Indication variable for whether DRAM SODIMM is present on this hardware

// Status information
static u32         cpu_low_status;			///< Tracking variable for lower-level CPU status
static u32         cpu_high_status;			///< Tracking variable for upper-level CPU status

// Debug GPIO State
static u8		   debug_gpio_state;			///< Current state of debug GPIO pins

// WARPNet information
#ifdef USE_WARPNET_WLAN_EXP
u8                 warpnet_initialized;		///< Indication variable for whether WARPnet has been initialized
#endif

// IPC variables
wlan_ipc_msg       ipc_msg_from_low;							///< IPC message from lower-level
u32                ipc_msg_from_low_payload[IPC_BUFFER_SIZE];	///< Buffer space for IPC message from lower-level

// Memory Allocation Debugging
static u32			num_malloc;				///< Tracking variable for number of times malloc has been called
static u32			num_free;				///< Tracking variable for number of times free has been called
static u32			num_realloc;			///< Tracking variable for number of times realloc has been called

/******************************** Functions **********************************/

/**
 * @brief Initialize Heap and Data Sections
 *
 * Dynamic memory allocation through malloc uses metadata in the data section
 * of the linker. This metadata is not reset upon software reset (i.e., when a
 * user presses the reset button on the hardware). This will cause failures on
 * subsequent boots because this metadata has not be reset back to its original
 * state at the first boot. This function offers a solution to this problem by
 * backing up the data section into dedicated BRAM on the first boot. In
 * subsequent boots, this data is restored from the dedicated BRAM.
 *
 * @param None
 * @return None
 *
 * @note This function should be the first thing called after boot. If it is
 * called after other parts have the code have started dynamic memory access,
 * there will be unpredictable results on software reset.
 */
void wlan_mac_high_heap_init(){
	u32 data_size;
	volatile u32* identifier;

	data_size = 4*(&__data_end - &__data_start);
	identifier = (u32*)INIT_DATA_BASEADDR;

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

/**
 * @brief Initialize MAC High Framework
 *
 * This function initializes the MAC High Framework by setting
 * up the hardware and other subsystems in the framework.
 *
 * @param None
 * @return None
 */
void wlan_mac_high_init(){
	int            Status;
    u32            i;
	u32            queue_len;
	u64            timestamp;
	u32            log_size;
	tx_frame_info* tx_mpdu;
	XAxiCdma_Config *cdma_cfg_ptr;

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
	mpdu_rx_callback        = (function_ptr_t)nullCallback;
	fcs_bad_rx_callback     = (function_ptr_t)nullCallback;
	mpdu_tx_done_callback   = (function_ptr_t)nullCallback;
	mpdu_tx_accept_callback = (function_ptr_t)nullCallback;

	wlan_lib_mailbox_set_rx_callback((function_ptr_t)wlan_mac_high_ipc_rx);

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
	cdma_cfg_ptr = XAxiCdma_LookupConfig(XPAR_AXI_CDMA_0_DEVICE_ID);
	Status = XAxiCdma_CfgInitialize(&cdma_inst, cdma_cfg_ptr, cdma_cfg_ptr->BaseAddress);
	if (Status != XST_SUCCESS) {
		warp_printf(PL_ERROR,"Error initializing CDMA: %d\n", Status);
	}
	XAxiCdma_IntrDisable(&cdma_inst, XAXICDMA_XR_IRQ_ALL_MASK);

	//Initialize the GPIO driver
	Status = XGpio_Initialize(&Gpio, GPIO_DEVICE_ID);
	//Initialize GPIO timestamp
	XGpio_Initialize(&Gpio_timestamp, TIMESTAMP_GPIO_DEVICE_ID);
	XGpio_SetDataDirection(&Gpio_timestamp, TIMESTAMP_GPIO_LSB_CHAN, 0xFFFFFFFF);
	XGpio_SetDataDirection(&Gpio_timestamp, TIMESTAMP_GPIO_MSB_CHAN, 0xFFFFFFFF);

	if (Status != XST_SUCCESS) {
		warp_printf(PL_ERROR, "Error initializing GPIO\n");
		return;
	}
	//Set direction of GPIO channels
	XGpio_SetDataDirection(&Gpio, GPIO_INPUT_CHANNEL, 0xFFFFFFFF);
	XGpio_SetDataDirection(&Gpio, GPIO_OUTPUT_CHANNEL, 0);

	//Clear any existing state in debug GPIO
	wlan_mac_high_clear_debug_gpio(0xFF);

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
			if(wlan_mac_high_memory_test()==0){
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
	warpnet_initialized = 0;
#endif

	wlan_fmc_pkt_init();
	wlan_eth_init();
	wlan_mac_schedule_init();
	wlan_mac_ltg_sched_init();

	//Create IPC message to receive into
	ipc_msg_from_low.payload_ptr = &(ipc_msg_from_low_payload[0]);
}

/**
 * @brief Initialize MAC High Framework's Interrupts
 *
 * This function initializes sets up the interrupt subsystem
 * of the MAC High Framework.
 *
 * @param None
 * @return int
 *      - nonzero if error
 */
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
	// Connect interrupt devices "owned" by wlan_mac_high
	// ***************************************************
	Result = XIntc_Connect(&InterruptController, INTC_GPIO_INTERRUPT_ID, (XInterruptHandler)wlan_mac_high_gpio_handler, &Gpio);
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
		warp_printf(PL_ERROR,"Failed to set up scheduler interrupt\n");
		return -1;
	}

	Result = wlan_lib_mailbox_setup_interrupt(&InterruptController);
	if (Result != XST_SUCCESS) {
		warp_printf(PL_ERROR,"Failed to set up wlan_lib mailbox interrupt\n");
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
	// Enable MicroBlaze exceptions
	// ***************************************************
	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,(Xil_ExceptionHandler)XIntc_InterruptHandler, &InterruptController);
	Xil_ExceptionEnable();

	return 0;
}

/**
 * @brief Start the interrupt controller
 *
 * This function starts the interrupt controller, allowing the executing code
 * to be interrupted.
 *
 * @param None
 * @return int
 *      - nonzero if error
 */
inline int wlan_mac_high_interrupt_start(){
	return XIntc_Start(&InterruptController, XIN_REAL_MODE);
}

/**
 * @brief Stop the interrupt controller
 *
 * This function stops the interrupt controller, effectively pausing interrupts. This can
 * be used alongside wlan_mac_high_interrupt_start() to wrap code that is not interrupt-safe.
 *
 * @param None
 * @return None
 *
 * @note Interrupts that occur while the interrupt controller is off will be executed once it is
 * turned back on. They will not be "lost" as the interrupt inputs to the controller will remain
 * high.
 */
inline void wlan_mac_high_interrupt_stop(){
	XIntc_Stop(&InterruptController);
}

/**
 * @brief Print Hardware Information
 *
 * This function stops the interrupt controller, effectively pausing interrupts. This can
 * be used alongside wlan_mac_high_interrupt_start() to wrap code that is not interrupt-safe.
 *
 * @param wlan_mac_hw_info* info
 *  - pointer to the hardware info struct that should be printed
 * @return None
 *
 */
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

/**
 * @brief UART Receive Interrupt Handler
 *
 * This function is the interrupt handler for UART receptions. It, in turn,
 * will execute a callback that the user has previously registered.
 *
 * @param void* CallBackRef
 *  - Argument supplied by the XUartLite driver. Unused in this application.
 * @param unsigned int EventData
 *  - Argument supplied by the XUartLite driver. Unused in this application.
 * @return None
 *
 * @see wlan_mac_high_set_uart_rx_callback()
 */
void wlan_mac_high_uart_rx_handler(void* CallBackRef, unsigned int EventData){
#ifdef _ISR_PERF_MON_EN_
	wlan_mac_high_set_debug_gpio(ISR_PERF_MON_GPIO_MASK);
#endif
	XUartLite_Recv(&UartLite, uart_rx_buffer, UART_BUFFER_SIZE);
	uart_callback(uart_rx_buffer[0]);
#ifdef _ISR_PERF_MON_EN_
	wlan_mac_high_clear_debug_gpio(ISR_PERF_MON_GPIO_MASK);
#endif
}

/**
 * @brief Find Station Information within a doubly-linked list from an AID
 *
 * Given a doubly-linked list of station_info structures, this function will return
 * the pointer to a particular entry whose association ID field matches the argument
 * to this function.
 *
 * @param dl_list* list
 *  - Doubly-linked list of station_info structures
 * @param u32 aid
 *  - Association ID to search for
 * @return station_info*
 *  - Returns the pointer to the entry in the doubly-linked list that has the
 *    provided AID.
 *  - Returns NULL if no station_info pointer is found that matches the search
 *    criteria
 *
 */
station_info* wlan_mac_high_find_station_info_AID(dl_list* list, u32 aid){
	u32 i;
	station_info* curr_station_info;
	curr_station_info = (station_info*)(list->first);

	for( i = 0; i < list->length; i++){
		if(curr_station_info->AID == aid){
			return curr_station_info;
		} else {
			curr_station_info = station_info_next(curr_station_info);
		}
	}
	return NULL;
}

/**
 * @brief Find Station Information within a doubly-linked list from an hardware address
 *
 * Given a doubly-linked list of station_info structures, this function will return
 * the pointer to a particular entry whose hardware address matches the argument
 * to this function.
 *
 * @param dl_list* list
 *  - Doubly-linked list of station_info structures
 * @param u8* addr
 *  - 6-byte hardware address to search for
 * @return station_info*
 *  - Returns the pointer to the entry in the doubly-linked list that has the
 *    provided hardware address.
 *  - Returns NULL if no station_info pointer is found that matches the search
 *    criteria
 *
 */
station_info* wlan_mac_high_find_station_info_ADDR(dl_list* list, u8* addr){
	u32 i;
	station_info* curr_station_info;
	curr_station_info = (station_info*)(list->first);

	for( i = 0; i < list->length; i++){
		if(wlan_addr_eq(curr_station_info->addr, addr)){
			return curr_station_info;
		} else {
			curr_station_info = station_info_next(curr_station_info);
		}
	}
	return NULL;
}

/**
 * @brief Find Statistics within a doubly-linked list from an hardware address
 *
 * Given a doubly-linked list of statistics structures, this function will return
 * the pointer to a particular entry whose hardware address matches the argument
 * to this function.
 *
 * @param dl_list* list
 *  - Doubly-linked list of statistics structures
 * @param u8* addr
 *  - 6-byte hardware address to search for
 * @return statistics*
 *  - Returns the pointer to the entry in the doubly-linked list that has the
 *    provided hardware address.
 *  - Returns NULL if no statistics pointer is found that matches the search
 *    criteria
 *
 */
statistics* wlan_mac_high_find_statistics_ADDR(dl_list* list, u8* addr){
	u32 i;
	statistics* curr_statistics;
	curr_statistics = (statistics*)(list->first);

	for( i = 0; i < list->length; i++){
		if(wlan_addr_eq(curr_statistics->addr, addr)){
			return curr_statistics;
		} else {
			curr_statistics = statistics_next(curr_statistics);
		}
	}
	return NULL;
}

/**
 * @brief GPIO Interrupt Handler
 *
 * Handles GPIO interrupts that occur from the GPIO core's input
 * channel. Depending on the signal, this function will execute
 * one of several different user-provided callbacks.
 *
 * @param void* InstancePtr
 *  - Pointer to the GPIO instance
 * @return None
 *
 * @see wlan_mac_high_set_pb_u_callback()
 * @see wlan_mac_high_set_pb_m_callback()
 * @see wlan_mac_high_set_pb_d_callback()
 *
 */
void wlan_mac_high_gpio_handler(void *InstancePtr){
	XGpio *GpioPtr;
	u32 gpio_read;

#ifdef _ISR_PERF_MON_EN_
	wlan_mac_high_set_debug_gpio(ISR_PERF_MON_GPIO_MASK);
#endif

	GpioPtr = (XGpio *)InstancePtr;

	XGpio_InterruptDisable(GpioPtr, GPIO_INPUT_INTERRUPT);
	gpio_read = XGpio_DiscreteRead(GpioPtr, GPIO_INPUT_CHANNEL);

	if(gpio_read & GPIO_MASK_PB_U) pb_u_callback();
	if(gpio_read & GPIO_MASK_PB_M) pb_m_callback();
	if(gpio_read & GPIO_MASK_PB_D) pb_d_callback();

	(void)XGpio_InterruptClear(GpioPtr, GPIO_INPUT_INTERRUPT);
	XGpio_InterruptEnable(GpioPtr, GPIO_INPUT_INTERRUPT);

#ifdef _ISR_PERF_MON_EN_
	wlan_mac_high_clear_debug_gpio(ISR_PERF_MON_GPIO_MASK);
#endif
	return;
}

/**
 * @brief Set "Up" Pushbutton Callback
 *
 * Tells the framework which function should be called when
 * the "up" button in the User I/O section of the hardware
 * is pressed.
 *
 * @param function_ptr_t callback
 *  - Pointer to callback function
 * @return None
 *
 */
void wlan_mac_high_set_pb_u_callback(function_ptr_t callback){
	pb_u_callback = callback;
}

/**
 * @brief Set "Middle" Pushbutton Callback
 *
 * Tells the framework which function should be called when
 * the "middle" button in the User I/O section of the hardware
 * is pressed.
 *
 * @param function_ptr_t callback
 *  - Pointer to callback function
 * @return None
 *
 */
void wlan_mac_high_set_pb_m_callback(function_ptr_t callback){
	pb_m_callback = callback;
}

/**
 * @brief Set "Down" Pushbutton Callback
 *
 * Tells the framework which function should be called when
 * the "down" button in the User I/O section of the hardware
 * is pressed.
 *
 * @param function_ptr_t callback
 *  - Pointer to callback function
 * @return None
 *
 */
void wlan_mac_high_set_pb_d_callback(function_ptr_t callback){
	pb_d_callback = callback;
}

/**
 * @brief Set UART Reception Callback
 *
 * Tells the framework which function should be called when
 * a byte is received from UART.
 *
 * @param function_ptr_t callback
 *  - Pointer to callback function
 * @return None
 *
 */
void wlan_mac_high_set_uart_rx_callback(function_ptr_t callback){
	uart_callback = callback;
}

/**
 * @brief Set MPDU Transmission Complete Callback
 *
 * Tells the framework which function should be called when
 * the lower-level CPU confirms that an MPDU has been transmitted.
 *
 * @param function_ptr_t callback
 *  - Pointer to callback function
 * @return None
 *
 * @note This callback is not executed for individual retransmissions.
 * It is instead only executed after a chain of retransmissions is complete
 * either through the reception of an ACK or the number of retransmissions
 * reaching the maximum number of retries specified by the MPDU's
 * tx_frame_info metadata.
 *
 */
void wlan_mac_high_set_mpdu_tx_done_callback(function_ptr_t callback){
	mpdu_tx_done_callback = callback;
}

/**
 * @brief Set Bad FCS Reception Callback
 *
 * Tells the framework which function should be called when
 * the lower-level CPU receives a frame whose frame check
 * sequence is incorrect.
 *
 * @param function_ptr_t callback
 *  - Pointer to callback function
 * @return None
 *
 */
void wlan_mac_high_set_fcs_bad_rx_callback(function_ptr_t callback){
	fcs_bad_rx_callback = callback;
}

/**
 * @brief Set MPDU Reception Callback
 *
 * Tells the framework which function should be called when
 * the lower-level CPU receives a valid MPDU frame.
 *
 * @param function_ptr_t callback
 *  - Pointer to callback function
 * @return None
 *
 */
void wlan_mac_high_set_mpdu_rx_callback(function_ptr_t callback){
	mpdu_rx_callback = callback;
}

/**
 * @brief Set MPDU Accept Callback
 *
 * Tells the framework which function should be called when
 * the lower-level CPU confirms that it has received the MPDU
 * from the upper-level CPU that it should transmit.
 *
 * @param function_ptr_t callback
 *  - Pointer to callback function
 * @return None
 *
 */
void wlan_mac_high_set_mpdu_accept_callback(function_ptr_t callback){
	mpdu_tx_accept_callback = callback;
}

/**
 * @brief Get Microsecond Counter Timestamp
 *
 * The Reference Design includes a 64-bit counter that increments with
 * every microsecond. This function returns this value and is used
 * throughout the framework as a timestamp.
 *
 * @param None
 * @return u64
 *  - Current number of microseconds that have elapsed since the hardware
 *  has booted.
 *
 */
u64 get_usec_timestamp(){
	u32 timestamp_high_u32;
	u32 timestamp_low_u32;
	u64 timestamp_u64;

	timestamp_high_u32 = XGpio_DiscreteRead(&Gpio_timestamp,TIMESTAMP_GPIO_MSB_CHAN);
	timestamp_low_u32  = XGpio_DiscreteRead(&Gpio_timestamp,TIMESTAMP_GPIO_LSB_CHAN);
	timestamp_u64      = (((u64)timestamp_high_u32)<<32) + ((u64)timestamp_low_u32);

	return timestamp_u64;
}

/**
 * @brief Process Transmission "Done" Event
 *
 * When the lower-level MAC finishes transmitting an MPDU, it is reported
 * back to the upper-level MAC via a transmission done message. This function
 * currently only updates statistics with data about the transmissions, but
 * would be a logical place for future extension to automatic rate control.
 *
 * @param tx_frame_info* frame
 * 	- pointer to the transmit frame information struct
 * @param station_info* station
 *  - pointer to the station information struct
 * @return None
 *
 */
void wlan_mac_high_process_tx_done(tx_frame_info* frame,station_info* station){
	//This is a good place to add an extension for automatic rate control

	(station->stats->num_tx_total)++;
	(station->stats->num_retry) += (frame->retry_count);
	if((frame->state_verbose) == TX_MPDU_STATE_VERBOSE_SUCCESS){
		(station->stats->num_tx_success)++;
		//If this transmission was successful, then we have implicitly received an
		//ACK for it. So, we should update the last RX timestamp
		(station->rx.last_timestamp = get_usec_timestamp());
	}
}

/**
 * @brief Display Memory Allocation Information
 *
 * This function is a wrapper around a call to mallinfo(). It prints
 * the information returned by mallinfo() to aid in the debugging of
 * memory leaks and other dynamic memory allocation issues.
 *
 * @param None
 * @return None
 *
 */
void wlan_mac_high_display_mallinfo(){
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

/**
 * @brief Dynamically Allocate Memory
 *
 * This function wraps malloc() and uses its same API.
 *
 * @param u32 size
 *  - Number of bytes that should be allocated
 * @return void*
 *  - Memory address of allocation if the allocation was successful
 *  - NULL if the allocation was unsuccessful
 *
 * @note The purpose of this function is to funnel all memory allocations through one place in
 * code to enable easier debugging of memory leaks when they occur. This function also updates
 * a variable maintained by the framework to track the number of memory allocations and prints
 * this value, along with the other data from wlan_mac_high_display_mallinfo() in the event that
 * malloc() fails to allocate the requested size.
 *
 */
void* wlan_mac_high_malloc(u32 size){
	void* return_value;
	return_value = malloc(size);

	if(return_value == NULL){
		xil_printf("malloc error. Try increasing heap size in linker script.\n");
		wlan_mac_high_display_mallinfo();
	} else {
		num_malloc++;
	}
	return return_value;
}

/**
 * @brief Dynamically Allocate and Initialize Memory
 *
 * This function wraps wlan_mac_high_malloc() and uses its same API. If successfully allocated,
 * this function will explicitly zero-initialize the allocated memory.
 *
 * @param u32 size
 *  - Number of bytes that should be allocated
 * @return void*
 *  - Memory address of allocation if the allocation was successful
 *  - NULL if the allocation was unsuccessful
 *
 * @see wlan_mac_high_malloc()
 *
 */
void* wlan_mac_high_calloc(u32 size){
	//This is just a simple wrapper around calloc to aid in debugging memory leak issues
	void* return_value;
	return_value = wlan_mac_high_malloc(size);

	if(return_value == NULL){
	} else {
		memset(return_value, 0 , size);
	}
	return return_value;
}

/**
 * @brief Dynamically Reallocate Memory
 *
 * This function wraps realloc() and uses its same API.
 *
 * @param void* addr
 *  - Address of dynamically allocated array that should be reallocated
 * @param u32 size
 *  - Number of bytes that should be allocated
 * @return void*
 *  - Memory address of allocation if the allocation was successful
 *  - NULL if the allocation was unsuccessful
 *
 * @note The purpose of this function is to funnel all memory allocations through one place in
 * code to enable easier debugging of memory leaks when they occur. This function also updates
 * a variable maintained by the framework to track the number of memory allocations and prints
 * this value, along with the other data from wlan_mac_high_display_mallinfo() in the event that
 * realloc() fails to allocate the requested size.
 *
 */
void* wlan_mac_high_realloc(void* addr, u32 size){
	void* return_value;
	return_value = realloc(addr, size);

	if(return_value == NULL){
		xil_printf("realloc error. Try increasing heap size in linker script.\n");
		wlan_mac_high_display_mallinfo();
	} else {
		num_realloc++;
	}

	return return_value;
}

/**
 * @brief Free Dynamically Allocated Memory
 *
 * This function wraps free() and uses its same API.
 *
 * @param void* addr
 *  - Address of dynamically allocated array that should be freed
 * @return None
 *
 * @note The purpose of this function is to funnel all memory freeing through one place in
 * code to enable easier debugging of memory leaks when they occur. This function also updates
 * a variable maintained by the framework to track the number of memory frees.
 *
 */
void wlan_mac_high_free(void* addr){
	free(addr);
	num_free++;
}

/**
 * @brief Get Current Transmit Rate for a Particular Station
 *
 * Currently, this function on pulls the transmit rate element from the station_info
 * struct the user provides. This function is another place where processing can be
 * inserted to alter the rate for automatic rate adaptation.
 *
 * @param station_info* station
 *  - Pointer to station information struct
 * @return u8
 *  - Transmit Rate (WLAN_MAC_RATE_6M, WLAN_MAC_RATE_9M, WLAN_MAC_RATE_12M,
 *  WLAN_MAC_RATE_18M, WLAN_MAC_RATE_24M, WLAN_MAC_RATE_36M, WLAN_MAC_RATE_48M
 *  WLAN_MAC_RATE_54M)
 *
 */
u8 wlan_mac_high_get_tx_rate(station_info* station){
	u8 return_value;

	if(((station->tx.rate) >= WLAN_MAC_RATE_6M) && ((station->tx.rate) <= WLAN_MAC_RATE_54M)){
		return_value = station->tx.rate;
	} else {
		xil_printf("Station 0x%08x has invalid rate selection (%d), defaulting to WLAN_MAC_RATE_6M\n",station,station->tx.rate);
		return_value = WLAN_MAC_RATE_6M;
	}

	return return_value;
}

/**
 * @brief Write a Decimal Value to the Hex Display
 *
 * This function will write a decimal value to the board's two-digit hex displays.
 *
 * @param u8 val
 *  - Value to be displayed (between 0 and 99)
 * @return None
 *
 */
void wlan_mac_high_write_hex_display(u8 val){
   userio_write_control(USERIO_BASEADDR, userio_read_control(USERIO_BASEADDR) | (W3_USERIO_HEXDISP_L_MAPMODE | W3_USERIO_HEXDISP_R_MAPMODE));
   userio_write_hexdisp_left(USERIO_BASEADDR, val/10);
   userio_write_hexdisp_right(USERIO_BASEADDR, val%10);
}

/**
 * @brief Write Decimal Points in the Hex Display
 *
 * This can toggle the decimal point in each hex digit on the board.
 *
 * @param u8 dots_on
 *  - 1 to turn on both the left and right hex display dots
 *  - 0 to turn off both the left and right hex display dots
 * @return None
 *
 */
void wlan_mac_high_write_hex_display_dots(u8 dots_on){
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

/**
 * @brief Test DDR3 SODIMM Memory Module
 *
 * This function tests the integrity of the DDR3 SODIMM module attached to the hardware
 * by performing various write and read tests.
 *
 * @param None
 * @return int
 * 	- 0 for memory test pass
 *	- -1 for memory test fail
 */
int wlan_mac_high_memory_test(){
	u8 i,j;

	u8 test_u8;
	u16 test_u16;
	u32 test_u32;
	u64 test_u64;

	void* memory_ptr;

	for(i=0;i<6;i++){
		memory_ptr = (void*)((u8*)DDR3_BASEADDR + (i*100000*1024));
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

/**
 * @brief Start Central DMA Transfer
 *
 * This function wraps the XAxiCdma call for a CDMA memory transfer and mimics the well-known
 * API of memcpy(). This function does not block once the transfer is started.
 *
 * @param void* dest
 *  - Pointer to destination address where bytes should be copied
 * @param void* stc
 *  - Pointer to source address from where bytes should be copied
 * @param u32 size
 *  - Number of bytes that should be copied
 * @return int
 *	- XST_SUCCESS for success of submission
 *	- XST_FAILURE for submission failure
 *	- XST_INVALID_PARAM if:
 *	 Length out of valid range [1:8M]
 *	 Or, address not aligned when DRE is not built in
 *
 *	 @note This function will block until any existing CDMA transfer is complete. It is therefore
 *	 safe to call this function successively as each call will wait on the preceeding call.
 *
 */
int wlan_mac_high_cdma_start_transfer(void* dest, void* src, u32 size){
	//This is a wrapper function around the central DMA simple transfer call. It's arguments
	//are intended to be similar to memcpy. Note: This function does not block on the transfer.
	int return_value;

	wlan_mac_high_cdma_finish_transfer();
	return_value = XAxiCdma_SimpleTransfer(&cdma_inst, (u32)src, (u32)dest, size, NULL, NULL);

	return return_value;
}

/**
 * @brief Finish Central DMA Transfer
 *
 * This function will block until an ongoing CDMA transfer is complete.
 * If there is no CDMA transfer underway when this function is called, it
 * returns immediately.
 *
 * @param None
 * @return None
 *
 */
void wlan_mac_high_cdma_finish_transfer(){
	while(XAxiCdma_IsBusy(&cdma_inst)) {}
	return;
}

/**
 * @brief Transmit MPDU
 *
 * This function passes off an MPDU to the lower-level processor for transmission.
 *
 * @param packet_bd* packet
 *  - Pointer to the packet that should be transmitted
 * @return None
 *
 */
void wlan_mac_high_mpdu_transmit(packet_bd* packet) {
	wlan_ipc_msg ipc_msg_to_low;
	tx_frame_info* tx_mpdu;
	station_info* station;

	tx_mpdu = (tx_frame_info*) TX_PKT_BUF_TO_ADDR(tx_pkt_buf);
	station = (station_info*)(packet->metadata_ptr);

	if(( tx_mpdu->state == TX_MPDU_STATE_TX_PENDING ) && ( wlan_mac_high_is_cpu_low_ready() )){
		//Copy the packet into the transmit packet buffer
		wlan_mac_high_cdma_start_transfer( (void*)TX_PKT_BUF_TO_ADDR(tx_pkt_buf), (void*)(packet->buf_ptr), ((tx_packet_buffer*)(packet->buf_ptr))->frame_info.length + sizeof(tx_frame_info) + PHY_TX_PKT_BUF_PHY_HDR_SIZE);
		wlan_mac_high_cdma_finish_transfer();

		if(station == NULL){
			//Broadcast transmissions have no station information, so we default to a nominal rate
			tx_mpdu->AID = 0;
			tx_mpdu->rate = WLAN_MAC_RATE_6M;
		} else {
			//Request the rate to use for this station
			tx_mpdu->AID = station->AID;
			tx_mpdu->rate = wlan_mac_high_get_tx_rate(station);
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
		warp_printf(PL_ERROR, "Bad state in wlan_mac_high_mpdu_transmit. Attempting to transmit but tx_buffer %d is not empty\n",tx_pkt_buf);
	}

	return;
}

/**
 * @brief Retrieve Hardware Information
 *
 * This function stops the interrupt controller, effectively pausing interrupts. This can
 * be used alongside wlan_mac_high_interrupt_start() to wrap code that is not interrupt-safe.
 *
 * @param None
 * @return wlan_mac_hw_info*
 *  - Pointer to the hardware info struct maintained by the MAC High Framework
 *
 */
wlan_mac_hw_info* wlan_mac_high_get_hw_info(){
	return &hw_info;
}

/**
 * @brief Retrieve Hardware MAC Address from EEPROM
 *
 * This function returns the 6-byte unique hardware MAC address of the board.
 *
 * @param None
 * @return u8*
 *  - Pointer to 6-byte MAC address
 *
 */
u8* wlan_mac_high_get_eeprom_mac_addr(){
	return (u8 *) &(hw_info.hw_addr_wlan);
}

/**
 * @brief Check Validity of Tagged Rate
 *
 * This function checks the validity of a given rate from a tagged field in a management frame.
 *
 * @param u8 rate
 *  - Tagged rate
 * @return u8
 *  - 1 if valid
 *  - 0 if invalid
 *
 *  @note This function checks against the 12 possible valid rates sent in 802.11b/a/g.
 *  The faster 802.11n rates will return as invalid when this function is used.
 *
 */
u8 wlan_mac_high_valid_tagged_rate(u8 rate){
	u32 i;
	u8 valid_rates[NUM_VALID_RATES] = {0x02, 0x04, 0x0b, 0x16, 0x0c, 0x12, 0x18, 0x24, 0x30, 0x48, 0x60, 0x6c};

	for(i = 0; i < NUM_VALID_RATES; i++ ){
		if((rate & ~RATE_BASIC) == valid_rates[i]) return 1;
	}

	return 0;
}

/**
 * @brief Convert Tagged Rate to Human-Readable String (in Mbps)
 *
 * This function takes a tagged rate as an input and fills in a provided
 * string with the rate in Mbps.
 *
 * @param u8 rate
 *  - Tagged rate
 * @param char* str
 *  - Empty string that will be filled in by this function
 * @return u8
 *  - 1 if valid
 *  - 0 if invalid
 *
 *  @note The str argument must have room for 4 bytes at most ("5.5" followed by NULL)
 *
 */
void wlan_mac_high_tagged_rate_to_readable_rate(u8 rate, char* str){

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

/**
 * @brief Set up the 802.11 Header
 *
 * This function
 *
 * @param u8 rate
 *  - Tagged rate
 * @param char* str
 *  - Empty string that will be filled in by this function
 * @return u8
 *  - 1 if valid
 *  - 0 if invalid
 *
 *  @note The str argument must have room for 4 bytes at most ("5.5" followed by NULL)
 *
 */
void wlan_mac_high_setup_tx_header( mac_header_80211_common * header, u8 * addr_1, u8 * addr_3 ) {
	// Set up Addresses in common header
	header->address_1 = addr_1;
    header->address_3 = addr_3;
}


void wlan_mac_high_setup_tx_queue( packet_bd * tx_queue, void * metadata, u32 tx_length, u8 retry, u8 gain_target, u8 flags  ) {

    // Set up metadata
	tx_queue->metadata_ptr     = metadata;

	// Set up frame info data
	((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.timestamp_create = get_usec_timestamp();
    ((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.length    = tx_length;
	((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.retry_max = retry;
	((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.gain_target = gain_target;
	((tx_packet_buffer*)(tx_queue->buf_ptr))->frame_info.flags     = flags;
}

/*****************************************************************************/
/**
* WLAN MAC IPC receive
*
* IPC receive function that will poll the mailbox for as many messages as are
*   available and then call the CPU high IPC processing function on each message
*
* @param    None.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void wlan_mac_high_ipc_rx(){

#ifdef _DEBUG_
	u32 numMsg = 0;
	xil_printf("Mailbox Rx:  ");
#endif

	while( ipc_mailbox_read_msg( &ipc_msg_from_low ) == IPC_MBOX_SUCCESS ) {
		wlan_mac_high_process_ipc_msg(&ipc_msg_from_low);

#ifdef _DEBUG_
		numMsg++;
#endif
	}

#ifdef _DEBUG_
	xil_printf("Processed %d msg in one ISR\n", numMsg);
#endif
}




/*****************************************************************************/
/**
* WLAN MAC IPC processing function for CPU High
*
* Process all IPC messages from CPU low
*
* @param    msg   - IPC message from CPU low
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void wlan_mac_high_process_ipc_msg( wlan_ipc_msg* msg ) {

	rx_frame_info* rx_mpdu;
	tx_frame_info* tx_mpdu;

	u8  rx_pkt_buf;
    u32 temp_1, temp_2;


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

		case IPC_MBOX_RX_BAD_FCS:
			//This message indicates CPU Low has received a frame whose FCS failed

			rx_pkt_buf = msg->arg0;

			//First attempt to lock the indicated Rx pkt buf (CPU Low must unlock it before sending this msg)
			if(lock_pkt_buf_rx(rx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
				warp_printf(PL_ERROR,"Error: unable to lock pkt_buf %d\n",rx_pkt_buf);
			} else {
				rx_mpdu = (rx_frame_info*)RX_PKT_BUF_TO_ADDR(rx_pkt_buf);

				//xil_printf("MB-HIGH: processing buffer %d, mpdu state = %d, length = %d, rate = %d\n",rx_pkt_buf,rx_mpdu->state, rx_mpdu->length,rx_mpdu->rate);
				fcs_bad_rx_callback((void*)(RX_PKT_BUF_TO_ADDR(rx_pkt_buf)), rx_mpdu->rate, rx_mpdu->length);

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

			mpdu_tx_accept_callback(TX_PKT_BUF_TO_ADDR(msg->arg0));

		break;

		case IPC_MBOX_TX_MPDU_DONE:
			//This message indicates CPU Low has finished the Tx process for the previously submitted-accepted frame
			// CPU High should do any necessary post-processing, then recycle the packet buffer

			tx_mpdu = (tx_frame_info*)TX_PKT_BUF_TO_ADDR(msg->arg0);
			mpdu_tx_done_callback(tx_mpdu);
		break;


		case IPC_MBOX_HW_INFO:
			// This message indicates CPU low is passing up node hardware information that only it has access to

			temp_1 = hw_info.type;
			temp_2 = hw_info.wn_exp_eth_device;

			// CPU Low updated the node's HW information
            //   NOTE:  this information is typically stored in the WARP v3 EEPROM, accessible only to CPU Low
			memcpy((void*) &hw_info, (void*) &(ipc_msg_from_low_payload[0]), sizeof( wlan_mac_hw_info ) );

			hw_info.type              = temp_1;
			hw_info.wn_exp_eth_device = temp_2;

#ifdef USE_WARPNET_WLAN_EXP

        	// Initialize WLAN Exp if it is being used
            if ( warpnet_initialized == 0 ) {

                wlan_exp_node_init( hw_info.type, hw_info.serial_number, hw_info.fpga_dna, hw_info.wn_exp_eth_device, hw_info.hw_addr_wn );

                warpnet_initialized = 1;
            }
#endif
		break;


		case IPC_MBOX_CPU_STATUS:
			// This message indicates CPU low's status

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





/*****************************************************************************/
/**
* Set MAC Channel
*
* Send an IPC message to CPU Low to set the MAC Channel
*
* @param    mac_channel  - Value of MAC channel
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void wlan_mac_high_set_channel( unsigned int mac_channel ) {

	wlan_ipc_msg       ipc_msg_to_low;
	u32                ipc_msg_to_low_payload[1];
	ipc_config_rf_ifc* config_rf_ifc;

	// Send message to CPU Low
	ipc_msg_to_low.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_RF_IFC);
	ipc_msg_to_low.num_payload_words = sizeof(ipc_config_rf_ifc)/sizeof(u32);
	ipc_msg_to_low.payload_ptr       = &(ipc_msg_to_low_payload[0]);

	// Initialize the payload
	init_ipc_config(config_rf_ifc, ipc_msg_to_low_payload, ipc_config_rf_ifc);

	config_rf_ifc->channel = mac_channel;

	ipc_mailbox_write_msg(&ipc_msg_to_low);
}



/*****************************************************************************/
/**
* Set DSSS
*
* Send an IPC message to CPU Low to set the DSSS value
*
* @param    dsss_value  - Value of DSSS to send to CPU Low
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void wlan_mac_high_set_dsss( unsigned int dsss_value ) {

	wlan_ipc_msg       ipc_msg_to_low;
	u32                ipc_msg_to_low_payload[1];
	ipc_config_phy_rx* config_phy_rx;

	// Send message to CPU Low
	ipc_msg_to_low.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_PHY_RX);
	ipc_msg_to_low.num_payload_words = sizeof(ipc_config_phy_rx)/sizeof(u32);
	ipc_msg_to_low.payload_ptr       = &(ipc_msg_to_low_payload[0]);

	// Initialize the payload
	init_ipc_config(config_phy_rx, ipc_msg_to_low_payload, ipc_config_phy_rx);

	config_phy_rx->enable_dsss = dsss_value;

	ipc_mailbox_write_msg(&ipc_msg_to_low);
}

void wlan_mac_high_set_backoff_slot_value( u32 num_slots ) {

	wlan_ipc_msg       ipc_msg_to_low;
	u32                ipc_msg_to_low_payload[1];
	ipc_config_mac*    config_mac;

	// Send message to CPU Low
	ipc_msg_to_low.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_MAC);
	ipc_msg_to_low.num_payload_words = sizeof(ipc_config_phy_rx)/sizeof(u32);
	ipc_msg_to_low.payload_ptr       = &(ipc_msg_to_low_payload[0]);

	// Initialize the payload
	config_mac = (ipc_config_mac*) ipc_msg_to_low_payload;
	config_mac->slot_config = num_slots;

	ipc_mailbox_write_msg(&ipc_msg_to_low);
}


void wlan_mac_high_set_time( u64 timestamp ){

	wlan_ipc_msg       ipc_msg_to_low;

	// Send message to CPU Low
	ipc_msg_to_low.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_SET_TIME);
	ipc_msg_to_low.num_payload_words = sizeof(u64)/sizeof(u32);
	ipc_msg_to_low.payload_ptr       = (u32*)(&(timestamp));

	ipc_mailbox_write_msg(&ipc_msg_to_low);

}



/*****************************************************************************/
/**
* Check variables on CPU low's state
*
* @param    None.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/

int wlan_mac_high_is_cpu_low_initialized(){
	wlan_mac_high_ipc_rx();
	return ( (cpu_low_status & CPU_STATUS_INITIALIZED) != 0 );
}

int wlan_mac_high_is_cpu_low_ready(){
	// xil_printf("cpu_high_status = 0x%08x\n",cpu_high_status);
	return ((cpu_high_status & CPU_STATUS_WAIT_FOR_IPC_ACCEPT) == 0);
}


inline u8 wlan_mac_high_pkt_type(void* mpdu, u16 length){

	mac_header_80211* hdr_80211;
	llc_header* llc_hdr;

	hdr_80211 = (mac_header_80211*)((void*)mpdu);

	if((hdr_80211->frame_control_1 & 0xF) == MAC_FRAME_CTRL1_TYPE_MGMT){
		return PKT_TYPE_MGMT;
	} else if((hdr_80211->frame_control_1 & 0xF) == MAC_FRAME_CTRL1_TYPE_CTRL) {
		return PKT_TYPE_CONTROL;
	} else if((hdr_80211->frame_control_1 & 0xF) == MAC_FRAME_CTRL1_TYPE_DATA) {
		llc_hdr = (llc_header*)((u8*)mpdu + sizeof(mac_header_80211));

		if(length < (sizeof(mac_header_80211) + sizeof(llc_header))){
			//This was a DATA packet, but it wasn't long enough to have an LLC header.
			return NULL;
		} else {
			switch(llc_hdr->type){
				case LLC_TYPE_ARP:
				case LLC_TYPE_IP:
					return PKT_TYPE_DATA_ENCAP_ETH;
				break;
				case LLC_TYPE_CUSTOM:
					return PKT_TYPE_DATA_ENCAP_LTG;
				break;
				default:
					return NULL;
				break;
			}
		}
	}
	return NULL;
}

inline void wlan_mac_high_set_debug_gpio(u8 val){
	debug_gpio_state |= (val & 0xF);
	XGpio_DiscreteWrite(&Gpio, GPIO_OUTPUT_CHANNEL, debug_gpio_state);
}

inline void wlan_mac_high_clear_debug_gpio(u8 val){
	debug_gpio_state &= ~(val & 0xF);
	XGpio_DiscreteWrite(&Gpio, GPIO_OUTPUT_CHANNEL, debug_gpio_state);
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














