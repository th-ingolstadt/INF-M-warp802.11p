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

//-----------------------------------------------
// Receive Entries

#define ENTRY_TYPE_RX_OFDM             10
#define ENTRY_TYPE_RX_DSSS             11

//-----------------------------------------------
// Transmit Entries

#define ENTRY_TYPE_TX_HIGH                  20

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
	u32     design_ver;           // WARPNet version
	u32     serial_number;        // Node serial number
	u64     fpga_dna;             // Node FPGA DNA
	u32     wlan_max_assn;        // Max associations of the node
	u32     wlan_event_log_size;  // Max size of the event log
	u32     wlan_max_stats;       // Max number of promiscuous statistics
} node_info_entry;


//-----------------------------------------------
// Experiment Info Entry
//
typedef struct{
	u64     timestamp;
	u16     reason;
	u16     length;
	u8    * msg;
} exp_info_entry;


//-----------------------------------------------
// Station Info Entry
//   NOTE:  rsvd field is to have a 32-bit aligned struct.  That way sizeof()
//          accurately reflects the number of bytes in the struct.  Unfortunately,
//          since the hostname is uses a #define for the length, we have to
//          use that to determine the size of the rsvd field.  It will be between
//          1 and 4 bytes.
//
typedef struct{
	u64     timestamp;                                  // Timestamp
	u8      addr[6];									// HW Address
	u8		hostname[STATION_INFO_HOSTNAME_MAXLEN+1]; 	// Hostname from DHCP requests
	u16     AID;										// Association ID
	u32		flags;										// 1-bit flags
	u8      rate;			                            // Rate of transmission
	u8      antenna_mode;	                            // Antenna mode (Placeholder)
	u8	    max_retry;                                  // Maximum number of retransmissions
	u8      rsvd[((STATION_INFO_HOSTNAME_MAXLEN+1)%4) + 1];
} station_info_entry;


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
// TxRx Statistics Entry
//   NOTE:  rsvd field is to have a 32-bit aligned struct.  That way sizeof()
//          accurately reflects the number of bytes in the struct.
//
typedef struct{
	u64     timestamp;      // Timestamp of the log entry
	u64     last_timestamp; // Timestamp of the last frame reception
	u8      addr[6];		// HW Address
	u8      is_associated;	// Is this device associated with me?
	u8      rsvd;
	u32     num_tx_total;	// Total number of transmissions to this device
	u32     num_tx_success; // Total number of successful transmissions to this device
	u32     num_retry;		// Total number of retransmissions to this device
	u32     mgmt_num_rx_success; // MGMT: Total number of successful receptions from this device
	u32     mgmt_num_rx_bytes;	// MGMT: Total number of received bytes from this device
	u32     data_num_rx_success; // DATA: Total number of successful receptions from this device
	u32     data_num_rx_bytes;	// DATA: Total number of received bytes from this device
} txrx_stats_entry;


//-----------------------------------------------
// Common Receive Entry
//   NOTE:  rsvd field is to have a 32-bit aligned struct.  That way sizeof()
//          accurately reflects the number of bytes in the struct.
//
typedef struct{
	u64  timestamp;
	mac_header_80211 mac_hdr;
	u16	 length;
	u8   rate;
	s8   power;
	u8	 fcs_status;
	u8 	 pkt_type;
	u8 	 chan_num;
	u8 	 ant_mode;
	u8   rf_gain;
	u8   bb_gain;
	u8   rsvd[2];
} rx_common_entry;

#define RX_ENTRY_FCS_GOOD 0
#define RX_ENTRY_FCS_BAD 1


//-----------------------------------------------
// Receive OFDM Entry
//
typedef struct{
	rx_common_entry rx_entry;
#ifdef WLAN_MAC_ENTRIES_LOG_CHAN_EST
	u32	 channel_est[64];
#endif
} rx_ofdm_entry;


//-----------------------------------------------
// Receive DSSS Entry
//
typedef struct{
	rx_common_entry rx_common_entry;
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
	mac_header_80211 mac_hdr;
	u8   retry_count;
	u8 	 gain_target;
	u8 	 chan_num;
	u8   rate;
	u16  length;
	u8 	 result;
	u8 	 pkt_type;
	u8	 ant_mode;
	u8	 rsvd[3];
} tx_high_entry;


/*************************** Function Prototypes *****************************/


//-----------------------------------------------
// Wrapper methods to get entries
//
exp_info_entry   * get_next_empty_exp_info_entry(u16 size);

rx_ofdm_entry    * get_next_empty_rx_ofdm_entry();
rx_dsss_entry    * get_next_empty_rx_dsss_entry();
station_info_entry* get_next_empty_station_info_entry();

tx_high_entry         * get_next_empty_tx_high_entry();


//-----------------------------------------------
// Print function for all entries
//
void print_entry( u32 entry_number, u32 entry_type, void * entry );


#endif /* WLAN_MAC_ENTRIES_H_ */
