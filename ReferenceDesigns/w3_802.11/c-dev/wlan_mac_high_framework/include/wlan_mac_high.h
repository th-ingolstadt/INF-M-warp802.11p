/** @file wlan_mac_high.h
 *  @brief Top-level WLAN MAC High Framework
 *
 *  This contains the top-level code for accessing the WLAN MAC High Framework.
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *                See LICENSE.txt included in the design archive or
 *                at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */


/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_HIGH_H_
#define WLAN_MAC_HIGH_H_

#include "wlan_mac_high_sw_config.h"
#include "xil_types.h"
#include "wlan_common_types.h"
#include "wlan_high_types.h"

// Forward declarations
struct mac_header_80211_common;
struct wlan_ipc_msg_t;
struct beacon_txrx_configure_t;
struct network_info_t;
enum userio_input_mask_t;
struct station_info_t;

/********************************************************************
 * Auxiliary (AUX) BRAM and DRAM (DDR) Memory Maps

 * The 802.11 Reference Design hardware includes a 64 KB BRAM block named
 * AUX BRAM. This block is mapped into the address space of CPU High
 * and provides a low-latency memory block for data storage beyond the
 * DLMB. The AUX BRAM is also accessible by the Ethernet DMAs and CDMA (the
 * DMAs cannot access the MicroBlaze DLMB memory area).
 *
 * The reference code uses the AUX BRAM to store various data structures
 * that provides references to larger blocks in DRAM. These data structures
 * benefit from the low-latency access of the BRAM block.
 *
 * For example, the doubly-linked list of Tx Queue entries is stored in the
 * AUX BRAM. Each list entry points to a dedicated 4KB buffer in DRAM. The C code
 * can manage a queue with quick list operations in BRAM while the queued packets
 * themselves are stored in the slower but *much* larger DRAM.
 *
 * The 64 KB AUX BRAM block is divided as follows:
 *
 * ***************************** AUX BRAM Map *********************************************
 * Description                                            | Size
 * -------------------------------------------------------------------------------------------------
 * Tx Queue list entries (entry.data points to DRAM)      | 40960 B (TX_QUEUE_DL_ENTRY_MEM_SIZE)
 * BSS Info list entries (entry.data points to DRAM)      |  2560 B (BSS_INFO_DL_ENTRY_MEM_SIZE)
 * Station Info list entries (entry.data points to DRAM)  |  6656 B (STATION_INFO_DL_ENTRY_MEM_SIZE)
 * Space for wlan_platform_ethernet use                   | 15360 B (ETH_MEM_SIZE)
 * -------------------------------------------------------------------------------------------------
 *
 * DRAM is mapped into the address space of CPU High.
 * This memory space is used as follows:
 *
 * ******************************* DRAM Map *********************************************************
 * Description                      | Size
 * --------------------------------------------------------------------------------------------------
 * wlan_exp Eth buffers             |    1024 KB (WLAN_EXP_ETH_BUFFERS_SECTION_SIZE)
 * Tx queue buffers	                |    1400 KB (TX_QUEUE_BUFFER_SIZE)
 * BSS Info buffers	                |      27 KB (BSS_INFO_BUFFER_SIZE)
 * Station Info buffers             |      69 KB (STATION_INFO_BUFFER_SIZE)
 * User scratch space               |   10000 KB (USER_SCRATCH_SIZE)
 * Event log                        | 1036056 KB (EVENT_LOG_SIZE)
 * --------------------------------------------------------------------------------------------------
 *
 * Platform must define:
 *  AUX_BRAM_BASEADDR - base address of AUX BRAM
 *  AUX_BRAM_HIGHADDR - high address of AUX BRAM
 *
 *  DRAM_BASEADDR - base address of DRAM
 *  DRAM_HIGHADDR - high address of DRAM
 *
 *  The per-section macros below are derived from these AUX_BRAM and DRAM address macros.
 *
 ************************************************************************************************/



#define CALC_HIGH_ADDR(base, size)         ((base)+((size)-1))

