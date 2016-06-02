/** @file wlan_mac_high.h
 *  @brief Top-level WLAN MAC High Framework
 *
 *  This contains the top-level code for accessing the WLAN MAC High Framework.
 *
 *  @copyright Copyright 2014-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *                See LICENSE.txt included in the design archive or
 *                at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */


/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_HIGH_H_
#define WLAN_MAC_HIGH_H_

#include "xil_types.h"
#include "wlan_mac_common.h"

//-----------------------------------------------
// Size Options
//

//---------- COMPILATION TOGGLES ----------
//  The following toggles directly affect the size of the .text section after compilation.
//  They also implicitly affect DRAM usage since DRAM is used for the storage of
//  station_info_t structs as well as Tx/Rx logs.


#define WLAN_SW_CONFIG_ENABLE_WLAN_EXP      1       //Top-level switch for compiling wlan_exp. Setting to 0 implicitly removes
                                                    // logging code if set to 0 since there would be no way to retrieve the log.
													//FIXME: Incomplete implementation

#define WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS   1       //Top-level switch for compiling counts_txrx.  Setting to 0 removes counts
                                                    // from station_info_t struct definition and disables counts retrieval via
                                                    // wlan_exp.

#define WLAN_SW_CONFIG_ENABLE_LOGGING       1       //Top-level switch for compiling Tx/Rx logging. Setting to 0 will not cause
                                                    // the design to not log any entries to DRAM. It will also disable any log
                                                    // retrieval capabilities in wlan_exp. Note: this is logically distinct from
                                                    // COMPILE_WLAN_EXP. (COMPILE_WLAN_EXP 1, COMPILE_LOGGING 0)  still allows
                                                    // wlan_exp control over a node but no logging capabilities.
													//FIXME: Incomplete implementation

#define WLAN_SW_CONFIG_ENABLE_LTG           1       //Top-level switch for compiling LTG functionality. Setting to 0 will remove
                                                    // all LTG-related code from the design as well we disable any wlan_exp
                                                    // commands that control LTGs.
													//FIXME: Incomplete implementation

//---------- USAGE TOGGLES ----------
// The following toggles affect the number of dl_entry structs that need to be stored
// in the AUX. BRAM. Disabling these options allows significant reduction in the
// AUX BRAM SIZE PARAMETERS definitions below.

#define WLAN_SW_CONFIG_ENABLE_PROMISCUOUS_STATION_INFO  1   //When set to 0, station_info_t structs will only be created
                                                            // explicitly by the top-level application (e.g. an AP adds an
                                                            // associatiated STA). Setting this to 0 would allow a significant
                                                            // reduction in WLAN_OPTIONS_SIZE_KB_STATION_INFO below, since
                                                            // the maximum number of station_info_t structs can be bounded
                                                            // by the maximum number of associations for an AP. Note: an IBSS
                                                            // node cannot bound the maximum number of station_info_t structs
															//FIXME

#define WLAN_SW_CONFIG_ENABLE_PROMISCUOUS_BSS_INFO      1   //When set to 0, bss_info_t structs will only be created
                                                            // explicitly by the top-level application (i.e. a call to the
                                                            // application's configure_bss() function). Note: this will break
                                                            // the framework's ability to perform an active/passive scan. It
                                                            // should only be set to 0 if the node is an AP or a STA whose
                                                            // association will be manipulation directly via configure_bss().
															//FIXME

//---------- AUX BRAM SIZE PARAMETERS ----------
// These options affect the usage of the AUX. BRAM memory. By disabling USAGE TOGGLES options
// above, these definitions can be reduced and still guarantee safe performance of the node.

#define WLAN_OPTIONS_AUX_SIZE_KB_STATION_INFO   4608    //dl_entry structs will fill WLAN_OPTIONS_AUX_SIZE_KB_STATION_INFO
                                                                    // kilobytes of memory. This parameter directly controls the number
                                                                    // of station_info_t structs that can be created. Note:
                                                                    // WLAN_OPTIONS_COMPILE_COUNTS_TXRX will affect the size of the
                                                                    // station_info_t structs in DRAM, but will not change the number
                                                                    // of station_info_t structs that can exists because that number
                                                                    // is constrained by the size of dl_entry and
                                                                    // WLAN_OPTIONS_AUX_SIZE_KB_STATION_INFO

