/** @file wlan_mac_event_log.h
 *  @brief MAC Event Log Framework
 *
 *  Contains code for logging MAC events in DRAM.
 *
 *  @copyright Copyright 2014, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

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

#define MAX_EVENT_LOG -1 ///< Maximum number of events to store in log
						 ///< @note A maximum event length of -1 is used to signal that the entire DRAM after the queue
						 ///< should be used for logging events. If MAX_EVENT_LOG > 0, then that number of events
						 ///< will be the maximum.


// Define event wrapping enable / disable
#define EVENT_LOG_WRAP_ENABLE          1
#define EVENT_LOG_WRAP_DISABLE         2


// Define magic number that will indicate the start of an event
//   - Magic number was chose so that:
//       - It would not be in DDR address space
//       - It is human readable
//
#define EVENT_LOG_MAGIC_NUMBER         0xACED0000



/*********************** Global Structure Definitions ************************/

//-----------------------------------------------
// Log Entry Header
//   - This is used by the event log but is not exposed to the user
//
typedef struct{
	u32 entry_id;
	u16 entry_type;
	u16 entry_length;
} entry_header;



/*************************** Function Prototypes *****************************/

void      event_log_init( char * start_address, u32 size );

void      event_log_reset();

int       event_log_config_wrap( u32 enable );

u32       event_log_get_data( u32 start_address, u32 size, char * buffer );
u32       event_log_get_size( void );
u32       event_log_get_current_index( void );
u32       event_log_get_oldest_entry_index( void );
void *    event_log_get_next_empty_entry( u16 entry_type, u16 entry_size );

int       event_log_update_type( void * entry_ptr, u16 entry_type );

void      print_event_log( u32 num_events );
void      print_event_log_size();

void      add_node_info_entry();
u32       add_txrx_statistics_to_log();

void      wn_transmit_log_entry(void * entry);

#endif /* WLAN_MAC_EVENT_LOG_H_ */
