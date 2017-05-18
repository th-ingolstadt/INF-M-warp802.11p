#ifndef WLAN_MAC_HIGH_TYPES_H_
#define WLAN_MAC_HIGH_TYPES_H_

#include "wlan_mac_common_types.h"

//-----------------------------------------------
// General High Framework Defines
//
//FIXME: wlan_exp should be updated to use this enum to report node type
typedef enum {
	APPLICATION_ROLE_AP			= 1,
	APPLICATION_ROLE_STA		= 2,
	APPLICATION_ROLE_IBSS		= 3,
	APPLICATION_ROLE_UNKNOWN	= 0xFF
} application_role_t;

typedef enum {INTERRUPTS_DISABLED, INTERRUPTS_ENABLED} interrupt_state_t;

//-----------------------------------------------
// Packet Types
//

typedef struct{
	u8* address_1;
	u8* address_2;
	u8* address_3;
	u8 frag_num;
	u8 reserved;
} mac_header_80211_common;

typedef struct{
	u16 auth_algorithm;
	u16 auth_sequence;
	u16 status_code;
} authentication_frame;

typedef struct{
	u16 reason_code;
} deauthentication_frame;

typedef struct{
	u16 capabilities;
	u16 status_code;
	u16 association_id;
} association_response_frame;

typedef struct{
	u16 capabilities;
	u16 listen_interval;
} association_request_frame;

typedef struct{
	u8 category;
	u8 action;

	// Channel Switch Announcement Element (Section 8.4.2.21)
	u8 element_id;                 // Set to 37 (Table 8-54 - Section 8.4.2.1)
	u8 length;                     // Set to 3
	u8 chan_switch_mode;           // Set to 0 - No restrictions on transmission until a channel switch
	u8 new_chan_num;
	u8 chan_switch_count;          // Set to 0 - Switch occurs any time after the frame is transmitted

} channel_switch_announcement_frame;

typedef struct{
	u8 category;
	u8 action;
	u8 dialog_token;
	u8 element_id;
	u8 length;
	u8 measurement_token;
	u8 request_mode;
	u8 measurement_type;
	///Note, technically measurement action frames can follow this comment with additional fields of unknown length
	///But currently, the three types of measurements are all the same so for ease we'll hardcode that structure here
	u8 channel;
	u8 start_time[8];
	u8 duration[2];
} measurement_common_frame;


//-----------------------------------------------
// Network Info
//


typedef enum __attribute__((__packed__)) {
	CHAN_TYPE_BW20 = 0,
	CHAN_TYPE_BW40_SEC_BELOW = 1,
	CHAN_TYPE_BW40_SEC_ABOVE = 2
} chan_type_t;
// Note: must align with definition in Python, so assert must be used
CASSERT(sizeof(chan_type_t) == 1, chan_type_t_alignment_check);

typedef struct __attribute__((__packed__)){
	u8             chan_pri;
	chan_type_t    chan_type;
} chan_spec_t;
CASSERT(sizeof(chan_spec_t) == 2, chan_spec_t_alignment_check);

#define NETWORK_INFO_COMMON_FIELDS              \
		bss_config_t bss_config;				\
		u32     flags;							\
		u32		capabilities;					\
		u64     latest_beacon_rx_time;			\
		s8      latest_beacon_rx_power;			\
		u8		padding1[3];					\


typedef struct __attribute__((__packed__)){
    u8              bssid[MAC_ADDR_LEN];               /* BSS ID - 48 bit HW address */
    chan_spec_t     chan_spec;                         /* Channel Specification */
    //----- 4-byte boundary ------
    char            ssid[SSID_LEN_MAX + 1];            /* SSID of the BSS - 33 bytes */
    u8              ht_capable;                        /* Support HTMF Tx/Rx */
    u16             beacon_interval;                   /* Beacon interval - In time units of 1024 us */
    //----- 4-byte boundary ------
    u8				dtim_period;					   /* DTIM Period (in beacon intervals) */
    u8				padding[3];
    //----- 4-byte boundary ------
} bss_config_t;
CASSERT(sizeof(bss_config_t) == 48, bss_config_t_alignment_check);

