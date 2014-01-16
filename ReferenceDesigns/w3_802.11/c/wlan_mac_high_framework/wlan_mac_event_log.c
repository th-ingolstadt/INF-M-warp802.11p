/** @file wlan_mac_event_log.c
 *  @brief MAC Event Log Framework
 *
 *  Contains code for logging MAC events in DRAM.
 *
 *  @copyright Copyright 2014, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *	@note
 *	  The event log implements a circular buffer that will record various
 * event entries that occur within a WLAN node.  If the buffer is full, then
 * entries will be dropped with only a single warning printed to the screen.
 *    There are configuration options to enable / disable wrapping (ie if
 * wrapping is enabled, then the buffer is never "full" and the oldest
 * events will be overwritten when there is no more free space).  Wrapping
 * is disabled by default.
 *    Internally, the event log is just an array of bytes which can be externally
 * viewed as indexed from 0 to log_size (address translation is done internally).
 * When a new entry is requested, the size of the entry is allocated from the
 * buffer and a pointer to the allocated entry is provided so that the caller
 * can fill in the event information.  By default, the event log will set up all
 * header information (defined in wlan_mac_event_log.h) and that information
 * will not be exposed to user code.
 *    The event log will always provide a contiguous piece of memory for events.
 * Therefore, some space could be wasted at the wrap boundary since a single event
 * will never wrap.
 *    Also, if an entry cannot be allocated due to it overflowing the array, then
 * the event log will check to see if wrapping is enabled.  If wrapping is disabled,
 * the event log will set the full flag and not allow any more events to be
 * allocated.  Otherwise, the event log will wrap and begin to overwrite the
 * oldest entries.
 *   Finally, the log does not keep track of event entries and it is up to
 * calling functions to interpret the bytes within the log correctly.
 *
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 *  @bug No known bugs.
 */

/***************************** Include Files *********************************/

// SDK includes
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "xil_types.h"

// WLAN includes
#include "wlan_mac_event_log.h"
#include "wlan_mac_entries.h"
#include "wlan_mac_high.h"
#include "wlan_exp_node.h"

/*************************** Constant Definitions ****************************/



/*********************** Global Variable Definitions *************************/



/*************************** Variable Definitions ****************************/

// Log definition variables
static u32   log_start_address;        // Absolute start address of the log
static u32   log_soft_end_address;     // Soft end address of the log
static u32   log_max_address;          // Absolute end address of the log

static u32   log_size;                 // Size of the log in bytes

// Log index variables
static u32   log_head_address;         // Pointer to the oldest entry
static u32   log_curr_address;

// Log config variables
static u8    log_wrap_enabled;         // Will the log wrap or stop; By default wrapping is DISABLED

// Log status variables
static u8    log_empty;                // log_empty = (log_head_address == log_curr_address);
static u8    log_full;                 // log_full  = (log_tail_address == log_curr_address);
static u16   log_count;

// Variable to control if events are recorded in to the log
u8           enable_event_logging;

// Mutex for critical allocation loop
static u8    allocation_mutex;



/*************************** Functions Prototypes ****************************/

// Internal functions;  Should not be called externally
//
void            event_log_increment_head_address( u32 size );
int             event_log_get_next_empty_address( u32 size, u32 * address );


/******************************** Functions **********************************/




/*****************************************************************************/
/**
* Initialize the event log
*
* @param    start_address  - Starting address of the event log
*           size           - Size in bytes of the event log
*
* @return	None.
*
* @note		The event log will only use a integer number of event entries so
*           any bytes over an integer number will be unused.
*
******************************************************************************/
void event_log_init( char * start_address, u32 size ) {

	xil_printf("Initializing Event log (%d bytes) at 0x%x \n", size, start_address );

	// If defined, enable event logging
	if(ENABLE_EVENT_LOGGING) enable_event_logging = 1;

	// Set the global variables that describe the log
    log_size          = size;
	log_start_address = (u32) start_address;
	log_max_address   = log_start_address + log_size - 1;

	// Set wrapping to be disabled
	log_wrap_enabled  = 0;

	// Reset all the event log variables
	event_log_reset();

#ifdef _DEBUG_
	xil_printf("    log_size             = 0x%x;\n", log_size );
	xil_printf("    log_start_address    = 0x%x;\n", log_start_address );
	xil_printf("    log_max_address      = 0x%x;\n", log_max_address );
	xil_printf("    log_soft_end_address = 0x%x;\n", log_soft_end_address );
	xil_printf("    log_head_address     = 0x%x;\n", log_head_address );
	xil_printf("    log_curr_address     = 0x%x;\n", log_curr_address );
	xil_printf("    log_empty            = 0x%x;\n", log_empty );
	xil_printf("    log_full             = 0x%x;\n", log_full );
	xil_printf("    allocation_mutex     = 0x%x;\n", allocation_mutex );
#endif
}



