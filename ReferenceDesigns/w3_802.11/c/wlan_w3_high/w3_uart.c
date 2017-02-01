#include "xuartlite.h"

#include "wlan_platform_high.h"
#include "w3_high.h"
#include "w3_uart.h"
#include "wlan_mac_common.h"
#include "xparameters.h"

static function_ptr_t		rx_callback;
static XUartLite            UartLite;

// UART interface
static u8                   uart_rx_buffer[UART_BUFFER_SIZE];       ///< Buffer for received byte from UART

// Private functions
void _w3_uart_rx_handler(void *CallBackRef, unsigned int EventData);

int w3_uart_init(){
	int Status;

	rx_callback = wlan_null_callback;

	// Initialize the UART driver
	Status = XUartLite_Initialize(&UartLite, PLATFORM_DEV_ID_UART);
	if (Status != XST_SUCCESS) {
		wlan_printf(PL_ERROR, "ERROR: Could not initialize UART\n");
	}
	return Status;
}
int w3_uart_setup_interrupt(XIntc* intc){
	int Status;
	// UART
	Status = XIntc_Connect(intc, PLATFORM_INT_ID_UART, (XInterruptHandler)XUartLite_InterruptHandler, &UartLite);
	if (Status != XST_SUCCESS) {
		wlan_printf(PL_ERROR, "Failed to set up UART interrupt\n");
		return Status;
	}
	XIntc_Enable(intc, PLATFORM_INT_ID_UART);
	XUartLite_SetRecvHandler(&UartLite, _w3_uart_rx_handler, &UartLite);
	XUartLite_EnableInterrupt(&UartLite);
	return Status;
}
void w3_uart_set_rx_callback(function_ptr_t callback){
	rx_callback = callback;
}


void _w3_uart_rx_handler(void *CallBackRef, unsigned int EventData){
	#ifdef _ISR_PERF_MON_EN_
		wlan_mac_set_dbg_hdr_out(ISR_PERF_MON_GPIO_MASK);
	#endif
		XUartLite_Recv(&UartLite, uart_rx_buffer, UART_BUFFER_SIZE);
		rx_callback(uart_rx_buffer[0]);
	#ifdef _ISR_PERF_MON_EN_
		wlan_mac_clear_dbg_hdr_out(ISR_PERF_MON_GPIO_MASK);
	#endif
}