#define WLAN_OPTIONS_AUX_SIZE_KB_BSS_INFO       4608    //dl_entry structs will fill WLAN_OPTIONS_AUX_SIZE_KB_BSS_INFO
                                                                    // kilobytes of memory. This parameter directly controls the number
                                                                    // of bss_info_t structs that can be created.

#define WLAN_OPTIONS_AUX_SIZE_KB_RX_ETH_BD      <some_number_kB>    //The XAxiDma_BdRing for Ethernet receptions will fill
                                                                    // WLAN_OPTIONS_AUX_SIZE_KB_RX_ETH_BD kilobytes of memory. This
                                                                    // parameter has a soft performance implication on the number of
                                                                    // bursty Ethernet receptions the design can handle.
																	//FIXME

//-----------------------------------------------
// Boot memory defines
//
#define INIT_DATA_BASEADDR                  XPAR_MB_HIGH_INIT_BRAM_CTRL_S_AXI_BASEADDR   ///< Base address of memory used for storing boot data
#define INIT_DATA_DOTDATA_IDENTIFIER        0x1234ABCD                                   ///< "Magic number" used as an identifier in boot data memory
#define INIT_DATA_DOTDATA_START            (INIT_DATA_BASEADDR+0x200)                    ///< Offset into memory for boot data
#define INIT_DATA_DOTDATA_SIZE             (4*(XPAR_MB_HIGH_INIT_BRAM_CTRL_S_AXI_HIGHADDR - INIT_DATA_DOTDATA_START))  ///< Amount of space available in boot data memory



//-----------------------------------------------
// Auxiliary (AUX) BRAM and DRAM (DDR) Memory Maps
//
#define AUX_BRAM_BASE                      (XPAR_MB_HIGH_AUX_BRAM_CTRL_S_AXI_BASEADDR)
#define AUX_BRAM_SIZE                      (XPAR_MB_HIGH_AUX_BRAM_CTRL_S_AXI_HIGHADDR - XPAR_MB_HIGH_AUX_BRAM_CTRL_S_AXI_BASEADDR + 1)
#define AUX_BRAM_HIGH                      (XPAR_MB_HIGH_AUX_BRAM_CTRL_S_AXI_HIGHADDR)

#define DRAM_BASE                          (XPAR_DDR3_SODIMM_S_AXI_BASEADDR)
#define DRAM_SIZE                          (XPAR_DDR3_SODIMM_S_AXI_HIGHADDR - XPAR_DDR3_SODIMM_S_AXI_BASEADDR + 1)
#define DRAM_HIGH                          (XPAR_DDR3_SODIMM_S_AXI_HIGHADDR)


/********************************************************************
 *
 * --------------------------------------------------------
 *      Aux. BRAM (64 KB)    |      DRAM (1048576 KB)
 * --------------------------------------------------------
 *                           |
 *                           |  CPU High Data Linker Space
 *                           |          (1024 KB)
 *   Tx Queue DL_ENTRY       |-----------------------------
 *        (40 KB)            |
 *                          -->      Tx Queue Buffer
 *                           |          (14000 KB)
 * --------------------------|-----------------------------
 *                           |
 *   BSS Info DL_ENTRY      -->      BSS Info Buffer
 *      ( 4.5 KB)            |          (27 KB)
 * --------------------------|-----------------------------
 *                           |
 *  Station Info DL_ENTRY   -->    Station Info Buffer
 *      ( 4.5 KB)            |          (51 KB)
 * --------------------------|-----------------------------
 *                           |
 *       Eth Tx BD           |      User Scratch Space
 *        (64 B)             |          (14318 KB)
 * --------------------------|-----------------------------
 *                           |
 *       Eth Rx BD           |          Event Log
 *        (Rest)             |          (Rest)
 * --------------------------|-----------------------------
 *
 ********************************************************************/

#define high_addr_calc(base, size)         ((base)+((size)-1))



