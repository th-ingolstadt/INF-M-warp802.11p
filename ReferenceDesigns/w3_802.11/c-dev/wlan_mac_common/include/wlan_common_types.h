#ifndef WLAN_COMMON_TYPES_H_
#define WLAN_COMMON_TYPES_H_

#include "xil_types.h"

//-----------------------------------------------
// Compile-time assertion defines
//
#define CASSERT(predicate, file)                           _impl_CASSERT_LINE(predicate, __LINE__, file)

#define _impl_PASTE(a, b)                                  a##b
#define _impl_CASSERT_LINE(predicate, line, file) \
    typedef char _impl_PASTE(assertion_failed_##file##_, line)[2*!!(predicate)-1];




//-----------------------------------------------
// Generic function pointer
//
typedef int (*function_ptr_t)();


//-----------------------------------------------
// Field size defines
//
#define MAC_ADDR_LEN	6	///< MAC Address Length (in bytes)
#define SSID_LEN_MAX	32	///< Maximum SSID length
#define WLAN_MAC_FPGA_DNA_LEN         2
#define FPGA_DNA_LEN                  2 //FIXME: Why are there two _DNA_LEN defines?

#define MAX_PKT_SIZE_KB									   2
#define MAX_PKT_SIZE_B									   (MAX_PKT_SIZE_KB << 10)




//-----------------------------------------------
// Compilation Details
//
typedef struct __attribute__((__packed__)){
	char	compilation_date[12]; // Must be at least 12 bytes.
    char	compilation_time[12];  // Must be at least 9 bytes. Unfortunately, wlan_exp places an additional requirement that each field in
    							  // wlan_exp_node_info must be u32 aligned, so we increase the size to 12 bytes.
} compilation_details_t;
CASSERT(sizeof(compilation_details_t) == 24, compilation_details_t_alignment_check);




//-----------------------------------------------
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

typedef enum {
	USERIO_DISP_STATUS_IDENTIFY     		= 0,
	USERIO_DISP_STATUS_APPLICATION_ROLE     = 1,
	USERIO_DISP_STATUS_MEMBER_LIST_UPDATE   = 2,
	USERIO_DISP_STATUS_WLAN_EXP_CONFIGURE   = 3,
	USERIO_DISP_STATUS_GOOD_FCS_EVENT       = 4,
	USERIO_DISP_STATUS_BAD_FCS_EVENT        = 5,
	USERIO_DISP_STATUS_CPU_ERROR    		= 255
} userio_disp_status_t;

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



//////FIXME//////
// Temporary elevation of these types. Need to return to
// wlan_mac_pkt_buf_util.h once tx_frame_into_t is out of
// tx_queue_buffer_t

typedef struct {
    u8                      id;                     	  ///< ID of the Queue
    pkt_buf_group_t         pkt_buf_group;                ///< Packet Buffer Group
    u16                     occupancy;                    ///< Number of elements in the queue when the                                                         ///<   packet was enqueued (including itself)
} tx_queue_details_t;
CASSERT(sizeof(tx_queue_details_t) == 4, tx_queue_details_t_alignment_check);

typedef enum __attribute__ ((__packed__)) {
   TX_PKT_BUF_UNINITIALIZED   = 0,
   TX_PKT_BUF_HIGH_CTRL       = 1,
   TX_PKT_BUF_READY           = 2,
   TX_PKT_BUF_LOW_CTRL        = 3,
   TX_PKT_BUF_DONE            = 4
} tx_pkt_buf_state_t;

//-----------------------------------------------
// TX frame information
//     - Defines the information passed in the packet buffer between CPU High and
//       CPU Low as part of transmitting packets.
//
//     IMPORTANT:  This structure must be 8-byte aligned.
//
typedef struct tx_frame_info_t{
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

#define PHY_TX_PKT_BUF_PHY_HDR_SIZE                        0x10

//////FIXME//////

//-----------------------------------------------
// Doubly-Linked List
//

typedef struct dl_entry dl_entry;

struct dl_entry{
    dl_entry* next;
    dl_entry* prev;
    void*     data;
};

//Forward declaration of dl_entry
typedef struct dl_list{
    dl_entry* first;
    dl_entry* last;
    u32       length;
} dl_list;

#endif /* WLAN_COMMON_TYPES_H_ */
