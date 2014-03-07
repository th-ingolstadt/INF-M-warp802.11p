/** @file wlan_mac_high.h
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
 */

#ifndef WLAN_MAC_UTIL_H_
#define WLAN_MAC_UTIL_H_

#include "wlan_mac_queue.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_ipc_util.h"
#include "wlan_mac_misc_util.h"
#include "wlan_mac_dl_list.h"

#define INIT_DATA_BASEADDR XPAR_MB_HIGH_INIT_BRAM_CTRL_S_AXI_BASEADDR   ///< Base address of memory used for storing boot data
#define INIT_DATA_DOTDATA_IDENTIFIER	0x1234ABCD                      ///< "Magic number" used as an identifier in boot data memory
#define INIT_DATA_DOTDATA_START (INIT_DATA_BASEADDR+0x200)              ///< Offset into memory for boot data
#define INIT_DATA_DOTDATA_SIZE	(4*(XPAR_MB_HIGH_INIT_BRAM_CTRL_S_AXI_HIGHADDR - INIT_DATA_DOTDATA_START))  ///< Amount of space available in boot data memory

#define ENCAP_MODE_AP	0   ///< Used as a flag for AP encapsulation and de-encapsulation
#define ENCAP_MODE_STA	1   ///< Used as a flag for STA encapsulation and de-encapsulation

#define TX_BUFFER_NUM        2  ///< Number of PHY transmit buffers to use. This should remain 2 (ping/pong buffering).

#define INTC_DEVICE_ID				XPAR_INTC_0_DEVICE_ID					///< XParameters rename of interrupt controller device ID
#define ETH_A_MAC_DEVICE_ID			XPAR_ETH_A_MAC_DEVICE_ID                ///< XParameters rename for ETH A
#define TIMESTAMP_GPIO_DEVICE_ID 	XPAR_MB_HIGH_TIMESTAMP_GPIO_DEVICE_ID   ///< XParameters rename for GPIO used as usec timestamp
#define UARTLITE_DEVICE_ID     		XPAR_UARTLITE_0_DEVICE_ID               ///< XParameters rename for UART

#define TIMESTAMP_GPIO_LSB_CHAN 1   ///< GPIO channel used for lower 32 bits of 64-bit timestamp
#define TIMESTAMP_GPIO_MSB_CHAN 2   ///< GPIO channel used for upper 32 bits of 64-bit timestamp

#define DDR3_BASEADDR XPAR_DDR3_SODIMM_S_AXI_BASEADDR               ///< XParameters rename for base address of DDR3 SODIMM
#define DDR3_SIZE 1073741824                                        ///< Available space in DDR3 SODIMM

#define DDR3_USER_DATA_MEM_BASEADDR		0xC05DC000													///< Space set aside in DDR3 for user extension
#define DDR3_USER_DATA_MEM_SIZE			0x01000000													///< 16MB for scratch work
#define DDR3_USER_DATA_MEM_HIGHADDR		(DDR3_USER_DATA_MEM_BASEADDR+DDR3_USER_DATA_MEM_SIZE-1)		///< Ending address for user extension memory

#define USERIO_BASEADDR XPAR_W3_USERIO_BASEADDR                     ///< XParameters rename of base address of User I/O

#define GPIO_DEVICE_ID			XPAR_MB_HIGH_SW_GPIO_DEVICE_ID      ///< XParameters rename of device ID of GPIO
#define INTC_GPIO_INTERRUPT_ID	XPAR_INTC_0_GPIO_0_VEC_ID           ///< XParameters rename of GPIO interrupt ID
#define UARTLITE_INT_IRQ_ID     XPAR_INTC_0_UARTLITE_0_VEC_ID       ///< XParameters rename of UART interrupt ID
#define TMRCTR_INTERRUPT_ID		XPAR_INTC_0_TMRCTR_0_VEC_ID         ///< XParameters rename of timer interrupt ID

#define GPIO_OUTPUT_CHANNEL     1	///< Channel used as output for GPIO
#define GPIO_INPUT_CHANNEL      2	///< Channel used as input for GPIO
#define GPIO_INPUT_INTERRUPT XGPIO_IR_CH2_MASK  ///< Mask for enabling interrupts on GPIO input

#define GPIO_MASK_DRAM_INIT_DONE 0x00000100		///< Mask for GPIO -- DRAM initialization bit
#define GPIO_MASK_PB_U			 0x00000040		///< Mask for GPIO -- "Up" Pushbutton
#define GPIO_MASK_PB_M			 0x00000020		///< Mask for GPIO -- "Middle" Pushbutton
#define GPIO_MASK_PB_D			 0x00000010		///< Mask for GPIO -- "Down" Pushbutton

#define UART_BUFFER_SIZE 1						///< UART is configured to read 1 byte at a time
#define IPC_BUFFER_SIZE      20 				///< Size of buffer for incoming IPC messages from lower to upper level CPUs

#define NUM_VALID_RATES 12 						///< Number of supported rates

