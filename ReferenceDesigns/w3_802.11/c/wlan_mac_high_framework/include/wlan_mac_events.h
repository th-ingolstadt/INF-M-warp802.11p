////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_util.h
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

/***************************** Include Files *********************************/



/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_EVENTS_H_
#define WLAN_MAC_EVENTS_H_

#define WLAN_MAC_EVENTS_LOG_CHAN_EST

// ****************************************************************************
// Define Event Constants
//

// Event Types
#define EVENT_TYPE_RX                  1
#define EVENT_TYPE_TX                  2
#define EVENT_TYPE_ERR                 3




/*********************** Global Structure Definitions ************************/

//-----------------------------------------------
// Receive Event
//
typedef struct{
	u8   state;
	u8   AID;
	char power;
	u8   rate;
	u16  length;
	u16  seq;
	u8   mac_type;
	u8   flags;
	u8   reserved[2];
#ifdef WLAN_MAC_EVENTS_LOG_CHAN_EST
	u32	 channel_est[64];
#endif
} rx_event;


//-----------------------------------------------
// Transmit Event
//
typedef struct{
	u8   state;
	u8   AID;
	char power;
	u8   rate;
	u16  length;
	u16  seq;
	u8   mac_type;
	u8   retry_count;
	u8   reserved[2];
} tx_event;


//-----------------------------------------------
// Error Event
//
typedef struct{
	u8 reserved[4];
} error_event;


/*************************** Function Prototypes *****************************/


//-----------------------------------------------
// Wrapper methods to get events
//
rx_event* get_next_empty_rx_event();
tx_event* get_next_empty_tx_event();


//-----------------------------------------------
// Print function for all events
//
void print_event( u32 event_number, u32 event_type, u32 timestamp, void * event );


#endif /* WLAN_MAC_EVENTS_H_ */