/********************************************************************
 * wlan_exp and IP/UDP library Ethernet buffers
 *
 * The wlan_exp Ethernet handling code uses large buffers for constructing packets
 * for transmission and for processing received packets. The IP/UDP library uses
 * multiple buffers to pipeline Ethernet operations. The library also supports jumbo
 * frames. As a result the wlan_exp and IP/UDP library Ethernet buffers are too large
 * to store in on-chip BRAM. Instead the first 1MB of the DRAM is reserved for use
 * by the wlan_exp Ethernet code.
 *
 * The linker script for CPU High *must* include a dedicated section allocated at the
 * base of the DRAM address range. The macros below assume this section exist and
 * are used to verify the wlan_exp and IP/UDP library code does not overflow the
 * DRAM allocation.
 *
 ********************************************************************/
#define WLAN_EXP_ETH_BUFFERS_SECTION_BASE   (platform_high_dev_info.dram_baseaddr)
#define WLAN_EXP_ETH_BUFFERS_SECTION_SIZE   (1024 * 1024)
#define WLAN_EXP_ETH_BUFFERS_SECTION_HIGH   CALC_HIGH_ADDR(WLAN_EXP_ETH_BUFFERS_SECTION_BASE, WLAN_EXP_ETH_BUFFERS_SECTION_SIZE)


/********************************************************************
 * TX Queue
 *
 * The Tx Queue consists of two pieces:
 *     (1) dl_entry structs that live in the AUX BRAM
 *     (2) Data buffers for the packets themselves than live in DRAM
 *
 * The definitions below reserve memory for these two pieces.  The default
 * value of 40 kB do the dl_entry memory space was chosen. Because each dl_entry has a
 * size of 12 bytes, this space allows for a potential of 3413 dl_entry structs describing
 * Tx queue elements.
 *
 * As far as the actual payload space in DRAM, 14000 kB was chosen because this is enough
 * to store each of the 3413 Tx queue elements. Each queue element points to a unique 4KB-sized
 * buffer.
 *
 ********************************************************************/

#define TX_QUEUE_DL_ENTRY_MEM_BASE         (platform_high_dev_info.aux_bram_baseaddr)
#define TX_QUEUE_DL_ENTRY_MEM_SIZE         (40 * 1024)
#define TX_QUEUE_DL_ENTRY_MEM_HIGH          CALC_HIGH_ADDR(TX_QUEUE_DL_ENTRY_MEM_BASE, TX_QUEUE_DL_ENTRY_MEM_SIZE)

#define TX_QUEUE_BUFFER_BASE               (WLAN_EXP_ETH_BUFFERS_SECTION_HIGH + 1)
#define TX_QUEUE_BUFFER_SIZE               (14000 * 1024)
#define TX_QUEUE_BUFFER_HIGH                CALC_HIGH_ADDR(TX_QUEUE_BUFFER_BASE, TX_QUEUE_BUFFER_SIZE)



/********************************************************************
 * BSS Info
 *
 * The BSS Info storage consists of two pieces:
 *     (1) dl_entry structs that live in the aux. BRAM and
 *     (2) network_info_t buffers with the actual content that live in DRAM
 *
 ********************************************************************/
#define BSS_INFO_DL_ENTRY_MEM_BASE         (TX_QUEUE_DL_ENTRY_MEM_BASE + TX_QUEUE_DL_ENTRY_MEM_SIZE)
#define BSS_INFO_DL_ENTRY_MEM_SIZE         (2560)
#define BSS_INFO_DL_ENTRY_MEM_HIGH          CALC_HIGH_ADDR(BSS_INFO_DL_ENTRY_MEM_BASE, BSS_INFO_DL_ENTRY_MEM_SIZE)

#define BSS_INFO_BUFFER_BASE               (TX_QUEUE_BUFFER_HIGH + 1)
#define BSS_INFO_BUFFER_SIZE			   ((BSS_INFO_DL_ENTRY_MEM_SIZE/sizeof(dl_entry))*sizeof(network_info_t))
#define BSS_INFO_BUFFER_HIGH                CALC_HIGH_ADDR(BSS_INFO_BUFFER_BASE, BSS_INFO_BUFFER_SIZE)


