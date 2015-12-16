/** @file wlan_mac_dl_list.c
 *  @brief Doubly-linked List Framework
 *
 *  This contains the code for managing doubly-linked lists.
 *
 *  @copyright Copyright 2014-2015, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */


/***************************** Include Files *********************************/

// Xilinx / Standard library includes
#include "stdio.h"

// WLAN includes
#include "wlan_mac_high.h"
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



void dl_entry_insertAfter(dl_list* list, dl_entry* entry, dl_entry* entry_new){
    interrupt_state_t   curr_interrupt_state;

    if (entry_new != NULL) {
        curr_interrupt_state = wlan_mac_high_interrupt_stop();

        dl_entry_prev(entry_new) = entry;
        dl_entry_next(entry_new) = dl_entry_next(entry);

        if(dl_entry_next(entry) == NULL){
            list->last = entry_new;
        } else {
            dl_entry_prev(dl_entry_next(entry)) = entry_new;
        }
        dl_entry_next(entry) = entry_new;

        (list->length)++;

        wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
    } else {
        xil_printf("ERROR: Attempted to insert a NULL dl_entry\n");
    }
}


void dl_entry_insertBefore(dl_list* list, dl_entry* entry, dl_entry* entry_new) {
    interrupt_state_t   curr_interrupt_state;

    if (entry_new != NULL) {
        curr_interrupt_state = wlan_mac_high_interrupt_stop();

        dl_entry_prev(entry_new) = dl_entry_prev(entry);
        dl_entry_next(entry_new) = entry;

        if(dl_entry_prev(entry) == NULL){
            list->first = entry_new;
        } else {
            dl_entry_next(dl_entry_prev(entry)) = entry_new;
        }
        dl_entry_prev(entry) = entry_new;

        (list->length)++;

        wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
    } else {
        xil_printf("ERROR: Attempted to insert a NULL dl_entry\n");
    }
}


void dl_entry_insertBeginning(dl_list* list, dl_entry* entry_new) {
    interrupt_state_t   curr_interrupt_state;

    if (entry_new != NULL) {
        if(list->first == NULL){
            curr_interrupt_state = wlan_mac_high_interrupt_stop();

            list->first              = entry_new;
            list->last               = entry_new;

            dl_entry_prev(entry_new) = NULL;
            dl_entry_next(entry_new) = NULL;

            (list->length)++;

            wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
        } else {
            dl_entry_insertBefore(list, list->first, entry_new);
        }
    } else {
        xil_printf("ERROR: Attempted to insert a NULL dl_entry\n");
    }
}


void dl_entry_insertEnd(dl_list* list, dl_entry* entry_new) {
    if (entry_new != NULL) {
        if (list->last == NULL) {
            dl_entry_insertBeginning(list, entry_new);
        } else {
            dl_entry_insertAfter(list, list->last, entry_new);
        }
    } else {
        xil_printf("ERROR: Attempted to insert a NULL dl_entry\n");
    }
}


void dl_entry_remove(dl_list* list, dl_entry* entry) {
    interrupt_state_t   curr_interrupt_state;

    if (list->length > 0) {
        if (entry != NULL) {
            curr_interrupt_state = wlan_mac_high_interrupt_stop();

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

            wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
        } else {
            xil_printf("ERROR: Attempted to remove a NULL dl_entry\n");
        }
    } else {
        xil_printf("ERROR: Attempted to remove dl_entry from dl_list that has nothing in it\n");
    }
}


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
