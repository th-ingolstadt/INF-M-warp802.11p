/** @file wlan_mac_counts_txrx.h
 *  @brief Tx/Rx Counts Subsytem
 *
 *  This contains code tracking transmissions and reception counts.
 *
 *  @copyright Copyright 2014-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 */

#ifndef WLAN_MAC_COUNTS_TXRX_H_
#define WLAN_MAC_COUNTS_TXRX_H_

/***************************** Include Files *********************************/

#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_common.h"


/*************************** Constant Definitions ****************************/

//-----------------------------------------------
// Timeout used to remove inactive BSS infos
//     - Part of bss_info_timestamp_check()
//
#define COUNTS_TXRX_TIMEOUT_USEC                           600000000

//-----------------------------------------------
// BSS Flags defines
//
#define COUNTS_TXRX_FLAGS_KEEP                             0x0001

/********************************************************************
 * @brief Frame Counts Structure
 *
 * This struct contains counts about the communications link. It is intended to
 * be instantiated multiple times in the broader counts_txrx struct so that
 * different packet types can be individually tracked.
 *
 ********************************************************************/
typedef struct{
    u64        rx_num_bytes;                ///< # of successfully received bytes (de-duplicated)
    u64        rx_num_bytes_total;          ///< # of successfully received bytes (including duplicates)
    u64        tx_num_bytes_success;        ///< # of successfully transmitted bytes (high-level MPDUs)
    u64        tx_num_bytes_total;          ///< Total # of transmitted bytes (high-level MPDUs)
    u32        rx_num_packets;              ///< # of successfully received packets (de-duplicated)
    u32        rx_num_packets_total;        ///< # of successfully received packets (including duplicates)
    u32        tx_num_packets_success;      ///< # of successfully transmitted packets (high-level MPDUs)
    u32        tx_num_packets_total;        ///< Total # of transmitted packets (high-level MPDUs)
    u64        tx_num_attempts;             ///< # of low-level attempts (including retransmissions)
} frame_counts_txrx_t;



/********************************************************************
 * @brief Counts Structure
 *
 * This struct contains counts about the communications link.  Additionally,
 * counting differnt parameters can be decoupled from station_info structs
 * entirely to enable promiscuous counts about unassociated devices seen in
 * the network.
 *
 * NOTE:  The reason that the reference design uses a #define for fields in
 *     two different structs is so that fields that must be in two different
 *     structs stay in sync and so there is not another level of indirection
 *     by using nested structs.
 *
 ********************************************************************/

#define COUNTS_TXRX_COMMON_FIELDS                                                                  						\
		u8                    addr[MAC_ADDR_LEN];            /* HW Address */											\
		u8                    flags;                         /* Bit flags */											\
		u8                    padding0;																					\
		 /*----- 8-byte boundary ------*/																				\
		frame_counts_txrx_t   data;                          /* Counts about data types	*/								\
		 /*----- 8-byte boundary ------*/																				\
		frame_counts_txrx_t   mgmt;                          /* Counts about management types */						\
		 /*----- 8-byte boundary ------*/																				\
		u64                   latest_txrx_timestamp;         /* Timestamp of the last frame reception	*/				\


typedef struct{
    COUNTS_TXRX_COMMON_FIELDS
    u16					  rx_latest_seq;				///< Sequence number of the last MPDU reception FIXME: Remove from wlan_exp
														///< @note This is a tracking variable used to de-duplicating receptions
    u8					  padding1[6];
} counts_txrx_t;

CASSERT(sizeof(counts_txrx_t) == 136, counts_txrx_alignment_check);

/*************************** Function Prototypes *****************************/

void             counts_txrx_init(u8 dram_present);
void             counts_txrx_init_finish();

dl_entry       * counts_txrx_checkout();
void             counts_txrx_checkin(dl_entry* entry);

inline void      counts_txrx_tx_process(void* pkt_buf_addr);
inline void      counts_txrx_rx_process(void* pkt_buf_addr);

void             counts_txrx_print_all();
void             counts_txrx_zero_all();
void             counts_txrx_timestamp_check();

dl_entry       * wlan_mac_high_find_counts_txrx_addr(u8* addr);

counts_txrx_t  * wlan_mac_high_create_counts_txrx(u8* addr);
void             wlan_mac_high_reset_counts_txrx_list();
void 			 wlan_mac_high_clear_counts_txrx(counts_txrx_t * counts_txrx);

inline dl_list * wlan_mac_high_get_counts_txrx_list();

#endif
