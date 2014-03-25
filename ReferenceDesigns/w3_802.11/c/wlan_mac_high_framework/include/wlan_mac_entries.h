/** @file wlan_mac_entries.h
 *  @brief Event log
 *
 *  This contains the code for accessing event log.
 *
 *  @copyright Copyright 2014, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *	@note
 *  This is the only code that the user should modify in order to add entries
 *  to the event log.  To add a new entry, please follow the template provided
 *  and create:
 *    1) A new entry type in wlan_mac_entries.h
 *    2) Wrapper function:  get_next_empty_*_entry()
 *    3) Update the print function so that it is easy to print the log to the
 *    terminal
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

/***************************** Include Files *********************************/



/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_ENTRIES_H_
#define WLAN_MAC_ENTRIES_H_

#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_high.h"

#define WLAN_MAC_ENTRIES_LOG_CHAN_EST

// ****************************************************************************
// Define Entry Constants
//

// Entry Types

//-----------------------------------------------
// Management Entries

#define ENTRY_TYPE_NODE_INFO           1
#define ENTRY_TYPE_EXP_INFO            2
#define ENTRY_TYPE_STATION_INFO        3
#define ENTRY_TYPE_TEMPERATURE         4
#define ENTRY_TYPE_WN_CMD              5

//-----------------------------------------------
// Receive Entries

#define ENTRY_TYPE_RX_OFDM             10
#define ENTRY_TYPE_RX_DSSS             11

//-----------------------------------------------
// Transmit Entries

#define ENTRY_TYPE_TX_HIGH             20
#define ENTRY_TYPE_TX_LOW              21

//-----------------------------------------------
// Statistics Entries

#define ENTRY_TYPE_TXRX_STATS          30





/*********************** Global Structure Definitions ************************/

//-----------------------------------------------
// Node Info Entry
//   NOTE:  This structure was designed to work easily with the WARPNet Tag
//       Parameters.  The order and size of the fields match the corresponding
//       Tag Parameter so that population of this structure is easy.
//
typedef struct{
	u32     type;                 // WARPNet Node type
	u32     id;                   // Node ID
	u32     hw_gen;               // WARP Hardware Generation
	u32     wn_ver;               // WARPNet version
	u64     fpga_dna;             // Node FPGA DNA
	u32     serial_number;        // Node serial number
	u32     wlan_exp_ver;         // WLAN Exp version
	u32     wlan_max_assn;        // Max associations of the node
	u32     wlan_event_log_size;  // Max size of the event log
	u32     wlan_mac_addr[2];     // WLAN MAC Address
	u32     wlan_max_stats;       // Max number of promiscuous statistics
} node_info_entry;


//-----------------------------------------------
// Experiment Info Entry
//
typedef struct{
	u64     timestamp;
	u16     info_type;
	u16     length;
	u8    * msg;
} exp_info_entry;


//-----------------------------------------------
// Station Info Entry
typedef struct{
	u64     timestamp;                                  // Timestamp
	station_info_base info;								// Framework's station_info struct
} station_info_entry;
CASSERT(sizeof(station_info_entry) == 60, station_info_entry_alignment_check);

//-----------------------------------------------
// Temperature Entry
//   NOTE: The temperature values are copied directly from the system monitor and need
//         to be converted to Celsius:
//           celsius = ((double(temp)/65536.0)/0.00198421639) - 273.15;
//
typedef struct{
	u64     timestamp;       // Timestamp of the log entry
	u32     id;              // Node ID
	u32     serial_number;   // Node serial number
	u32     curr_temp;       // Current Temperature of the node
	u32     min_temp;        // Minimum recorded temperature of the node
	u32     max_temp;		 // Maximum recorded temperature of the node
} temperature_entry;


//-----------------------------------------------
// WARPNet Command Entry
//
typedef struct{
	u64     timestamp;      // Timestamp of the log entry
	u32     command;        // WARPNet command
	u16     src_id;         // Source ID of the command
	u16     num_args;       // Number of arguments
	u32     args[10];	    // Data from the arguments
} wn_cmd_entry;


//-----------------------------------------------
// TxRx Statistics Entry
//   NOTE:  rsvd field is to have a 32-bit aligned struct.  That way sizeof()
//          accurately reflects the number of bytes in the struct.
//
typedef struct{
	u64             timestamp;      // Timestamp of the log entry
	statistics_txrx stats;			// Framework's statistics struct
} txrx_stats_entry;


