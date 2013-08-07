////////////////////////////////////////////////////////////////////////////////
// File   : wlan_lib.c
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

#ifndef WLAN_LIB_H_
#define WLAN_LIB_H_


#define max(A,B) (((A)>(B))?(A):(B))
#define min(A,B) (((A)<(B))?(A):(B))


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

#define PKT_BUF_MUTEX_DEVICE_ID		XPAR_MUTEX_0_DEVICE_ID
#define MAILBOX_DEVICE_ID			XPAR_MBOX_0_DEVICE_ID


typedef struct{
	u8 state;
	u8 rate;
	u16 length;
	u8 flags;
	u8 retry_count;
	u8 retry_max;
	u8 state_verbose;
	u16 AID;
	u16 reserved0;
	u32 reserved1;
} tx_frame_info;


#define TX_MPDU_STATE_EMPTY			0
#define TX_MPDU_STATE_TX_PENDING	1
#define TX_MPDU_STATE_READY			2

#define TX_MPDU_STATE_VERBOSE_SUCCESS	0
#define TX_MPDU_STATE_VERBOSE_FAILURE	1

#define TX_MPDU_FLAGS_REQ_TO				0x01
#define TX_MPDU_FLAGS_FILL_TIMESTAMP		0x02
#define TX_MPDU_FLAGS_FILL_DURATION			0x04




typedef struct{
	u8 state;
	u8 rate;
	u16 length;
	u16 rssi;
	u16 reserved0;
	u64 reserved1;
} rx_frame_info;

#define RX_MPDU_STATE_EMPTY 	 	0
#define RX_MPDU_STATE_RX_PENDING	1
#define RX_MPDU_STATE_FCS_GOOD 	 	2

#define CPU_STATUS_INITIALIZED			0x00000001
#define CPU_STATUS_WAIT_FOR_IPC_ACCEPT	0x00000002
#define CPU_STATUS_EXCEPTION			0x80000000

#define EXC_MUTEX_TX_FAILURE 1
#define EXC_MUTEX_RX_FAILURE 2

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

#define PKT_BUF_MUTEX_SUCCESS 				0
#define PKT_BUF_MUTEX_FAIL_INVALID_BUF		-1
#define PKT_BUF_MUTEX_FAIL_ALREADY_LOCKED	-2
#define PKT_BUF_MUTEX_FAIL_NOT_LOCK_OWNER	-3

#define PKT_BUF_MUTEX_TX_BASE	0
#define PKT_BUF_MUTEX_RX_BASE	16

#define IPC_MBOX_MSG_ID_DELIM		0xF000
#define IPC_MBOX_MAX_MSG_WORDS		255

//IPC Groups
#define IPC_MBOX_GRP_CMD			0
#define IPC_MBOX_GRP_MAC_ADDR		1
#define IPC_MBOX_GRP_CPU_STATUS		2
#define IPC_MBOX_GRP_PARAM			3

//IPC Messages
#define IPC_MBOX_CMD_RX_MPDU_READY		0
//#define IPC_MBOX_CMD_RX_MPDU_ACCEPT	1
#define IPC_MBOX_CMD_TX_MPDU_READY		2
#define IPC_MBOX_CMD_TX_MPDU_ACCEPT		3
#define IPC_MBOX_CMD_TX_MPDU_DONE		4

#define IPC_MBOX_PARAM_SET_CHANNEL		0


#define IPC_MBOX_GRP_ID(id) (IPC_MBOX_MSG_ID_DELIM | ((id<<8) & 0xF00))
#define IPC_MBOX_MSG_ID(id) (IPC_MBOX_MSG_ID_DELIM | ((id) & 0x0FF))

#define IPC_MBOX_MSG_ID_TO_GRP(id) (((id) & 0xF00)>>8)
#define IPC_MBOX_MSG_ID_TO_MSG(id) (id) & 0x0FF

#define IPC_MBOX_SUCCESS			0
#define IPC_MBOX_INVALID_MSG		-1
#define IPC_MBOX_NO_MSG_AVAIL		-2

typedef struct {
	u16 msg_id;
	u8	num_payload_words;
	u8	arg0;
	u32* payload_ptr;
} wlan_ipc_msg;

