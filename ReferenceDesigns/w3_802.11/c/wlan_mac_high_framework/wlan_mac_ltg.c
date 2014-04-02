/** @file wlan_mac_ltg.c
 *  @brief Local Traffic Generator
 *
 *  This contains code for scheduling local traffic directly from the
 *  board.
 *
 *  @copyright Copyright 2014, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 *  @bug No known bugs.
 */

#include "xil_types.h"
#include "stdlib.h"
#include "stdio.h"
#include "xparameters.h"
#include "xintc.h"
#include "string.h"
#include "wlan_mac_dl_list.h"

#include "wlan_mac_ipc_util.h"
#include "wlan_mac_high.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_schedule.h"

static dl_list tg_list;

static function_ptr_t ltg_callback;

static u64 num_sched_checks;
static u32 schedule_id;
static u8  schedule_running;

int wlan_mac_ltg_sched_init(){

	int return_value = 0;
	schedule_running = 0;
	num_sched_checks = 0;
	ltg_sched_remove(LTG_REMOVE_ALL);
	dl_list_init(&tg_list);
	ltg_callback = (function_ptr_t)nullCallback;

	return return_value;
}

void wlan_mac_ltg_sched_set_callback(void(*callback)()){
	ltg_callback = (function_ptr_t)callback;
}

int ltg_sched_configure(u32 id, u32 type, void* params, void* callback_arg, void(*cleanup_callback)()){
	//This function can be called on uninitalized schedule IDs or on schedule IDs that had
	//been previously configured. In the event that they had been previously configured, care
	//must be taken by the calling application to free previous the previous callback_arg if
	//applicable.

	tg_schedule* curr_tg;
	dl_entry*	 curr_tg_dl_entry;

	u8 create_new = 1;
	u8 is_enabled = 0;

	curr_tg_dl_entry = ltg_sched_find_tg_schedule(id);

	if(curr_tg_dl_entry != NULL){
		//A schedule with this ID has already been configured. We'll destroy it
		//and overwrite its parameters.
		curr_tg = (tg_schedule*)(curr_tg_dl_entry->data);

		create_new = 0;
		is_enabled = ((ltg_sched_state_hdr*)(curr_tg->state))->enabled;

		//Stop this LTG to make this function interrupt safe. The ltg_sched_check function
		//might be called at any point during this function. A partially-configured LTG
		//that is currently running would result in very difficult-to-debug results if
		//it happens to execute.
		((ltg_sched_state_hdr*)(curr_tg->state))->enabled = 0;
		ltg_sched_destroy_params(curr_tg);
	}

	//Create a new tg for this id if we didn't find it in the list
	if(create_new){
		curr_tg_dl_entry = ltg_sched_create();
		dl_entry_insertEnd(&tg_list,curr_tg_dl_entry);
	}

	if(curr_tg_dl_entry != NULL){

		curr_tg = (tg_schedule*)(curr_tg_dl_entry->data);

		curr_tg->id = id;
		curr_tg->type = type;
		curr_tg->cleanup_callback = (function_ptr_t)cleanup_callback;
		switch(type){
			case LTG_SCHED_TYPE_PERIODIC:
				curr_tg->params = wlan_mac_high_malloc(sizeof(ltg_sched_periodic_params));
				curr_tg->state = wlan_mac_high_malloc(sizeof(ltg_sched_periodic_state));

				if(curr_tg->params != NULL){
					memcpy(curr_tg->params, params, sizeof(ltg_sched_periodic_params));
					curr_tg->callback_arg = callback_arg;
				} else {
					xil_printf("Failed to initialize parameter struct\n");
					ltg_sched_destroy(curr_tg_dl_entry);
					return -1;
				}
			break;
			case LTG_SCHED_TYPE_UNIFORM_RAND:
				curr_tg->params = wlan_mac_high_malloc(sizeof(ltg_sched_uniform_rand_params));
				curr_tg->state = wlan_mac_high_malloc(sizeof(ltg_sched_uniform_rand_params));

				if(curr_tg->params != NULL){
					memcpy(curr_tg->params, params, sizeof(ltg_sched_uniform_rand_params));
					curr_tg->callback_arg = callback_arg;
				} else {
					xil_printf("Failed to initialize parameter struct\n");
					ltg_sched_destroy(curr_tg_dl_entry);
					return -1;
				}
			break;
			default:
				xil_printf("Unknown type %d, destroying tg_schedule struct\n");
				ltg_sched_destroy(curr_tg_dl_entry);
				return -1;
			break;
		}
		((ltg_sched_state_hdr*)(curr_tg->state))->enabled = is_enabled;


	} else {
		xil_printf("Failed to initialize tg_schedule struct\n");
		return -1;
	}

	return 0;
}

