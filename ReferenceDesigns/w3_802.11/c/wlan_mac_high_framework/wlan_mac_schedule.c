////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_schedule.c
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
//          Erik Welsh (welsh [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

/***************************** Include Files *********************************/
#include "xparameters.h"
#include "stdlib.h"
#include "xil_types.h"
#include "wlan_mac_util.h"
#include "wlan_mac_schedule.h"
#include "xtmrctr.h"
#include "xil_exception.h"
#include "xintc.h"


/*************************** Constant Definitions ****************************/




/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/
static XTmrCtr     TimerCounterInst;
static XIntc*      InterruptController_ptr;

static u32 schedule_count;
static dl_list wlan_sched_coarse;
static dl_list wlan_sched_fine;

/*************************** Functions Prototypes ****************************/


/******************************** Functions **********************************/

int wlan_mac_schedule_init(){
	int Status;

	//Initialize internal variables
	schedule_count = 0;
	dl_list_init(&wlan_sched_coarse);
	dl_list_init(&wlan_sched_fine);

	//Set up the timer
	Status = XTmrCtr_Initialize(&TimerCounterInst, TMRCTR_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("XTmrCtr failed to initialize\n");
		return -1;
	}

	//Set the handler for Timer
	XTmrCtr_SetHandler(&TimerCounterInst, timer_handler, &TimerCounterInst);

	//Enable interrupt of timer and auto-reload so it continues repeatedly
	XTmrCtr_SetOptions(&TimerCounterInst, TIMER_CNTR_FAST, XTC_DOWN_COUNT_OPTION | XTC_INT_MODE_OPTION);
	XTmrCtr_SetOptions(&TimerCounterInst, TIMER_CNTR_SLOW, XTC_DOWN_COUNT_OPTION | XTC_INT_MODE_OPTION);


	return 0;
}

int wlan_mac_schedule_setup_interrupt(XIntc* intc){
	//Connect Timer to Interrupt Controller
	InterruptController_ptr = intc;
	return XIntc_Connect(InterruptController_ptr, TMRCTR_INTERRUPT_ID, (XInterruptHandler)XTmrCtr_CustomInterruptHandler, &TimerCounterInst);
}

u32 wlan_mac_schedule_event(u8 scheduler_sel, u32 delay, void(*callback)()){
	u64 timestamp;
	u32 id;

	wlan_sched* sched_ptr = wlan_malloc(sizeof(wlan_sched));

	if(sched_ptr == NULL){
		//malloc has failed. Return failure condition
		return SCHEDULE_FAILURE;
	}

	timestamp = get_usec_timestamp();
	id = (schedule_count++);
	sched_ptr->id = id;
	sched_ptr->callback = (function_ptr_t)callback;
	sched_ptr->target = timestamp + (u64)delay;

	//This seems circular, but it's a way to be able to figure out what kind of
	//node this is even if all you have is a pointer to the generic dl_node.
	sched_ptr->node.container = sched_ptr;

	switch(scheduler_sel){
		case SCHEDULE_COARSE:
			if(wlan_sched_coarse.length == 0){
				XTmrCtr_SetResetValue(&TimerCounterInst, TIMER_CNTR_SLOW, SLOW_TIMER_DUR_US*(TIMER_FREQ/1000000));
				XTmrCtr_Start(&TimerCounterInst, TIMER_CNTR_SLOW);
			}
			dl_node_insertEnd(&wlan_sched_coarse, &(sched_ptr->node));
		break;
		case SCHEDULE_FINE:
			if(wlan_sched_fine.length == 0){
				XTmrCtr_SetResetValue(&TimerCounterInst, TIMER_CNTR_FAST, FAST_TIMER_DUR_US*(TIMER_FREQ/1000000));
				XTmrCtr_Start(&TimerCounterInst, TIMER_CNTR_SLOW);
			}
			dl_node_insertEnd(&wlan_sched_fine, &(sched_ptr->node));
		break;
	}

	return id;
}


