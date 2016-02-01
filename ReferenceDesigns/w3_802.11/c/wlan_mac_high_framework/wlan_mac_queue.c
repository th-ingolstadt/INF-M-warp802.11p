/** @file wlan_mac_queue.c
 *  @brief Transmit Queue Framework
 *
 *  This contains code for accessing the transmit queue.
 *
 *  @copyright Copyright 2013-2015, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

#include "xil_types.h"
#include "stdlib.h"
#include "stdio.h"
#include "xparameters.h"
#include "xintc.h"
#include "string.h"

#include "wlan_mac_ipc_util.h"
#include "wlan_mac_high.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_eth_util.h"

#include "wlan_exp_common.h"


//This list holds all of the empty, free elements
static dl_list               queue_free;

//This queue_tx vector will get filled in with elements from the queue_free list
//Note: this implementation sparsely packs the queue_tx array to allow fast
//indexing at the cost of some wasted memory. The queue_tx array will be
//reallocated whenever the upper-level MAC asks to enqueue at an index
//that is larger than the current size of the array. It is assumed that this
//index will not continue to grow over the course of execution, otherwise
//this array will continue to grow and eventually be unable to be reallocated.
//Practically speaking, this means an AP needs to re-use the AIDs it issues
//stations if it wants to use the AIDs as an index into the tx queue.
static dl_list*              queue_tx;
static u16                   num_queue_tx;

extern function_ptr_t        tx_poll_callback;             ///< User callback when higher-level framework is ready to send a packet to low

volatile static u32          num_tx_queue;


void queue_init(u8 dram_present){
	u32 i;
	//The number of Tx Queue elements we can initialize is limited by the smaller of two values:
	//	(1) The number of dl_entry structs we can squeeze into TX_QUEUE_DL_ENTRY_MEM_SIZE
	//  (2) The number of QUEUE_BUFFER_SIZE MPDU buffers we can squeeze into TX_QUEUE_BUFFER_SIZE
	num_tx_queue =  min(TX_QUEUE_DL_ENTRY_MEM_SIZE/sizeof(dl_entry), TX_QUEUE_BUFFER_SIZE/QUEUE_BUFFER_SIZE );

	if(dram_present != 1){
		xil_printf("A working DRAM SODIMM has not been detected on this board.\n");
		xil_printf("DRAM is required for the wireless transmission queue.  Halting.\n");

		set_hex_display_error_status(ERROR_NODE_DRAM_NOT_PRESENT);
		blink_hex_display(0, 250000);
	}


	dl_list_init(&queue_free);
	bzero((void*)TX_QUEUE_BUFFER_BASE, TX_QUEUE_BUFFER_SIZE);

	//At boot, every dl_entry buffer descriptor is free
	//To set up the doubly linked list, we exploit the fact that we know the starting state is sequential.
	//This matrix addressing is not safe once the queue is used. The insert/remove helper functions should be used
	dl_entry* dl_entry_base;
	dl_entry_base = (dl_entry*)(TX_QUEUE_DL_ENTRY_MEM_BASE);

	for(i=0;i<num_tx_queue;i++){
		dl_entry_base[i].data = (void*)(TX_QUEUE_BUFFER_BASE + (i*QUEUE_BUFFER_SIZE));
		dl_entry_insertEnd(&queue_free,&(dl_entry_base[i]));
	}

	xil_printf("Tx Queue of %d placed in DRAM: using %d kB\n", num_tx_queue, (num_tx_queue*QUEUE_BUFFER_SIZE)/1024);

	num_queue_tx = 0;
	queue_tx     = NULL;
	return;
}

/**
 * @return Total number of queue entries; sum of all free and occupied entries
 */
int queue_total_size(){
	return num_tx_queue;
}

/**
 * @brief Flushes the contents of the selected queue
 *
 * Purges all entries from the selected queue and returns them to the free pool. Packets contained
 * in the purged entries will be dropped. This function should only be called when enqueued packets
 * should no longer be transmitted wirelessly, such as when a node leaves a BSS.
 *
 * @param u16 queue_sel
 *  -ID of the queue to purge
 */
