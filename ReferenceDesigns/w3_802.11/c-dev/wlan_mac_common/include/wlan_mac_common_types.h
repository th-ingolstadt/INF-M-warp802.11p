#ifndef WLAN_MAC_COMMON_TYPES_H_
#define WLAN_MAC_COMMON_TYPES_H_

#include "xil_types.h"
//-----------------------------------------------
// Definitions needed by typedefs below
//
#define CASSERT(predicate, file)                           _impl_CASSERT_LINE(predicate, __LINE__, file)
#define _impl_PASTE(a, b)                                  a##b
#define _impl_CASSERT_LINE(predicate, line, file) \
    typedef char _impl_PASTE(assertion_failed_##file##_, line)[2*!!(predicate)-1];

#define MAC_ADDR_LEN	6	///< MAC Address Length (in bytes)

#define MAX_PKT_SIZE_KB									   2
#define MAX_PKT_SIZE_B									   (MAX_PKT_SIZE_KB << 10)


//-----------------------------------------------
// Miscellaneous Definitions
//

// Generic function pointer
//
typedef int (*function_ptr_t)();


// PHY Bandwidth Configuration
//
typedef enum {
    PHY_10M   = 10,
    PHY_20M   = 20,
    PHY_40M   = 40
} phy_samp_rate_t;


// LLC Header
//
typedef struct{
    u8   dsap;
    u8   ssap;
    u8   control_field;
    u8   org_code[3];
    u16  type;
} llc_header_t;


// LTG Payload Contents
//
typedef struct {
    llc_header_t   llc_hdr;
    u64            unique_seq;
    u32            ltg_id;
} ltg_packet_id_t;


// Compilation Details
//
typedef struct __attribute__((__packed__)){
	char	compilation_date[12]; // Must be at least 12 bytes.
    char	compilation_time[12];  // Must be at least 9 bytes. Unfortunately, wlan_exp places an additional requirement that each field in
    							  // wlan_exp_node_info must be u32 aligned, so we increase the size to 12 bytes.
} compilation_details_t;
CASSERT(sizeof(compilation_details_t) == 24, compilation_details_t_alignment_check);

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

typedef struct __attribute__((__packed__)){
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


typedef struct{
	u32 hr;
	u32 min;
	u32 sec;
} time_hr_min_sec_t;

//-----------------------------------------------
// 802.11

// Field size defines
#define SSID_LEN_MAX	32	///< Maximum SSID length

typedef struct{
	u8  frame_control_1;
	u8  frame_control_2;
	u16 duration_id;
	u8  address_1[MAC_ADDR_LEN];
	u8  address_2[MAC_ADDR_LEN];
	u8  address_3[MAC_ADDR_LEN];
	u16 sequence_control;
	//u8  address_4[MAC_ADDR_LEN];
} mac_header_80211;

typedef struct{
	u8  frame_control_1;
	u8  frame_control_2;
	u16 duration_id;
	u8  address_ra[MAC_ADDR_LEN];
} mac_header_80211_ACK;

typedef struct{
	u8  frame_control_1;
	u8  frame_control_2;
	u16 duration_id;
	u8  address_ra[MAC_ADDR_LEN];
} mac_header_80211_CTS;

typedef struct{
	u8  frame_control_1;
	u8  frame_control_2;
	u16 duration_id;
	u8  address_ra[MAC_ADDR_LEN];
	u8  address_ta[MAC_ADDR_LEN];
} mac_header_80211_RTS;

typedef struct{
	u64 timestamp;
	u16 beacon_interval;
	u16 capabilities;
} beacon_probe_frame;

typedef struct{
	u16 capabilities;
	u16 listen_interval;
} association_req_frame;

typedef struct{
	u16 control;
} qos_control;

typedef struct{
	u8 tag_element_id;
	u8 tag_length;
} mgmt_tag_header;

// Note: mgmt_tag_template should never be instantiated. Instead,
// it should be used as pointer on top of existing memory
typedef struct{
	mgmt_tag_header header;
	u8				data[256];
} mgmt_tag_template_t;

typedef struct __attribute__ ((__packed__)){
	u16		ht_capabilities_info;
	u8		a_mpdu_parameters;
	u32     rx_supported_mcs[4];
	u16		ht_extended_capabilities;
	u32		tx_beamforming;
	u8		ant_sel;
} ht_capabilities;

typedef struct __attribute__ ((__packed__)){
	u8		channel;
	u8		ht_info_subset_1;
	u16		ht_info_subset_2;
	u16		ht_info_subset_3;
	u32		rx_supported_mcs[4];
} ht_information;

typedef struct __attribute__ ((__packed__)){
	u8		oui[3];
	u8		vendor_specific_oui_type;
	u8		wme_subtype;
	u8		wme_version;
	u8		wme_qos_info;
	u8		reserved;
	u32		aci0;
	u32		aci1;
	u32		aci2;
	u32		aci3;
} wmm_parameter_t;

//-----------------------------------------------
// DL Entry

typedef struct dl_entry dl_entry;

struct dl_entry{
    dl_entry* next;
    dl_entry* prev;
    void*     data;
};


typedef struct {
    dl_entry* first;
    dl_entry* last;
    u32       length;
} dl_list;


//-----------------------------------------------
// Mailbox

// IPC Message structure
//     - msg_id              - Any of the message IDs defined above
//     - num_payload_words   - Number of u32 words in the payload
//     - arg0                - Used to pass a single u8 argument as part of the message
//     - payload_ptr         - Pointer to payload (can be array of u32 or structure defined below)
//
typedef struct {
    u16       msg_id;
    u8        num_payload_words;
    u8        arg0;
    u32*      payload_ptr;
} wlan_ipc_msg_t;

// IPC_MBOX_MEM_READ_WRITE payload structure
//     - Must be u32 aligned
//
typedef struct{
    u32       baseaddr;
    u32       num_words;
} ipc_reg_read_write_t;


//-----------------------------------------------
// Packet Buffers

#define PHY_TX_PKT_BUF_PHY_HDR_SIZE                        0x10

// TX parameters
//     - Be careful when modifying these structures, there are alignment concerns
//       for many of the structures that contain these structures.  In general,
//       tx_params_t should be 8-byte aligned.
//
typedef struct{
    u8                       mcs;                          ///< MCS index
    u8                       phy_mode;                     ///< PHY mode selection and flags
    u8                       antenna_mode;                 ///< Tx antenna selection
    s8                       power;                        ///< Tx power (in dBm)
} phy_tx_params_t;


typedef struct{
    u8                       flags;                        ///< Flags affecting waveform construction
    u8                       reserved[3];                  ///< Reserved for 32-bit alignment
} mac_tx_params_t;


typedef struct{
    phy_tx_params_t          phy;                          ///< PHY Tx params
    mac_tx_params_t          mac;                          ///< Lower-level MAC Tx params
} tx_params_t;


// Packet buffer state
//
typedef enum __attribute__ ((__packed__)) {
   TX_PKT_BUF_UNINITIALIZED   = 0,
   TX_PKT_BUF_HIGH_CTRL       = 1,
   TX_PKT_BUF_READY           = 2,
   TX_PKT_BUF_LOW_CTRL        = 3,
   TX_PKT_BUF_DONE            = 4
} tx_pkt_buf_state_t;

typedef enum __attribute__ ((__packed__)) {
   RX_PKT_BUF_UNINITIALIZED   = 0,
   RX_PKT_BUF_HIGH_CTRL       = 1,
   RX_PKT_BUF_READY           = 2,
   RX_PKT_BUF_LOW_CTRL        = 3
} rx_pkt_buf_state_t;

// Meta-data about transmissions from CPU Low
//     - This struct must be padded to be an integer number of u32 words.
//
typedef struct __attribute__ ((__packed__)){
    u64                      tx_start_timestamp_mpdu;
    u64                      tx_start_timestamp_ctrl;
    phy_tx_params_t          phy_params_mpdu;
    phy_tx_params_t          phy_params_ctrl;

    u8                       tx_details_type;
    u8                       chan_num;
    u16                      duration;

    s16                      num_slots;
    u16                      cw;

    u8                       tx_start_timestamp_frac_mpdu;
    u8                       tx_start_timestamp_frac_ctrl;
    u8                       src;
    u8                       lrc;

    u16                      ssrc;
    u16                      slrc;

    u8						 flags;
    u8						 reserved;
    u16						 attempt_number;

} wlan_mac_low_tx_details_t;
CASSERT(sizeof(wlan_mac_low_tx_details_t) == 44, wlan_mac_low_tx_details_t_alignment_check);


//-----------------------------------------------
// RX PHY details
//     - Information recorded from the RX PHY when receiving a packet
//     - While N_DBPS can be calculated from (mcs, phy_mode), it is easier to calculate
//       the value once in CPU Low and pass it up vs re-calculating it in CPU High.
//     - This structure must be 32-bit aligned
//
typedef struct {
    u8                       mcs;
    u8                       phy_mode;
    u8                       reserved[2];
    u16                      length;
    u16                      N_DBPS;                       ///< Number of data bits per OFDM symbol
} phy_rx_details_t;


//-----------------------------------------------
// TX queue information
//     - Information about the TX queue that contained the packet while in CPU High.
//     - This structure must be 32-bit aligned.
//
typedef enum __attribute__ ((__packed__)){
	PKT_BUF_GROUP_GENERAL		= 0,
	PKT_BUF_GROUP_DTIM_MCAST    = 1,
	PKT_BUF_GROUP_OTHER 		= 0xFF,
} pkt_buf_group_t;
CASSERT(sizeof(pkt_buf_group_t) == 1, pkt_buf_group_t_alignment_check);

typedef struct {
    u8                      id;                     	  ///< ID of the Queue
    pkt_buf_group_t         pkt_buf_group;                ///< Packet Buffer Group
    u16                     occupancy;                    ///< Number of elements in the queue when the                                                         ///<   packet was enqueued (including itself)
} tx_queue_details_t;
CASSERT(sizeof(tx_queue_details_t) == 4, tx_queue_details_t_alignment_check);

//-----------------------------------------------
// TX frame information
//     - Defines the information passed in the packet buffer between CPU High and
//       CPU Low as part of transmitting packets.
//
//     IMPORTANT:  This structure must be 8-byte aligned.
//
typedef struct{
    u64                      	timestamp_create;             ///< MAC timestamp of packet creation
    u64                      	timestamp_accept;                 ///< Time in microseconds between timestamp_create and packet acceptance by CPU Low
    u64                      	timestamp_done;                   ///< Time in microseconds between acceptance and transmit completion
    //----- 8-byte boundary ------
    u64                      	unique_seq;                   ///< Unique sequence number for this packet (12 LSB used as 802.11 MAC sequence number)
    //----- 8-byte boundary ------
    tx_queue_details_t       	queue_info;                   ///< Information about the TX queue used for the packet (4 bytes)
    u16                       	num_tx_attempts;              ///< Number of transmission attempts for this frame
    u8                       	tx_result;                    ///< Result of transmission attempt - TX_MPDU_RESULT_SUCCESS or TX_MPDU_RESULT_FAILURE
    u8                       	reserved;
    //----- 8-byte boundary ------
    volatile tx_pkt_buf_state_t tx_pkt_buf_state;             ///< State of the Tx Packet Buffer
    u8                       	flags;                        ///< Bit flags en/disabling certain operations by the lower-level MAC
    u8                       	phy_samp_rate;                ///< PHY Sampling Rate
    u8                       	padding0;                     ///< Used for alignment of fields (can be appropriated for any future use)

    u16                      	length;                       ///< Number of bytes in MAC packet, including MAC header and FCS
    u16                      	ID;                           ///< Station ID of the node to which this packet is addressed
    //----- 8-byte boundary ------

    //
    // Place additional fields here.  Make sure the new fields keep the structure 8-byte aligned
    //

    //----- 8-byte boundary ------
    tx_params_t              params;                       ///< Additional lower-level MAC and PHY parameters (8 bytes)
} tx_frame_info_t;

// The above structure assumes that pkt_buf_state_t is a u8.  However, that is architecture dependent.
// Therefore, the code will check the size of the structure using a compile-time assertion.  This check
// will need to be updated if fields are added to the structure
//
CASSERT(sizeof(tx_frame_info_t) == 56, tx_frame_info_alignment_check);

//-----------------------------------------------
// RX frame information
//     - Defines the information passed in the packet buffer between CPU High and
//       CPU Low as part of receiving packets.
//     - The struct is padded to give space for the PHY to fill in channel estimates.
//
//     IMPORTANT:  This structure must be 8-byte aligned.
//
typedef struct __attribute__ ((__packed__)){
    u8                       	  	flags;                        ///< Bit flags
    u8                       	  	ant_mode;                     ///< Rx antenna selection
    s8                       	 	rx_power;                     ///< Rx power, in dBm
    u8                       	 	rx_gain_index;                ///< Rx gain index - interpretation is radio-specific
    u8                       	  	channel;                      ///< Channel index
    volatile rx_pkt_buf_state_t     rx_pkt_buf_state;             ///< State of the Rx Packet Buffer
    u16                       	  	reserved0;
    //----- 8-byte boundary ------
    u32                      	  	cfo_est;                      ///< Carrier Frequency Offset Estimate
    u32							  	reserved1;
    //----- 8-byte boundary ------
    phy_rx_details_t         	  	phy_details;                  ///< Details from PHY used in this reception
    //----- 8-byte boundary ------
    u8                       	  	timestamp_frac;               ///< Fractional timestamp beyond usec timestamp for time of reception
    u8                       	  	phy_samp_rate;                ///< PHY Sampling Rate
    u8                       	  	reserved2[2];                 ///< Reserved bytes for alignment
    u32                      	  	additional_info;              ///< Field to hold MAC-specific info, such as a pointer to a station_info
    //----- 8-byte boundary ------
    wlan_mac_low_tx_details_t     	resp_low_tx_details;			///< Tx Low Details for a control resonse (e.g. ACK or CTS)
    u32								reserved3;
    //----- 8-byte boundary ------
    u64                      	  	timestamp;                    ///< MAC timestamp at time of reception
    //----- 8-byte boundary ------
    u32                      	  	channel_est[64];              ///< Rx PHY channel estimates
} rx_frame_info_t;
// The above structure assumes that pkt_buf_state_t is a u8.  However, that is architecture dependent.
// Therefore, the code will check the size of the structure using a compile-time assertion.  This check
// will need to be updated if fields are added to the structure
//
CASSERT(sizeof(rx_frame_info_t) == 344, rx_frame_info_alignment_check);

//-----------------------------------------------
// Platform
#define WLAN_MAC_FPGA_DNA_LEN         2

typedef struct {
	const char*	serial_number_prefix;
    u32  		serial_number;
    u32  		fpga_dna[WLAN_MAC_FPGA_DNA_LEN];
    u8   		hw_addr_wlan[MAC_ADDR_LEN];
    u8   		hw_addr_wlan_exp[MAC_ADDR_LEN];
} wlan_mac_hw_info_t;

typedef enum {
	USERIO_DISP_STATUS_IDENTIFY     		= 0,
	USERIO_DISP_STATUS_APPLICATION_ROLE     = 1,
	USERIO_DISP_STATUS_MEMBER_LIST_UPDATE   = 2,
	USERIO_DISP_STATUS_WLAN_EXP_CONFIGURE   = 3,
	USERIO_DISP_STATUS_GOOD_FCS_EVENT       = 4,
	USERIO_DISP_STATUS_BAD_FCS_EVENT        = 5,
	USERIO_DISP_STATUS_CPU_ERROR    		= 255
} userio_disp_status_t;

typedef enum {
	USERIO_INPUT_MASK_PB_0		= 0x00000001,
	USERIO_INPUT_MASK_PB_1		= 0x00000002,
	USERIO_INPUT_MASK_PB_2		= 0x00000004,
	USERIO_INPUT_MASK_PB_3		= 0x00000008,
	USERIO_INPUT_MASK_SW_0		= 0x00000010,
	USERIO_INPUT_MASK_SW_1		= 0x00000020,
	USERIO_INPUT_MASK_SW_2		= 0x00000040,
	USERIO_INPUT_MASK_SW_3		= 0x00000080,
} userio_input_mask_t;

//---------------------------------------
// Platform information struct
typedef struct{
	u32	platform_id;
	u32 cpu_id;
	u32 is_cpu_high;
	u32 is_cpu_low;
	u32 mailbox_dev_id;
	u32	pkt_buf_mutex_dev_id;
	u32	tx_pkt_buf_baseaddr;
	u32	rx_pkt_buf_baseaddr;
} platform_common_dev_info_t;


#endif /* WLAN_MAC_COMMON_TYPES_H_ */
