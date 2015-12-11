/** @file wlan_mac_misc_util.h
 *  @brief Common Miscellaneous Definitions
 *
 *  This contains miscellaneous definitions of required by both the upper and
 *  lower-level CPUs.
 *
 *  @copyright Copyright 2013-2015, Mango Communications. All rights reserved.
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
// Base Addresses
//
#define USERIO_BASEADDR                                    XPAR_W3_USERIO_BASEADDR            ///< XParameters rename of base address of User I/O


//-----------------------------------------------
// WLAN MAC TIME HW register definitions
//
// RO:
#define WLAN_MAC_TIME_REG_SYSTEM_TIME_MSB                  XPAR_WLAN_MAC_TIME_HW_MEMMAP_SYSTEM_TIME_USEC_MSB
#define WLAN_MAC_TIME_REG_SYSTEM_TIME_LSB                  XPAR_WLAN_MAC_TIME_HW_MEMMAP_SYSTEM_TIME_USEC_LSB
#define WLAN_MAC_TIME_REG_MAC_TIME_MSB                     XPAR_WLAN_MAC_TIME_HW_MEMMAP_MAC_TIME_USEC_MSB
#define WLAN_MAC_TIME_REG_MAC_TIME_LSB                     XPAR_WLAN_MAC_TIME_HW_MEMMAP_MAC_TIME_USEC_LSB

// RW:
#define WLAN_MAC_TIME_REG_NEW_MAC_TIME_MSB                 XPAR_WLAN_MAC_TIME_HW_MEMMAP_NEW_MAC_TIME_MSB
#define WLAN_MAC_TIME_REG_NEW_MAC_TIME_LSB                 XPAR_WLAN_MAC_TIME_HW_MEMMAP_NEW_MAC_TIME_LSB
#define WLAN_MAC_TIME_REG_CONTROL                          XPAR_WLAN_MAC_TIME_HW_MEMMAP_CONTROL

// Control reg masks
#define WLAN_MAC_TIME_CTRL_REG_RESET_SYSTEM_TIME           0x00000001
#define WLAN_MAC_TIME_CTRL_REG_UPDATE_MAC_TIME             0x00000002


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
#define T_SIFS                                             10


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

// Tx and Rx packet buffers
//     NOTE:  These definitions will need to be changed if the number of packet
//         buffers increase.  Currently, the "& 0x7" limits the number of packet
//         buffers to 8.
//
#define TX_PKT_BUF_TO_ADDR(n)                             (XPAR_PKT_BUFF_TX_BRAM_CTRL_S_AXI_BASEADDR + ((n) & 0x7) * PKT_BUF_SIZE)
#define RX_PKT_BUF_TO_ADDR(n)                             (XPAR_PKT_BUFF_RX_BRAM_CTRL_S_AXI_BASEADDR + ((n) & 0x7) * PKT_BUF_SIZE)

#define PHY_RX_PKT_BUF_PHY_HDR_OFFSET                     (sizeof(rx_frame_info))
#define PHY_TX_PKT_BUF_PHY_HDR_OFFSET                     (sizeof(tx_frame_info))

#define PHY_RX_PKT_BUF_PHY_HDR_SIZE                        0x10      // Was 0x8 through release v1.2 / xps v48
#define PHY_TX_PKT_BUF_PHY_HDR_SIZE                        0x8

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
#define ERROR_NODE_INSUFFICIENT_SIZE_TX_BD                 1
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
// TX parameters
//
typedef struct{
    u8                       rate;                         ///< PHY rate index
    u8                       antenna_mode;                 ///< Tx antenna selection
    s8                       power;                        ///< Tx power (in dBm)
    u8                       flags;                        ///< Flags affecting waveform construction
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

#define TX_DETAILS_MPDU		0
#define TX_DETAILS_RTS_ONLY	1
#define TX_DETAILS_RTS_MPDU 2
#define TX_DETAILS_CTS		3
#define TX_DETAILS_ACK		4


// NOTE: This struct must be padded to be an integer number of u32 words.
typedef struct {
    u32                      tx_start_delta;
    phy_tx_params            mpdu_phy_params;
    s16                      num_slots;
    u16                      cw;
    u8                       chan_num;
    u8                       tx_details_type;
    u16                      duration;
    u16                      timestamp_offset;
    u16                      ssrc;
    u16                      slrc;
    u8                       src;
    u8                       lrc;
    phy_tx_params            ctrl_phy_params;
} wlan_mac_low_tx_details;


typedef struct {
    u8                       phy_mode;
    u8                       mcs;
    u16                      length;
    u16                      N_DBPS;                       ///< Number of data bits per OFDM symbol
} phy_rx_details;


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
    u8                       padding1[3];                  ///< Used for alignment of fields (can be appropriated for any future use)

    u16                      length;                       ///< Number of bytes in MAC packet, including MAC header and FCS
    u16                      AID;                          ///< Association ID of the node to which this packet is addressed

    //
    // Place additional fields here.  Make sure the new fields keep the structure 8-byte aligned
    //

    tx_params                params;                       ///< Additional lower-level MAC and PHY parameters (8 bytes)
} tx_frame_info;


#define TX_POWER_MAX_DBM                                  (21)
#define TX_POWER_MIN_DBM                                  (-9)

#define TX_MPDU_RESULT_SUCCESS                             0
#define TX_MPDU_RESULT_FAILURE                             1

#define TX_MPDU_FLAGS_REQ_TO                               0x01
#define TX_MPDU_FLAGS_FILL_TIMESTAMP                       0x02
#define TX_MPDU_FLAGS_FILL_DURATION                        0x04
#define TX_MPDU_FLAGS_REQ_BO                               0x08
#define TX_MPDU_FLAGS_AUTOCANCEL                           0x10



//-----------------------------------------------
// RX frame information
//     NOTE:  This structure defines the information passed in the packet buffer between
//         CPU High and CPU Low as part of receiving packets.  The struct is padded to
//         give space for the PHY to fill in channel estimates.
//
typedef struct{
    u8                       state;                        ///< Packet buffer state - RX_MPDU_STATE_EMPTY, RX_MPDU_STATE_RX_PENDING, RX_MPDU_STATE_FCS_GOOD or RX_MPDU_STATE_FCS_BAD
    u8                       flags;                        ///< Bit flags
    u8                       ant_mode;                     ///< Rx antenna selection
    s8                       rx_power;                     ///< Rx power, in dBm

    phy_rx_details           phy_details;                  ///< Details from PHY used in this reception
    u8                       reserved[2];                  ///< Reserved bytes for alignment

    u8                       rf_gain;                      ///< Gain setting of radio Rx LNA, in [0,1,2]
    u8                       bb_gain;                      ///< Gain setting of radio Rx VGA, in [0,1,...31]
    u8                       channel;                      ///< Channel index
    u8                       padding1;                     ///< Used for alignment of fields (can be appropriated for any future use)

    u32                      additional_info;              ///< Field to hold MAC-specific info, such as a pointer to a station_info
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
u64                get_mac_time_usec();
void               set_mac_time_usec(u64 new_time);
void               apply_mac_time_delta_usec(s64 time_delta);
u64                get_system_time_usec();
void               usleep(u64 delay);

void               enable_hex_pwm();
void               disable_hex_pwm();
void               set_hex_pwm_period(u16 period);
void               set_hex_pwm_min_max(u16 min, u16 max);

void               write_hex_display(u8 val);
void               write_hex_display_with_pwm(u8 val);
void               blink_hex_display(u32 num_blinks, u32 blink_time);

void               set_hex_display_right_dp(u8 val);
void               set_hex_display_left_dp(u8 val);

void               set_hex_display_error_status(u8 status);

int                str2num(char* str);
u8                 hex_to_seven_segment(u8 hex_value);




#endif   // END WLAN_MAC_MISC_UTIL_H_
