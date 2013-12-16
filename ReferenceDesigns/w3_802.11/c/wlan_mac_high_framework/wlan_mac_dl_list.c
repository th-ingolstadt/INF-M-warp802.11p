////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_dl_list.c
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
//          Erik Welsh (welsh [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

/***************************** Include Files *********************************/
#include "wlan_mac_dl_list.h"


/******************************** Functions **********************************/
void dl_node_insertAfter(dl_list* list, dl_node* node, dl_node* node_new){
	dl_node_prev(node_new) = node;
	dl_node_next(node_new) = dl_node_next(node);
	if(dl_node_next(node) == NULL){
		list->last = node_new;
	} else {
		dl_node_prev(dl_node_next(node)) = node_new;
	}
	dl_node_next(node) = node_new;
	(list->length)++;
	return;
}

void dl_node_insertBefore(dl_list* list, dl_node* node, dl_node* node_new){
	dl_node_prev(node_new) = dl_node_prev(node);
	dl_node_next(node_new) = node;
	if(dl_node_prev(node) == NULL){
		list->first = node_new;
	} else {
		dl_node_next(dl_node_prev(node)) = node_new;
	}
	dl_node_prev(node) = node_new;
	(list->length)++;
	return;
}

void dl_node_insertBeginning(dl_list* list, dl_node* node_new){
	if(list->first == NULL){
		list->first = node_new;
		list->last = node_new;
		dl_node_prev(node_new) = NULL;
		dl_node_next(node_new) = NULL;
		(list->length)++;
	} else {
		dl_node_insertBefore(list, list->first, node_new);
	}
	return;
}

void dl_node_insertEnd(dl_list* list, dl_node* node_new){
	if(list->last == NULL){
		dl_node_insertBeginning(list,node_new);
	} else {
		dl_node_insertAfter(list,list->last, node_new);
	}
	return;
}

void dl_node_remove(dl_list* list, dl_node* node){
	if(dl_node_prev(node) == NULL){
		list->first = dl_node_next(node);
	} else {
		dl_node_next(dl_node_prev(node)) = dl_node_next(node);
	}

	if(dl_node_next(node) == NULL){
		list->last = dl_node_prev(node);
	} else {
		dl_node_prev(dl_node_next(node)) = dl_node_prev(node);
	}
	(list->length)--;
}

void dl_list_init(dl_list* list){
	list->first = NULL;
	list->last = NULL;
	list->length = 0;
	return;
}
