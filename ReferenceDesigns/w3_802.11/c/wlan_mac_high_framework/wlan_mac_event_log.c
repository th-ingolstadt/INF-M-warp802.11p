////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_event_log.c
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
//          Erik Welsh (welsh [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////
//
// Notes:
//   The event log implements a circular buffer that will record various
// events that occur within a WLAN node.  If the buffer is full, then events
// will be dropped with only a single warning printed to the screen.
//
//   Internally, the event log is just an array of bytes.  When a new event
// is requested, the size of the event is allocated from the buffer and a pointer
// to the allocated event is provided so that the caller can fill in the event
// information.
//
//   The event log will always provide a contiguous piece of memory for events.
// Therefore, some space could be wasted at the wrap boundary since a single event
// will never wrap.
//
//   Also, if an event cannot be allocated due to it overflowing the array, then
// the event log will set the full flag and not allow any more events to be
// allocated until event_log_delete() is called to free up space.  One limitation
// of this approach is that if a large event fails to be allocated, the event
// log will lock out all subsequent events even if there was sufficient space
// for them.  However, this should not cause too many issues given that events
// are extremely small relative to the event log.
//
//   Finally, the log does not keep track of event entries and it is up to
// calling functions to interpret the bytes within the log correctly.
//
////////////////////////////////////////////////////////////////////////////////

/***************************** Include Files *********************************/

// SDK includes
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "xil_types.h"

// WLAN includes
#include "wlan_mac_event_log.h"
#include "wlan_mac_util.h"


/*************************** Constant Definitions ****************************/



/*********************** Global Variable Definitions *************************/



/*************************** Variable Definitions ****************************/

// Log definition variables
static u32   log_start_address;        // Absolute start address of the log
static u32   log_soft_end_address;     // Soft end address of the log
static u32   log_max_address;          // Absolute end address of the log

static u32   log_size;                 // Size of the log in bytes

// Log index variables
static u32   log_head_address;         // Theoretically:  log_tail_address = ( log_head_address - 1 ) % log_max_address;
static u32   log_curr_address;

// Log status variables
static u8    log_empty;                // log_empty = (log_head_address == log_curr_address);
static u8    log_full;                 // log_full  = (log_tail_address == log_curr_address);

// Variable to control if events are recorded in to the log
u8           enable_event_logging;

// Mutex for critical allocation loop
static u8    allocation_mutex;



/*************************** Functions Prototypes ****************************/

// Internal functions;  Should not be called externally
//
u32             event_log_increment_head_address( u32 size );
int             event_log_get_next_empty_address( u32 size, u32 * address );
default_event * event_log_get_next_empty_event( u32 size );


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
* @note		None.
*
******************************************************************************/
void event_log_reset(){
	log_soft_end_address = log_max_address;

	log_head_address     = log_start_address;
	log_curr_address     = log_start_address;

	log_empty            = 1;
	log_full             = 0;

	allocation_mutex     = 0;
}



/*****************************************************************************/
/**
* Returns bytes that have been allocated in the event log to the free pool
*   so they can be reallocated for new entries
*
* @param    size        - Number of bytes of entries to "delete"
*
* @return	-1          - If the log is empty, this function will return -1
*           num_bytes   - The number of bytes deleted from the log
*
* @note		None.
*
******************************************************************************/
int  event_log_delete( u32 size ) {

	int return_value = -1;

	// If the log is empty, return -1
	if ( log_empty ) { return return_value; }

	// Move the log_head_address; All wrapping is handled in the function; Empty flag is set if necessary
	return_value = event_log_increment_head_address( size );

	// Remove the full flag if set
	if ( return_value > 0 ) {
		if ( log_full ) { log_full = 0; }
	}

	return return_value;
}