int ltg_sched_start(u32 id){
	dl_entry*	curr_tg_dl_entry;

	curr_tg_dl_entry = ltg_sched_find_tg_schedule(id);
	if(curr_tg_dl_entry != NULL){
		return ltg_sched_start_l(curr_tg_dl_entry);
	}
	else {
		xil_printf("Failed to start LTG ID: %d. Please ensure LTG is configured before starting\n", id);
		return -1;
	}

}


int ltg_sched_start_all(){
	u32 i;
	u32 list_len;
	int ret_val = 0;
	tg_schedule* curr_tg;
	dl_entry* next_tg_dl_entry;
	dl_entry* curr_tg_dl_entry;

	list_len = tg_list.length;

	next_tg_dl_entry = tg_list.first;
	for(i = 0; i < list_len; i++ ){
		curr_tg_dl_entry = next_tg_dl_entry;
		next_tg_dl_entry = dl_entry_next(next_tg_dl_entry);

		curr_tg = (tg_schedule*)(curr_tg_dl_entry->data);

		if(ltg_sched_start_l(curr_tg_dl_entry) == -1) {
			xil_printf("Failed to start LTG ID: %d. Please ensure LTG is configured before starting\n", (curr_tg->id));
			ret_val = -1;
		}
	}

	return ret_val;
}


int ltg_sched_start_l(dl_entry* curr_tg_dl_entry){
	tg_schedule* curr_tg = (tg_schedule*)(curr_tg_dl_entry->data);
	u64 random_timestamp;

	switch(curr_tg->type){
		case LTG_SCHED_TYPE_PERIODIC:
			curr_tg->target = num_sched_checks + (((ltg_sched_periodic_params*)(curr_tg->params))->interval_count);
			((ltg_sched_state_hdr*)(curr_tg->state))->enabled = 1;
		break;
		case LTG_SCHED_TYPE_UNIFORM_RAND:
			random_timestamp = (rand()%(((ltg_sched_uniform_rand_params*)(curr_tg->params))->max_interval_count - ((ltg_sched_uniform_rand_params*)(curr_tg->params))->min_interval_count))+((ltg_sched_uniform_rand_params*)(curr_tg->params))->min_interval_count;
			curr_tg->target = num_sched_checks + (random_timestamp/FAST_TIMER_DUR_US);
			((ltg_sched_state_hdr*)(curr_tg->state))->enabled = 1;
		break;

		default:
			xil_printf("Unknown type %d, destroying tg_schedule struct\n");
			ltg_sched_destroy(curr_tg_dl_entry);
			return -1;
		break;
	}

	if(schedule_running == 0){
		schedule_running = 1;

		schedule_id = wlan_mac_schedule_event_repeated(SCHEDULE_FINE, 0, SCHEDULE_REPEAT_FOREVER, (void*)ltg_sched_check);
	}

	return 0;
}


