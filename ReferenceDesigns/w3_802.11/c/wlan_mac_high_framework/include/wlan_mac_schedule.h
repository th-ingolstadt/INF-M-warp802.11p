////////////////////////////////////////////////////////////////////////////////
// File   : wlan_mac_schedule.h
// Authors: Patrick Murphy (murphpo [at] mangocomm.com)
//			Chris Hunter (chunter [at] mangocomm.com)
// License: Copyright 2013, Mango Communications. All rights reserved.
//          Distributed under the Mango Communications Reference Design License
//				See LICENSE.txt included in the design archive or
//				at http://mangocomm.com/802.11/license
////////////////////////////////////////////////////////////////////////////////

#ifndef WLAN_MAC_SCHEDULE_H_
#define WLAN_MAC_SCHEDULE_H_

#include "xintc.h"
#include "wlan_mac_dl_list.h"

typedef struct {
	dl_node node;
	u32 id;
	u64 target;
	function_ptr_t callback;
} wlan_sched;

#define SCHEDULE_FINE	0
#define SCHEDULE_COARSE 1

#define TMRCTR_DEVICE_ID			XPAR_TMRCTR_0_DEVICE_ID

#define TIMER_FREQ          XPAR_TMRCTR_0_CLOCK_FREQ_HZ
#define TIMER_CNTR_FAST	 0
#define TIMER_CNTR_SLOW	 1

#define	FAST_TIMER_DUR_US 100
#define	SLOW_TIMER_DUR_US 100000


//Return value from wlan_mac_schedule_event
#define SCHEDULE_FAILURE 0xFFFFFFFF

int wlan_mac_schedule_init();
int wlan_mac_schedule_setup_interrupt(XIntc* intc);

u32 wlan_mac_schedule_event(u8 scheduler_sel, u32 delay, void(*callback)());
void timer_handler(void *CallBackRef, u8 TmrCtrNumber);
void XTmrCtr_CustomInterruptHandler(void *InstancePtr);

#endif /* WLAN_MAC_SCHEDULE_H_ */
