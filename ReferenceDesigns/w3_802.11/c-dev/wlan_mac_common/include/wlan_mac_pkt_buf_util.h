/** @file wlan_mac_pkt_buf_util.h
 *  @brief Packet Buffer Definitions
 *
 *  This contains code common to both CPU_LOW and CPU_HIGH that allows them
 *  to use the packet buffers.
 *
 *  @copyright Copyright 2013-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */


/*************************** Constant Definitions ****************************/

#ifndef WLAN_MAC_PKT_BUF_UTIL_H_
#define WLAN_MAC_PKT_BUF_UTIL_H_


// Base index of mutex for Tx/Rx packet buffers
#define PKT_BUF_MUTEX_TX_BASE                              0
#define PKT_BUF_MUTEX_RX_BASE                              16


// Mutex status values
#define PKT_BUF_MUTEX_SUCCESS                              0
#define PKT_BUF_MUTEX_FAIL_INVALID_BUF                    -1
#define PKT_BUF_MUTEX_FAIL_ALREADY_LOCKED                 -2
#define PKT_BUF_MUTEX_FAIL_NOT_LOCK_OWNER                 -3
#define	PKT_BUF_MUTEX_ALREADY_UNLOCKED				  	  -4


//-----------------------------------------------
// Packet buffer defines
//
#define NUM_TX_PKT_BUFS                                    16
#define NUM_RX_PKT_BUFS                                    8


// Packet buffer size (in bytes)
#define PKT_BUF_SIZE                                       4096


// Tx packet buffer assignments
//     The definitions for the MPDU packet buffers are not directly
//     used. Instead, the actual packet buffer values are used to
//     make iteration easier.
//
// Packet Buffers owned by CPU_HIGH at boot

#define NUM_TX_PKT_BUF_MPDU								   6

#define TX_PKT_BUF_MPDU_1                                  0
#define TX_PKT_BUF_MPDU_2                                  1
#define TX_PKT_BUF_MPDU_3                                  2
#define TX_PKT_BUF_MPDU_4  		                           3
#define TX_PKT_BUF_MPDU_5                                  4
#define TX_PKT_BUF_MPDU_6                                  5
#define TX_PKT_BUF_BEACON                                  6
// Packet Buffers owned by CPU_LOW at boot
#define TX_PKT_BUF_RTS                                     7
#define TX_PKT_BUF_ACK_CTS                                 8

//#define TX_PKT_BUF_DTIM_MCAST							   0x80


// Tx / Rx packet buffer macros
//
// Packet buffer memory format:
//   - [(M -  1):       0] - Frame info structure (M is the size of the frame info for Tx / Rx)
//                           - See definitions below for rx_frame_info / tx_frame_info
//
//   - [(M + 15):       M] - PHY Header           (16 bytes)
//                           - 11a:
//                                 [ 2: 0] SIGNAL
//                                 [ 4: 3] SERVICE (must be 0)
//                                 [15: 5] Reserved (should be 0)
//                                 [ N:16] MAC payload - first header byte at [16]
//                           - 11n:
//                                 [ 2: 0] L-SIG (same format as 11a SIGNAL)
//                                 [ 8: 3] HT-SIG
//                                 [10: 9] SERVICE (must be 0)
//                                 [15:11] Reserved (should be 0)
//                                 [ N:16] MAC payload - first header byte at [16]
//
//   - [(M +  N):(M + 16)] - MAC payload          (N is the size of the MAC payload)
//                           - Standard 802.11 MAC payload
//

#define CALC_PKT_BUF_ADDR(baseaddr, buf_idx)			  ( (baseaddr) + (((buf_idx) & 0xF) * PKT_BUF_SIZE) )

#define PHY_RX_PKT_BUF_PHY_HDR_OFFSET                     (sizeof(rx_frame_info_t))
#define PHY_TX_PKT_BUF_PHY_HDR_OFFSET                     (sizeof(tx_frame_info_t))

#define PHY_RX_PKT_BUF_PHY_HDR_SIZE                        0x10

#define PHY_RX_PKT_BUF_MPDU_OFFSET                        (PHY_RX_PKT_BUF_PHY_HDR_SIZE + PHY_RX_PKT_BUF_PHY_HDR_OFFSET)
#define PHY_TX_PKT_BUF_MPDU_OFFSET                        (PHY_TX_PKT_BUF_PHY_HDR_SIZE + PHY_TX_PKT_BUF_PHY_HDR_OFFSET)




#define TX_DETAILS_FLAGS_RECEIVED_RESPONSE				   1

// tx_details_type defines
#define TX_DETAILS_MPDU                                    0
#define TX_DETAILS_RTS_ONLY                                1
#define TX_DETAILS_RTS_MPDU                                2
#define TX_DETAILS_CTS                                     3
#define TX_DETAILS_ACK                                     4


// Defines for power field in phy_tx_params_t in tx_params_t
#define TX_POWER_MAX_DBM                                  (21)
#define TX_POWER_MIN_DBM                                  (-9)

// Defines for tx_result field
#define TX_FRAME_INFO_RESULT_SUCCESS                             0
#define TX_FRAME_INFO_RESULT_FAILURE                             1

// Defines for flags field
#define TX_FRAME_INFO_FLAGS_REQ_TO                               0x01
#define TX_FRAME_INFO_FLAGS_FILL_TIMESTAMP                       0x02
#define TX_FRAME_INFO_FLAGS_FILL_DURATION                        0x04
#define TX_FRAME_INFO_FLAGS_WAIT_FOR_LOCK						 0x10
#define TX_FRAME_INFO_FLAGS_FILL_UNIQ_SEQ                        0x20
#define TX_FRAME_INFO_FLAGS_PKT_BUF_PREPARED                     0x80

// Defines for flags field
#define RX_FRAME_INFO_FLAGS_CTRL_RESP_TX                         0x1
#define RX_FRAME_INFO_FLAGS_RETRY                                0x2
#define RX_FRAME_INFO_FLAGS_FCS_GOOD                             0x4
#define RX_FRAME_INFO_UNEXPECTED_RESPONSE						 0x8


/*************************** Function Prototypes *****************************/

int init_pkt_buf();

// TX packet buffer functions
int lock_tx_pkt_buf(u8 pkt_buf_ind);
int force_lock_tx_pkt_buf(u8 pkt_buf_ind);
int unlock_tx_pkt_buf(u8 pkt_buf_ind);
int get_tx_pkt_buf_status(u8 pkt_buf_ind, u32* locked, u32 *owner);


// RX packet buffer functions
int lock_rx_pkt_buf(u8 pkt_buf_ind);
int force_lock_rx_pkt_buf(u8 pkt_buf_ind);
int unlock_rx_pkt_buf(u8 pkt_buf_ind);
int get_rx_pkt_buf_status(u8 pkt_buf_ind, u32* locked, u32 *owner);


#endif /* WLAN_MAC_IPC_UTIL_H_ */
