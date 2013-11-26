////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_dl_list.h
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

#ifndef WLAN_MAC_DL_LIST_H_

#include "xil_types.h"

#define WLAN_MAC_DL_LIST_H_

typedef struct dl_node dl_node;
struct dl_node{
	dl_node* next;
	dl_node* prev;
	void*	 container;
};

typedef struct {
	dl_node* first;
	dl_node* last;
	u16 length;
} dl_list;

void dl_node_insertAfter(dl_list* list, dl_node* node, dl_node* node_new);
void dl_node_insertBefore(dl_list* list, dl_node* node, dl_node* node_new);
void dl_node_insertBeginning(dl_list* list, dl_node* node_new);
void dl_node_insertEnd(dl_list* list, dl_node* node_new);
void dl_node_remove(dl_list* list, dl_node* node);
void dl_list_init(dl_list* list);


#endif /* WLAN_MAC_DL_LIST_H_ */