/********************************************************************
 * Station Info
 *
 * The Station Info storage consists of two pieces:
 *     (1) dl_entry structs that live in the aux. BRAM and
 *     (2) station_info_t buffers with the actual content that live in DRAM
 *
 ********************************************************************/
#define STATION_INFO_DL_ENTRY_MEM_BASE     (BSS_INFO_DL_ENTRY_MEM_HIGH + 1)
#define STATION_INFO_DL_ENTRY_MEM_SIZE     (6656)
#define STATION_INFO_DL_ENTRY_MEM_NUM      (STATION_INFO_DL_ENTRY_MEM_SIZE/sizeof(dl_entry))
#define STATION_INFO_DL_ENTRY_MEM_HIGH      CALC_HIGH_ADDR(STATION_INFO_DL_ENTRY_MEM_BASE, STATION_INFO_DL_ENTRY_MEM_SIZE)

#define STATION_INFO_BUFFER_BASE          (BSS_INFO_BUFFER_HIGH + 1)
#define STATION_INFO_BUFFER_SIZE		  ((STATION_INFO_DL_ENTRY_MEM_SIZE/sizeof(dl_entry))*sizeof(station_info_t))
#define STATION_INFO_BUFFER_HIGH           CALC_HIGH_ADDR(STATION_INFO_BUFFER_BASE, STATION_INFO_BUFFER_SIZE)

/********************************************************************
 * User Scratch Space
 *
 * We have set aside 10MB of space for users to use the DRAM in their applications.
 * We do not use the below definitions in any part of the reference design.
 *
 ********************************************************************/
#define USER_SCRATCH_BASE                  (STATION_INFO_BUFFER_HIGH + 1)
#define USER_SCRATCH_SIZE                  (10000 * 1024)
#define USER_SCRATCH_HIGH                   CALC_HIGH_ADDR(USER_SCRATCH_BASE, USER_SCRATCH_SIZE)


/********************************************************************
 * Event Log
 *
 * The remaining space in DRAM is used for the WLAN Experiment Framework event log. The above
 * sections in DRAM are much smaller than the space set aside for the event log. In the current
 * implementation, the event log is ~995 MB.
 *
 ********************************************************************/
#define EVENT_LOG_BASE                     (USER_SCRATCH_HIGH + 1)
#define EVENT_LOG_SIZE                     ((CALC_HIGH_ADDR(platform_high_dev_info.dram_baseaddr, platform_high_dev_info.dram_size)) - EVENT_LOG_BASE + 1) // log occupies all remaining DRAM
#define EVENT_LOG_HIGH                      CALC_HIGH_ADDR(EVENT_LOG_BASE, EVENT_LOG_SIZE)

//-----------------------------------------------
// WLAN Constants
//

//-----------------------------------------------
// Callback Return Flags
//
#define MAC_RX_CALLBACK_RETURN_FLAG_DUP							0x00000001
#define MAC_RX_CALLBACK_RETURN_FLAG_NO_COUNTS					0x00000002
#define MAC_RX_CALLBACK_RETURN_FLAG_NO_LOG_ENTRY				0x00000004



/************************** Global Type Definitions **************************/

typedef enum interrupt_state_t{
	INTERRUPTS_DISABLED,
	INTERRUPTS_ENABLED
} interrupt_state_t;


/******************** Global Structure/Enum Definitions **********************/

/***************************** Global Constants ******************************/

extern const  u8 bcast_addr[MAC_ADDR_LEN];
extern const  u8 zero_addr[MAC_ADDR_LEN];


/*************************** Function Prototypes *****************************/
void               wlan_mac_high_init();
void 			   wlan_mac_high_malloc_init();

int                wlan_mac_high_interrupt_init();
void 						 wlan_mac_high_uart_rx_callback(u8 rxByte);
int                wlan_mac_high_interrupt_restore_state(interrupt_state_t new_interrupt_state);
interrupt_state_t  wlan_mac_high_interrupt_stop();

void 			   wlan_mac_high_userio_inputs_callback(u32 userio_state, enum userio_input_mask_t userio_delta);

