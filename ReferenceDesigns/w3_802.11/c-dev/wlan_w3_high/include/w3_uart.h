#ifndef W3_UART_H_
#define W3_UART_H_

#include "xintc.h"

#include "wlan_mac_common.h"

//-----------------------------------------------
// xparameter.h definitions
//
// UART
#define PLATFORM_DEV_ID_UART                 XPAR_UARTLITE_0_DEVICE_ID          ///< XParameters rename for UART
#define PLATFORM_INT_ID_UART                 XPAR_INTC_0_UARTLITE_0_VEC_ID      ///< XParameters rename of UART interrupt ID

#define UART_BUFFER_SIZE                                   1                                  ///< UART is configured to read 1 byte at a time

int w3_uart_init();
int w3_uart_setup_interrupt(XIntc* intc);
void w3_uart_set_rx_callback(function_ptr_t callback);

#endif /* W3_UART_H_ */