void ltg_sched_check(){
	tg_schedule* curr_tg;
	dl_entry*	 curr_tg_dl_entry;

	u32 i;

	num_sched_checks++;
	if(tg_list.length > 0){

		curr_tg_dl_entry = tg_list.first;
		for(i = 0; i < tg_list.length; i++ ){
			curr_tg = (tg_schedule*)(curr_tg_dl_entry->data);

			if(((ltg_sched_state_hdr*)(curr_tg->state))->enabled && num_sched_checks >= ( curr_tg->target) ){
				ltg_sched_stop_l(curr_tg_dl_entry);
				ltg_sched_start_l(curr_tg_dl_entry);
				ltg_callback(curr_tg->id, curr_tg->callback_arg);
			}
			curr_tg_dl_entry = dl_entry_next(curr_tg_dl_entry);
		}
	}

	return;
}

int ltg_sched_stop(u32 id){
	dl_entry*	 curr_tg_dl_entry;

	curr_tg_dl_entry = ltg_sched_find_tg_schedule(id);
	if(curr_tg_dl_entry != NULL){
		return ltg_sched_stop_l(curr_tg_dl_entry);
	}
	return -1;
}


int ltg_sched_stop_all(){
	u32 i;
	u32 list_len;
	dl_entry*    next_tg_dl_entry;
	dl_entry*    curr_tg_dl_entry;

	list_len = tg_list.length;

	next_tg_dl_entry = tg_list.first;
	for(i = 0; i < list_len; i++ ){
		curr_tg_dl_entry = next_tg_dl_entry;
		next_tg_dl_entry = dl_entry_next(curr_tg_dl_entry);
		ltg_sched_stop_l(curr_tg_dl_entry);
	}

	return 0;
}


int ltg_sched_stop_l(dl_entry* curr_tg_dl_entry){
	tg_schedule* curr_tg = (tg_schedule*)(curr_tg_dl_entry->data);

	((ltg_sched_state_hdr*)(curr_tg->state))->enabled = 0;
	if(tg_list.length == 0 && schedule_running == 1){
		wlan_mac_remove_schedule(SCHEDULE_FINE, schedule_id);
		schedule_running = 0;
	}
	return 0;
}

int ltg_sched_get_state(u32 id, u32* type, void** state){
	//This function returns the type of schedule corresponding to the id argument
	//It fills in the state argument with the state of the schedule

	tg_schedule* curr_tg;
	dl_entry*	 curr_tg_dl_entry;

	curr_tg_dl_entry = ltg_sched_find_tg_schedule(id);
	if(curr_tg_dl_entry == NULL){
		return -1;
	}

	curr_tg = (tg_schedule*)(curr_tg_dl_entry->data);

	*type = curr_tg->type;
	*state = curr_tg->state;

	switch(curr_tg->type){
		case LTG_SCHED_TYPE_PERIODIC:
			if(num_sched_checks < (curr_tg->target) ){
				((ltg_sched_periodic_state*)(curr_tg->state))->time_to_next_count = (u32)(curr_tg->target - num_sched_checks);
			} else {
				((ltg_sched_periodic_state*)(curr_tg->state))->time_to_next_count = 0;
			}
		break;

		case LTG_SCHED_TYPE_UNIFORM_RAND:
			if(num_sched_checks < (curr_tg->target) ){
				((ltg_sched_uniform_rand_state*)(curr_tg->state))->time_to_next_usec = (u32)(curr_tg->target - num_sched_checks);
			} else {
				((ltg_sched_uniform_rand_state*)(curr_tg->state))->time_to_next_usec = 0;
			}
		break;
		default:
			xil_printf("Unknown type %d\n", curr_tg->type);
			return -1;
		break;
	}


	return 0;

}
int ltg_sched_get_params(u32 id, void** params){
	//This function returns the type of the schedule corresponding to the id argument
	//It fills in the current parameters of the schedule into the params argument
	tg_schedule* curr_tg;
	dl_entry*	 curr_tg_dl_entry;

	curr_tg_dl_entry = ltg_sched_find_tg_schedule(id);
	if(curr_tg_dl_entry == NULL){
		return -1;
	}

	curr_tg = (tg_schedule*)(curr_tg_dl_entry->data);

	*params = curr_tg->params;

	return 0;
}

