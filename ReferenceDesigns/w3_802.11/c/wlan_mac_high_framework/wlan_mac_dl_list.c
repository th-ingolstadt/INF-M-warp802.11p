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


/*************************** Constant Definitions ****************************/

/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/

/*************************** Functions Prototypes ****************************/


/******************************** Functions **********************************/
void dl_node_insertAfter(dl_list* list, dl_node* node, dl_node* node_new){
	node_new->prev = node;
	node_new->next = node->next;
	if(node->next == NULL){
		list->last = node_new;
	} else {
		node->next->prev = node_new;
	}
	node->next = node_new;
	(list->length)++;
	return;
}

void dl_node_insertBefore(dl_list* list, dl_node* node, dl_node* node_new){
	node_new->prev = node->prev;
	node_new->next = node;
	if(node->prev == NULL){
		list->first = node_new;
	} else {
		node->prev->next = node_new;
	}
	node->prev = node_new;
	(list->length)++;
	return;
}

void dl_node_insertBeginning(dl_list* list, dl_node* node_new){
	if(list->first == NULL){
		list->first = node_new;
		list->last = node_new;
		node_new->prev = NULL;
		node_new->next = NULL;
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
	if(node->prev == NULL){
		list->first = node->next;
	} else {
		node->prev->next = node->next;
	}

	if(node->next == NULL){
		list->last = node->prev;
	} else {
		node->next->prev = node->prev;
	}
	(list->length)--;
}

void dl_list_init(dl_list* list){
	list->first = NULL;
	list->last = NULL;
	list->length = 0;
	return;
}