void purge_queue(u16 queue_sel){
	u32                        num_queued;
	u32                        i;
	tx_queue_element         * curr_tx_queue_element;
	volatile interrupt_state_t prev_interrupt_state;

	num_queued = queue_num_queued(queue_sel);

	if (num_queued > 0) {
		wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_queue, "Purging %d packets from queue %d\n", num_queued, queue_sel);

		// NOTE:  There is an interesting condition that can occur if an LTG is running (ie creating new TX queue
		//     entries) and purge_queue is called.  In this case, you have one process removing elements from the
		//     queue while another process is adding elements to the queue.  Therefore, purge_queue will only
		//     remove a fixed number of elements from the queue (ie all queued elements at the time the function
		//     is called).  If purge_queue used a while loop with no checking on the number of elements removed,
		//     then it could conceivably run forever.
		//
		for(i = 0; i < num_queued; i++) {
			// The queue purge is not interrupt safe
			//     NOTE:  Since there could be many elements in the queue, we need to toggle the interrupts
			//         inside the for loop so that we do not block CPU High for an extended period of time.
			//         This could result in CPU Low dropping a reception.
			//
			prev_interrupt_state = wlan_mac_high_interrupt_stop();

			curr_tx_queue_element = dequeue_from_head(queue_sel);
			queue_checkin(curr_tx_queue_element);

			// Re-enable interrupts
			wlan_mac_high_interrupt_restore_state(prev_interrupt_state);
		}
	}
}

/**
 * @brief Adds a queue entry to a specified queue
 *
 * Adds the queue entry pointed to by tqe to the queue with ID queue_sel. The calling context must ensure
 * tqe points to a queue entry containing a packet ready for wireless transmission. If a queue with ID quele_sel
 * does not already exist this function will create it, then add tqe to the new queue.
 *
 * @param u16 queue_sel
 *  -ID of the queue to which tqe is added. A new queue with ID queue_sel will be created if it does not already exist.
 * @param tx_queue_element* tqe
 *  -Queue entry containing packet for transmission
 */
void enqueue_after_tail(u16 queue_sel, tx_queue_element* tqe){
	u32 i;

	// Create queues up to and including queue_sel if they don't already exist
	// Queue IDs are low-valued integers, allowing for fast lookup by indexing the queue_tx array
	if((queue_sel+1) > num_queue_tx){
    	queue_tx = wlan_mac_high_realloc(queue_tx, (queue_sel+1)*sizeof(dl_list));

    	if(queue_tx == NULL){
    		wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_queue, "Could not reallocate %d bytes for queue %d\n", (queue_sel+1)*sizeof(dl_list), queue_sel);
    	}

    	for(i = num_queue_tx; i <= queue_sel; i++){
    		dl_list_init(&(queue_tx[i]));
    	}

    	num_queue_tx = queue_sel+1;
    }

	//Insert the queue entry into the dl_list representing the selected queue
	dl_entry_insertEnd(&(queue_tx[queue_sel]), (dl_entry*)tqe);

	// Update the occupancy of the tx queue for the tx_queue_element
	//     NOTE:  This is the best place to record this value since it will catch all cases.  However,
	//         when populating the tx_frame_info, be careful to not overwrite this value.  Also, this
	//         field is set after the current tx queue element has been added to the queue, so the
	//         occupancy value includes itself.
	//
	((tx_queue_buffer*)(tqe->data))->frame_info.queue_info.occupancy = (queue_tx[queue_sel].length & 0xFFFF);

    // Poll the TX queues to see if anything needs to be transmitted
	tx_poll_callback();

	return;
}

/**
 * @brief Removes the head entry from the specified queue
 *
 * If queue_sel is not empty this function returns a tx_queue_element pointer for the head
 * entry in the queue. If the specified queue is empty this function returns NULL.
 *
 * @param u16 queue_sel
 *  -ID of the queue from which to dequeue an entry
 * @return
 *  -Pointer to queue entry if available, NULL if queue is empty
 */
tx_queue_element* dequeue_from_head(u16 queue_sel){
	tx_queue_element* tqe;
	dl_entry* curr_dl_entry;

	if((queue_sel+1) > num_queue_tx){
		// The specified queue does not exist; this can happen if a node has associated (has a valid AID=queue_sel)
		//     but no packet has ever been enqueued to it, as queues are created upon first insertion - see enqueue_after_tail()
		return NULL;
	} else {
		if(queue_tx[queue_sel].length == 0){
			// Requested queue exists but is empty
			return NULL;
		} else {
			curr_dl_entry = (queue_tx[queue_sel].first);
			dl_entry_remove(&queue_tx[queue_sel],curr_dl_entry);
			tqe = (tx_queue_element*)curr_dl_entry;
			return tqe;
		}
	}
}

/**
 * @return Number of queue entries in the free pool
 */
u32 queue_num_free(){
	return queue_free.length;
}

/**
 * @param u16 queue_sel ID of queue
 * @return Number of entries in the specified queue
 */