typedef struct __attribute__((__packed__)){
	NETWORK_INFO_COMMON_FIELDS
    dl_list members;
} network_info_t;
CASSERT(sizeof(network_info_t) == 80, network_info_t_alignment_check);

//Define a new type of dl_entry for pointing to network_info_t
// structs that contains some extra fields for faster searching
// without needing to jump to DRAM.
typedef struct network_info_entry_t network_info_entry_t;
struct network_info_entry_t{
	network_info_entry_t* next;
	network_info_entry_t* prev;
	network_info_t*       data;
	u8			    	  bssid[6];
	u16			          padding;
};
CASSERT(sizeof(network_info_entry_t) == 20, network_info_entry_t_alignment_check);

//-----------------------------------------------
// Address Whitelist
//
typedef struct {

	u8   mask[MAC_ADDR_LEN];
	u8   compare[MAC_ADDR_LEN];

} whitelist_range;


//-----------------------------------------------
// Ethernet
//

#define ETH_ADDR_SIZE                                      6                   // Length of Ethernet MAC address (in bytes)
#define IP_ADDR_SIZE                                       4                   // Length of IP address (in bytes)

typedef struct{
	u8  op;
	u8  htype;
	u8  hlen;
	u8  hops;
	u32 xid;
	u16 secs;
	u16 flags;
	u8  ciaddr[4];
	u8  yiaddr[4];
	u8  siaddr[4];
	u8  giaddr[4];
	u8  chaddr[MAC_ADDR_LEN];
	u8  chaddr_padding[10];
	u8  padding[192];
	u32 magic_cookie;
} dhcp_packet;

typedef struct {
    u8                       dest_mac_addr[ETH_ADDR_SIZE];                      // Destination MAC address
    u8                       src_mac_addr[ETH_ADDR_SIZE];                       // Source MAC address
    u16                      ethertype;                                        // EtherType
} ethernet_header_t;

typedef struct {
    u8                       version_ihl;                                      // [7:4] Version; [3:0] Internet Header Length
    u8                       dscp_ecn;                                         // [7:2] Differentiated Services Code Point; [1:0] Explicit Congestion Notification
    u16                      total_length;                                     // Total Length (includes header and data - in bytes)
    u16                      identification;                                   // Identification
    u16                      fragment_offset;                                  // [15:14] Flags;   [13:0] Fragment offset
    u8                       ttl;                                              // Time To Live
    u8                       protocol;                                         // Protocol
    u16                      header_checksum;                                  // IP header checksum
    u32                      src_ip_addr;                                      // Source IP address (big endian)
    u32                      dest_ip_addr;                                     // Destination IP address (big endian)
} ipv4_header_t;

typedef struct {
    u16                      htype;                                            // Hardware Type
    u16                      ptype;                                            // Protocol Type
    u8                       hlen;                                             // Length of Hardware address
    u8                       plen;                                             // Length of Protocol address
    u16                      oper;                                             // Operation
    u8                       sender_haddr[ETH_ADDR_SIZE];                       // Sender hardware address
    u8                       sender_paddr[IP_ADDR_SIZE];                        // Sender protocol address
    u8                       target_haddr[ETH_ADDR_SIZE];                       // Target hardware address
    u8                       target_paddr[IP_ADDR_SIZE];                        // Target protocol address
} arp_ipv4_packet_t;

typedef struct {
    u16                      src_port;                                         // Source port number
    u16                      dest_port;                                        // Destination port number
    u16                      length;                                           // Length of UDP header and UDP data (in bytes)
    u16                      checksum;                                         // Checksum
} udp_header_t;

//-----------------------------------------------
// Local Traffic Generation
//

