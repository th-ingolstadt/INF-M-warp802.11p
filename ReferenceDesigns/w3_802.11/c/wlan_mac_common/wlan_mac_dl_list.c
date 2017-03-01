/** @file wlan_mac_dl_list.c
 *  @brief Doubly-linked List Framework
 *
 *  This contains the code for managing doubly-linked (DL) lists.
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */


/***************************** Include Files *********************************/

// Xilinx / Standard library includes
#include "stdio.h"

// WLAN includes
#include "wlan_mac_common.h"

#if WLAN_COMPILE_FOR_CPU_HIGH
#include "wlan_mac_high.h"
#endif
#include "wlan_mac_dl_list.h"


/******************************** Functions **********************************/

/*****************************************************************************/
/**
 * NOTE:  Given that list operations can occur outside of an interrupt context,
 * specifically wlan_exp can add and remove entries from lists in code that is
 * interruptible, all list operations should be atomic (ie interrupts should be
 * disabled before manipulating the list and re-enabled after the list has been
 * modified.
 *
 *****************************************************************************/


/*****************************************************************************/
/**
 * @brief   Initialize a dl_list
 *
 * @param   dl_list * list        - DL list to initialize
 *
 * @return  None
 *
 *****************************************************************************/
void dl_list_init(dl_list* list) {
    if (list != NULL) {
        list->first  = NULL;
        list->last   = NULL;
        list->length = 0;
    } else {
        xil_printf("ERROR: Attempted to initialize a NULL list\n");
    }
    return;
}



/*****************************************************************************/
/**
 * @brief   Insert dl_entry after provided dl_entry in provided dl_list
 *
 * @param   dl_list  * list       - DL list to insert new entry
 * @param   dl_entry * entry      - DL entry in DL list to insert entry after
 * @param   dl_entry * new_entry  - DL entry to insert into list
 *
 * @return  None
 *
 *****************************************************************************/
void dl_entry_insertAfter(dl_list* list, dl_entry* entry, dl_entry* new_entry){
#if WLAN_COMPILE_FOR_CPU_HIGH
	interrupt_state_t   curr_interrupt_state;
#endif

    if (new_entry != NULL) {
#if WLAN_COMPILE_FOR_CPU_HIGH
        curr_interrupt_state = wlan_mac_high_interrupt_stop();
#endif

        dl_entry_prev(new_entry) = entry;
        dl_entry_next(new_entry) = dl_entry_next(entry);

        if(dl_entry_next(entry) == NULL){
            list->last = new_entry;
        } else {
            dl_entry_prev(dl_entry_next(entry)) = new_entry;
        }
        dl_entry_next(entry) = new_entry;

        (list->length)++;
#if WLAN_COMPILE_FOR_CPU_HIGH
        wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
#endif
    } else {
        xil_printf("ERROR: Attempted to insert a NULL dl_entry\n");
    }
}



/*****************************************************************************/
/**
 * @brief   Insert dl_entry before provided dl_entry in provided dl_list
 *
 * @param   dl_list  * list       - DL list to insert new entry
 * @param   dl_entry * entry      - DL entry in DL list to insert entry before
 * @param   dl_entry * new_entry  - DL entry to insert into list
 *
 * @return  None
 *
 *****************************************************************************/
void dl_entry_insertBefore(dl_list* list, dl_entry* entry, dl_entry* new_entry) {
#if WLAN_COMPILE_FOR_CPU_HIGH
    interrupt_state_t   curr_interrupt_state;
#endif

    if (new_entry != NULL) {
#if WLAN_COMPILE_FOR_CPU_HIGH
        curr_interrupt_state = wlan_mac_high_interrupt_stop();
#endif

        dl_entry_prev(new_entry) = dl_entry_prev(entry);
        dl_entry_next(new_entry) = entry;

        if(dl_entry_prev(entry) == NULL){
            list->first = new_entry;
        } else {
            dl_entry_next(dl_entry_prev(entry)) = new_entry;
        }
        dl_entry_prev(entry) = new_entry;

        (list->length)++;
#if WLAN_COMPILE_FOR_CPU_HIGH
        wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
#endif
    } else {
        xil_printf("ERROR: Attempted to insert a NULL dl_entry\n");
    }
}



/*****************************************************************************/
/**
 * @brief   Insert dl_entry at the beginning of the provided dl_list
 *
 * @param   dl_list  * list       - DL list to insert new entry
 * @param   dl_entry * new_entry  - DL entry to insert into list
 *
 * @return  None
 *
 *****************************************************************************/
void dl_entry_insertBeginning(dl_list* list, dl_entry* new_entry) {
#if WLAN_COMPILE_FOR_CPU_HIGH
    interrupt_state_t   curr_interrupt_state;
#endif

    if (new_entry != NULL) {
        if(list->first == NULL){
#if WLAN_COMPILE_FOR_CPU_HIGH
            curr_interrupt_state = wlan_mac_high_interrupt_stop();
#endif

            list->first              = new_entry;
            list->last               = new_entry;

            dl_entry_prev(new_entry) = NULL;
            dl_entry_next(new_entry) = NULL;

            (list->length)++;
#if WLAN_COMPILE_FOR_CPU_HIGH
            wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
#endif
        } else {
            dl_entry_insertBefore(list, list->first, new_entry);
        }
    } else {
        xil_printf("ERROR: Attempted to insert a NULL dl_entry\n");
    }
}