/*****************************************************************************/
/**
* Reset the event log
*
* @param    None.
*
* @return	None.
*
* @note		This will not change the state of the wrapping configuration
*
******************************************************************************/
void event_log_reset(){
	log_soft_end_address = log_max_address;

	log_head_address     = log_start_address;
	log_curr_address     = log_start_address;

	log_empty            = 1;
	log_full             = 0;
	log_count            = 0;

	allocation_mutex     = 0;

	add_node_info_entry();
}



/*****************************************************************************/
/**
* Set the wrap configuration parameter
*
* @param    enable    - Is wrapping enabled?
*                           EVENT_LOG_WRAP_ENABLE
*                           EVENT_LOG_WRAP_DISABLE
*
* @return	status    - SUCCESS = 0
*                       FAILURE = -1
*
* @note		None.
*
******************************************************************************/
int       event_log_config_wrap( u32 enable ) {
	int ret_val = 0;

	switch ( enable ) {
	    case EVENT_LOG_WRAP_ENABLE:
	    	log_wrap_enabled  = 1;
	        break;

	    case EVENT_LOG_WRAP_DISABLE:
	    	log_wrap_enabled  = 0;
	        break;

        default:
        	ret_val = -1;
            break;
	}

	return ret_val;
}



/*****************************************************************************/
/**
* Get event log data
*   Based on the start address and the size, the function will fill in the
* appropriate number of bytes in to the buffer.  It is up to the caller
* to determine if the bytes are "valid".
*
* @param    start_address    - Address in the event log to start the transfer
*                                (ie byte index from 0 to log_size)
*           size             - Size in bytes of the buffer
*           buffer           - Pointer to the buffer to be filled in with event data
*                                (buffer must be pre-allocated and be at least size bytes)
*
* @return	num_bytes        - The number of bytes filled in to the buffer
*
* @note		Any requests for data that is out of bounds will print a warning and
*           return 0 bytes.  If a request exceeds the size of the array, then
*           the request will be truncated.
*
******************************************************************************/
u32  event_log_get_data( u32 start_address, u32 size, char * buffer ) {

	u64 end_address;
	u32 num_bytes     = 0;

	// If the log is empty, then return 0
    if ( log_empty == 1 ) { return num_bytes; }

    // Check that the start_address is less than the log_size
    if ( start_address > log_size ) {
    	xil_printf("WARNING:  EVENT LOG - Index out of bounds\n");
    	xil_printf("          Data request from %d when the log only has %d bytes\n", start_address, log_size);
    	return num_bytes;
    }

    // Translate the start address from an index to the actual memory location
    start_address = start_address + log_start_address;

    // Compute the end address for validity checks
    end_address = log_start_address + start_address + size;

    // Check that the end address is less than the end of the buffer
    if ( end_address > log_soft_end_address ) {
    	num_bytes = log_soft_end_address - start_address;
    } else {
    	num_bytes = size;
    }

	// Copy the data in to the buffer
	memcpy( (void *) buffer, (void *) start_address, num_bytes );

    return num_bytes;
}