#define MIN_MAC_PAYLOAD_LOG_LEN 24

#define MAX_MAC_PAYLOAD_LOG_LEN MIN_MAC_PAYLOAD_LOG_LEN
//#define MAX_MAC_PAYLOAD_LOG_LEN 1500


//-----------------------------------------------
// Common Receive Entry
//   NOTE:  rsvd field is to have a 32-bit aligned struct.  That way sizeof()
//          accurately reflects the number of bytes in the struct.
//
typedef struct{
	u64  timestamp;
	u16	 length;
	u8   rate;
	s8   power;
	u8	 fcs_status;
	u8 	 pkt_type;
	u8 	 chan_num;
	u8 	 ant_mode;
	u8   rf_gain;
	u8   bb_gain;
	u16  flags; //TODO
} rx_common_entry;

#define RX_ENTRY_FCS_GOOD 0
#define RX_ENTRY_FCS_BAD 1

#define RX_ENTRY_FLAGS_IS_DUPLICATE	0x0001


//-----------------------------------------------
// Receive OFDM Entry
//
typedef struct{
	rx_common_entry rx_entry;
#ifdef WLAN_MAC_ENTRIES_LOG_CHAN_EST
	u32	 channel_est[64];
#endif
    u32 mac_payload_log_len; //number of payload bytes actually recorded in log entry
    u32 mac_payload[MIN_MAC_PAYLOAD_LOG_LEN/4]; //store as u32's to preserve alignment
} rx_ofdm_entry;


//-----------------------------------------------
// Receive DSSS Entry
//
typedef struct{
	rx_common_entry rx_common_entry;
	u32 mac_payload_log_len; //number of payload bytes actually recorded in log entry
	u32 mac_payload[MIN_MAC_PAYLOAD_LOG_LEN/4]; //store as u32's to preserve alignment
} rx_dsss_entry;


//-----------------------------------------------
// High-level Transmit Entry
//   NOTE:  rsvd field is to have a 32-bit aligned struct.  That way sizeof()
//          accurately reflects the number of bytes in the struct.
//
typedef struct{
	u64  timestamp_create;
	u32  delay_accept;
	u32  delay_done;
	u8   num_tx;
	s8 	 power;
	u8 	 chan_num;
	u8   rate;
	u16  length;
	u8 	 result;
	u8 	 pkt_type;
	u8	 ant_mode;
	u8	 rsvd[3];
	u32 mac_payload_log_len; //number of payload bytes actually recorded in log entry
	u32 mac_payload[MIN_MAC_PAYLOAD_LOG_LEN/4]; //store as u32's to preserve alignment
} tx_high_entry;

//-----------------------------------------------
// Low-level Transmit Entry
//   NOTE:  rsvd field is to have a 32-bit aligned struct.  That way sizeof()
//          accurately reflects the number of bytes in the struct.
//
typedef struct{
	u64  timestamp_send;
	phy_tx_params phy_params;
	u8	 transmission_count;
	u8 	 chan_num;
	u16  length;
	u16  num_slots;
	u8 	 pkt_type;
	u8	 reserved[1];
	u32 mac_payload_log_len; //number of payload bytes actually recorded in log entry
	u32 mac_payload[MIN_MAC_PAYLOAD_LOG_LEN/4]; //store as u32's to preserve alignment
} tx_low_entry;




/*************************** Function Prototypes *****************************/


//-----------------------------------------------
// Wrapper methods to get entries
//
exp_info_entry       * get_next_empty_exp_info_entry(u16 size);
wn_cmd_entry         * get_next_empty_wn_cmd_entry();

rx_ofdm_entry        * get_next_empty_rx_ofdm_entry(u32 payload_log_len);
rx_dsss_entry        * get_next_empty_rx_dsss_entry(u32 payload_log_len);
station_info_entry   * get_next_empty_station_info_entry();

tx_high_entry        * get_next_empty_tx_high_entry(u32 payload_log_len);
tx_low_entry         * get_next_empty_tx_low_entry();


//-----------------------------------------------
// Print function for all entries
//
void print_entry( u32 entry_number, u32 entry_type, void * entry );


#endif /* WLAN_MAC_ENTRIES_H_ */
