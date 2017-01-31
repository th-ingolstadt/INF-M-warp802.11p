#ifndef W3_UART_H_
#define W3_UART_H_

#include "xintc.h"
//-----------------------------------------------
// xparameter.h definitions
//
// UART
#define PLATFORM_DEV_ID_UART                 XPAR_UARTLITE_0_DEVICE_ID          ///< XParameters rename for UART
#define PLATFORM_INT_ID_UART                 XPAR_INTC_0_UARTLITE_0_VEC_ID      ///< XParameters rename of UART interrupt ID

#define UART_BUFFER_SIZE                                   1                                  ///< UART is configured to read 1 byte at a time

int w3_wlan_platform_uart_init();
int w3_wlan_platform_uart_setup_interrupt(XIntc* intc);
void w3_wlan_platform_uart_set_rx_callback(function_ptr_t callback);

void _w3_uart_rx_handler(void *CallBackRef, unsigned int EventData);

#endif /* W3_UART_H_ */