/*****************************************************************************/
/**
* Get the size of the log in bytes
*
* @param    None.
*
* @return	size    - Size of the log in bytes
*
* @note		None.
*
******************************************************************************/
u32  event_log_get_size( void ) {
	u32 size;

	// Implemented this way b/c we are using unsigned integers, so we always need
	//   to have positive integers at each point in the calculation
	if ( log_curr_address >= log_head_address ) {
		size = log_curr_address - log_head_address;
	} else {
		size = log_size - ( log_head_address - log_curr_address );
	}

#ifdef _DEBUG_
	xil_printf("Event Log:  size             = 0x%x\n", size );
	xil_printf("Event Log:  log_curr_address = 0x%x\n", log_curr_address );
	xil_printf("Event Log:  log_head_address = 0x%x\n", log_head_address );
#endif

	return size;
}



/*****************************************************************************/
/**
* Get the address of the current write pointer
*
* @param    None.
*
* @return	u32    - Index of the event log of the current write pointer
*
* @note		None.
*
******************************************************************************/
u32  event_log_get_current_index( void ) {
    return ( log_curr_address - log_start_address );
}



/*****************************************************************************/
/**
* Get the index of the oldest entry
*
* @param    None.
*
* @return	u32    - Index of the event log of the oldest entry
*
* @note		None.
*
******************************************************************************/
u32  event_log_get_oldest_entry_index( void ) {
    return ( log_head_address - log_start_address );
}



/*****************************************************************************/
/**
* Update the entry type
*
* @param    entry_ptr   - Pointer to entry contents
*           entry_type  - Value to update entry_type field
*
* @return	u32         -  0 - Success
*                         -1 - Failure
*
* @note		None.
*
******************************************************************************/
int       event_log_update_type( void * entry_ptr, u16 entry_type ) {
    int            return_value = -1;
    entry_header * entry_hdr;

    // If the entry_ptr is within the event log, then update the type field of the entry
    if ( ( ((u32) entry_ptr) > log_start_address ) && ( ((u32) entry_ptr) < log_max_address ) ) {

    	entry_hdr = (entry_header *) ( ((u32) entry_ptr) - sizeof( entry_header ) );

    	// Check to see if the entry has a valid magic number
    	if ( ( entry_hdr->entry_id & 0xFFFF0000 ) == EVENT_LOG_MAGIC_NUMBER ) {

        	entry_hdr->entry_type = entry_type;

        	return_value = 0;
    	} else {
    		xil_printf("WARNING:  event_log_update_type() - entry_ptr (0x%8x) is not valid \n", entry_ptr );
    	}
    } else {
		xil_printf("WARNING:  event_log_update_type() - entry_ptr (0x%8x) is not in event log \n", entry_ptr );
    }

    return return_value;
}



/*****************************************************************************/
/**
* Increment the head address
*
* @param    size        - Number of bytes to increment the head address
*
* @return	num_bytes   - Number of bytes that the head address moved
*
* @note		This function will blindly increment the head address by 'size'
*           bytes (ie it does not check the log_head_address relative to the
*           log_curr_address).  It is the responsibility of the calling
*           function to make sure this is only called when appropriate.
*
******************************************************************************/
void event_log_increment_head_address( u32 size ) {

    u64            end_address;
    entry_header * entry;

	// Calculate end address (need to make sure we don't overflow u32)
    end_address = log_head_address + size;

    // Check to see if we will wrap with the current increment
    if ( end_address > log_soft_end_address ) {
        // We will wrap the log

    	// Reset the log_soft_end_address to the end of the array
    	log_soft_end_address = log_max_address;

    	// Move the log_soft_head_address to the beginning of the array and move it
    	//   at least 'size' bytes from the front of the array.  This is done to mirror
    	//   how allocation of the log_curr_address works.  Also, b/c of this allocation
    	//   scheme, we are guaranteed that log_start_address is the beginning of an entry.
    	log_head_address = log_start_address;
    	end_address      = log_start_address + size;

		// Move the head address an integer number of entrys until it points to the
		//   first entry after the allocation
		entry = (entry_header *) log_head_address;

		while ( log_head_address < end_address ) {
			log_head_address += ( entry->entry_length + sizeof( entry_header ) );
			entry             = (entry_header *) log_head_address;
		}

    } else {
    	// We will not wrap

		// Move the head address an integer number of entrys until it points to the
		//   first entry after the allocation
		entry = (entry_header *) log_head_address;

		while ( log_head_address < end_address ) {
			log_head_address += ( entry->entry_length + sizeof( entry_header ) );
			entry             = (entry_header *) log_head_address;
		}
    }
}