//In spirit, tg_schedule is derived from dl_entry. Since C
//lacks a formal notion of inheritance, we adopt a popular
//alternative idiom for inheritance where the dl_entry
//is the first entry in the new structure. Since structures
//will never be padded before their first entry, it is safe
//to cast back and forth between the tg_schedule and dl_entry.
typedef struct tg_schedule tg_schedule;
struct tg_schedule{
	u32 id;
	u32 type;
	u64 target;
	u64	stop_target;
	void* params;
	void* callback_arg;
	function_ptr_t cleanup_callback;
	void* state;
};

//LTG Schedules

#define LTG_DURATION_FOREVER 0

typedef struct {
	u8  enabled;
	u8  reserved[3];
	u64 start_timestamp;
	u64 stop_timestamp;
} ltg_sched_state_hdr;

typedef struct {
	u32 interval_count;
	u64 duration_count;
} ltg_sched_periodic_params;

typedef struct {
	ltg_sched_state_hdr hdr;
	u32 time_to_next_count;
} ltg_sched_periodic_state;

typedef struct {
	u32 min_interval_count;
	u32 max_interval_count;
	u64 duration_count;
} ltg_sched_uniform_rand_params;

typedef struct {
	ltg_sched_state_hdr hdr;
	u32 time_to_next_count;
} ltg_sched_uniform_rand_state;

//LTG Payload Profiles

typedef struct {
	u32 type;
} ltg_pyld_hdr;

typedef struct {
	ltg_pyld_hdr hdr;
	u8  addr_da[MAC_ADDR_LEN];
	u16 length;
} ltg_pyld_fixed;

typedef struct {
	ltg_pyld_hdr hdr;
	u16 length;
	u16 padding;
} ltg_pyld_all_assoc_fixed;

typedef struct {
	ltg_pyld_hdr hdr;
	u8  addr_da[MAC_ADDR_LEN];
	u16 min_length;
	u16 max_length;
	u16 padding;
} ltg_pyld_uniform_rand;


//-----------------------------------------------
// Station Info
//

typedef struct{
    u64        rx_num_bytes;                ///< # of successfully received bytes (de-duplicated)
    u64        rx_num_bytes_total;          ///< # of successfully received bytes (including duplicates)
    u64        tx_num_bytes_success;        ///< # of successfully transmitted bytes (high-level MPDUs)
    u64        tx_num_bytes_total;          ///< Total # of transmitted bytes (high-level MPDUs)
    u32        rx_num_packets;              ///< # of successfully received packets (de-duplicated)
    u32        rx_num_packets_total;        ///< # of successfully received packets (including duplicates)
    u32        tx_num_packets_success;      ///< # of successfully transmitted packets (high-level MPDUs)
    u32        tx_num_packets_total;        ///< Total # of transmitted packets (high-level MPDUs)
    u64        tx_num_attempts;             ///< # of low-level attempts (including retransmissions)
} txrx_counts_sub_t;
CASSERT(sizeof(txrx_counts_sub_t) == 56,txrx_counts_sub_alignment_check);


#define STATION_TXRX_COUNTS_COMMON_FIELDS                                                                  			\
		txrx_counts_sub_t   data;                          /* Counts about data types	*/							\
		 /*----- 8-byte boundary ------*/																			\
		txrx_counts_sub_t   mgmt;                          /* Counts about management types */						\
		 /*----- 8-byte boundary ------*/																			\


typedef struct{
	STATION_TXRX_COUNTS_COMMON_FIELDS
} station_txrx_counts_t;

CASSERT(sizeof(station_txrx_counts_t) == 112, station_txrx_counts_alignment_check);

typedef struct{
    u16 rate_selection_scheme;
    u8	reserved[6];
} rate_selection_info_t;

#define STATION_INFO_HOSTNAME_MAXLEN                       19

