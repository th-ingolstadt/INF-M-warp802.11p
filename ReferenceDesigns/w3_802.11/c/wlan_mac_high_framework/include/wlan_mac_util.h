////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_util.h
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

#ifndef WLAN_MAC_UTIL_H_
#define WLAN_MAC_UTIL_H_

#include "wlan_mac_queue.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_ipc_util.h"
#include "wlan_mac_misc_util.h"

#ifdef XPAR_MB_HIGH_INIT_DATA_CTRL_DEVICE_ID
//Init Data Definitions
#define INIT_DATA_BASEADDR XPAR_MB_HIGH_INIT_DATA_CTRL_S_AXI_BASEADDR
#define INIT_DATA_DOTDATA_IDENTIFIER	0x1234ABCD
#define INIT_DATA_DOTDATA_START (INIT_DATA_BASEADDR+0x200)
#define INIT_DATA_DOTDATA_SIZE	(4*(XPAR_MB_HIGH_INIT_DATA_CTRL_S_AXI_HIGHADDR - INIT_DATA_DOTDATA_START))
#endif

//Encapsulation modes
#define ENCAP_MODE_AP	0
#define ENCAP_MODE_STA	1

//Scheduler
#define SCHEDULER_NUM_EVENTS 6
#define NUM_SCHEDULERS 2
#define SCHEDULE_FINE	0
#define SCHEDULE_COARSE 1


// 802.11 Transmit interface defines
#define TX_BUFFER_NUM        2



#define ETH_A_MAC_DEVICE_ID			XPAR_ETH_A_MAC_DEVICE_ID
#define ETH_A_FIFO_DEVICE_ID		XPAR_ETH_A_FIFO_DEVICE_ID
#define TIMESTAMP_GPIO_DEVICE_ID 	XPAR_MB_HIGH_TIMESTAMP_GPIO_DEVICE_ID
#define UARTLITE_DEVICE_ID     		XPAR_UARTLITE_0_DEVICE_ID
#define TMRCTR_DEVICE_ID			XPAR_TMRCTR_0_DEVICE_ID

#define TIMESTAMP_GPIO_LSB_CHAN 1
#define TIMESTAMP_GPIO_MSB_CHAN 2

#define DDR3_BASEADDR XPAR_DDR3_SODIMM_S_AXI_BASEADDR

#define USERIO_BASEADDR XPAR_W3_USERIO_BASEADDR

#define GPIO_DEVICE_ID			XPAR_MB_HIGH_SW_GPIO_DEVICE_ID
#define INTC_GPIO_INTERRUPT_ID	XPAR_INTC_0_GPIO_0_VEC_ID
#define UARTLITE_INT_IRQ_ID     XPAR_INTC_0_UARTLITE_0_VEC_ID
#define TMRCTR_INTERRUPT_ID		XPAR_INTC_0_TMRCTR_0_VEC_ID

#define GPIO_OUTPUT_CHANNEL 	1
#define GPIO_INPUT_CHANNEL 		2
#define GPIO_INPUT_INTERRUPT XGPIO_IR_CH2_MASK  /* Channel 1 Interrupt Mask */

#define INTC_DEVICE_ID	XPAR_INTC_0_DEVICE_ID

#define GPIO_MASK_DRAM_INIT_DONE 0x00000100
#define GPIO_MASK_PB_U			 0x00000040
#define GPIO_MASK_PB_M			 0x00000020
#define GPIO_MASK_PB_D			 0x00000010

#define UART_BUFFER_SIZE 1

#define TIMER_FREQ          XPAR_TMRCTR_0_CLOCK_FREQ_HZ
#define TIMER_CNTR_FAST	 0
#define TIMER_CNTR_SLOW	 1

#define	FAST_TIMER_DUR_US 100
#define	SLOW_TIMER_DUR_US 100000


typedef struct{
	u16 AID;
	u16 seq;
	u8 addr[6];
	u8 tx_rate;
	char last_rx_power;
	u64 rx_timestamp;
	u32 num_tx_total;
	u32 num_tx_success;
	u32 num_rx_success;
	u32 num_rx_bytes;
} station_info;

typedef struct{
	u8 address_destination[6];
	u8 address_source[6];
	u16 type;
} ethernet_header;


#define ETH_TYPE_ARP 	0x0608
#define ETH_TYPE_IP 	0x0008

typedef struct{
	u8 dsap;
	u8 ssap;
	u8 control_field;
	u8 org_code[3];
	u16 type;
} llc_header;

#define LLC_SNAP 						0xAA
#define LLC_CNTRL_UNNUMBERED			0x03
#define LLC_TYPE_ARP					0x0608
#define LLC_TYPE_IP						0x0008
#define LLC_TYPE_CUSTOM					0x9090

void wlan_mac_util_init_data();
void wlan_mac_util_init();
void gpio_timestamp_initialize();
inline u64 get_usec_timestamp();
void wlan_mac_util_set_eth_rx_callback(void(*callback)());
void wlan_mac_util_set_mpdu_tx_done_callback(void(*callback)());
void wlan_mac_util_set_mpdu_rx_callback(void(*callback)());
void wlan_mac_util_set_pb_u_callback(void(*callback)());
void wlan_mac_util_set_pb_m_callback(void(*callback)());
void wlan_mac_util_set_pb_d_callback(void(*callback)());
void wlan_mac_util_set_uart_rx_callback(void(*callback)());
void wlan_mac_util_set_check_queue_callback(void(*callback)());
void wlan_mac_util_set_eth_encap_mode(u8 mode);
void wlan_mac_schedule_event(u8 scheduler_sel, u32 delay, void(*callback)());
inline void poll_schedule();
inline int wlan_mac_poll_tx_queue(u16 queue_sel);
void write_hex_display(u8 val);
void write_hex_display_dots(u8 dots_on);
int memory_test();
void write_hex_display_raw(u8 val1,u8 val2);
int interrupt_init();
void GpioIsr(void *InstancePtr);
void SendHandler(void *CallBackRef, unsigned int EventData);
void RecvHandler(void *CallBackRef, unsigned int EventData);
void wlan_mac_util_set_ipc_rx_callback(void(*callback)());
inline int interrupt_start();
inline void interrupt_stop();
void timer_handler(void *CallBackRef, u8 TmrCtrNumber);
u8* get_eeprom_mac_addr();

void wlan_mac_util_process_tx_done(tx_frame_info* frame,station_info* station);
u8 wlan_mac_util_get_tx_rate(station_info* station);

int is_tx_buffer_empty();
void mpdu_transmit(packet_bd* tx_queue);
u8 valid_tagged_rate(u8 rate);
void tagged_rate_to_readable_rate(u8 rate, char* str);

void setup_tx_header( mac_header_80211_common * header, u8 * addr_1, u8 * addr_3 );
void setup_tx_queue( packet_bd * tx_queue, void * metadata, u32 tx_length, u8 retry, u8 flags  );



#endif /* WLAN_MAC_UTIL_H_ */