/*****************************************************************************/
/**
* Get the address of the next empty entry in the log and allocate size bytes
*   for that entry
*
* @param    size        - Size (in bytes) of entry to allocate
*           address *   - Pointer to address of empty entry of length 'size'
*
* @return	int         - Status - 0 = Success
*                                  1 = Failure
*
* @note		This will handle the circular nature of the buffer.  It will also
*           set the log_full flag if there is no additional space and print
*           a warning message.  If this function is called while the event log
*           is full, then it will always return max_entry_index
*
******************************************************************************/
int  event_log_get_next_empty_address( u32 size, u32 * address ) {

	int   status         = 1;            // Initialize status to FAILED
	u32   return_address = 0;

	u64   end_address;

	// If the log is empty, then set the flag to zero to indicate the log is not empty
	if ( log_empty ) { log_empty = 0; }

	// If the log is not full, then find the next address
	if ( !log_full && !allocation_mutex ) {

		// Set the mutex to '1' so that if an interrupt occurs, the event log allocation
		//   will not be ruined
		allocation_mutex = 1;

		// Compute the end address of the newly allocated entry
	    end_address = log_curr_address + size;

	    // Check if the log has wrapped
	    if ( log_curr_address >= log_head_address ) {
	    	// The log has not wrapped

		    // Check to see if we will wrap with the current allocation
		    if ( end_address > log_soft_end_address ) {
		    	// Current allocation will wrap the log

		    	// Check to see if wrapping is enabled
		    	if ( log_wrap_enabled ) {

					// Compute new end address
					end_address = log_start_address + size;

					// Check that we are not going to pass the head address
					if ( end_address > log_head_address ) {

						event_log_increment_head_address( size );
					}

					// Set the log_soft_end_address and allocate the new entry from the beginning of the buffer
					log_soft_end_address = log_curr_address;
					log_curr_address     = end_address;

					// Return address is the beginning of the buffer
					return_address = log_start_address;
					status         = 0;

		    	} else {
					// Set the full flag and fail
					log_full = 1;

					// Log is now full, print warning
					xil_printf("---------------------------------------- \n");
					xil_printf("EVENT LOG:  WARNING - Event Log FULL !!! \n");
					xil_printf("---------------------------------------- \n");
		    	}
		    } else {
		    	// Current allocation does not wrap

		    	// NOTE: This should be the most common case; since we know the log has not wrapped
		    	//   we do not need to increment the log_head_address.

	    		// Set the return address and then move the log_curr_address
	    		return_address   = log_curr_address;
	    		log_curr_address = end_address;
	    		status           = 0;
		    }
	    } else {
	    	// The log has wrapped
	    	//   NOTE:  Even though the log has wrapped, we cannot assume that the wrap flag
	    	//     continues to allow the log to wrap.

			// Check that we are not going to pass the head address
	    	//   NOTE:  This will set the log_soft_end_address if the head_address passes the end of the array
			if ( end_address > log_head_address ) {

				event_log_increment_head_address( size );
			}

		    // Check to see if we will wrap with the current allocation
		    if ( end_address > log_soft_end_address ) {
		    	// Current allocation will wrap the log

		    	// Check to see if wrapping is enabled
		    	if ( log_wrap_enabled ) {

					// Compute new end address
					end_address = log_start_address + size;

					// NOTE:  We have already incremented the log_head_address by size.  Since the
					//   event_log_increment_head_address() function follows the same allocation scheme
					//   we are guaranteed that at least 'size' bytes are available at the beginning of the
					//   array if we wrapped.  Therefore, we do not need to check the log_head_address again.

					// Set the log_soft_end_address and allocate the new entry from the beginning of the buffer
					log_soft_end_address = log_curr_address;
					log_curr_address     = end_address;

					// Return address is the beginning of the buffer
					return_address = log_start_address;
					status         = 0;

		    	} else {
					// Set the full flag and fail
					log_full = 1;

					// Log is now full, print warning
					xil_printf("---------------------------------------- \n");
					xil_printf("EVENT LOG:  WARNING - Event Log FULL !!! \n");
					xil_printf("---------------------------------------- \n");
		    	}
		    } else {
		    	// Current allocation does not wrap

	    		// Set the return address and then move the log_curr_address
	    		return_address   = log_curr_address;
	    		log_curr_address = end_address;
	    		status           = 0;
		    }
	    }

		// Set the mutex to '0' to allow for future allocations
		allocation_mutex = 0;
	}

	// Set return parameter
	*address = return_address;

	return status;
}



