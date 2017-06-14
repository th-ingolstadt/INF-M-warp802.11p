/** @file wlan_mac_common.h
 *  @brief Common Definitions
 *
 *  This contains common definitions of required by both the upper and
 *  lower-level CPUs.
 *
 *  @copyright Copyright 2013-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_COMMON_H_
#define WLAN_MAC_COMMON_H_

#include "xil_types.h"
#include "wlan_common_types.h"

// Forward declarations
struct wlan_mac_hw_info_t;
struct station_info_t;

//-----------------------------------------------
// Common function defines
//
#define max(a, b)                                       (((a) > (b)) ? (a) : (b))
#define min(a, b)                                       (((a) < (b)) ? (a) : (b))

#define abs_64(a)                                       (((a) < 0) ? (-1 * a) : (a))

#define sat_add16(a, b)                                 (((a) > (0xFFFF - (b))) ? 0xFFFF : ((a) + (b)))
#define sat_add32(a, b)                                 (((a) > (0xFFFFFFFF - (b))) ? 0xFFFFFFFF : ((a) + (b)))

#define sat_sub(a, b)                                   (((a) > (b)) ? ((a) - (b)) : 0)

#define wlan_addr_eq(addr1, addr2)                        (memcmp((void*)(addr1), (void*)(addr2), 6)==0)
#define wlan_addr_mcast(addr)                          ((((u8*)(addr))[0] & 1) == 1)

//-----------------------------------------------
// Level Print function defines
//
#define PRINT_LEVEL                                        PL_ERROR

#define PL_NONE                                            0
#define PL_ERROR                                           1
#define PL_WARNING                                         2
#define PL_VERBOSE                                         3

#define wlan_printf(severity, format, args...) \
    do { \
        if (PRINT_LEVEL >= severity){ xil_printf(format, ##args); } \
    } while(0)


//-----------------------------------------------
// PHY defines
//
#define WLAN_PHY_FCS_NBYTES                                4U

#define PHY_MODE_DSSS                                      0x0
#define PHY_MODE_NONHT                                     0x1       // 11a OFDM
#define PHY_MODE_HTMF                                      0x2       // 11n OFDM, HT mixed format

//-----------------------------------------------
// Unique sequence number defines
//
#define UNIQUE_SEQ_INVALID                                 0xFFFFFFFFFFFFFFFFULL


//-----------------------------------------------
// WLAN defines
//

// Reference: http://standards.ieee.org/develop/regauth/tut/macgrp.pdf
#define MAC_ADDR_MSB_MASK_MCAST                            0x01
#define MAC_ADDR_MSB_MASK_LOCAL                            0x02


//-----------------------------------------------
// CPU Status defines
//
#define CPU_STATUS_INITIALIZED                             0x00000001
#define CPU_STATUS_EXCEPTION                               0x80000000


//-----------------------------------------------
// Antenna mode defines
//     NOTE:  These values are enumerated and are *not* written to PHY registers
//
#define RX_ANTMODE_SISO_ANTA                               0x0
#define RX_ANTMODE_SISO_ANTB                               0x1
#define RX_ANTMODE_SISO_ANTC                               0x2
#define RX_ANTMODE_SISO_ANTD                               0x3
#define RX_ANTMODE_SISO_SELDIV_2ANT                        0x4
#define RX_ANTMODE_SISO_SELDIV_4ANT                        0x5

#define TX_ANTMODE_SISO_ANTA                               0x10
#define TX_ANTMODE_SISO_ANTB                               0x20
#define TX_ANTMODE_SISO_ANTC                               0x30
#define TX_ANTMODE_SISO_ANTD                               0x40


//-----------------------------------------------
// Receive filter defines
//     NOTE:  These filters allow the user to select the types of received packets to process
//
#define RX_FILTER_FCS_GOOD                                 0x1000    ///< Pass only packets with good checksum result
#define RX_FILTER_FCS_ALL                                  0x2000    ///< Pass packets with any checksum result
#define RX_FILTER_FCS_MASK                                 0xF000
#define RX_FILTER_FCS_NOCHANGE                             RX_FILTER_FCS_MASK

#define RX_FILTER_HDR_ADDR_MATCH_MPDU                      0x0001    ///< Pass any unicast-to-me or multicast data or management packet
#define RX_FILTER_HDR_ALL_MPDU                             0x0002    ///< Pass any data or management packet (no address filter)
#define RX_FILTER_HDR_ALL                                  0x0003    ///< Pass any packet (no type or address filters)
#define RX_FILTER_HDR_MASK                                 0x0FFF
#define RX_FILTER_HDR_NOCHANGE                             RX_FILTER_HDR_MASK


//-----------------------------------------------
// Error defines
//     Currently, the framework supports error values of 0 - 0xF.  These will
//     be displayed on the hex display as Ex, where x is the error value.
//
#define WLAN_ERROR_CODE_RIGHT_SHIFT                        0
#define WLAN_ERROR_CODE_INSUFFICIENT_BD_SIZE               1
#define WLAN_ERROR_CODE_DRAM_NOT_PRESENT                   2
#define WLAN_ERROR_CODE_CPU_LOW_TX_MUTEX                   3
#define WLAN_ERROR_CODE_CPU_LOW_RX_MUTEX                   4


#define WLAN_ERROR_CPU_STOP                                0x80000000



//-----------------------------------------------
// Debug / Monitor defines
//
#define ISR_PERF_MON_GPIO_MASK                             0x01
// #define _ISR_PERF_MON_EN_                                         ///< ISR Performance Monitor Toggle


/*********************** Global Structure Definitions ************************/

