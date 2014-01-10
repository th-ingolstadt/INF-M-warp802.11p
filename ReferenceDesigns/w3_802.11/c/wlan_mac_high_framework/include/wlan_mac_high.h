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
 *  @bug No known bugs.
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

#define USERIO_BASEADDR XPAR_W3_USERIO_BASEADDR                     ///< XParameters rename of base address of User I/O

#define GPIO_DEVICE_ID			XPAR_MB_HIGH_SW_GPIO_DEVICE_ID      ///< XParameters rename of device ID of GPIO
#define INTC_GPIO_INTERRUPT_ID	XPAR_INTC_0_GPIO_0_VEC_ID           ///< XParameters rename of GPIO interrupt ID
#define UARTLITE_INT_IRQ_ID     XPAR_INTC_0_UARTLITE_0_VEC_ID       ///< XParameters rename of UART interrupt ID
#define TMRCTR_INTERRUPT_ID		XPAR_INTC_0_TMRCTR_0_VEC_ID         ///< XParameters rename of timer interrupt ID

#define GPIO_MASK_DRAM_INIT_DONE 0x00000100		///< Mask for GPIO -- DRAM initialization bit
#define GPIO_MASK_PB_U			 0x00000040		///< Mask for GPIO -- "Up" Pushbutton
#define GPIO_MASK_PB_M			 0x00000020		///< Mask for GPIO -- "Middle" Pushbutton
#define GPIO_MASK_PB_D			 0x00000010		///< Mask for GPIO -- "Down" Pushbutton

#define UART_BUFFER_SIZE 1						///< UART is configured to read 1 byte at a time

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
	u8      last_rate;      ///< Rate of last MPDU reception (TODO: Needs to be filled in)
} rx_info;

/**
 * @brief Transmit Parameters Structure
 *
 * This struct contains transmission parameters. Typically, this struct is included
 * in a larger station_info struct to describe transmission parameters to a particular
 * station in the network.
 */
typedef struct{
	u8      rate;			///< Rate of transmission
	u8      antenna_mode;	///< Antenna mode (Placeholder)
	u8	    max_retry;		///< Maximum number of retransmissions
	u8      reserved;
} tx_params;

/**
 * @brief Statistics Structure
 *
 * This struct contains statistics about the communications link. Typically, this struct is
 * pointed to by a larger station_info struct to catalog statistics about a particular
 * station in the network
 */
typedef struct{
	dl_node node;			///< Doubly-linked list entry
	u64     last_timestamp; ///< Timestamp of the last frame reception
	u8      addr[6];		///< HW Address
	u8      is_associated;	///< Is this device associated with me?
	u8      reserved;
	u32     num_tx_total;	///< Total number of transmissions to this device
	u32     num_tx_success; ///< Total number of successful transmissions to this device
	u32     num_retry;		///< Total number of retransmissions to this device
	u32     num_rx_success; ///< Total number of successful receptions from this device
	u32     num_rx_bytes;	///< Total number of received bytes from this device
} statistics;

//Helper macros for traversing the doubly-linked list

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
 * @see dl_node_next()
 */
#define statistics_next(x) ( (statistics*)dl_node_next(&(x->node)) )
#define statistics_prev(x) ( (statistics*)dl_node_prev(&(x->node)) )

typedef struct{
	dl_node     node;
	u8          addr[6];
	u16         AID;
	u32			flags; //TODO - Disable assoc check
	rx_info     rx;
	tx_params   tx;
	statistics* stats;
} station_info;

//Helper macros for traversing the doubly-linked list
#define station_info_next(x) ( (station_info*)dl_node_next(&(x->node)) )
#define station_info_prev(x) ( (station_info*)dl_node_prev(&(x->node)) )

typedef struct{
	u8 address_destination[6];
	u8 address_source[6];
	u16 type;
} ethernet_header;

typedef struct{
	u8 ver_ihl;
	u8 tos;
	u16 length;
	u16 id;
	u16 flags_fragOffset;
	u8 ttl;
	u8 prot;
	u16 checksum;
	u8 ip_src[4];
	u8 ip_dest[4];
} ipv4_header;

#define IPV4_PROT_UDP 0x11

typedef struct{
	u16 htype;
	u16 ptype;
	u8 hlen;
	u8 plen;
	u16 oper;
	u8 eth_src[6];
	u8 ip_src[4];
	u8 eth_dst[6];
	u8 ip_dst[4];
} arp_packet;