u32 queue_num_queued(u16 queue_sel){
	if((queue_sel+1) > num_queue_tx){
		return 0;
	} else {
		return queue_tx[queue_sel].length;
	}
}

/**
 * @brief Checks out one queue entry from the free pool
 *
 * The queue framework maintains a pool of free queue entries. This function removes one entry
 * from the free pool and returns it for use by the MAC application. If the free pool is empty
 * NULL is returned.
 *
 * @return New queue entry, NULL if none is available
 */
tx_queue_element* queue_checkout(){
	tx_queue_element* tqe;

	if(queue_free.length > 0){
		tqe = ((tx_queue_element*)(queue_free.first));
		dl_entry_remove(&queue_free,queue_free.first);
		((tx_queue_buffer*)(tqe->data))->metadata.metadata_type = QUEUE_METADATA_TYPE_IGNORE;
		return tqe;
	} else {
		return NULL;
	}
}

/**
 * @brief Checks out one queue entry from the free pool
 *
 * The queue framework maintains a pool of free queue entries. This function returns one entry
 * to the free pool. tqe must be a valid pointer to a queue entry which the MAC application no
 * longer needs. The application must not use the entry pointed to by tqe after calling this function.
 *
 * @param   tx_queue_element* tqe      - Pointer to queue entry to be returned to free pool
 *
 * @return  None
 */
void queue_checkin(tx_queue_element* tqe){
	if (tqe != NULL) {
	    dl_entry_insertEnd(&queue_free, (dl_entry*) tqe);
	}
}


/**
 * @brief Checks out multiple queue entries from the free pool
 *
 * The queue framework maintains a pool of free queue entries. This function attempts to check out
 * num_tqe queue entries from the free pool. The number of queue entries successfully checked out
 * is returned. This may be less than requested if the free pool had fewer than num_tqe entries
 * available.
 *
 * @param dl_list* new_list Pointer to doubly linked list to which new queue entries are appended.
 * @param u16 num_tqe Number of queue entries requested
 *
 * @return Number of queue entries successfully checked out and appended to new_list
 */
int queue_checkout_list(dl_list* list, u16 num_tqe){
	// Checks out up to num_tqe number of entries from the free list. If num_tqe are not free,
	// then this function will return the number that are free and only check out that many.

#if 0
	// Approx. performance metrics:
	//    incremental processing = 3.3 us / queue entry
	//    overhead               = 0.3 us
	//
	u32       i;
	u32       num_checkout;
	dl_entry* curr_dl_entry;

	if(num_tqe <= queue_free.length){
		num_checkout = num_tqe;
	} else {
		num_checkout = queue_free.length;
	}

	//Traverse the queue_free and update the pointers
	for (i = 0; i < num_checkout; i++){
		curr_dl_entry = (queue_free.first);

		//Remove from free list
		dl_entry_remove(&queue_free, curr_dl_entry);

		//Add to new checkout list
		dl_entry_insertEnd(list, curr_dl_entry);
	}
#else
	// Approx. performance metrics:
	//    incremental processing = 0.15 us / queue entry
	//    overhead               = 1.65 us
	//
	u32                 i;
	u32                 num_checkout;
	dl_entry          * curr_dl_entry;
	interrupt_state_t   curr_interrupt_state;

	// If the caller is not checking out any entries or if there are no entries
	// in the free queue, then just return
	if ((num_tqe == 0) || (queue_free.length == 0)) { return 0; }

	//
	// NOTE:  At this point, the caller is trying to check out at least 1
	//     entry and the free queue has at least one entry to check out.
	//

	// Stop the interrupts since we are modifying list state
	curr_interrupt_state = wlan_mac_high_interrupt_stop();

	if (num_tqe < queue_free.length) {
		// There will be entries left in the free queue after checkout
		//
		// Process:
		//   - Get the head of the free queue
		//   - Move thru the free queue for num_tqe entries
		//   - Update the list to add checked out entries
		//   - Update the free queue to remove checked out entries
		//   - Separate the lists by modifying the last entry of the
		//     new list and the new first entry of the free queue
		//
		num_checkout  = num_tqe;
		curr_dl_entry = queue_free.first;

		// Move thru the list to find the last item in the checkout list
		//     NOTE:  This iteration starts at 1 since we have already "moved"
		//         one item to the new list.
		for (i = 1; i < num_checkout; i++) {
			curr_dl_entry  = dl_entry_next(curr_dl_entry);

			if (curr_dl_entry == NULL) {
				xil_printf("Free Queue list not in sync.\n");
                
                // Restore interrupts
                wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
                
				return 0;
			}
		}

		//
		// NOTE: At this point, curr_dl_entry should be pointing to the last
		//     entry to be checked out.  Nothing has been modified yet so it
		//     is easy to just return zero above in case of an error.
		//

		// Update the list to add the checked out entries
		if (list->length == 0) {
			// Transfer portion of free queue to beginning of the list
			list->first                  = queue_free.first;
		} else {
			// Transfer portion of free queue to end of the list
			dl_entry_next(list->last)       = queue_free.first;
			dl_entry_prev(queue_free.first) = list->last;
		}

		list->last         = curr_dl_entry;
		list->length      += num_checkout;

		// Update the free queue to remove the checked out entries
		queue_free.first   = dl_entry_next(curr_dl_entry);
		queue_free.length -= num_checkout;

		// Modify the entries to break the two lists
		dl_entry_next(list->last)       = NULL;
		dl_entry_prev(queue_free.first) = NULL;

	} else {
		// The free queue will be empty after checkout
		//
		num_checkout = queue_free.length;

		// Update the list to add the checked out entries
		if (list->length == 0) {
			// Transfer free queue to beginning of the list
			list->first = queue_free.first;
		} else {
			// Transfer free queue to end of the list
			dl_entry_next(list->last)       = queue_free.first;
			dl_entry_prev(queue_free.first) = list->last;
		}

		list->last    = queue_free.last;
		list->length += queue_free.length;

		// The free queue should now be empty
		queue_free.first  = NULL;
		queue_free.last   = NULL;
		queue_free.length = 0;
	}

	// Restore interrupts
	wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
#endif

	return num_checkout;
}