#define PKT_TYPE_DATA_ENCAP_ETH	1				///< Encapsulated Ethernet Type
#define PKT_TYPE_DATA_ENCAP_LTG	2				///< Encapsulated LTG Type
#define PKT_TYPE_MGMT			3				///< Management Type
#define PKT_TYPE_CONTROL		4				///< Control Type

#define ADD_ASSOCIATION_ANY_AID 0				///< Special argument to function that adds associations

#define ALLOW_PROMISC_STATISTICS				///< Allow promiscuous statistics kept for unassociated stations.
#define MAX_NUM_PROMISC_STATS 50				///< Maximum number of promiscuous statistics

/**
 * @brief Reception Information Structure
 *
 * This structure contains information about the previous reception. It is used in high
 * level MACs to de-duplicate incoming receptions.
 */
typedef struct{
	u64     last_timestamp; ///< Timestamp of the last frame reception
	u16     last_seq;       ///< Sequence number of the last MPDU reception
	char    last_power;     ///< Power of last frame reception (in dBm)
	u8      last_rate;      ///< Rate of last MPDU reception
} rx_info;

/**
 * @brief Statistics Structure
 *
 * This struct contains statistics about the communications link. Typically, this struct is
 * pointed to by a larger station_info struct to catalog statistics about a particular
 * station in the network. Additionally, statistics can be decoupled from station_info
 * structs entirely to enable promiscuous statistics about unassociated devices seen in
 * the network.
 */
typedef struct{
	dl_entry entry;				 ///< Doubly-linked list entry
	u64     last_timestamp; 	 ///< Timestamp of the last frame reception
	u8      addr[6];			 ///< HW Address
	u8      is_associated;		 ///< Is this device associated with me?
	u8      reserved;
	u32     num_tx_total;		 ///< Total number of transmissions to this device
	u32     num_tx_success; 	 ///< Total number of successful transmissions to this device
	u32     num_retry;			 ///< Total number of retransmissions to this device
	u32     mgmt_num_rx_success; ///< Total number of successful receptions from this device (Management)
	u32     mgmt_num_rx_bytes;	 ///< Total number of received bytes from this device (Management)
	u32     data_num_rx_success; ///< Total number of successful receptions from this device (Data)
	u32     data_num_rx_bytes;	 ///< Total number of received bytes from this device (Data)
} statistics_txrx;

/**
 * @brief Traverse to next statistics entry in doubly-linked list
 *
 * This function takes a pointer to a statistics structure as an argument
 * and returns a pointer to the next statistics structure in the doubly-linked
 * list. If the argument does not belong to a doubly-linked list, it returns NULL.
 *
 * @param x Pointer to statistics structure
 * @return	Pointer to next statistics structure in doubly-linked list
 * 
 * @see dl_entry_next()
 */
#define statistics_next(x) ( (statistics_txrx*)dl_entry_next(&(x->entry)) )

/**
 * @brief Traverse to previous statistics entry in doubly-linked list
 *
 * This function takes a pointer to a statistics structure as an argument
 * and returns a pointer to the previous statistics structure in the doubly-linked
 * list. If the argument does not belong to a doubly-linked list, it returns NULL.
 *
 * @param x Pointer to statistics structure
 * @return	Pointer to previous statistics structure in doubly-linked list
 *
 * @see dl_entry_prev()
 */
#define statistics_prev(x) ( (statistics_txrx*)dl_entry_prev(&(x->entry)) )

#define STATION_INFO_FLAG_DISABLE_ASSOC_CHECK 0x0001 ///< Mask for flag in station_info -- disable association check
#define STATION_INFO_FLAG_NEVER_REMOVE 0x0002 ///< Mask for flag in station_info -- never remove

#define STATION_INFO_HOSTNAME_MAXLEN 15

/**
 * @brief Station Information Structure
 *
 * This struct contains information about associated stations (or, in the
 * case of a station, information about the associated access point).
 */
typedef struct{
	dl_entry    entry;										///< Doubly-linked list entry
	u8          addr[6];									///< HW Address
	char		hostname[STATION_INFO_HOSTNAME_MAXLEN+1]; 	///< Hostname from DHCP requests
	u16         AID;										///< Association ID
	u32			flags;										///< 1-bit flags
	rx_info     rx;											///< Reception Information Structure
	tx_params   tx;											///< Transmission Parameters Structure
	statistics_txrx* stats;										///< Statistics Information Structure
															///< @note This is a pointer to the statistics structure
                            								///< because statistics can survive outside of the context
                            								///< of associated station_info structs.
} station_info;

/**
 * @brief Traverse to next station_info entry in doubly-linked list
 *
 * This function takes a pointer to a station_info structure as an argument
 * and returns a pointer to the next station_info structure in the doubly-linked
 * list. If the argument does not belong to a doubly-linked list, it returns NULL.
 *
 * @param x Pointer to station_info structure
 * @return	Pointer to next station_info structure in doubly-linked list
 *
 * @see dl_entry_next()
 */
