#include "xuartlite.h"

#include "wlan_platform_high.h"
#include "wlan_mac_high.h"
#include "w3_high.h"
#include "w3_uart.h"
#include "wlan_mac_common.h"
#include "xparameters.h"

static XUartLite            UartLite;

// UART interface
static u8                   uart_rx_buffer[UART_BUFFER_SIZE];       ///< Buffer for received byte from UART

// Private functions
void _w3_uart_rx_handler(void *CallBackRef, unsigned int EventData);

int w3_uart_init(){
	int Status;

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

void _w3_uart_rx_handler(void *CallBackRef, unsigned int EventData){
	#ifdef _ISR_PERF_MON_EN_
		wlan_mac_set_dbg_hdr_out(ISR_PERF_MON_GPIO_MASK);
	#endif
		XUartLite_Recv(&UartLite, uart_rx_buffer, UART_BUFFER_SIZE);
		wlan_mac_high_uart_rx_callback(uart_rx_buffer[0]);
	#ifdef _ISR_PERF_MON_EN_
		wlan_mac_clear_dbg_hdr_out(ISR_PERF_MON_GPIO_MASK);
	#endif
}
