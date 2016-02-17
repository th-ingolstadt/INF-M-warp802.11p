/** @file wlan_mac_misc_util.h
 *  @brief Common Miscellaneous Definitions
 *
 *  This contains miscellaneous definitions of required by both the upper and
 *  lower-level CPUs.
 *
 *  @copyright Copyright 2013-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */


/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_MISC_UTIL_H_
#define WLAN_MAC_MISC_UTIL_H_


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
// Compile-time assertion defines
//
#define CASSERT(predicate, file)                           _impl_CASSERT_LINE(predicate, __LINE__, file)

#define _impl_PASTE(a, b)                                  a##b
#define _impl_CASSERT_LINE(predicate, line, file) \
    typedef char _impl_PASTE(assertion_failed_##file##_, line)[2*!!(predicate)-1];


//-----------------------------------------------
// PHY defines
//
#define WLAN_PHY_FCS_NBYTES                                4


//-----------------------------------------------
// Unique sequence number defines
//
#define UNIQUE_SEQ_INVALID	                               0xFFFFFFFFFFFFFFFF


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
// WLAN defines
//

//Reference: http://standards.ieee.org/develop/regauth/tut/macgrp.pdf
#define MAC_ADDR_MSB_MASK_MCAST                            0x01
#define MAC_ADDR_MSB_MASK_LOCAL                            0x02


//-----------------------------------------------
// CPU Status defines
//
#define CPU_STATUS_INITIALIZED                             0x00000001
#define CPU_STATUS_EXCEPTION                               0x80000000


//-----------------------------------------------
// Packet buffer defines
//
#define NUM_TX_PKT_BUFS                                    8
#define NUM_RX_PKT_BUFS                                    8

// Packet buffer size (in bytes)
#define PKT_BUF_SIZE                                       4096

// Tx packet buffer assignments
//   Note: the definitions for the MPDU packet buffers
//   are not directly used. Instead, the actual packet
//   buffer values are used to make iteration easier.
#define TX_PKT_BUF_MPDU_1								   0
#define TX_PKT_BUF_MPDU_2								   1
#define TX_PKT_BUF_MPDU_3								   2
#define TX_PKT_BUF_BEACON								   3
#define TX_PKT_BUF_RTS                                     6
#define TX_PKT_BUF_ACK_CTS                                 7

// Tx and Rx packet buffers

// Tx pkt buf byte indexes:
// 11a:
//  [ 2: 0] SIGNAL
//  [ 4: 3] SERVICE (must be 0)
//  [15: 5] Reserved (should be 0)
//  [ N:16] MAC payload - first header byte at [16]
// 11n:
//  [ 2: 0] L-SIG (same format as 11a SIGNAL)
//  [ 8: 3] HT-SIG
//  [10: 9] SERVICE (must be 0)
//  [15:11] Reserved (should be 0)
//  [ N:16] MAC payload - first header byte at [16]

#define TX_PKT_BUF_TO_ADDR(n)                             (XPAR_PKT_BUFF_TX_BRAM_CTRL_S_AXI_BASEADDR + ((n) & 0x7) * PKT_BUF_SIZE)
#define RX_PKT_BUF_TO_ADDR(n)                             (XPAR_PKT_BUFF_RX_BRAM_CTRL_S_AXI_BASEADDR + ((n) & 0x7) * PKT_BUF_SIZE)

#define PHY_RX_PKT_BUF_PHY_HDR_OFFSET                     (sizeof(rx_frame_info))
#define PHY_TX_PKT_BUF_PHY_HDR_OFFSET                     (sizeof(tx_frame_info))

#define PHY_RX_PKT_BUF_PHY_HDR_SIZE                        0x10
#define PHY_TX_PKT_BUF_PHY_HDR_SIZE                        0x10 //payload starts at byte 16

#define PHY_RX_PKT_BUF_MPDU_OFFSET                        (PHY_RX_PKT_BUF_PHY_HDR_SIZE + PHY_RX_PKT_BUF_PHY_HDR_OFFSET)
#define PHY_TX_PKT_BUF_MPDU_OFFSET                        (PHY_TX_PKT_BUF_PHY_HDR_SIZE + PHY_TX_PKT_BUF_PHY_HDR_OFFSET)



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

#define PHY_MODE_NONHT	0x1 //11a OFDM
#define PHY_MODE_HTMF	0x2 //11n OFDM, HT mixed format

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
// Node Error defines
//
#define ERROR_NODE_RIGHT_SHIFT                             0
#define ERROR_NODE_INSUFFICIENT_BD_SIZE                    1
#define ERROR_NODE_DRAM_NOT_PRESENT                        2



//-----------------------------------------------
// Debug / Monitor defines
//
#define ISR_PERF_MON_GPIO_MASK                             0x01
// #define _ISR_PERF_MON_EN_                                         ///< ISR Performance Monitor Toggle


/*********************** Global Structure Definitions ************************/

//-----------------------------------------------
// Generic function pointer
//
typedef int (*function_ptr_t)();


//-----------------------------------------------
// PHY Bandwidth Configuration
//
typedef enum {
    PHY_5M    =  5, 
    PHY_10M   = 10, 
    PHY_20M   = 20, 
    PHY_40M   = 40
} phy_samp_rate_t;


//-----------------------------------------------
// LLC Header
//
typedef struct{
	u8  dsap;
	u8  ssap;
	u8  control_field;
	u8  org_code[3];
	u16 type;
} llc_header;


//-----------------------------------------------
// LTG Payload Contents
//
typedef struct {
	llc_header  llc_hdr;
	u64         unique_seq;
	u32         ltg_id;
} ltg_packet_id;


//-----------------------------------------------
// TX parameters
//

typedef struct{
    u8 mcs;          ///< MCS index
    u8 phy_mode;     ///< PHY mode selection and flags
    u8 antenna_mode; ///< Tx antenna selection
    s8 power;        ///< Tx power (in dBm)
} phy_tx_params;


typedef struct{
    u8                       flags;                        ///< Flags affecting waveform construction
    u8                       reserved[3];                  ///< Reserved for 32-bit alignment
} mac_tx_params;


typedef struct{
    phy_tx_params            phy;                          ///< PHY Tx params
    mac_tx_params            mac;                          ///< Lower-level MAC Tx params
} tx_params;


//-----------------------------------------------
// TX information
//     NOTE:  These types / structs contain information about the PHY and low-level MAC
//         when transmitting packets.
//

typedef enum __attribute__ ((__packed__)) {UNINITIALIZED = 0, EMPTY = 1, READY = 2, CURRENT = 3, DONE = 4} tx_pkt_buf_state_t;

#define TX_DETAILS_MPDU		0
#define TX_DETAILS_RTS_ONLY	1
#define TX_DETAILS_RTS_MPDU 2
#define TX_DETAILS_CTS		3
#define TX_DETAILS_ACK		4


// NOTE: This struct must be padded to be an integer number of u32 words.
typedef struct {
    u64                      tx_start_timestamp_mpdu;
    u64                      tx_start_timestamp_ctrl;
    phy_tx_params            phy_params_mpdu;
    phy_tx_params            phy_params_ctrl;
    s16                      num_slots;
    u16                      cw;
    u8                       chan_num;
    u8                       tx_details_type;
    u16                      duration;
    u8						 tx_start_timestamp_frac_mpdu;
    u8						 tx_start_timestamp_frac_ctrl;
    u16                      ssrc;
    u16                      slrc;
    u8                       src;
    u8                       lrc;
} wlan_mac_low_tx_details;

typedef struct {
    u8                       mcs;
    u8                       phy_mode;
    u8						 reserved[2];
    u16                      length;
    u16                      N_DBPS;                       ///< Number of data bits per OFDM symbol
} phy_rx_details;
//FIXME: why N_DBPS here? this can be calculated from phy_mode and mcs

#define PHY_RX_DETAILS_MODE_DSSS                          (0)
#define PHY_RX_DETAILS_MODE_11AG                          (WLAN_MAC_PHY_RX_PARAMS_PHY_MODE_11AG)
#define PHY_RX_DETAILS_MODE_11N                           (WLAN_MAC_PHY_RX_PARAMS_PHY_MODE_11N)


//-----------------------------------------------
// TX queue information
//     NOTE:  This structure defines the information about the TX queue that contained the packet
//         while in CPU High.  This structure must be 32-bit aligned.
//
typedef struct {
    u16                      QID;                          ///< ID of the Queue
    u16                      occupancy;                    ///< Number of elements in the queue when the
                                                           ///<   packet was enqueued (including itself)
} tx_queue_details;


//-----------------------------------------------
// TX frame information
//     NOTE:  This structure defines the information passed in the packet buffer between
//         CPU High and CPU Low as part of transmitting packets.
//
//     IMPORTANT:  This structure must be 8-byte aligned so that the phy can properly insert
//         timestamps into management packets (see wlan_phy_tx_timestamp_ins_* in wlan_mac_low.c
//         for relevant code).
//
typedef struct{
    u64                      timestamp_create;             ///< MAC timestamp of packet creation
    u32                      delay_accept;                 ///< Time in microseconds between timestamp_create and packet acceptance by CPU Low
    u32                      delay_done;                   ///< Time in microseconds between acceptance and transmit completion
    u64                      unique_seq;                   ///< Unique sequence number for this packet (12 LSB used as 802.11 MAC sequence number)
    tx_queue_details         queue_info;                   ///< Information about the TX queue used for the packet (4 bytes)

    u8                       tx_result;                    ///< Result of transmission attempt - TX_MPDU_RESULT_SUCCESS or TX_MPDU_RESULT_FAILURE
    u8                       short_retry_count;            ///<
    u8                       long_retry_count;             ///<
    u8                       num_tx_attempts;              ///< Number of transmission attempts for this frame

    u8                       flags;                        ///< Bit flags en/disabling certain operations by the lower-level MAC
    u8						 phy_samp_rate;				   ///< PHY Sampling Rate
    tx_pkt_buf_state_t		 tx_pkt_buf_state;			   ///< State of the Tx Packet Buffer
    u8                       padding0;                  ///< Used for alignment of fields (can be appropriated for any future use)

    u16                      length;                       ///< Number of bytes in MAC packet, including MAC header and FCS
    u16                      AID;                          ///< Association ID of the node to which this packet is addressed

    //
    // Place additional fields here.  Make sure the new fields keep the structure 8-byte aligned
    //

    tx_params                params;                       ///< Additional lower-level MAC and PHY parameters (8 bytes)
} tx_frame_info;
//Here, we are assuming that tx_pkt_buf_state_t is a u8 -- a decision that is architecture dependent. However, we can at least
//check the assumption by producing a compile-time assertion should the assumption be invalid.
CASSERT(sizeof(tx_frame_info) == 48, tx_frame_info_alignment_check);


#define TX_POWER_MAX_DBM                                  (21)
#define TX_POWER_MIN_DBM                                  (-9)

#define TX_MPDU_RESULT_SUCCESS                             0
#define TX_MPDU_RESULT_FAILURE                             1

#define TX_MPDU_FLAGS_REQ_TO                               0x01
#define TX_MPDU_FLAGS_FILL_TIMESTAMP                       0x02
#define TX_MPDU_FLAGS_FILL_DURATION                        0x04
#define TX_MPDU_FLAGS_REQ_BO                               0x08
#define TX_MPDU_FLAGS_BEACONCANCEL                         0x10
#define TX_MPDU_FLAGS_FILL_UNIQ_SEQ                        0x20



//-----------------------------------------------
// RX frame information
//     NOTE:  This structure defines the information passed in the packet buffer between
//         CPU High and CPU Low as part of receiving packets.  The struct is padded to
//         give space for the PHY to fill in channel estimates. This struct must be
//		   8-byte aligned
//
typedef struct{
    u8                       state;                        ///< Packet buffer state - RX_MPDU_STATE_EMPTY, RX_MPDU_STATE_RX_PENDING, RX_MPDU_STATE_FCS_GOOD or RX_MPDU_STATE_FCS_BAD
    u8                       flags;                        ///< Bit flags
    u8                       ant_mode;                     ///< Rx antenna selection
    s8                       rx_power;                     ///< Rx power, in dBm
    u8                       rf_gain;                      ///< Gain setting of radio Rx LNA, in [0,1,2]
    u8                       bb_gain;                      ///< Gain setting of radio Rx VGA, in [0,1,...31]
    u8                       channel;                      ///< Channel index
    u8                       padding1;                     ///< Used for alignment of fields (can be appropriated for any future use)
    //----- 8-byte boundary ------
    u32						 cfo_est;					   ///< Carrier Frequency Offset Estimate
    u32						 reserved0;
    //----- 8-byte boundary ------
    phy_rx_details           phy_details;                  ///< Details from PHY used in this reception
    //----- 8-byte boundary ------
    u8						 timestamp_frac;			   ///< Fractional timestamp beyond usec timestamp for time of reception
    u8						 phy_samp_rate;					   ///< PHY Sampling Rate
    u8                       reserved1[2];                 ///< Reserved bytes for alignment
    u32                      additional_info;              ///< Field to hold MAC-specific info, such as a pointer to a station_info
    //----- 8-byte boundary ------
    wlan_mac_low_tx_details  resp_low_tx_details;
    u64                      timestamp;                    ///< MAC timestamp at time of reception
    u32                      channel_est[64];              ///< Rx PHY channel estimates
} rx_frame_info;


#define RX_MPDU_FLAGS_FORMED_RESPONSE                      0x1
#define RX_MPDU_FLAGS_RETRY                                0x2

#define RX_MPDU_STATE_EMPTY                                0
#define RX_MPDU_STATE_RX_PENDING                           1
#define RX_MPDU_STATE_FCS_GOOD                             2
#define RX_MPDU_STATE_FCS_BAD                              3



/*************************** Function Prototypes *****************************/


#endif   // END WLAN_MAC_MISC_UTIL_H_