/**
 * @brief Checks in multiple queue entries into the free pool
 *
 * The queue framework maintains a pool of free queue entries. This function will check in
 * all queue entries from the provided list to the free pool.
 *
 * @param dl_list* list Pointer to doubly linked list from which queue entries will be checked in.
 *
 * @return Number of queue entries successfully checked in
 */
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
		dl_entry_insertEnd(&queue_free, curr_dl_entry);
	}
#else
	u32                 num_checkin;
	interrupt_state_t   curr_interrupt_state;

	num_checkin          = list->length;
	curr_interrupt_state = wlan_mac_high_interrupt_stop();

	// Update the free queue
	if (queue_free.length != 0) {
		// Stitch lists together
		dl_entry_prev(list->first)     = queue_free.last;
		dl_entry_next(queue_free.last) = list->first;
	} else {
		// Update the first entry pointer
		queue_free.first  = list->first;
	}

	queue_free.last    = list->last;
	queue_free.length += num_checkin;

	// Remove all elements from the list
	list->first  = NULL;
	list->last   = NULL;
	list->length = 0;

	wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
#endif

	return num_checkin;
}



/**
 * @brief Dequeues one packet and submits it the lower MAC for wireless transmission
 *
 * This function is the link between the Tx queue framework and the lower-level MAC. The calling
 * function has determined the next wireless transmission should come from the queue with ID queue_sel.
 * This function attempts to checkout 1 packet from that queue. If a packet is available it is
 * passed to wlan_mac_high_mpdu_transmit(), which handles the actual copy-to-pkt-buf and Tx process.
 *
 * When a packet is successfully de-queued and submitted for transmission the corresponding queue entry
 * (tx_queue_element) is returned to the free pool.
 *
 * This function returns 0 if no packets are available in the requested queue or if the MAC state machine
 * is not ready to submit a new packet to the lower MAC for transmission.
 *
 * @param u16 queue_sel Queue ID from which to dequeue packet
 *
 * @return Number of successfully transmitted packets (0 or 1)
 */
inline int dequeue_transmit_checkin(u16 queue_sel){
	int return_value = 0;
	int tx_pkt_buf = -1;
	tx_queue_element* curr_tx_queue_element;

	tx_pkt_buf = wlan_mac_high_lock_new_tx_packet_buffer();

	if(tx_pkt_buf != -1){
		curr_tx_queue_element = dequeue_from_head(queue_sel);

		if(curr_tx_queue_element != NULL){
			return_value = 1;
			wlan_mac_high_mpdu_transmit(curr_tx_queue_element, tx_pkt_buf);
			queue_checkin(curr_tx_queue_element);
			wlan_eth_dma_update();
		} else {
			wlan_mac_high_release_tx_packet_buffer(tx_pkt_buf);
		}
	}
	return return_value;
}