int ltg_sched_get_callback_arg(u32 id, void** callback_arg){
	tg_schedule* curr_tg;
	dl_entry*	 curr_tg_dl_entry;

	curr_tg_dl_entry = ltg_sched_find_tg_schedule(id);
	if(curr_tg_dl_entry == NULL){
		return -1;
	}

	curr_tg = (tg_schedule*)(curr_tg_dl_entry->data);

	*callback_arg = curr_tg->callback_arg;

	return 0;
}



int ltg_sched_remove(u32 id){
//	u32 i;
//	u32 list_len;
	tg_schedule* curr_tg;
	dl_entry* 	 next_tg_dl_entry;
	dl_entry* 	 curr_tg_dl_entry;

//	list_len = tg_list.length;

	next_tg_dl_entry = tg_list.first;
	//for(i = 0; i < list_len; i++ ){
	while(next_tg_dl_entry != NULL){
		curr_tg_dl_entry = next_tg_dl_entry;
		next_tg_dl_entry = dl_entry_next(curr_tg_dl_entry);

		curr_tg = (tg_schedule*)(curr_tg_dl_entry->data);

		if( (curr_tg->id)==id || id == LTG_REMOVE_ALL){
			dl_entry_remove(&tg_list, curr_tg_dl_entry);
			ltg_sched_stop_l(curr_tg_dl_entry);
			curr_tg->cleanup_callback(curr_tg->id, curr_tg->callback_arg);
			ltg_sched_destroy(curr_tg_dl_entry);
			if(id != LTG_REMOVE_ALL) return 0;
		}
	}

	if(id != LTG_REMOVE_ALL){
		return -1;
	} else {
		return 0;
	}
}

dl_entry* ltg_sched_create(){
	dl_entry* curr_tg_dl_entry;
	tg_schedule* curr_tg;

	curr_tg_dl_entry = (dl_entry*)wlan_mac_high_malloc(sizeof(dl_entry));

	if(curr_tg_dl_entry == NULL){
		return NULL;
	}

	curr_tg = (tg_schedule*)wlan_mac_high_malloc(sizeof(tg_schedule));

	if(curr_tg == NULL){
		wlan_mac_high_free(curr_tg_dl_entry);
		return NULL;
	}

	curr_tg_dl_entry->data = (void*)curr_tg;

	return curr_tg_dl_entry;
}

void ltg_sched_destroy_params(tg_schedule *tg){
	switch(tg->type){
		case LTG_SCHED_TYPE_PERIODIC:
		case LTG_SCHED_TYPE_UNIFORM_RAND:
			wlan_mac_high_free(tg->params);
			wlan_mac_high_free(tg->state);
		break;
	}
}

void ltg_sched_destroy(dl_entry* tg_dl_entry){
	tg_schedule* curr_tg;

	curr_tg = (tg_schedule*)(tg_dl_entry->data);

	ltg_sched_destroy_params(curr_tg);
	wlan_mac_high_free(tg_dl_entry);
	wlan_mac_high_free(curr_tg);
	return;
}

dl_entry* ltg_sched_find_tg_schedule(u32 id){
	u32 i;
	dl_entry*	 curr_tg_dl_entry;
	tg_schedule* curr_tg;

	curr_tg_dl_entry = tg_list.first;
	for(i = 0; i < tg_list.length; i++ ){
		curr_tg = (tg_schedule*)(curr_tg_dl_entry->data);
		if( (curr_tg->id)==id){
			return curr_tg_dl_entry;
		}
		curr_tg_dl_entry = dl_entry_next(curr_tg_dl_entry);
	}
	return NULL;
}


