/** @file wlan_mac_queue.c
 *  @brief Transmit Queue Framework
 *
 *  This contains code for accessing the transmit queue.
 *
 *  @copyright Copyright 2013-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/***************************** Include Files *********************************/

#include "wlan_mac_high_sw_config.h"

// Xilinx / Standard library includes
#include "xil_types.h"
#include "stdlib.h"
#include "stdio.h"
#include "wlan_platform_high.h"
#include "xintc.h"
#include "string.h"

// WLAN includes
#include "wlan_mac_common.h"
#include "wlan_mac_high.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_pkt_buf_util.h"
#include "wlan_platform_common.h"
#include "wlan_mac_station_info.h"

// WLAN Exp includes
#include "wlan_exp_common.h"


/*************************** Constant Definitions ****************************/

/*********************** Global Variable Definitions *************************/

// User callback to see if the higher-level framework can send a packet to
// the lower-level framework to be transmitted.
extern platform_common_dev_info_t	 platform_common_dev_info;
extern function_ptr_t        tx_poll_callback;
static function_ptr_t        queue_state_change_callback;

extern platform_high_dev_info_t	 platform_high_dev_info;


/*************************** Functions Prototypes ****************************/

/*************************** Variable Definitions ****************************/

// List to hold all of the empty, free entries
static dl_list               free_queue;

// The tx_queues variable is an array of lists that will be filled with queue
// entries from the free_queue list
//
// NOTE:  This implementation sparsely packs the tx_queues array to allow fast
//     indexing at the cost of some wasted memory. The tx_queues array will be
//     reallocated whenever the upper-level MAC asks to enqueue at an index
//     that is larger than the current size of the array. It is assumed that
//     this index will not continue to grow over the course of execution,
//     otherwise this array will continue to grow and eventually be unable to
//     be reallocated. Practically speaking, this means an AP needs to re-use
//     the AIDs it issues stations if it wants to use the AIDs as an index into
//     the tx queue.
//
static dl_list*              tx_queues;
static u16                   num_tx_queues;


// Total number of Tx queue entries
static volatile u32          total_tx_queue_entries;


/******************************** Functions **********************************/

/*****************************************************************************/
/**
 * @brief  Initialize the Tx Queue framework
 *
 * The number of Tx Queue elements we can initialize is limited by the smaller of two values:
 *     (1) The number of dl_entry structs we can squeeze into TX_QUEUE_DL_ENTRY_MEM_SIZE
 *     (2) The number of QUEUE_BUFFER_SIZE MPDU buffers we can squeeze into TX_QUEUE_BUFFER_SIZE
 *
 * @param  u8 dram_present        - Flag to indicate if DRAM is present
 *                                  (1 = present; other = not present)
 *
 *****************************************************************************/
void queue_init() {
	u32            i;
	dl_entry     * dl_entry_base;

	// Set the total number of supported Tx Queue entries
	total_tx_queue_entries = min((TX_QUEUE_DL_ENTRY_MEM_SIZE / sizeof(dl_entry)),   // Max dl_entry
	                             (TX_QUEUE_BUFFER_SIZE / QUEUE_BUFFER_SIZE));       // Max buffers

	// Initialize the Tx queue
	//     NOTE:  tx_queues is initially NULL because it will be dynamically allocated and
	//         initialized.
	//
	tx_queues     = NULL;
	num_tx_queues = 0;

	queue_state_change_callback = (function_ptr_t)wlan_null_callback;

	// Initialize the free queue
	dl_list_init(&free_queue);

	// Zero all Tx queue elements
	bzero((void*)TX_QUEUE_BUFFER_BASE, TX_QUEUE_BUFFER_SIZE);

	// Allocate the memory space into Tx Queue entries
	//
	// All Tx Queue entries are initially part of the free queue.  Each Tx Queue entry consists of:
	//     (1) Buffer to hold data
	//     (1) dl_entry to describe the buffer
	//
	// NOTE:  The code below exploits the fact that the starting state of all Tx Queue entries is
	//     sequential.  Therefore, it can use matrix addressing.  Matrix addressing is not safe
	//     once the queue is used and the insert/remove helper functions should be used instead.
	//
	dl_entry_base = (dl_entry*)(TX_QUEUE_DL_ENTRY_MEM_BASE);

	for (i = 0; i < total_tx_queue_entries; i++) {
		// Segment the buffer in QUEUE_BUFFER_SIZE pieces
		dl_entry_base[i].data = (void*)(TX_QUEUE_BUFFER_BASE + (i * QUEUE_BUFFER_SIZE));

        // Copy the pointer to the dl_entry into the DRAM payload. This will allow any context
		// (like Ethernet Rx) to find the dl_entry that points to a given DRAM payload
        ((tx_queue_buffer_t*)(dl_entry_base[i].data))->tx_queue_entry = &(dl_entry_base[i]);

		// Insert new dl_entry into the free queue
		dl_entry_insertEnd(&free_queue, &(dl_entry_base[i]));
	}




	// Print status
	xil_printf("Tx Queue of %d placed in DRAM: using %d kB\n", total_tx_queue_entries,
	           ((total_tx_queue_entries * QUEUE_BUFFER_SIZE) / 1024));
}