#define STATION_INFO_COMMON_FIELDS                                                                                          \
        u8                 addr[MAC_ADDR_LEN];                         /* HW Address */                                     \
        u16                ID;                                         /* Identification Index for this station */          \
        char               hostname[STATION_INFO_HOSTNAME_MAXLEN+1];   /* Hostname from DHCP requests */                    \
        u32                flags;                                      /* 1-bit flags */                                    \
        u64                latest_rx_timestamp;               		   /* Timestamp of most recent reception */   		    \
		u64                latest_txrx_timestamp;               	   /* Timestamp of most recent reception or transmission */    \
        u16                latest_rx_seq;                              /* Sequence number of the last MPDU reception */     \
        u8                 reserved0[6];                                                                                    \
        tx_params_t        tx;                                         /* Transmission Parameters Structure */


typedef struct{
    STATION_INFO_COMMON_FIELDS
#if WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS
    station_txrx_counts_t		txrx_counts;                        			/* Tx/Rx Counts */
#endif
    rate_selection_info_t		rate_info;
    u32 						num_queued_packets;
} station_info_t;
#if WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS
CASSERT(sizeof(station_info_t) == 188, station_info_alignment_check);
#else
CASSERT(sizeof(station_info_t) == 76, station_info_alignment_check);
#endif



//Define a new type of dl_entry for pointing to station_info_t
// structs that contains some extra fields for faster searching
// without needing to jump to DRAM.
typedef struct station_info_entry_t station_info_entry_t;
struct station_info_entry_t{
	station_info_entry_t* next;
	station_info_entry_t* prev;
	station_info_t*     data;
	u8				    addr[6];
	u16					id;
};
CASSERT(sizeof(station_info_entry_t) == 20, station_info_entry_t_alignment_check);


//-----------------------------------------------
// Tx Queue
//

typedef struct{
	u8    metadata_type;
	u8    reserved[3];
	u32   metadata_ptr;
} tx_queue_metadata_t;

typedef struct{
	tx_queue_metadata_t   metadata;
	station_info_t*		  station_info;
	dl_entry*			  tx_queue_entry;
	tx_frame_info_t       tx_frame_info;
	u8                    phy_hdr_pad[PHY_TX_PKT_BUF_PHY_HDR_SIZE];
	u8                    frame[MAX_PKT_SIZE_B];
} tx_queue_buffer_t;


//-----------------------------------------------
// Scanning
//

typedef struct {
    u32       time_per_channel_usec;
    u32       probe_tx_interval_usec;
    u8*       channel_vec;
    u32       channel_vec_len;
    char*     ssid;
} scan_parameters_t;

typedef enum {
    SCAN_IDLE,
    SCAN_RUNNING,
    SCAN_PAUSED
} scan_state_t;


//-----------------------------------------------
// Event Scheduler
//

typedef struct {
    u32            id;
    u8			   enabled;
    u32            delay_us;
    u32            num_calls;
    u64            target_us;
    function_ptr_t callback;
} wlan_sched;

typedef struct {
	dl_list		enabled_list;
	dl_entry*	next;
} wlan_sched_state_t;


//---------------------------------------
// Platform information struct
//
typedef struct{
	u32		dlmb_baseaddr;
	u32		dlmb_size;
	u32		ilmb_baseaddr;
	u32		ilmb_size;
	u32		aux_bram_baseaddr;
	u32		aux_bram_size;
	u32		dram_baseaddr;
	u32		dram_size;
	u32		intc_dev_id;
	u32		timer_dev_id;
	u32		timer_int_id;
	u32		timer_freq;
	u32		cdma_dev_id;
	u32 	mailbox_int_id;
	u32		wlan_exp_eth_mac_dev_id;
	u32		wlan_exp_eth_dma_dev_id;
	u32		wlan_exp_phy_addr;
} platform_high_dev_info_t;


#endif /* WLAN_MAC_HIGH_TYPES_H_ */