/*****************************************************************************/
/**
 * @brief   Insert dl_entry at the end of the provided dl_list
 *
 * @param   dl_list  * list       - DL list to insert new entry
 * @param   dl_entry * new_entry  - DL entry to insert into list
 *
 * @return  None
 *
 *****************************************************************************/
void dl_entry_insertEnd(dl_list* list, dl_entry* new_entry) {
    if (new_entry != NULL) {
        if (list->last == NULL) {
            dl_entry_insertBeginning(list, new_entry);
        } else {
            dl_entry_insertAfter(list, list->last, new_entry);
        }
    } else {
        xil_printf("ERROR: Attempted to insert a NULL dl_entry\n");
    }
}



/*****************************************************************************/
/**
 * @brief   Move multiple dl_entrys from the beginning of the src_list to the end
 *     of the dest_list
 *
 * @param   dl_list  * src_list   - DL list to remove entries (from beginning)
 * @param   dl_list  * dest_list  - DL list to add entries (to end)
 * @param   u16 num_entries       - Number of entries to move
 *
 * @return  int                   - Number of entries moved
 *
 *****************************************************************************/
int dl_entry_move(dl_list* src_list, dl_list* dest_list, u16 num_entries){
	int num_moved;
	u32 idx;
	dl_entry* src_first;      //Pointer to the first entry of the list that will be moved
	dl_entry* src_last;       //Pointer to the last entry of the list that will be moved
	dl_entry* src_remaining;  //Pointer to the first entry that remains in the src list after the move (can be NULL if none remain)

#if WLAN_COMPILE_FOR_CPU_HIGH
    interrupt_state_t   curr_interrupt_state;
#endif

    // If the caller is not moving any entries or if there are no entries
    // in the source list, then just return
    if ((num_entries == 0) || (src_list->length == 0)) { return 0; }

#if WLAN_COMPILE_FOR_CPU_HIGH
    // Stop the interrupts since we are modifying list state
    curr_interrupt_state = wlan_mac_high_interrupt_stop();
#endif


    //1. Assign the dl_entry* endpoint markers
    src_first = src_list->first;

    if( num_entries >= src_list->length) {
    	num_moved = src_list->length;
    	src_last = src_list->last;
    	src_remaining = NULL;
    } else {
    	num_moved = num_entries;
    	//Because we are moving a subset of the src_list,
    	//we need to traverse it and find the last entry
    	//we will move
    	src_last = src_list->first;
    	for(idx = 0; idx < (num_entries - 1); idx++){
    		src_last = dl_entry_next(src_last);
    	}
    	src_remaining = dl_entry_next(src_last);
    }

    //2. Stitch together the endpoints on the two lists
    if(dest_list->length > 0){
    	// There are currently entries in the destination list, so we will have to touch
    	// the last entry to connect it to src_first
    	dl_entry_next(dest_list->last) = src_first;
    	dl_entry_prev(src_first) = dest_list->last;
    } else {
    	dest_list->first = src_first;
    	dl_entry_prev(src_first) = NULL;
    }

    dl_entry_next(src_last) = NULL;
    dest_list->last = src_last;
    dest_list->length += num_moved;

    //3. Clean up the source list
    if (src_remaining != NULL){
    	// There are remaining entries in the source list that we need to clean up
    	src_list->first = src_remaining;
    	dl_entry_prev(src_remaining) = NULL;
    } else {
    	src_list->first = NULL;
    }
    src_list->length -= num_moved;

#if WLAN_COMPILE_FOR_CPU_HIGH
    // Restore interrupts
    wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
#endif

    return num_moved;
}



/*****************************************************************************/
/**
 * @brief   Remove dl_entry from dl_list
 *
 * @param   dl_list  * list       - DL list to remove entry
 * @param   dl_entry * entry      - DL entry to remove from list
 *
 * @return  None
 *
 *****************************************************************************/
void dl_entry_remove(dl_list* list, dl_entry* entry) {
#if WLAN_COMPILE_FOR_CPU_HIGH
    interrupt_state_t   curr_interrupt_state;
#endif

    if (list->length > 0) {
        if (entry != NULL) {
#if WLAN_COMPILE_FOR_CPU_HIGH
            curr_interrupt_state = wlan_mac_high_interrupt_stop();
#endif

            if(dl_entry_prev(entry) == NULL){
                list->first = dl_entry_next(entry);
            } else {
                dl_entry_next(dl_entry_prev(entry)) = dl_entry_next(entry);
            }

            if(dl_entry_next(entry) == NULL){
                list->last = dl_entry_prev(entry);
            } else {
                dl_entry_prev(dl_entry_next(entry)) = dl_entry_prev(entry);
            }

            (list->length)--;

            // NULL fields in removed entry
            //     NOTE:  This will help in the case of pointers to "stale" entries.
            //
            //     NOTE:  There was discussion about whether to set the entry "data" pointer
            //         to NULL.  Currently, the code does not do this because the first
            //         priority of the reference design is to not crash in hard to debug
            //         ways.  Trying to access or fill in a NULL data pointer would cause
            //         the node to crash in non-obvious ways.  This decision will be revisited
            //         in future revisions of the reference design.
            //
            entry->next = NULL;
            entry->prev = NULL;
#if WLAN_COMPILE_FOR_CPU_HIGH
            wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
#endif
        } else {
            xil_printf("ERROR: Attempted to remove a NULL dl_entry\n");
        }
    } else {
        xil_printf("ERROR: Attempted to remove dl_entry from dl_list that has nothing in it\n");
    }
}