/*****************************************************************************/
/**
* Get the next empty entry
*
* @param    entry_type  - Type of entry
*           entry_size  - Size of the entry payload
*
* @return	void *      - Pointer to the next entry payload
*
* @note		None.
*
******************************************************************************/
void * event_log_get_next_empty_entry( u16 entry_type, u16 entry_size ) {

	u32            log_address;
	u32            total_size;
	entry_header * header       = NULL;
	u32            header_size  = sizeof( entry_header );
	void *         return_entry = NULL;


    // If Event Logging is enabled, then allocate entry
	if( enable_event_logging ){

		total_size = entry_size + header_size;

		// Try to allocate the next entry
	    if ( !event_log_get_next_empty_address( total_size, &log_address ) ) {

	    	// Use successfully allocated address for the entry
			header = (entry_header*) log_address;

			// Zero out entry
			bzero( (void *) header, total_size );

			// Set header parameters
			//   - Use the upper 16 bits of the timestamp to place a magic number
            header->entry_id     = EVENT_LOG_MAGIC_NUMBER + ( 0x0000FFFF & log_count++ );
			header->entry_type   = entry_type;
			header->entry_length = entry_size;

            // Get a pointer to the entry payload
            return_entry         = (void *) ( log_address + header_size );

#ifdef _DEBUG_
			xil_printf("Entry (%6d bytes) = 0x%8x    0x%8x    0x%6x\n", entry_size, return_entry, header, total_size );
#endif
	    }
	}

	return return_entry;
}



/*****************************************************************************/
/**
* Prints everything in the event log from log_start_index to log_curr_index
*
* @param    None.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void print_event_log( u32 num_entrys ) {
	u32            i;
	u32            entry_address;
	u32            entry_count;
	entry_header * entry_hdr;
	u32            entry_hdr_size;
	void         * event;

	// Initialize count
	entry_hdr_size = sizeof( entry_header );
	entry_count    = 0;
	entry_address  = log_head_address;

    // Check if the log has wrapped
    if ( log_curr_address >= log_head_address ) {
    	// The log has not wrapped

    	// Print the log from log_head_address to log_curr_address
    	for( i = entry_count; i < num_entrys; i++ ){

    		// Check that the current entry address is less than log_curr_address
    		if ( entry_address < log_curr_address ) {

    			entry_hdr = (entry_header*) entry_address;
    			event     = (void *) ( entry_address + entry_hdr_size );

#ifdef _DEBUG_
                xil_printf(" Entry [%d] - addr = 0x%8x;  size = 0x%4x \n", i, entry_address, entry_hdr->entry_length );
#endif

        		// Print entry
    		    print_entry( (0x0000FFFF & entry_hdr->entry_id), entry_hdr->entry_type, event );

        	    // Get the next entry
        		entry_address += ( entry_hdr->entry_length + entry_hdr_size );

    		} else {
    			// Exit the loop
    			break;
    		}
    	}

    } else {
    	// The log has wrapped

    	// Print the log from log_head_address to the end of the buffer
    	for( i = entry_count; i < num_entrys; i++ ){

    		// Check that the current entry address is less than the end of the log
    		if ( entry_address < log_soft_end_address ) {

    			entry_hdr = (entry_header*) entry_address;
    			event     = (void * ) ( entry_hdr + entry_hdr_size );

        		// Print entry
    		    print_entry( (0x0000FFFF & entry_hdr->entry_id), entry_hdr->entry_type, event );

        	    // Get the next entry
        		entry_address += ( entry_hdr->entry_length + entry_hdr_size );

    		} else {
    			// Exit the loop and set the entry to the beginning of the buffer
    			entry_address  = log_start_address;
    			entry_count    = i;
    			break;
    		}
    	}

    	// If we still have entries to print, then start at the beginning
    	if ( entry_count < num_entrys ) {

			// Print the log from the beginning to log_curr_address
			for( i = entry_count; i < num_entrys; i++ ){

				// Check that the current entry address is less than log_curr_address
				if ( entry_address < log_curr_address ) {

	    			entry_hdr = (entry_header*) entry_address;
	    			event     = (void * ) ( entry_hdr + entry_hdr_size );

	        		// Print entry
	    		    print_entry( (0x0000FFFF & entry_hdr->entry_id), entry_hdr->entry_type, event );

					// Get the next entry
	        		entry_address += ( entry_hdr->entry_length + entry_hdr_size );

				} else {
					// Exit the loop
					break;
				}
			}
    	}
    }
}



/*****************************************************************************/
/**
* Prints the size of the event log
*
* @param    None.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void print_event_log_size(){
	u32 size;
	u64 timestamp;

	size      = event_log_get_size();
	timestamp = get_usec_timestamp();

	xil_printf("Event Log (%10d us): %10d of %10d bytes used\n", (u32)timestamp, size, log_size );
}





/*****************************************************************************/
// Built-in function to add fields to the log