// NOTE:  The src information is from the network and must be byte swapped
void * ltg_sched_deserialize(u32 * src, u32 * ret_type, u32 * ret_size) {
	u32    temp;
    u16    type;
    u16    size;
    void * ret_val = NULL;

    temp  = Xil_Ntohl(src[0]);
    type  = (temp >> 16) & 0xFFFF;
    size  = (temp & 0xFFFF);

    // xil_printf("LTG Sched:  type = %d, size = %d\n", type, size);

    switch(type){
        case LTG_SCHED_TYPE_PERIODIC:
        	if (size == 1){
        		ret_val = (void *) wlan_mac_high_malloc(sizeof(ltg_sched_periodic_params));
        	    if (ret_val != NULL){
        	    	((ltg_sched_periodic_params *)ret_val)->interval_count = (Xil_Ntohl(src[1]))/LTG_POLL_INTERVAL;

        	    	xil_printf("LTG Sched Periodic: %d usec\n", LTG_POLL_INTERVAL*((ltg_sched_periodic_params *)ret_val)->interval_count);
        	    }
        	}
    	break;

        case LTG_SCHED_TYPE_UNIFORM_RAND:
        	if (size == 2){
        		ret_val = (void *) wlan_mac_high_malloc(sizeof(ltg_sched_uniform_rand_params));
        	    if (ret_val != NULL){
        	    	((ltg_sched_uniform_rand_params *)ret_val)->min_interval_count = Xil_Ntohl(src[1])/LTG_POLL_INTERVAL;
        	    	((ltg_sched_uniform_rand_params *)ret_val)->max_interval_count = Xil_Ntohl(src[2])/LTG_POLL_INTERVAL;

        	    	xil_printf("LTG Sched Uniform Rand: [%d %d] usec\n", LTG_POLL_INTERVAL*((ltg_sched_uniform_rand_params *)ret_val)->min_interval_count, LTG_POLL_INTERVAL*((ltg_sched_uniform_rand_params *)ret_val)->max_interval_count);
        	    }
        	}
        break;
    }

    // Set return values
    *ret_type = type;
    *ret_size = size;
	return ret_val;
}


// NOTE:  The src information is from the network and must be byte swapped
void * ltg_payload_deserialize(u32 * src, u32 * ret_type, u32 * ret_size) {
	u32    temp;
    u16    type;
    u16    size;
    void * ret_val = NULL;

    temp  = Xil_Ntohl(src[0]);
    type  = (temp >> 16) & 0xFFFF;
    size  = (temp & 0xFFFF);

    // xil_printf("LTG Payload:  type = %d, size = %d\n", type, size);

    switch(type){
        case LTG_PYLD_TYPE_FIXED:
        	if (size == 1){
        		ret_val = (void *) wlan_mac_high_malloc(sizeof(ltg_pyld_fixed));
        	    if (ret_val != NULL){
					((ltg_pyld_fixed *)ret_val)->hdr.type = LTG_PYLD_TYPE_FIXED;
        	    	((ltg_pyld_fixed *)ret_val)->length   = Xil_Ntohl(src[1]) & 0xFFFF;

        	    	xil_printf("LTG Payload Fixed: %d bytes\n", ((ltg_pyld_fixed *)ret_val)->length);
        	    }
        	}
    	break;

        case LTG_PYLD_TYPE_UNIFORM_RAND:
        	if (size == 2){
        		ret_val = (void *) wlan_mac_high_malloc(sizeof(ltg_pyld_uniform_rand));
        	    if (ret_val != NULL){
					((ltg_pyld_uniform_rand *)ret_val)->hdr.type   = LTG_PYLD_TYPE_UNIFORM_RAND;
        	    	((ltg_pyld_uniform_rand *)ret_val)->min_length = Xil_Ntohl(src[1]) & 0xFFFF;
        	    	((ltg_pyld_uniform_rand *)ret_val)->max_length = Xil_Ntohl(src[2]) & 0xFFFF;

        	    	xil_printf("LTG Payload Uniform Rand: [%d %d] bytes\n", ((ltg_pyld_uniform_rand *)ret_val)->min_length, ((ltg_pyld_uniform_rand *)ret_val)->max_length);
        	    }
        	}
        break;
    }

    // Set return values
    *ret_type = type;
    *ret_size = size;
	return ret_val;
}






