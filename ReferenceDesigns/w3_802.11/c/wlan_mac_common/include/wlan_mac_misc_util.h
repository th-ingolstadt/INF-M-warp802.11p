/** @file wlan_mac_misc_util.h
 *  @brief Common Miscellaneous Definitions
 *
 *  This contains miscellaneous definitions of required by both the upper and
 *  lower-level CPUs.
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

#ifndef WLAN_MAC_MISC_UTIL_H_
#define WLAN_MAC_MISC_UTIL_H_

typedef int (*function_ptr_t)();

#define max(A,B) (((A)>(B))?(A):(B))
#define min(A,B) (((A)<(B))?(A):(B))

#define _ISR_PERF_MON_EN_	///< ISR Performance Monitor Toggle
#define ISR_PERF_MON_GPIO_MASK	0x01

#define PRINT_LEVEL PL_ERROR

#define PL_NONE		0
#define PL_ERROR 	1
#define PL_WARNING  2
#define PL_VERBOSE  3

#define warp_printf(severity,format,args...) \
	do { \
	 if(PRINT_LEVEL>=severity){xil_printf(format, ##args);} \
	} while(0)

#define wlan_addr_eq(addr1, addr2) (memcmp((void*)(addr1), (void*)(addr2), 6)==0)
#define wlan_addr_mcast(addr) ( (((u8*)(addr))[0] & 1) == 1 )

typedef struct{
	u8      rate;			///< Rate of transmission
	u8      antenna_mode;	///< Antenna mode (Placeholder)
	s8      power;			///< Power of transmission (in dBm)
	u8		flags;			///< Flags affecting waveform construction
} phy_tx_params;

typedef struct{
	u8		num_tx_max;		///< Maximum number of transmission attempts
	u8		flags;			///< Flags affecting waveform construction
	u8 		reserved[2];
} mac_tx_params;




/**
 * @brief Transmit Parameters Structure
 *
 * This struct contains transmission parameters.
 */
typedef struct{
	phy_tx_params phy;
	mac_tx_params mac;
} tx_params;

typedef struct{
	u64 timestamp_create;
	u32 delay_accept;
	u32 delay_done;
	u8 state;
	u8 state_verbose;
	u8 flags;
	u8 num_tx;
	u16 length;
	u16 AID;
	tx_params params;
} tx_frame_info;

#define TX_POWER_MAX_DBM (19)
#define TX_POWER_MIN_DBM (-12)

#define TX_MPDU_STATE_EMPTY			0
#define TX_MPDU_STATE_TX_PENDING	1
#define TX_MPDU_STATE_READY			2

#define TX_MPDU_STATE_VERBOSE_SUCCESS	0
#define TX_MPDU_STATE_VERBOSE_FAILURE	1

#define TX_MPDU_FLAGS_REQ_TO				0x01
#define TX_MPDU_FLAGS_FILL_TIMESTAMP		0x02
#define TX_MPDU_FLAGS_FILL_DURATION			0x04


//The rx_frame_info struct is padded to give space for the PHY to fill in channel estimates. The offset where
//the PHY fills in this information must be written to the wlan_phy_rx_pkt_buf_h_est_offset macro
typedef struct{
	u8 state;
	u8 rate;
	u16 length;
	s8 rx_power;
	u8 rf_gain;
	u8 bb_gain;
	u8 channel;
	u8 flags;
	u8 ant_mode;
	u8 reserved[2];
	u32 additional_info;
	u64 timestamp;
	u32 channel_est[64];
} rx_frame_info;

#define RX_MPDU_FLAGS_ACKED		0x1
#define RX_MPDU_FLAGS_RETRY		0x2

#define RX_MPDU_STATE_EMPTY 	 	0
#define RX_MPDU_STATE_RX_PENDING	1
#define RX_MPDU_STATE_FCS_GOOD 	 	2
#define RX_MPDU_STATE_FCS_BAD 	 	3

#define CPU_STATUS_INITIALIZED			0x00000001
#define CPU_STATUS_WAIT_FOR_IPC_ACCEPT	0x00000002
#define CPU_STATUS_EXCEPTION			0x80000000

#define NUM_TX_PKT_BUFS	16
#define NUM_RX_PKT_BUFS	16

#define PKT_BUF_SIZE 4096

//Tx and Rx packet buffers
#define TX_PKT_BUF_TO_ADDR(n)	(XPAR_PKT_BUFF_TX_BRAM_CTRL_S_AXI_BASEADDR + (n)*PKT_BUF_SIZE)
#define RX_PKT_BUF_TO_ADDR(n)	(XPAR_PKT_BUFF_RX_BRAM_CTRL_S_AXI_BASEADDR + (n)*PKT_BUF_SIZE)

#define PHY_RX_PKT_BUF_PHY_HDR_OFFSET (sizeof(rx_frame_info))
#define PHY_TX_PKT_BUF_PHY_HDR_OFFSET (sizeof(tx_frame_info))

#define PHY_TX_PKT_BUF_PHY_HDR_SIZE 0x8

#define PHY_RX_PKT_BUF_MPDU_OFFSET (PHY_TX_PKT_BUF_PHY_HDR_SIZE+PHY_RX_PKT_BUF_PHY_HDR_OFFSET)
#define PHY_TX_PKT_BUF_MPDU_OFFSET (PHY_TX_PKT_BUF_PHY_HDR_SIZE+PHY_TX_PKT_BUF_PHY_HDR_OFFSET)

typedef struct{
	tx_frame_info frame_info;
	u8 phy_hdr_pad[PHY_TX_PKT_BUF_PHY_HDR_SIZE];
	u8 frame[PKT_BUF_SIZE - PHY_TX_PKT_BUF_PHY_HDR_SIZE - sizeof(tx_frame_info)];
} tx_packet_buffer;


#define WLAN_TX_ANTMODE_SISO_ANTA	0x1
#define WLAN_TX_ANTMODE_SISO_ANTB	0x2
#define WLAN_TX_ANTMODE_SISO_ANTC	0x3
#define WLAN_TX_ANTMODE_SISO_ANTD	0x4


#endif /* WLAN_MAC_MISC_UTIL_H_ */