#define station_info_next(x) ( (station_info*)dl_entry_next(&(x->entry)) )

/**
 * @brief Traverse to previous station_info entry in doubly-linked list
 *
 * This function takes a pointer to a station_info structure as an argument
 * and returns a pointer to the previous station_info structure in the doubly-linked
 * list. If the argument does not belong to a doubly-linked list, it returns NULL.
 *
 * @param x Pointer to station_info structure
 * @return	Pointer to prev station_info structure in doubly-linked list
 *
 * @see dl_entry_prev()
 */
#define station_info_prev(x) ( (station_info*)dl_entry_prev(&(x->entry)) )

//////////// Initialization Functions ////////////
void wlan_mac_high_heap_init();
void wlan_mac_high_init();
int wlan_mac_high_interrupt_init();


inline int wlan_mac_high_interrupt_start();
inline void wlan_mac_high_interrupt_stop();
void wlan_mac_high_print_hw_info( wlan_mac_hw_info * info );
void wlan_mac_high_uart_rx_handler(void *CallBackRef, unsigned int EventData);
station_info* wlan_mac_high_find_station_info_AID(dl_list* list, u32 aid);
station_info* wlan_mac_high_find_station_info_ADDR(dl_list* list, u8* addr);
statistics_txrx* wlan_mac_high_find_statistics_ADDR(dl_list* list, u8* addr);
void wlan_mac_high_gpio_handler(void *InstancePtr);
void wlan_mac_high_set_pb_u_callback(function_ptr_t callback);
void wlan_mac_high_set_pb_m_callback(function_ptr_t callback);
void wlan_mac_high_set_pb_d_callback(function_ptr_t callback);
void wlan_mac_high_set_uart_rx_callback(function_ptr_t callback);
void wlan_mac_high_set_mpdu_tx_done_callback(function_ptr_t callback);
void wlan_mac_high_set_mpdu_rx_callback(function_ptr_t callback);
void wlan_mac_high_set_mpdu_accept_callback(function_ptr_t callback);
u64  get_usec_timestamp();
void wlan_mac_high_process_tx_done(tx_frame_info* frame,station_info* station);
void wlan_mac_high_display_mallinfo();
void* wlan_mac_high_malloc(u32 size);
void* wlan_mac_high_calloc(u32 size);
void* wlan_mac_high_realloc(void* addr, u32 size);
void wlan_mac_high_free(void* addr);
u8 wlan_mac_high_get_tx_rate(station_info* station);
void wlan_mac_high_write_hex_display(u8 val);
void wlan_mac_high_write_hex_display_dots(u8 dots_on);
int wlan_mac_high_memory_test();
int wlan_mac_high_cdma_start_transfer(void* dest, void* src, u32 size);
void wlan_mac_high_cdma_finish_transfer();
void wlan_mac_high_mpdu_transmit(packet_bd* tx_queue);
wlan_mac_hw_info* wlan_mac_high_get_hw_info();
u8* wlan_mac_high_get_eeprom_mac_addr();
u8 wlan_mac_high_valid_tagged_rate(u8 rate);
void wlan_mac_high_tagged_rate_to_readable_rate(u8 rate, char* str);
void wlan_mac_high_setup_tx_header( mac_header_80211_common * header, u8 * addr_1, u8 * addr_3 );
void wlan_mac_high_setup_tx_frame_info( packet_bd * tx_queue, void * metadata, u32 tx_length, u8 retry, u8 flags  );

void wlan_mac_high_ipc_rx();
void wlan_mac_high_process_ipc_msg(wlan_ipc_msg* msg);
void wlan_mac_high_set_channel( unsigned int mac_channel );
void wlan_mac_high_set_dsss( unsigned int dsss_value );
void wlan_mac_high_set_timestamp( u64 timestamp );
int  wlan_mac_high_is_cpu_low_initialized();
int  wlan_mac_high_is_cpu_low_ready();
inline u8 wlan_mac_high_pkt_type(void* mpdu, u16 length);
inline void wlan_mac_high_set_debug_gpio(u8 val);
inline void wlan_mac_high_clear_debug_gpio(u8 val);
int str2num(char* str);
void usleep(u64 delay);

station_info* wlan_mac_high_add_association(dl_list* assoc_tbl, dl_list* stat_tbl, u8* addr, u16 requested_AID);
int wlan_mac_high_remove_association(dl_list* assoc_tbl, dl_list* stat_tbl, u8* addr);
void wlan_mac_high_print_associations(dl_list* assoc_tbl);
statistics_txrx* wlan_mac_high_add_statistics(dl_list* stat_tbl, station_info* station, u8* addr);
void wlan_mac_high_reset_statistics(dl_list* stat_tbl);
u8 wlan_mac_high_is_valid_association(dl_list* assoc_tbl, station_info* station);
void wlan_mac_high_copy_comparison();

// Common functions that must be implemented by users of the framework
dl_list * get_statistics();
dl_list * get_station_info_list();

#endif /* WLAN_MAC_UTIL_H_ */