typedef int (*function_ptr_t)();

typedef struct{
	u8 frame_control_1;
	u8 frame_control_2;
	u16 duration_id;
	u8 address_1[6];
	u8 address_2[6];
	u8 address_3[6];
	u16 sequence_control;
	//u8 address_4[6];
} mac_header_80211;

//IEEE 802.11-2012 section 8.2.4:
//frame_control_1 bits[7:0]:
// 7:4: Subtype
// 3:2: Type
// 1:0: Protocol Version

#define MAC_FRAME_CTRL1_MASK_TYPE		0x0C
#define MAC_FRAME_CTRL1_MASK_SUBTYPE	0xF0

//Frame types (Table 8-1)
#define MAC_FRAME_CTRL1_TYPE_MGMT		0x00
#define MAC_FRAME_CTRL1_TYPE_CTRL		0x04
#define MAC_FRAME_CTRL1_TYPE_DATA		0x08
#define MAC_FRAME_CTRL1_TYPE_RSVD		0x0C

#define WLAN_IS_CTRL_FRAME(f) ((((mac_header_80211*)f)->frame_control_1) & MAC_FRAME_CTRL1_TYPE_CTRL)

//Frame sub-types (Table 8-1)
//Management (MAC_FRAME_CTRL1_TYPE_MGMT) sub-types
#define MAC_FRAME_CTRL1_SUBTYPE_ASSOC_REQ		(MAC_FRAME_CTRL1_TYPE_MGMT | 0x00)
#define MAC_FRAME_CTRL1_SUBTYPE_ASSOC_RESP		(MAC_FRAME_CTRL1_TYPE_MGMT | 0x10)
#define MAC_FRAME_CTRL1_SUBTYPE_REASSOC_REQ		(MAC_FRAME_CTRL1_TYPE_MGMT | 0x20)
#define MAC_FRAME_CTRL1_SUBTYPE_REASSOC_RESP	(MAC_FRAME_CTRL1_TYPE_MGMT | 0x30)
#define MAC_FRAME_CTRL1_SUBTYPE_PROBE_REQ		(MAC_FRAME_CTRL1_TYPE_MGMT | 0x40)
#define MAC_FRAME_CTRL1_SUBTYPE_PROBE_RESP		(MAC_FRAME_CTRL1_TYPE_MGMT | 0x50)
#define MAC_FRAME_CTRL1_SUBTYPE_BEACON 			(MAC_FRAME_CTRL1_TYPE_MGMT | 0x80)
#define MAC_FRAME_CTRL1_SUBTYPE_ATIM			(MAC_FRAME_CTRL1_TYPE_MGMT | 0x90)
#define MAC_FRAME_CTRL1_SUBTYPE_DISASSOC		(MAC_FRAME_CTRL1_TYPE_MGMT | 0xA0)
#define MAC_FRAME_CTRL1_SUBTYPE_AUTH  			(MAC_FRAME_CTRL1_TYPE_MGMT | 0xB0)
#define MAC_FRAME_CTRL1_SUBTYPE_DEAUTH  		(MAC_FRAME_CTRL1_TYPE_MGMT | 0xC0)
#define MAC_FRAME_CTRL1_SUBTYPE_ACTION			(MAC_FRAME_CTRL1_TYPE_MGMT | 0xD0)

//Control (MAC_FRAME_CTRL1_TYPE_CTRL) sub-types
#define MAC_FRAME_CTRL1_SUBTYPE_BLK_ACK_REQ		(MAC_FRAME_CTRL1_TYPE_CTRL | 0x80)
#define MAC_FRAME_CTRL1_SUBTYPE_BLK_ACK			(MAC_FRAME_CTRL1_TYPE_CTRL | 0x90)
#define MAC_FRAME_CTRL1_SUBTYPE_PS_POLL			(MAC_FRAME_CTRL1_TYPE_CTRL | 0xA0)
#define MAC_FRAME_CTRL1_SUBTYPE_RTS				(MAC_FRAME_CTRL1_TYPE_CTRL | 0xB0)
#define MAC_FRAME_CTRL1_SUBTYPE_CTS				(MAC_FRAME_CTRL1_TYPE_CTRL | 0xC0)
#define MAC_FRAME_CTRL1_SUBTYPE_ACK				(MAC_FRAME_CTRL1_TYPE_CTRL | 0xD0)
#define MAC_FRAME_CTRL1_SUBTYPE_CF_END			(MAC_FRAME_CTRL1_TYPE_CTRL | 0xE0)
#define MAC_FRAME_CTRL1_SUBTYPE_CF_END_CF_ACK	(MAC_FRAME_CTRL1_TYPE_CTRL | 0xF0)

