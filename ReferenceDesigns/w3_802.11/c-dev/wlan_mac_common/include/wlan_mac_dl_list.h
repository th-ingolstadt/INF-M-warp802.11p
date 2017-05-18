/** @file wlan_mac_dl_list.h
 *  @brief Doubly-linked List Framework
 *
 *  This contains the code for managing doubly-linked lists.
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

#ifndef WLAN_MAC_DL_LIST_H_
#define WLAN_MAC_DL_LIST_H_

/***************************** Include Files *********************************/
#include "wlan_mac_common_types.h"

/*************************** Constant Definitions ****************************/

//-----------------------------------------------
// Macros
//
#define dl_entry_next(x)                                (((x))->next)
#define dl_entry_prev(x)                                (((x))->prev)

/*************************** Function Prototypes *****************************/

void dl_list_init(dl_list* list);
void dl_entry_insertAfter(dl_list* list, dl_entry* entry, dl_entry* entry_new);
void dl_entry_insertBefore(dl_list* list, dl_entry* entry, dl_entry* entry_new);
void dl_entry_insertBeginning(dl_list* list, dl_entry* entry_new);
void dl_entry_insertEnd(dl_list* list, dl_entry* entry_new);
int  dl_entry_move(dl_list* src_list, dl_list* dest_list, u16 num_entries);
void dl_entry_remove(dl_list* list, dl_entry* entry);


#endif /* WLAN_MAC_DL_LIST_H_ */