/*****************************************************************************/
/**
 * @brief  Total number of Tx Queue entries
 *
 * This is the sum of all free and occupied Tx Queue entries
 *
 * @return u32
 *
 *****************************************************************************/
u32 queue_total_size(){
	return total_tx_queue_entries;
}



/*****************************************************************************/
/**
 * @brief  Number of free Tx Queue entries
 *
 * @return u32
 *
 *****************************************************************************/
u32 queue_num_free(){
	return free_queue.length;
}



/*****************************************************************************/
/**
 * @brief  Number of Tx Queue entries in a given queue
 *
 * @param  u16 queue_sel          - ID of the queue
 *
 * @return u32
 *
 *****************************************************************************/
u32 queue_num_queued(u16 queue_sel){
	if((queue_sel+1) > num_tx_queues){
		return 0;
	} else {
		return tx_queues[queue_sel].length;
	}
}



/*****************************************************************************/
/**
 * @brief  Removes all Tx Queue entries in the selected queue
 *
 * This function removes all entries from the selected queue and returns them to
 * the free pool. Packets contained in the purged entries will be dropped. This
 * function should only be called when enqueued packets should no longer be
 * transmitted wirelessly, such as when a node leaves a BSS.
 *
 * @param  u16 queue_sel          - ID of the queue to purge
 *
 *****************************************************************************/
void purge_queue(u16 queue_sel){
	u32                        num_queued;
	u32                        i;
	dl_entry       * curr_tx_queue_element;
	volatile interrupt_state_t prev_interrupt_state;

	num_queued = queue_num_queued(queue_sel);

	if (num_queued > 0) {
#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP
		wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_queue, "Purging %d packets from queue %d\n", num_queued, queue_sel);
#endif

		// NOTE:  There is an interesting condition that can occur if an LTG is running (ie creating new TX queue
		//     entries) and purge_queue is called.  In this case, you have one process removing elements from the
		//     queue while another process is adding elements to the queue.  Therefore, purge_queue will only
		//     remove a fixed number of elements from the queue (ie all queued elements at the time the function
		//     is called).  If purge_queue used a while loop with no checking on the number of elements removed,
		//     then it could conceivably run forever.
		//
		for (i = 0; i < num_queued; i++) {
			// The queue purge is not interrupt safe
			//     NOTE:  Since there could be many elements in the queue, we need to toggle the interrupts
			//         inside the for loop so that we do not block CPU High for an extended period of time.
			//         This could result in CPU Low dropping a reception.
			//
			prev_interrupt_state = wlan_mac_high_interrupt_stop();

			curr_tx_queue_element = dequeue_from_head(queue_sel);

			// Decrement the num_tx_queued field in the attached station_info_t. If this was the
			// last queued packet for this station, this will allow the framework to recycle this
			// station_info_t if it also has not been flagged as something to keep.
			((tx_queue_buffer_t*)(curr_tx_queue_element->data))->station_info->num_tx_queued--;

			queue_checkin(curr_tx_queue_element);

			// Re-enable interrupts
			wlan_mac_high_interrupt_restore_state(prev_interrupt_state);
		}
	}
}



/*****************************************************************************/
/**
 * @brief  Adds a queue entry to a specified queue
 *
 * Adds the queue entry pointed to by tqe to the queue with ID queue_sel. The
 * calling context must ensure tqe points to a queue entry containing a packet
 * ready for wireless transmission.  If a queue with ID quele_sel does not
 * already exist this function will create it, then add tqe to the new queue.
 *
 * @param  u16 queue_sel          - ID of the queue to which tqe is added. A new
 *                                  queue with ID queue_sel will be created if it
 *                                  does not already exist.
 * @param  dl_entry* tqe  - Tx Queue entry containing packet for transmission
 *
 *****************************************************************************/