typedef struct{
	u16 src_port;
	u16 dest_port;
	u16 length;
	u16 checksum;
} udp_header;

typedef struct{
	u8 op;
	u8 htype;
	u8 hlen;
	u8 hops;
	u32 xid;
	u16 secs;
	u16 flags;
	u8 ciaddr[4];
	u8 yiaddr[4];
	u8 siaddr[4];
	u8 giaddr[4];
	u8 chaddr[6];
	u8 chaddr_padding[10];
	u8 padding[192];
	u32 magic_cookie;
} dhcp_packet;

#define DHCP_BOOTP_FLAGS_BROADCAST	0x8000
#define DHCP_MAGIC_COOKIE 0x63825363
#define DHCP_OPTION_TAG_TYPE		53
	#define DHCP_OPTION_TYPE_DISCOVER 1
	#define DHCP_OPTION_TYPE_OFFER 2
	#define DHCP_OPTION_TYPE_REQUEST  3
	#define DHCP_OPTION_TYPE_ACK	  5
#define DHCP_OPTION_TAG_IDENTIFIER	61
#define DHCP_OPTION_END				255

#define UDP_SRC_PORT_BOOTPC 68
#define UDP_SRC_PORT_BOOTPS 67



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


// IPC defines
#define IPC_BUFFER_SIZE      20


void wlan_mac_high_heap_init();
void wlan_mac_high_init();
int wlan_mac_high_interrupt_init();
inline int wlan_mac_high_interrupt_start();
inline void wlan_mac_high_interrupt_stop();
wlan_mac_hw_info* wlan_mac_high_get_hw_info();
void wlan_mac_high_print_hw_info( wlan_mac_hw_info * info );
void wlan_mac_high_uart_rx_handler(void *CallBackRef, unsigned int EventData);
station_info* wlan_mac_high_find_station_info_AID(dl_list* list, u32 aid);
station_info* wlan_mac_high_find_station_info_ADDR(dl_list* list, u8* addr);
statistics* wlan_mac_high_find_statistics_ADDR(dl_list* list, u8* addr);
void wlan_mac_high_gpio_handler(void *InstancePtr);
void wlan_mac_high_set_pb_u_callback(void(*callback)());
void wlan_mac_high_set_pb_m_callback(void(*callback)());
void wlan_mac_high_set_pb_d_callback(void(*callback)());
void wlan_mac_high_set_uart_rx_callback(void(*callback)());
void wlan_mac_high_set_mpdu_tx_done_callback(void(*callback)());
void wlan_mac_high_set_fcs_bad_rx_callback(void(*callback)());
void wlan_mac_high_set_mpdu_rx_callback(void(*callback)());
void wlan_mac_high_set_mpdu_accept_callback(void(*callback)());
void wlan_mac_high_set_check_queue_callback(void(*callback)());
void wlan_mac_high_gpio_timestamp_init();
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
int wlan_mac_high_is_tx_buffer_empty();
int wlan_mac_high_cdma_start_transfer(void* dest, void* src, u32 size);
void wlan_mac_high_cdma_finish_transfer();
void wlan_mac_high_mpdu_transmit(packet_bd* tx_queue);
u8* wlan_mac_high_get_eeprom_mac_addr();
u8 wlan_mac_high_valid_tagged_rate(u8 rate);
void wlan_mac_high_tagged_rate_to_readable_rate(u8 rate, char* str);
void wlan_mac_high_setup_tx_header( mac_header_80211_common * header, u8 * addr_1, u8 * addr_3 );
void wlan_mac_high_setup_tx_queue( packet_bd * tx_queue, void * metadata, u32 tx_length, u8 retry, u8 gain_target, u8 flags  );


int str2num(char* str);
void usleep(u64 delay);












void wlan_mac_high_ipc_rx();

void wlan_mac_high_process_ipc_msg(wlan_ipc_msg* msg);

void wlan_mac_high_set_channel( unsigned int mac_channel );
void wlan_mac_high_set_dsss( unsigned int dsss_value );
void wlan_mac_high_set_backoff_slot_value( u32 num_slots );


int  wlan_mac_high_is_cpu_low_initialized();
int  wlan_mac_high_is_cpu_low_ready();





#endif /* WLAN_MAC_UTIL_H_ */