/*****************************************************************************/
/**
* Get event log data
*   Based on the start address and the size, the function will fill in the
* appropriate number of bytes in to the buffer.  Only "valid" bytes (ie bytes
* between log_head_address and log_curr_address will be returned.
*
*   This function hides all aspects that this is implemented as a circular buffer.
*
* @param    start_address    - Address in the event log to start the transfer
*           size             - Size in bytes of the buffer
*           buffer           - Pointer to the buffer to be filled in with event data
*
* @return	num_bytes        - The number of bytes filled in to the buffer
*
* @note		Callers should only use start addresses provided by either:
*               event_log_get_head_address()
*               event_log_get_address_from_head( size )
*           these functions will perform the appropriate wrapping so there
*           is no worry about overflowing a u32.
*
******************************************************************************/
u32  event_log_get_data( u32 start_address, u32 size, char * buffer ) {

	u32 temp_size;
	u32 temp_address;

	u64 end_address;
	u32 num_bytes     = 0;

	// If the log is empty, then return 0
    if ( log_empty == 1 ) { return num_bytes; }

    // Compute the end address for validity checks
    end_address = start_address + size;

    if ( log_curr_address >= log_head_address ) {
    	// The log has not wrapped

    	// Check that the start address is valid
    	if ( ( start_address >= log_head_address ) && ( start_address < log_curr_address ) ) {

    		if ( end_address < log_curr_address ) {
        		// There is enough data for a full transfer
				num_bytes = size;
    		} else {
    			// There is not enough data for a full transfer
    			num_bytes = log_curr_address - start_address;
    		}

			// Copy the data in to the buffer
			memcpy( (void *) buffer, (void *) start_address, num_bytes );

    	} else {
        	xil_printf("ERROR:  Invalid Log request:  Address = 0x%x   Size = 0x%x \n", start_address, size);
    	}
    } else {
    	// The log has wrapped

    	// Check that the start address is valid
    	if ( ( ( start_address >= log_start_address ) && ( start_address < log_curr_address     ) ) ||
             ( ( start_address >= log_head_address  ) && ( start_address < log_soft_end_address ) ) ) {

			if ( end_address <= log_soft_end_address ) {
				// The transfer does not need to wrap

				// There is guaranteed to be enough data since the buffer wraps but the transfer does not
				num_bytes = size;

				// Copy the data in to the buffer
				memcpy( (void *) buffer, (void *) start_address, num_bytes );

			} else {
				// The transfer needs to wrap

				// Transfer all data before the wrap
				temp_size = log_soft_end_address - start_address;
				memcpy( (void *) buffer, (void *) start_address, temp_size );

				// Get the remaining bytes to transfer
				temp_size = size - temp_size;

				// Compute the wrapped end address
				temp_address = log_start_address + temp_size;

				// Transfer the remaining bytes
	    		if ( temp_address < log_curr_address ) {
	        		// There is enough data for a full transfer
					num_bytes = size;
	    		} else {
	    			// There is not enough data for a full transfer
	    			temp_size = temp_size - (temp_address - log_curr_address);
	    			num_bytes = size - (temp_address - log_curr_address);
	    		}

				// Copy the data in to the buffer
				memcpy( (void *) buffer, (void *) start_address, temp_size );
			}
    	} else {
        	xil_printf("ERROR:  Invalid Log request:  Address = 0x%x   Size = 0x%x \n", start_address, size);
    	}
    }

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
* Get the address of the head of the log
*
* @param    None.
*
* @return	u32    - Address of the head of the log
*
* @note		None.
*
******************************************************************************/
u32  event_log_get_head_address( void ) {
    return log_head_address;
}



/*****************************************************************************/
/**
* Get the address of the head of the log
*
* @param    None.
*
* @return	u32    - Address of the head of the log
*
* @note		None.
*
******************************************************************************/
u32  event_log_get_address_from_head( u32 size ) {

	u64 end_address;
	u32 temp_size;
	u32 return_address;

	// Calculate end address (need to make sure we don't overflow u32)
	end_address = log_head_address + size;

	if ( end_address > log_soft_end_address ) {
		// Wrap the address
        temp_size      = size - ( log_soft_end_address - log_head_address );
        return_address = (u32) (log_start_address + temp_size);
	} else {
	    return_address = (u32) end_address;
	}

	return return_address;
}



/*****************************************************************************/
/**
* Increment the head address
*
* @param    size        - Number of bytes to increment the head address
*
* @return	num_bytes   - Number of bytes that the head address moved
*
* @note		None.
*
******************************************************************************/
u32 event_log_increment_head_address( u32 size ) {

    u64 end_address;
	u32 temp_size;
	u32 return_value = 0;

	// Calculate end address (need to make sure we don't overflow u32)
    end_address = log_head_address + size;

    // Check if the log has wrapped
    if ( log_curr_address >= log_head_address ) {
    	// The log has not wrapped

        // Check that we don't pass the log_curr_address
    	if ( end_address < log_curr_address ) {

    		// Move the log_head_address
           	return_value     = size;
			log_head_address = (u32) end_address;
    	} else {

    		// Set the head equal to the curr and return the size
    		return_value     = log_head_address - log_curr_address;
    		log_head_address = log_curr_address;
    		log_empty        = 1;
    	}
    } else {
    	// The log has wrapped

        if ( end_address > log_soft_end_address ) {
        	// Wrap the address
            temp_size        = size - ( log_soft_end_address - log_head_address );
            end_address      = log_start_address + temp_size;

            // Check that we don't pass the log_curr_address
        	if ( end_address < log_curr_address ) {

        		// Move the log_head_address
            	return_value     = size;
            	log_head_address = (u32) end_address;
        	} else {

        		// Set the head equal to the curr and return the size
        		return_value     = size - ( end_address - log_curr_address );
        		log_head_address = log_curr_address;
        		log_empty        = 1;
        	}

            // Reset the log_soft_end_address back to the end of the buffer since we wrapped
        	//   log_head_address back to the beginning of the buffer
            log_soft_end_address = log_max_address;
        } else {
            // Address does not wrap; also since log_curr_address <= log_head_address we
        	//   do not have to check that we pass log_curr_address

        	log_head_address = (u32) end_address;
        	return_value     = size;
        }
    }

    return return_value;
}



/*****************************************************************************/
/**
* Get the address of the next empty event in the log and allocate size bytes
*   for that event
*
* @param    size        - Size (in bytes) of event to allocate
*           address *   - Pointer to address of empty event of length 'size'
*
* @return	int         - Status - 0 = Success
*                                  1 = Failure
*
* @note		This will handle the circular nature of the buffer.  It will also
*           set the log_full flag if there is no additional space and print
*           a warning message.  If this function is called while the event log
*           is full, then it will always return max_event_index
*
******************************************************************************/
int  event_log_get_next_empty_address( u32 size, u32 * address ) {

	int status         = 1;            // Initialize status to FAILED
	u32 return_address = 0;

	u64 end_address;

	// If the log is empty, then set the flag to zero to indicate the log is not empty
	if ( log_empty ) { log_empty = 0; }

	// If the log is not full, then find the next address
	if ( !log_full && !allocation_mutex ) {

		// Set the mutex to '1' so that if an interrupt occurs, the event log allocation
		//   will not be ruined
		allocation_mutex = 1;

		// Compute the end address of the newly allocated event
	    end_address = log_curr_address + size;

	    // Check if the log has wrapped
	    if ( log_curr_address >= log_head_address ) {
	    	// The log has not wrapped

		    // Check to see if we will wrap with the current allocation
		    if ( end_address > log_soft_end_address ) {
		    	// Current allocation will wrap the log

		    	// Compute new end address
		    	end_address = log_start_address + size;

		    	// Check that we have enough space in the buffer for the allocation
		    	if ( end_address < log_head_address ) {
			    	// Set the log_soft_end_address and allocate the new event from the beginning of the buffer
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

		    	// NOTE: This should be the most common case

	    		// Set the return address and then move the log_curr_address
	    		return_address   = log_curr_address;
	    		log_curr_address = end_address;
	    		status           = 0;
		    }
	    } else {
	    	// The log has wrapped

	    	// Check that we have enough space in the buffer for the allocation
	    	if ( end_address < log_head_address ) {

	    		// Set the return address and then move the log_curr_address
	    		return_address   = log_curr_address;
	    		log_curr_address = end_address;
	    		status           = 0;

	    	} else {
	    		// Set the full flag and fail
	    		log_full = 1;

				// Log is now full, print warning
				xil_printf("---------------------------------------- \n");
				xil_printf("EVENT LOG:  WARNING - Event Log FULL !!! \n");
				xil_printf("---------------------------------------- \n");
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
* Get the next empty event
*
* @param    None.
*
* @return	default_event *   - Pointer to the next event
*
* @note		None.
*
******************************************************************************/
default_event * event_log_get_next_empty_event( u32 size ) {

	u32            log_address;
	default_event* return_event = NULL;

    // If Event Logging is enabled, then allocate event
	if( enable_event_logging ){

		// Try to allocate the next event
	    if ( !event_log_get_next_empty_address( size, &log_address ) ) {

	    	// Use successfully allocated address for the event entry
			return_event = (default_event*) log_address;

			// Zero out event
			bzero( (void *) return_event, size );

			// Set common parameters and leave the type as an invalid value
			return_event->timestamp    = get_usec_timestamp();
			return_event->event_length = size;

#ifdef _DEBUG_
			xil_printf("Event (%6d bytes) = 0x%x  \n", size, return_event );
#endif
	    }
	}

	return return_event;
}



/*****************************************************************************/
/**
* Get the next empty RX event
*
* @param    None.
*
* @return	rx_event *   - Pointer to the next "empty" RX event or NULL
*
* @note		None.
*
******************************************************************************/
rx_event* get_next_empty_rx_event(){
	rx_event* return_value;

	// Get the next empty event
	return_value = (rx_event *)event_log_get_next_empty_event( EVENT_SIZE );

	// Set the event type; other parameters set by call to get_next_empty_event()
	if ( return_value != NULL ) {
		return_value->event_type = EVENT_TYPE_RX;
	}

	return return_value;
}



/*****************************************************************************/
/**
* Get the next empty TX event
*
* @param    None.
*
* @return	tx_event *   - Pointer to the next "empty" TX event or NULL
*
* @note		None.
*
******************************************************************************/
tx_event* get_next_empty_tx_event(){
	tx_event* return_value;

	// Get the next empty event
	return_value = (tx_event *)event_log_get_next_empty_event( EVENT_SIZE );

	// Set the event type; other parameters set by call to get_next_empty_event()
	if ( return_value != NULL ) {
		return_value->event_type = EVENT_TYPE_TX;
	}

	return return_value;
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
void print_event_log(){
	u64             i;
	u32             event_count;
	default_event * default_event_log_item;

	// Initialize count
	event_count = 0;

    // Check if the log has wrapped
    if ( log_curr_address >= log_head_address ) {
    	// The log has not wrapped

    	// Print the log from log_head_address to log_curr_address
    	for( i = log_head_address; i < log_curr_address; ){
    		default_event_log_item = (default_event*) ((u32) i);

    		// Print event
    		print_event( event_count, default_event_log_item);

    		// Increment event count
    	    event_count++;

    	    // Increment loop variable
    		i += default_event_log_item->event_length;
    	}

    } else {
    	// The log has wrapped

    	// Print the log from log_head_address to the end of the buffer
    	for( i = log_head_address; i < log_soft_end_address; ){
    		default_event_log_item = (default_event*) ((u32) i);

    		// Print event
    		print_event( event_count, default_event_log_item);

    		// Increment event count
    	    event_count++;

    	    // Increment loop variable
    		i += default_event_log_item->event_length;
    	}

    	// Print the log from the start of the buffer to log_curr_address
    	for( i = log_start_address; i < log_curr_address; ){
    		default_event_log_item = (default_event*) ((u32) i);

    		// Print event
    		print_event( event_count, default_event_log_item);

    		// Increment event count
    	    event_count++;

    	    // Increment loop variable
    		i += default_event_log_item->event_length;
    	}
    }
}



/*****************************************************************************/
/**
* Prints an event
*
* @param    None.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void print_event( u32 event_number, default_event * event ){
	rx_event      * rx_event_log_item;
	tx_event      * tx_event_log_item;

	switch( event->event_type ){
		case EVENT_TYPE_RX:
			rx_event_log_item = (rx_event*) event;
			xil_printf("%d: [%d] - Rx Event\n", event_number, (u32)rx_event_log_item->timestamp);
			xil_printf("   Pow:      %d\n",     rx_event_log_item->power);
			xil_printf("   Seq:      %d\n",     rx_event_log_item->seq);
			xil_printf("   Rate:     %d\n",     rx_event_log_item->rate);
			xil_printf("   Length:   %d\n",     rx_event_log_item->length);
			xil_printf("   State:    %d\n",     rx_event_log_item->state);
			xil_printf("   MAC Type: 0x%x\n",   rx_event_log_item->mac_type);
			xil_printf("   Flags:    0x%x\n",   rx_event_log_item->flags);
		break;

		case EVENT_TYPE_TX:
			tx_event_log_item = (tx_event*) event;
			xil_printf("%d: [%d] - Tx Event\n", event_number, (u32)tx_event_log_item->timestamp);
			xil_printf("   Pow:      %d\n",     tx_event_log_item->power);
			xil_printf("   Seq:      %d\n",     tx_event_log_item->seq);
			xil_printf("   Rate:     %d\n",     tx_event_log_item->rate);
			xil_printf("   Length:   %d\n",     tx_event_log_item->length);
			xil_printf("   State:    %d\n",     tx_event_log_item->state);
			xil_printf("   MAC Type: 0x%x\n",   tx_event_log_item->mac_type);
			xil_printf("   Retry:    %d\n",     tx_event_log_item->retry_count);
		break;

		default:
			xil_printf("%d: [%d] - Unknown Event\n", event_number, (u32)event->timestamp);
		break;
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