//Data (MAC_FRAME_CTRL1_TYPE_DATA) sub-types

#define MAC_FRAME_CTRL1_SUBTYPE_DATA			(MAC_FRAME_CTRL1_TYPE_DATA | 0x00)

//IEEE 802.11-2012 section 8.2.4:
//frame_control_2 bits[7:0]:
#define MAC_FRAME_CTRL2_FLAG_ORDER 		0x80
#define MAC_FRAME_CTRL2_FLAG_WEP_DS		0x40
#define MAC_FRAME_CTRL2_FLAG_MORE_DATA	0x20
#define MAC_FRAME_CTRL2_FLAG_POWER_MGMT	0x10
#define MAC_FRAME_CTRL2_FLAG_RETRY		0x08
#define MAC_FRAME_CTRL2_FLAG_MORE_FLAGS	0x04
#define MAC_FRAME_CTRL2_FLAG_FROM_DS	0x02
#define MAC_FRAME_CTRL2_FLAG_TO_DS		0x01

typedef struct{
	u64 timestamp;
	u16 beacon_interval;
	u16 capabilities;
} beacon_probe_frame;

///////Capabilities
#define CAPABILITIES_ESS					0x0001
#define CAPABILITIES_IBSS					0x0002
//#define CAPABILITIES_CFP
#define CAPABILITIES_PRIVACY				0x0010
#define CAPABILITIES_SHORT_PREAMBLE			0x0020
#define CAPABILITIES_PBCC					0x0040
#define CAPABILITIES_CHAN_AGILITY			0x0080
#define CAPABILITIES_SPEC_MGMT				0x0100
#define CAPABILITIES_SHORT_TIMESLOT			0x0400
#define CAPABILITIES_APSD					0x0800
#define CAPABILITIES_DSSS_OFDM				0x2000
#define CAPABILITIES_DELAYED_BLOCK_ACK		0x4000
#define CAPABILITIES_IMMEDIATE_BLOCK_ACK	0x8000

/////// Management frame Tags
#define TAG_SSID_PARAMS				0x00
#define TAG_SUPPORTED_RATES			0x01
#define TAG_EXT_SUPPORTED_RATES		0x32
#define TAG_DS_PARAMS				0x03
#define TAG_HT_CAPABILITIES			0x45

#define RATE_BASIC 0x80


//Warning: DSSS rate is only valid for Rx. There is no DSSS transmitter.
//0x66 is an arbitrary value which cannot be confused with another PHY rate
#define WLAN_MAC_RATE_DSSS_1M	0x66

#define WLAN_MAC_RATE_BPSK12	1
#define WLAN_MAC_RATE_BPSK34	2
#define WLAN_MAC_RATE_QPSK12	3
#define WLAN_MAC_RATE_QPSK34	4
#define WLAN_MAC_RATE_16QAM12	5
#define WLAN_MAC_RATE_16QAM34	6
#define WLAN_MAC_RATE_64QAM23	7
#define WLAN_MAC_RATE_64QAM34	8

int wlan_lib_init ();
int lock_pkt_buf_tx(u8 pkt_buf_ind);
int lock_pkt_buf_rx(u8 pkt_buf_ind);
int unlock_pkt_buf_tx(u8 pkt_buf_ind);
int unlock_pkt_buf_rx(u8 pkt_buf_ind);
int status_pkt_buf_tx(u8 pkt_buf_ind, u32* Locked, u32 *Owner);
int status_pkt_buf_rx(u8 pkt_buf_ind, u32* Locked, u32 *Owner);

int ipc_mailbox_read_msg(wlan_ipc_msg* msg);
int ipc_mailbox_write_msg(wlan_ipc_msg* msg);

#endif /* WLAN_LIB_H_ */
