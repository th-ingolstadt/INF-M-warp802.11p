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

#include "wlan_mac_common.h"


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




/*********************** Global Structure Definitions ************************/

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