void enqueue_after_tail(u16 queue_sel, dl_entry* tqe){
	u32 i;

	// Create queues up to and including queue_sel if they don't already exist
	// Queue IDs are low-valued integers, allowing for fast lookup by indexing the tx_queues array
	if ((queue_sel + 1) > num_tx_queues) {
		tx_queues = wlan_mac_high_realloc(tx_queues, ((queue_sel + 1) * sizeof(dl_list)));

		if(tx_queues == NULL){
#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP
			wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_queue, "Could not reallocate %d bytes for queue %d\n",
			                ((queue_sel + 1) * sizeof(dl_list)), queue_sel);
#endif
		}

		for(i = num_tx_queues; i <= queue_sel; i++){
			dl_list_init(&(tx_queues[i]));
		}

		num_tx_queues = queue_sel + 1;
	}

	// Insert the queue entry into the dl_list representing the selected queue
	dl_entry_insertEnd(&(tx_queues[queue_sel]), (dl_entry*)tqe);

	// Update the occupancy of the tx queue for the tx_queue_element
	//     NOTE:  This is the best place to record this value since it will catch all cases.  However,
	//         when populating the tx_frame_info, be careful to not overwrite this value.  Also, this
	//         field is set after the current tx queue element has been added to the queue, so the
	//         occupancy value includes itself.
	//
	((tx_queue_buffer_t*)(tqe->data))->queue_info.enqueue_timestamp = get_mac_time_usec();
	((tx_queue_buffer_t*)(tqe->data))->queue_info.occupancy = (tx_queues[queue_sel].length & 0xFFFF);
	((tx_queue_buffer_t*)(tqe->data))->queue_info.id = queue_sel;

	//Increment the num_tx_queued field in the attached station_info_t. This will prevent
	// the framework from removing the station_info_t out from underneath us while this
	// packet is enqueued.
	((tx_queue_buffer_t*)(tqe->data))->station_info->num_tx_queued++;

	if(tx_queues[queue_sel].length == 1){
		//If the queue element we just added is now the only member of this queue, we should inform
		//the top-level MAC that the queue has transitioned from empty to non-empty.
		queue_state_change_callback(queue_sel, 1);
	}

    // Poll the TX queues to see if anything needs to be transmitted
	tx_poll_callback();

	return;
}



/*****************************************************************************/
/**
 * @brief  Removes the head entry from the specified queue
 *
 * If queue_sel is not empty this function returns a tx_queue_element pointer
 * for the head entry in the queue. If the specified queue is empty this
 * function returns NULL.
 *
 * @param  u16 queue_sel          - ID of the queue from which to dequeue an entry
 *
 * @return dl_entry *     - Pointer to queue entry if available,
 *                                  NULL if queue is empty
 *
 *****************************************************************************/
dl_entry* dequeue_from_head(u16 queue_sel){
	dl_entry          * curr_dl_entry;

	if ((queue_sel + 1) > num_tx_queues) {
		// The specified queue does not exist; this can happen if a node has
		//     associated (has a valid AID = queue_sel) but no packet has ever
		//     been enqueued to it, as queues are created upon first insertion
		//     - see enqueue_after_tail()
		return NULL;
	} else {
		if (tx_queues[queue_sel].length == 0) {
			// Requested queue exists but is empty
			return NULL;
		} else {
			curr_dl_entry = (tx_queues[queue_sel].first);
			dl_entry_remove(&tx_queues[queue_sel], curr_dl_entry);

			if(tx_queues[queue_sel].length == 0){
				//If the queue element we just removed empties the queue, we should inform
				//the top-level MAC that the queue has transitioned from non-empty to empty.
				queue_state_change_callback(queue_sel, 0);
			}

			return (dl_entry *) curr_dl_entry;
		}
	}
}



/*****************************************************************************/
/**
 * @brief  Checks out one queue entry from the free pool
 *
 * The queue framework maintains a pool of free queue entries. This function
 * removes one entry from the free pool and returns it for use by the MAC
 * application. If the free pool is empty NULL is returned.
 *
 * @return dl_entry *     - Pointer to new queue entry if available,
 *                                  NULL if free queue is empty
 *
 *****************************************************************************/
dl_entry* queue_checkout(){
	dl_entry* tqe;

	if(free_queue.length > 0){
		tqe = ((dl_entry*)(free_queue.first));
		dl_entry_remove(&free_queue,free_queue.first);

		((tx_queue_buffer_t*)(tqe->data))->station_info = NULL;

		return tqe;
	} else {
		return NULL;
	}
}