/********************************************************************
 * CPU High Linker Data Space
 *
 * In order for the linker to store data sections in the DDR, we must reserve the
 * beginning of DDR.  You can see how this is used by looking for the .cpu_high_data
 * section in the linker command file (lscript.ld).  By default, 1024 KB (ie 1 MB)
 * of space is reserved for this section.
 *
 * In the default reference design, only the WARP IP/UDP library is loaded in this
 * section since it requires memory space to store buffers / information for sending /
 * receiving packets.
 *
 * NOTE:  Please be aware that while the linker can link into this section, it cannot
 *     be loaded by the SDK.  Therefore, the CPU must perform any necessary
 *     initialization.
 *
 ********************************************************************/
#define CPU_HIGH_DDR_LINKER_DATA_BASE      (DRAM_BASE)
#define CPU_HIGH_DDR_LINKER_DATA_SIZE      (1024 * 1024)
#define CPU_HIGH_DDR_LINKER_DATA_HIGH       high_addr_calc(CPU_HIGH_DDR_LINKER_DATA_BASE, CPU_HIGH_DDR_LINKER_DATA_HIGH)



/********************************************************************
 * TX Queue
 *
 * The Tx Queue consists of two pieces:
 *     (1) dl_entry structs that live in the AUX BRAM
 *     (2) Data buffers for the packets themselves than live in DRAM
 *
 * The below definitions carve out the sizes of memory for these two pieces.  The default
 * value of 40 kB do the dl_entry memory space was chosen. Because each dl_entry has a
 * size of 12 bytes, this space allows for a potential of 3413 dl_entry structs describing
 * Tx queue elements.
 *
 * As far as the actual payload space in DRAM, 14000 kB was chosen because this is enough
 * to store each of the 3413 Tx queue elements. Each queue element points to a unique 4KB-sized
 * buffer.
 *
 ********************************************************************/
#define TX_QUEUE_DL_ENTRY_MEM_BASE         (AUX_BRAM_BASE)
#define TX_QUEUE_DL_ENTRY_MEM_SIZE         (40 * 1024)
#define TX_QUEUE_DL_ENTRY_MEM_HIGH          high_addr_calc(TX_QUEUE_DL_ENTRY_MEM_BASE, TX_QUEUE_DL_ENTRY_MEM_SIZE)

#define TX_QUEUE_BUFFER_BASE               (CPU_HIGH_DDR_LINKER_DATA_BASE + CPU_HIGH_DDR_LINKER_DATA_SIZE)
#define TX_QUEUE_BUFFER_SIZE               (14000 * 1024)
#define TX_QUEUE_BUFFER_HIGH                high_addr_calc(TX_QUEUE_BUFFER_BASE, TX_QUEUE_BUFFER_SIZE)



/********************************************************************
 * BSS Info
 *
 * The BSS Info storage consists of two pieces:
 *     (1) dl_entry structs that live in the aux. BRAM and
 *     (2) bss_info_t buffers with the actual content that live in DRAM
 *
 * The below definitions carve out the sizes of memory for these two pieces. The default
 * value of 4.5 kB for the dl_entry memory space was chosen. Because each dl_entry has a
 * size of 12 bytes, this space allows for a potential of 384 dl_entry structs describing
 * bss_info_t elements.
 *
 * Each bss_info struct is a total of 72 bytes in size. So, 384 bss_info_t structs require
 * 27 kB of memory. This is why BSS_INFO_BUFFER_SIZE is set to 27 kB.
 *
 ********************************************************************/
#define BSS_INFO_DL_ENTRY_MEM_BASE         (TX_QUEUE_DL_ENTRY_MEM_BASE + TX_QUEUE_DL_ENTRY_MEM_SIZE)
#define BSS_INFO_DL_ENTRY_MEM_SIZE         (WLAN_OPTIONS_AUX_SIZE_KB_BSS_INFO)
#define BSS_INFO_DL_ENTRY_MEM_HIGH          high_addr_calc(BSS_INFO_DL_ENTRY_MEM_BASE, BSS_INFO_DL_ENTRY_MEM_SIZE)

#define BSS_INFO_BUFFER_BASE               (TX_QUEUE_BUFFER_BASE + TX_QUEUE_BUFFER_SIZE)
#define BSS_INFO_BUFFER_SIZE			   ((BSS_INFO_DL_ENTRY_MEM_SIZE/sizeof(dl_entry))*sizeof(bss_info_t))
#define BSS_INFO_BUFFER_HIGH                high_addr_calc(BSS_INFO_BUFFER_BASE, BSS_INFO_BUFFER_SIZE)


