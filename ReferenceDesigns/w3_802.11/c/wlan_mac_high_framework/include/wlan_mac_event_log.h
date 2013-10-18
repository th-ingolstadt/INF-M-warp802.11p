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
#ifndef WLAN_MAC_EVENT_LOG_H_
#define WLAN_MAC_EVENT_LOG_H_



// ****************************************************************************
// Define Event Log Constants
//


// Define to enable event logging at compile time
//   - If '1' then events will be logged
//   - If '0' then no events will be logged
//
#define ENABLE_EVENT_LOGGING           1


// NOTE: rx_event, tx_event, error_event must be the same size and begin with a timestamp and event_type
#define EVENT_SIZE                     (sizeof(rx_event))


// Event Types
#define EVENT_TYPE_RX                  1
#define EVENT_TYPE_TX                  2
#define EVENT_TYPE_ERR                 3




/*********************** Global Structure Definitions ************************/

typedef struct{
	u64 timestamp;
	u16 event_type;
	u16 event_length;
	u8 reserved[12];
} default_event;


typedef struct{
	u64 timestamp;
	u16 event_type;
	u16 event_length;
	u8 state;
	u8 AID;
	char power;
	u8 rate;
	u16 length;
	u16 seq;
	u8 mac_type;
	u8 flags;
	u8 reserved[2];
} rx_event;


typedef struct{
	u64 timestamp;
	u16 event_type;
	u16 event_length;
	u8 state;
	u8 AID;
	char power;
	u8 rate;
	u16 length;
	u16 seq;
	u8 mac_type;
	u8 retry_count;
	u8 reserved[2];
} tx_event;


typedef struct{
	u64 timestamp;
	u16 event_type;
	u16 event_length;
	u8 reserved[12];
} error_event;



/*************************** Function Prototypes *****************************/

void      event_log_init( char * start_address, u32 size );

void      event_log_reset();

int       event_log_delete( u32 size );

u32       event_log_get_data( u32 start_address, u32 size, char * buffer );
u32       event_log_get_size( void );
u32       event_log_get_head_address( void );
u32       event_log_get_address_from_head( u32 size );

rx_event* get_next_empty_rx_event();
tx_event* get_next_empty_tx_event();

void      print_event( u32 event_number, default_event * event );
void      print_event_log();
void      print_event_log_size();


#endif /* WLAN_MAC_EVENT_LOG_H_ */