void               wlan_mac_high_set_press_pb_0_callback(function_ptr_t callback);
void               wlan_mac_high_set_release_pb_0_callback(function_ptr_t callback);
void               wlan_mac_high_set_press_pb_1_callback(function_ptr_t callback);
void               wlan_mac_high_set_release_pb_1_callback(function_ptr_t callback);
void               wlan_mac_high_set_press_pb_2_callback(function_ptr_t callback);
void               wlan_mac_high_set_release_pb_2_callback(function_ptr_t callback);

void               wlan_mac_high_set_uart_rx_callback(function_ptr_t callback);
void               wlan_mac_high_set_mpdu_tx_high_done_callback(function_ptr_t callback);
void               wlan_mac_high_set_mpdu_tx_low_done_callback(function_ptr_t callback);
void               wlan_mac_high_set_beacon_tx_done_callback(function_ptr_t callback);
void 			   wlan_mac_high_set_mpdu_rx_callback(function_ptr_t callback);
void               wlan_mac_high_set_poll_tx_queues_callback(function_ptr_t callback);
void               wlan_mac_high_set_mpdu_dequeue_callback(function_ptr_t callback);
void 			   wlan_mac_high_set_cpu_low_reboot_callback(function_ptr_t callback);

void*              wlan_mac_high_malloc(u32 size);
void*              wlan_mac_high_calloc(u32 size);
void*              wlan_mac_high_realloc(void* addr, u32 size);
void               wlan_mac_high_free(void* addr);
void               wlan_mac_high_display_mallinfo();

int                wlan_mac_high_memory_test();
int                wlan_mac_high_right_shift_test();

int                wlan_mac_high_cdma_start_transfer(void* dest, void* src, u32 size);
void               wlan_mac_high_cdma_finish_transfer();

void               wlan_mac_high_mpdu_transmit(dl_entry* packet, int tx_pkt_buf);

void               wlan_mac_high_setup_tx_header(struct mac_header_80211_common* header, u8* addr_1, u8* addr_3);

void 			   wlan_mac_high_process_ipc_msg(struct wlan_ipc_msg_t* msg, u32* ipc_msg_from_low_payload);

void               wlan_mac_high_set_srand(u32 seed);
u8                 wlan_mac_high_bss_channel_spec_to_radio_chan(chan_spec_t chan_spec);
void               wlan_mac_high_set_radio_channel(u32 mac_channel);
void 			   wlan_mac_high_enable_mcast_buffering(u8 enable);

void               wlan_mac_high_config_txrx_beacon(struct beacon_txrx_configure_t* beacon_txrx_configure);
void               wlan_mac_high_set_rx_ant_mode(u8 ant_mode);
void               wlan_mac_high_set_tx_ctrl_power(s8 pow);
void               wlan_mac_high_set_radio_tx_power(s8 pow);
void               wlan_mac_high_set_rx_filter_mode(u32 filter_mode);
void               wlan_mac_high_set_dsss(u32 dsss_value);

int                wlan_mac_high_write_low_mem(u32 num_words, u32* payload);
int                wlan_mac_high_read_low_mem(u32 num_words, u32 baseaddr, u32* payload);
int                wlan_mac_high_write_low_param(u32 num_words, u32* payload);

void               wlan_mac_high_request_low_state();
int 			   wlan_mac_high_is_cpu_low_initialized();
int                wlan_mac_num_tx_pkt_buf_available(pkt_buf_group_t pkt_buf_group);
int                wlan_mac_high_get_empty_tx_packet_buffer();
u8                 wlan_mac_high_is_pkt_ltg(void* mac_payload, u16 length);

int                wlan_mac_high_configure_beacon_tx_template(struct mac_header_80211_common* tx_header_common_ptr, struct network_info_t* network_info, tx_params_t* tx_params_ptr, u8 flags);
int                wlan_mac_high_update_beacon_tx_params(tx_params_t* tx_params_ptr);
tx_params_t		   wlan_mac_sanitize_tx_params(struct station_info_t* station_info, tx_params_t* tx_params);




// Common functions that must be implemented by users of the framework
// TODO: Make these into callback. We should not require these implementations
dl_list*     get_network_member_list();

#endif /* WLAN_MAC_HIGH_H_ */