/********************************************************************
 * Station Info
 *
 * The Station Info storage consists of two pieces:
 *     (1) dl_entry structs that live in the aux. BRAM and
 *     (2) station_info_t buffers with the actual content that live in DRAM
 *
 * The below definitions carve out the sizes of memory for these two pieces. The default
 * value of 4.5 kB for the dl_entry memory space was chosen. Because each dl_entry has a
 * size of 12 bytes, this space allows for a potential of 384 dl_entry structs describing
 * station_info_t elements.
 *
 * FIXME: Update all comments for new flexible sizes
 *
 ********************************************************************/
#define STATION_INFO_DL_ENTRY_MEM_BASE     (BSS_INFO_DL_ENTRY_MEM_BASE + BSS_INFO_DL_ENTRY_MEM_SIZE)
#define STATION_INFO_DL_ENTRY_MEM_SIZE     (WLAN_OPTIONS_AUX_SIZE_KB_STATION_INFO)
#define STATION_INFO_DL_ENTRY_MEM_HIGH      high_addr_calc(STATION_INFO_DL_ENTRY_MEM_BASE, STATION_INFO_DL_ENTRY_MEM_SIZE)

#define STATION_INFO_BUFFER_BASE          (BSS_INFO_BUFFER_BASE + BSS_INFO_BUFFER_SIZE)
#define STATION_INFO_BUFFER_SIZE		  ((STATION_INFO_DL_ENTRY_MEM_SIZE/sizeof(dl_entry))*sizeof(station_info_t))
#define STATION_INFO_BUFFER_HIGH           high_addr_calc(STATION_INFO_BUFFER_BASE, STATION_INFO_BUFFER_SIZE)



/********************************************************************
 * Ethernet TX Buffer Descriptors
 *
 * The current architecture blocks on Ethernet transmissions. As such, only a single
 * Eth DMA buffer descriptor (BD) is needed. Each BD is 64 bytes in size (see
 * XAXIDMA_BD_MINIMUM_ALIGNMENT), so we set ETH_TX_BD_SIZE to 64.
 *
 ********************************************************************/
#define ETH_TX_BD_BASE                     (STATION_INFO_DL_ENTRY_MEM_BASE + STATION_INFO_DL_ENTRY_MEM_SIZE)
#define ETH_TX_BD_SIZE                     (64)
#define ETH_TX_BD_HIGH                      high_addr_calc(ETH_TX_BD_BASE, ETH_TX_BD_SIZE)



/********************************************************************
 * Ethernet RX Buffer Descriptors
 *
 * Since it is the last section we are defining in the aux. BRAM, we allow the ETH_RX
 * to fill up the rest of the BRAM. This remaining space allows for a total of 239 Eth
 * Rx DMA BDs.
 *
 ********************************************************************/
#define ETH_RX_BD_BASE                     (ETH_TX_BD_BASE + ETH_TX_BD_SIZE)
#define ETH_RX_BD_SIZE                     (AUX_BRAM_SIZE - (TX_QUEUE_DL_ENTRY_MEM_SIZE + BSS_INFO_DL_ENTRY_MEM_SIZE + STATION_INFO_DL_ENTRY_MEM_SIZE + ETH_TX_BD_SIZE))
#define ETH_RX_BD_HIGH                      high_addr_calc(ETH_RX_BD_BASE, ETH_RX_BD_SIZE)



/********************************************************************
 * User Scratch Space
 *
 * We have set aside 10MB of space for users to use the DRAM in their applications.
 * We do not use the below definitions in any part of the reference design.
 *
 ********************************************************************/
#define USER_SCRATCH_BASE                  (STATION_INFO_BUFFER_BASE + STATION_INFO_BUFFER_SIZE)
#define USER_SCRATCH_SIZE                  (10000 * 1024)
#define USER_SCRATCH_HIGH                   high_addr_calc(USER_SCRATCH_BASE, USER_SCRATCH_SIZE)