/*****************************************************************************/
/**
* Add a node info entry
*
* @param    None.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void add_node_info_entry(){

	node_info_entry * entry;
	unsigned int     temp0;
	unsigned int     max_words = sizeof(node_info_entry) >> 2;

#ifdef USE_WARPNET_WLAN_EXP
	entry = (node_info_entry *)event_log_get_next_empty_entry( ENTRY_TYPE_NODE_INFO, sizeof(node_info_entry) );

    // Add the node parameters
    temp0 = node_get_parameter_values( (u32 *)entry, max_words);

    // Check to make sure that there was no mismatch in sizes
    //   NOTE: During initialization of the log, the hardware parameters are not yet defined.
    //       Therefore, we need to ignore when we get zero and be sure to reset the log
    //       before normal operation
    if ( (temp0 != max_words) && (temp0 != 0) ) {
    	xil_printf("WARNING:  Node info size = %d, param size = %d\n", max_words, temp0);
    	print_entry(0, ENTRY_TYPE_NODE_INFO, entry);
    }
#endif
}




/*****************************************************************************/
/**
* Add the current tx/rx statistics to the log
*
* @param    None.
*
* @return	num_statistics -- Number of statistics added to the log.
*
* @note		None.
*
******************************************************************************/
u32 add_txrx_statistics_to_log(){

	u32 i;
	u32                event_size = sizeof(txrx_stats_entry);
	u32                stats_size = sizeof(statistics) - sizeof(dl_node);
	dl_list          * list = get_statistics();
	statistics       * curr_statistics;
	txrx_stats_entry * entry;

	// Check to see if we have valid statistics
	if (list == NULL) { return 0; }

    if ( stats_size >= event_size ) {
    	// If the statistics structure in wlan_mac_high.h is bigger than the statistics
    	// entry, print a warning and return since there is a mismatch in the definition of
    	// statistics.
    	xil_printf("WARNING:  Statistics definitions do not match.  Statistics log entry is too small\n");
    	xil_printf("    to hold statistics structure.\n");
    	return 0;
    }

	curr_statistics = (statistics*)(list->first);

	for( i = 0; i < list->length; i++){

		entry = (txrx_stats_entry *)event_log_get_next_empty_entry( ENTRY_TYPE_TXRX_STATS, event_size );

		if ( entry != NULL ) {
			entry->timestamp = get_usec_timestamp();

			// Copy the statistics to the log entry
			//   NOTE:  This assumes that the statistics entry in wlan_mac_entries.h has a contiguous piece of memory
			//          equivalent to the statistics structure in wlan_mac_high.h (without the dl_node)
			memcpy( (void *)(&entry->last_timestamp), (void *)(&curr_statistics->last_timestamp), stats_size );

		    curr_statistics = statistics_next(curr_statistics);
		} else {
			break;
		}
	}

	return i;
}