void timer_handler(void *CallBackRef, u8 TmrCtrNumber){
	u32 i;
	u64 timestamp;
	wlan_sched* curr_sched_ptr;
	wlan_sched* next_sched_ptr;

	timestamp = get_usec_timestamp();

	switch(TmrCtrNumber){
		case TIMER_CNTR_FAST:

			next_sched_ptr = (wlan_sched*)((wlan_sched_fine.first)->container);

			for(i=0; i<(wlan_sched_fine.length); i++){
				curr_sched_ptr = next_sched_ptr;
				next_sched_ptr = (wlan_sched*)(((curr_sched_ptr->node).next)->container);
				if(timestamp > (curr_sched_ptr->target)){
					curr_sched_ptr->callback();
					dl_node_remove(&wlan_sched_fine,&(curr_sched_ptr->node));
					wlan_free(curr_sched_ptr);
				}
			}

			if(wlan_sched_fine.length > 0){
				//There are still schedules pending. Restart the timer.
				XTmrCtr_SetResetValue(&TimerCounterInst, TIMER_CNTR_FAST, FAST_TIMER_DUR_US*(TIMER_FREQ/1000000));
				XTmrCtr_Start(&TimerCounterInst, TIMER_CNTR_FAST);
			}

		break;

		case TIMER_CNTR_SLOW:

			next_sched_ptr = (wlan_sched*)((wlan_sched_coarse.first)->container);

			for(i=0; i<(wlan_sched_coarse.length); i++){
				curr_sched_ptr = next_sched_ptr;
				next_sched_ptr = (wlan_sched*)(((curr_sched_ptr->node).next)->container);
				if(timestamp > (curr_sched_ptr->target)){
					curr_sched_ptr->callback();
					dl_node_remove(&wlan_sched_coarse,&(curr_sched_ptr->node));
					wlan_free(curr_sched_ptr);
				}
			}

			if(wlan_sched_coarse.length > 0){
				//There are still schedules pending. Restart the timer.
				XTmrCtr_SetResetValue(&TimerCounterInst, TIMER_CNTR_SLOW, SLOW_TIMER_DUR_US*(TIMER_FREQ/1000000));
				XTmrCtr_Start(&TimerCounterInst, TIMER_CNTR_SLOW);
			}

		break;
	}
}

void XTmrCtr_CustomInterruptHandler(void *InstancePtr){
	//FIXME: Temporarily moved ISR to MAC framework

	//xil_printf("Custom Timer ISR\n");

	XTmrCtr *TmrCtrPtr = NULL;
	u8 TmrCtrNumber;
	u32 ControlStatusReg;

	/*
	 * Verify that each of the inputs are valid.
	 */
	Xil_AssertVoid(InstancePtr != NULL);

	/*
	 * Convert the non-typed pointer to an timer/counter instance pointer
	 * such that there is access to the timer/counter
	 */
	TmrCtrPtr = (XTmrCtr *) InstancePtr;

	/*
	 * Loop thru each timer counter in the device and call the callback
	 * function for each timer which has caused an interrupt
	 */
	for (TmrCtrNumber = 0;
		TmrCtrNumber < XTC_DEVICE_TIMER_COUNT; TmrCtrNumber++) {

		ControlStatusReg = XTmrCtr_ReadReg(TmrCtrPtr->BaseAddress,
						   TmrCtrNumber,
						   XTC_TCSR_OFFSET);
		/*
		 * Check if interrupt is enabled
		 */
		if (ControlStatusReg & XTC_CSR_ENABLE_INT_MASK) {

			/*
			 * Check if timer expired and interrupt occured
			 */
			if (ControlStatusReg & XTC_CSR_INT_OCCURED_MASK) {
				/*
				 * Increment statistics for the number of
				 * interrupts and call the callback to handle
				 * any application specific processing
				 */
				TmrCtrPtr->Stats.Interrupts++;
				TmrCtrPtr->Handler(TmrCtrPtr->CallBackRef,
						   TmrCtrNumber);
				/*
				 * Read the new Control/Status Register content.
				 */
				ControlStatusReg =
					XTmrCtr_ReadReg(TmrCtrPtr->BaseAddress,
								TmrCtrNumber,
								XTC_TCSR_OFFSET);
				/*
				 * If in compare mode and a single shot rather
				 * than auto reload mode then disable the timer
				 * and reset it such so that the interrupt can
				 * be acknowledged, this should be only temporary
				 * till the hardware is fixed
				 */
#if 0
				if (((ControlStatusReg &
					XTC_CSR_AUTO_RELOAD_MASK) == 0) &&
					((ControlStatusReg &
					  XTC_CSR_CAPTURE_MODE_MASK)== 0)) {
						/*
						 * Disable the timer counter and
						 * reset it such that the timer
						 * counter is loaded with the
						 * reset value allowing the
						 * interrupt to be acknowledged
						 */
						ControlStatusReg &=
							~XTC_CSR_ENABLE_TMR_MASK;

						XTmrCtr_WriteReg(
							TmrCtrPtr->BaseAddress,
							TmrCtrNumber,
							XTC_TCSR_OFFSET,
							ControlStatusReg |
							XTC_CSR_LOAD_MASK);

						/*
						 * Clear the reset condition,
						 * the reset bit must be
						 * manually cleared by a 2nd write
						 * to the register
						 */
						XTmrCtr_WriteReg(
							TmrCtrPtr->BaseAddress,
							TmrCtrNumber,
							XTC_TCSR_OFFSET,
							ControlStatusReg);
				}
#endif
				/*
				 * Acknowledge the interrupt by clearing the
				 * interrupt bit in the timer control status
				 * register, this is done after calling the
				 * handler so the application could call
				 * IsExpired, the interrupt is cleared by
				 * writing a 1 to the interrupt bit of the
				 * register without changing any of the other
				 * bits
				 */
				XTmrCtr_WriteReg(TmrCtrPtr->BaseAddress,
						 TmrCtrNumber,
						 XTC_TCSR_OFFSET,
						 ControlStatusReg |
						 XTC_CSR_INT_OCCURED_MASK);
			}
		}
	}
}