/********************************************************************
 * Event Log
 *
 * The remaining space in DRAM is used for the WLAN Experiment Framwork event log. The above
 * sections in DRAM are much smaller than the space set aside for the event log. In the current
 * implementation, the event log is ~995 MB.
 *
 ********************************************************************/
#define EVENT_LOG_BASE                     (USER_SCRATCH_BASE + USER_SCRATCH_SIZE)
#define EVENT_LOG_SIZE                     (DRAM_SIZE - (CPU_HIGH_DDR_LINKER_DATA_SIZE + TX_QUEUE_BUFFER_SIZE + BSS_INFO_BUFFER_SIZE + STATION_INFO_BUFFER_SIZE + USER_SCRATCH_SIZE))
#define EVENT_LOG_HIGH                      high_addr_calc(EVENT_LOG_BASE, EVENT_LOG_SIZE)



//-----------------------------------------------
// Device IDs
//
// NOTE:  These are re-definitions of xparameters.h #defines so that the name of the
//     underlying hardware component can change and it will only require modifying
//     one line of code in the SDK project.
//
#define INTC_DEVICE_ID                                     XPAR_INTC_0_DEVICE_ID              ///< XParameters rename of interrupt controller device ID
#define UARTLITE_DEVICE_ID                                 XPAR_UARTLITE_0_DEVICE_ID          ///< XParameters rename for UART
#define GPIO_DEVICE_ID                                     XPAR_MB_HIGH_SW_GPIO_DEVICE_ID     ///< XParameters rename of device ID of GPIO
#define INTC_GPIO_INTERRUPT_ID                             XPAR_INTC_0_GPIO_0_VEC_ID          ///< XParameters rename of GPIO interrupt ID
#define UARTLITE_INT_IRQ_ID                                XPAR_INTC_0_UARTLITE_0_VEC_ID      ///< XParameters rename of UART interrupt ID
#define TMRCTR_INTERRUPT_ID                                XPAR_INTC_0_TMRCTR_0_VEC_ID        ///< XParameters rename of timer interrupt ID



//-----------------------------------------------
// WLAN Constants
//
#define ENCAP_MODE_AP                                      0                                  ///< Used as a flag for AP encapsulation and de-encapsulation
#define ENCAP_MODE_STA                                     1                                  ///< Used as a flag for STA encapsulation and de-encapsulation
#define ENCAP_MODE_IBSS                                    2                                  ///< Used as a flag for IBSS encapsulation and de-encapsulation

#define TX_BUFFER_NUM                                      2                                  ///< Number of PHY transmit buffers to use. This should remain 2 (ping/pong buffering).

#define GPIO_OUTPUT_CHANNEL                                1                                  ///< Channel used as output for GPIO
#define GPIO_INPUT_CHANNEL                                 2                                  ///< Channel used as input for GPIO
#define GPIO_INPUT_INTERRUPT                               XGPIO_IR_CH2_MASK                  ///< Mask for enabling interrupts on GPIO input

#define GPIO_MASK_DRAM_INIT_DONE                           0x00000100                         ///< Mask for GPIO -- DRAM initialization bit
#define GPIO_MASK_PB_U                                     0x00000040                         ///< Mask for GPIO -- "Up" Pushbutton
#define GPIO_MASK_PB_M                                     0x00000020                         ///< Mask for GPIO -- "Middle" Pushbutton
#define GPIO_MASK_PB_D                                     0x00000010                         ///< Mask for GPIO -- "Down" Pushbutton
#define GPIO_MASK_DS_3                                     0x00000008                         ///< Mask for GPIO -- MSB of Dip Switch

#define UART_BUFFER_SIZE                                   1                                  ///< UART is configured to read 1 byte at a time

#define NUM_VALID_RATES                                    12                                 ///< Number of supported rates


//-----------------------------------------------
// Callback Return Flags
//
#define MAC_RX_CALLBACK_RETURN_FLAG_DUP							0x00000001
#define MAC_RX_CALLBACK_RETURN_FLAG_NO_COUNTS					0x00000002
#define MAC_RX_CALLBACK_RETURN_FLAG_NO_LOG_ENTRY				0x00000004


/***************************** Include Files *********************************/

/********************************************************************
 * Include other framework headers
 *
 * NOTE:  Includes have to be after any #define that are needed in the typedefs within the includes.
 *
 ********************************************************************/
