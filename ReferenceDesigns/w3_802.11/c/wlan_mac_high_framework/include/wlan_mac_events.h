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

#define WLAN_MAC_EVENTS_LOG_CHAN_EST

// ****************************************************************************
// Define Event Constants
//

// Event Types
#define EVENT_TYPE_RX_OFDM             1
#define EVENT_TYPE_RX_DSSS             2
#define EVENT_TYPE_TX                  3
#define EVENT_TYPE_BAD_FCS_RX          4




/*********************** Global Structure Definitions ************************/

//-----------------------------------------------
// Receive OFDM Event
//
typedef struct{
	u8   state;
	u8   AID;
	s8   power;
	u8   rate;
	u16  length;
	u16  seq;
	u8   mac_type;
	u8   flags;
	u8   rf_gain;
	u8   bb_gain;
#ifdef WLAN_MAC_EVENTS_LOG_CHAN_EST
	u32	 channel_est[64];
#endif
} rx_ofdm_event;

typedef struct{
	u8   state;
	u8   AID;
	s8   power;
	u8   rate;
	u16  length;
	u16  seq;
	u8   mac_type;
	u8   flags;
	u8   rf_gain;
	u8   bb_gain;
} rx_dsss_event;


//-----------------------------------------------
// Transmit Event
//
typedef struct{
	u8   state;
	u8   AID;
	s8 	 power;
	u8   rate;
	u16  length;
	u16  seq;
	u64	 tx_mpdu_accept_timestamp;
	u64	 tx_mpdu_done_timestamp;
	u8   mac_type;
	u8   retry_count;
	u8   reserved[6];
} tx_event;


//-----------------------------------------------
// Bad FCS Event
//
typedef struct{
	u8  rate;
	u8  reserved;
	u16  length;
} bad_fcs_event;


/*************************** Function Prototypes *****************************/


//-----------------------------------------------
// Wrapper methods to get events
//
rx_ofdm_event* get_next_empty_rx_ofdm_event();
rx_dsss_event* get_next_empty_rx_dsss_event();
tx_event* get_next_empty_tx_event();
bad_fcs_event* get_next_empty_bad_fcs_event();


//-----------------------------------------------
// Print function for all events
//
void print_event( u32 event_number, u32 event_type, u32 timestamp, void * event );


#endif /* WLAN_MAC_EVENTS_H_ */