/*****************************************************************************/
/**
 * @brief  Checks in one queue entry to the free pool
 *
 * The queue framework maintains a pool of free queue entries. This function
 * returns one entry to the free pool.  tqe must be a valid pointer to a queue
 * entry which the MAC application no longer needs. The application must not
 * use the entry pointed to by tqe after calling this function.
 *
 * @param  dl_entry* tqe  - Pointer to queue entry to be returned to free pool
 *
 *****************************************************************************/
void queue_checkin(dl_entry* tqe){

	if (tqe != NULL) {
	    dl_entry_insertEnd(&free_queue, (dl_entry*) tqe);
	}

	wlan_platform_free_queue_entry_notify();
}



/*****************************************************************************/
/**
 * @brief Checks out multiple queue entries from the free pool
 *
 * The queue framework maintains a pool of free queue entries. This function
 * attempts to check out num_tqe queue entries from the free pool. The number of
 * queue entries successfully checked out is returned. This may be less than
 * requested if the free pool had fewer than num_tqe entries available.
 *
 * @param   dl_list * new_list    - Pointer to dl_list to which queue entries are appended
 * @param   u16 num_tqe           - Number of queue entries requested
 *
 * @return  Number of queue entries successfully checked out and appended to new_list
 *
 *****************************************************************************/
int queue_checkout_list(dl_list* list, u16 num_tqe){
#if 0
	// Approx. performance metrics:
	//    incremental processing = 3.3 us / queue entry
	//    overhead               = 0.3 us
	//
	// Ex.  For one queue entry, function will take 3.6 us (ie (3.3 * 1) + 0.3 = 3.6)
	//
	u32       i;
	u32       num_checkout;
	dl_entry* curr_dl_entry;

	if(num_tqe <= free_queue.length){
		num_checkout = num_tqe;
	} else {
		num_checkout = free_queue.length;
	}

	//Traverse the free_queue and update the pointers
	for (i = 0; i < num_checkout; i++){
		curr_dl_entry = (free_queue.first);

		//Remove from free list
		dl_entry_remove(&free_queue, curr_dl_entry);

		//Add to new checkout list
		dl_entry_insertEnd(list, curr_dl_entry);
	}

	return num_checkout;
#else
	// Approx. performance metrics:
	//    incremental processing = 0.16 us / queue entry
	//    overhead               = 1.84 us
	//
	// Ex.  For one queue entry, function will take 2.0 us (ie (0.16 * 1) + 1.84 = 2.0)
	//

    return dl_entry_move(&free_queue, list, num_tqe);
#endif
}



/*****************************************************************************/
/**
 * @brief Checks in multiple queue entries into the free pool
 *
 * The queue framework maintains a pool of free queue entries. This function will
 * check in all queue entries from the provided list to the end of the free pool.
 *
 * @param   dl_list * new_list    - Pointer to dl_list from which queue entries will be checked in
 *
 * @return  Number of queue entries successfully checked in
 *
 *****************************************************************************/
int queue_checkin_list(dl_list * list) {
#if 0
	u32       i;
	u32       num_checkin;
	dl_entry* curr_dl_entry;

	num_checkin = list->length;

	// Traverse the list and update the pointers
	for (i = 0; i < num_checkin; i++){
		curr_dl_entry = (list->first);

		//Remove from free list
		dl_entry_remove(list, curr_dl_entry);

		//Add to new checkout list
		dl_entry_insertEnd(&free_queue, curr_dl_entry);
	}
#else
    return dl_entry_move(list, &free_queue, list->length);
#endif
}

void transmit_checkin(dl_entry* tx_queue_buffer_entry){
	int                 tx_pkt_buf               = -1;
	tx_pkt_buf = wlan_mac_high_get_empty_tx_packet_buffer();

	if (tx_queue_buffer_entry == NULL) return;

	if( tx_pkt_buf != -1 ){
		// Transmit the Tx Queue element
		//     NOTE:  This copies all the contents of the queue element to the
		//         packet buffer so that the queue element can be safely returned
		//         to the free pool
		wlan_mac_high_mpdu_transmit(tx_queue_buffer_entry, tx_pkt_buf);

		// Decrement the num_tx_queued field in the attached station_info_t. If this was the
		// last queued packet for this station, this will allow the framework to recycle this
		// station_info_t if it also has not been flagged as something to keep.
		((tx_queue_buffer_t*)(tx_queue_buffer_entry->data))->station_info->num_tx_queued--;
	} else {
		xil_printf("Error in transmit_checkin(): no free Tx packet buffers. Packet was freed without being sent\n");
	}

	// Check in the Tx Queue element because it is not long being used
	queue_checkin(tx_queue_buffer_entry);
}

inline void queue_set_state_change_callback(function_ptr_t callback){
	queue_state_change_callback = callback;
}
