#ifndef WLAN_COMMON_TYPES_H_
#define WLAN_COMMON_TYPES_H_

#include "xil_types.h"

// Define a compile-time  macros that act like assert
//  We use these primarily to check that struct sizes match corresponding sizes expected by wlan_exp
//  This check catches cases where unexpected packing/padding change alignment of struct fields
//  Inspired by https://stackoverflow.com/a/809465
#define CASSERT(test_cond, failure_msg) \
	typedef char CASSERT_FAILED_##failure_msg[2*!!(test_cond)-1];

#define ASSERT_TYPE_SIZE(check_type, req_size) \

    typedef char ASSERT_TYPE_SIZE_FAILED_##check_type##_neq_##req_size[2*!!(req_size == sizeof(check_type))-1]


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
ASSERT_TYPE_SIZE(compilation_details_t, 24);




//-----------------------------------------------
// TX parameters
//     - Be careful when modifying these structures, there are alignment concerns
//       for many of the structures that contain these structures.  In general,
//       tx_params_t should be 8-byte aligned.
//
typedef struct phy_tx_params_t{
    u8                       mcs;                          ///< MCS index
    u8                       phy_mode;                     ///< PHY mode selection and flags
    u8                       antenna_mode;                 ///< Tx antenna selection
    s8                       power;                        ///< Tx power (in dBm)
} phy_tx_params_t;

typedef struct mac_tx_params_t{
    u8                       flags;                        ///< Flags affecting waveform construction
    u8                       reserved[3];                  ///< Reserved for 32-bit alignment
} mac_tx_params_t;

typedef struct tx_params_t{
    phy_tx_params_t          phy;                          ///< PHY Tx params
    mac_tx_params_t          mac;                          ///< Lower-level MAC Tx params
} tx_params_t;

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
ASSERT_TYPE_SIZE(pkt_buf_group_t, 1);

typedef struct tx_queue_details_t{
    u8                      id;                     	  ///< ID of the Queue
    pkt_buf_group_t         pkt_buf_group;                ///< Packet Buffer Group
    u16                     occupancy;                    ///< Number of elements in the queue when the
    													  ///<   packet was enqueued (including itself)
    u64						enqueue_timestamp;
} tx_queue_details_t;
ASSERT_TYPE_SIZE(tx_queue_details_t, 12);

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