// Forward declarations
struct wlan_mac_hw_info_t;

typedef enum userio_input_mask_t{
	USERIO_INPUT_MASK_PB_0		= 0x00000001,
	USERIO_INPUT_MASK_PB_1		= 0x00000002,
	USERIO_INPUT_MASK_PB_2		= 0x00000004,
	USERIO_INPUT_MASK_PB_3		= 0x00000008,
	USERIO_INPUT_MASK_SW_0		= 0x00000010,
	USERIO_INPUT_MASK_SW_1		= 0x00000020,
	USERIO_INPUT_MASK_SW_2		= 0x00000040,
	USERIO_INPUT_MASK_SW_3		= 0x00000080,
} userio_input_mask_t;

//-----------------------------------------------
// PHY Bandwidth Configuration
//
typedef enum phy_samp_rate_t{
    PHY_10M   = 10, 
    PHY_20M   = 20, 
    PHY_40M   = 40
} phy_samp_rate_t;


//-----------------------------------------------
// LLC Header
//
typedef struct llc_header_t{
    u8   dsap;
    u8   ssap;
    u8   control_field;
    u8   org_code[3];
    u16  type;
} llc_header_t;


//-----------------------------------------------
// LTG Payload Contents
//
typedef struct ltg_packet_id_t{
    llc_header_t   llc_hdr;
    u64            unique_seq;
    u32            ltg_id;
} ltg_packet_id_t;

//-----------------------------------------------
// Beacon Tx/Rx Configuration Struct
//
typedef enum __attribute__((__packed__)){
    NEVER_UPDATE,
    ALWAYS_UPDATE,
    FUTURE_ONLY_UPDATE,
} mactime_update_mode_t;
CASSERT(sizeof(mactime_update_mode_t) == 1, mactime_update_mode_t_alignment_check);

typedef enum __attribute__((__packed__)){
    NO_BEACON_TX,
    AP_BEACON_TX,
    IBSS_BEACON_TX,
} beacon_tx_mode_t;
CASSERT(sizeof(beacon_tx_mode_t) == 1, beacon_tx_mode_t_alignment_check);

typedef struct __attribute__((__packed__)) beacon_txrx_configure_t{
    // Beacon Rx Configuration Parameters
    mactime_update_mode_t    ts_update_mode;               // Determines how MAC time is updated on reception of beacons
    u8                       bssid_match[MAC_ADDR_LEN];    // BSSID of current association for Rx matching

    // Beacon Tx Configuration Parameters
    u8                       beacon_template_pkt_buf;      // Packet Buffer that contains the the beacon template to transmit
    u32                      beacon_interval_tu;           // Beacon Interval (in TU)
    beacon_tx_mode_t         beacon_tx_mode;               // Tx Beacon Mode
	u8  					 dtim_period;				   // DTIM Period (in beacon intervals)
	u8						 reserved0;
	u8						 reserved1;
	u16						 dtim_tag_byte_offset;		   // # of bytes into the payload that contains the start of the DTIM tag
	u16						 reserved2;
} beacon_txrx_configure_t;
CASSERT(sizeof(beacon_txrx_configure_t) == 20, beacon_txrx_configure_t_alignment_check);

typedef struct time_hr_min_sec_t{
	u32 hr;
	u32 min;
	u32 sec;
} time_hr_min_sec_t;

/*************************** Function Prototypes *****************************/

int                     wlan_null_callback(void * param);
int                     wlan_verify_channel(u32 channel);
void                    init_mac_hw_info();
time_hr_min_sec_t 		wlan_mac_time_to_hr_min_sec(u64 time);

struct wlan_mac_hw_info_t* get_mac_hw_info();
u8* get_mac_hw_addr_wlan();
u8* get_mac_hw_addr_wlan_exp();

#endif   // END WLAN_MAC_COMMON_H_