#include "wlan_mac_queue.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_mailbox_util.h"
#include "wlan_mac_dl_list.h"



/************************** Global Type Definitions **************************/

typedef enum {INTERRUPTS_DISABLED, INTERRUPTS_ENABLED} interrupt_state_t;



/******************** Global Structure/Enum Definitions **********************/

/***************************** Global Constants ******************************/

extern const  u8 bcast_addr[MAC_ADDR_LEN];
extern const  u8 zero_addr[MAC_ADDR_LEN];


/*************************** Function Prototypes *****************************/
void               wlan_mac_high_init();
void               wlan_mac_high_heap_init();

int                          wlan_mac_high_interrupt_init();
inline int                   wlan_mac_high_interrupt_restore_state(interrupt_state_t new_interrupt_state);
inline interrupt_state_t     wlan_mac_high_interrupt_stop();

void               wlan_mac_high_uart_rx_handler(void *CallBackRef, unsigned int EventData);
void               wlan_mac_high_gpio_handler(void *InstancePtr);

dl_entry*          wlan_mac_high_find_counts_ADDR(dl_list* list, u8* addr);

u32                wlan_mac_high_get_user_io_state();

void               wlan_mac_high_set_pb_u_callback(function_ptr_t callback);
void               wlan_mac_high_set_pb_m_callback(function_ptr_t callback);
void               wlan_mac_high_set_pb_d_callback(function_ptr_t callback);
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

void               wlan_mac_high_mpdu_transmit(tx_queue_element_t* packet, int tx_pkt_buf);

u8                 wlan_mac_high_valid_tagged_rate(u8 rate);
void               wlan_mac_high_tagged_rate_to_readable_rate(u8 rate, char* str);

void               wlan_mac_high_setup_tx_header( mac_header_80211_common * header, u8 * addr_1, u8 * addr_3 );
void               wlan_mac_high_setup_tx_frame_info( mac_header_80211_common * header, tx_queue_element_t * curr_tx_queue_element, u32 tx_length, u8 flags, u8 QID );

void               wlan_mac_high_ipc_rx();
void               wlan_mac_high_process_ipc_msg(wlan_ipc_msg_t * msg);

void               wlan_mac_high_set_srand(u32 seed);
u8                 wlan_mac_high_bss_channel_spec_to_radio_chan(chan_spec_t chan_spec);
void               wlan_mac_high_set_radio_channel(u32 mac_channel);
void               wlan_mac_high_config_txrx_beacon(beacon_txrx_configure_t* beacon_txrx_configure);
void               wlan_mac_high_set_rx_ant_mode(u8 ant_mode);
void               wlan_mac_high_set_tx_ctrl_pow(s8 pow);
void               wlan_mac_high_set_rx_filter_mode(u32 filter_mode);
void               wlan_mac_high_set_dsss(u32 dsss_value);

int                wlan_mac_high_write_low_mem(u32 num_words, u32* payload);
int                wlan_mac_high_read_low_mem(u32 num_words, u32 baseaddr, u32* payload);
int                wlan_mac_high_write_low_param(u32 num_words, u32* payload);

void               wlan_mac_high_request_low_state();
int 			   wlan_mac_high_is_cpu_low_initialized();
inline int         wlan_mac_high_is_dequeue_allowed();
int                wlan_mac_high_get_empty_tx_packet_buffer();
u8                 wlan_mac_high_is_pkt_ltg(void* mac_payload, u16 length);

inline void        wlan_mac_high_set_debug_gpio(u8 val);
inline void        wlan_mac_high_clear_debug_gpio(u8 val);


int                wlan_mac_high_configure_beacon_tx_template(mac_header_80211_common* tx_header_common_ptr, bss_info_t* bss_info, tx_params_t* tx_params_ptr, u8 flags);
int                wlan_mac_high_update_beacon_tx_params(tx_params_t* tx_params_ptr);

void               wlan_mac_high_print_station_infos(dl_list* assoc_tbl);


// Common functions that must be implemented by users of the framework
// TODO: Make these into callback. We should not require these implementations
dl_list*          get_bss_member_list();
u8*          	  get_wlan_mac_addr();

#endif /* WLAN_MAC_HIGH_H_ */
