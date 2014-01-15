/** @file wlan_mac_events.h
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
 *  This is the only code that the user should modify in order to add events
 *  to the event log.  To add a new event, please follow the template provided
 *  and create:
 *    1) A new event type in wlan_mac_events.h
 *    2) Wrapper function:  get_next_empty_*_event()
 *    3) Update the print function so that it is easy to print the log to the
 *    terminal
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

/***************************** Include Files *********************************/



/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_EVENTS_H_
#define WLAN_MAC_EVENTS_H_

#include "wlan_mac_802_11_defs.h"

#define WLAN_MAC_EVENTS_LOG_CHAN_EST

// ****************************************************************************
// Define Event Constants
//

// Event Types

//-----------------------------------------------
// Management Events

#define EVENT_TYPE_LOG_INFO            1
#define EVENT_TYPE_EXP_INFO            2
#define EVENT_TYPE_STATISTICS          3

//-----------------------------------------------
// Receive Events

#define EVENT_TYPE_RX_OFDM             10
#define EVENT_TYPE_RX_DSSS             11

//-----------------------------------------------
// Transmit Events

#define EVENT_TYPE_TX                  20




/*********************** Global Structure Definitions ************************/

//-----------------------------------------------
// Log Info Event
//   NOTE:  This structure was designed to work easily with the WARPNet Tag
//       Parameters.  The order and size of the fields match the corresponding
//       Tag Parameter so that population of this structure is easy.
//
typedef struct{
	u32     node_type;                 // WARPNet Node type
	u32     node_id;                   // Node ID
	u32     node_hw_gen;               // WARP Hardware Generation
	u32     node_design_ver;           // WARPNet version
	u32     node_serial_number;        // Node serial number
	u64     node_fpga_dna;             // Node FPGA DNA
	u32     node_wlan_max_assn;        // Max associations of the node
	u32     node_wlan_event_log_size;  // Max size of the event log

	u32     transport_type;            // WARPNet Transport type
	u64     transport_hw_addr;         // WARP node MAC address
	u32     transport_ip_addr;         // WARP node IP address
	u32     transport_unicast_port;    // WARP node unicast port
	u32     transport_bcast_port;      // WARP node broadcast port
	u32     transport_grp_id;          // WARP node group id
} log_info_event;


//-----------------------------------------------
// Experiment Info Event
//
typedef struct{
	u64     timestamp;
	u16     reason;
	u16     length;
	u8    * msg;
} exp_info_event;


//-----------------------------------------------
// Statistics Event
//   NOTE:  rsvd field is to have a 32-bit aligned struct.  That way sizeof()
//          accurately reflects the number of bytes in the struct.
//
typedef struct{
	u64     last_timestamp; ///< Timestamp of the last frame reception
	u8      addr[6];		///< HW Address
	u8      is_associated;	///< Is this device associated with me?
	u8      rsvd;
	u32     num_tx_total;	///< Total number of transmissions to this device
	u32     num_tx_success; ///< Total number of successful transmissions to this device
	u32     num_retry;		///< Total number of retransmissions to this device
	u32     num_rx_success; ///< Total number of successful receptions from this device
	u32     num_rx_bytes;	///< Total number of received bytes from this device
} statistics_event;


//-----------------------------------------------
// Receive OFDM Event
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
#ifdef WLAN_MAC_EVENTS_LOG_CHAN_EST
	u32	 channel_est[64];
#endif
} rx_ofdm_event;


//-----------------------------------------------
// Receive DSSS Event
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
} rx_dsss_event;

#define RX_EVENT_FCS_GOOD 0
#define RX_EVENT_FCS_BAD 1


//-----------------------------------------------
// Transmit Event
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
} tx_event;


/*************************** Function Prototypes *****************************/


//-----------------------------------------------
// Wrapper methods to get events
//
log_info_event   * get_next_empty_log_info_event();
exp_info_event   * get_next_empty_exp_info_event(u16 size);
statistics_event * get_next_empty_statistics_event();

rx_ofdm_event    * get_next_empty_rx_ofdm_event();
rx_dsss_event    * get_next_empty_rx_dsss_event();

tx_event         * get_next_empty_tx_event();


//-----------------------------------------------
// Print function for all events
//
void print_entry( u32 entry_number, u32 entry_type, void * event );


#endif /* WLAN_MAC_EVENTS_H_ */
