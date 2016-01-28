/** @file wlan_mac_schedule.h
 *  @brief Scheduler
 *
 *  This set of functions allows upper-level MAC implementations
 *	to schedule the execution of a provided callback for some point
 *	in the future.
 *
 *  @copyright Copyright 2014-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */


/***************************** Include Files *********************************/

#include "xintc.h"


/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_SCHEDULE_H_
#define WLAN_MAC_SCHEDULE_H_


//-----------------------------------------------
// Timer defines
//
#define TMRCTR_DEVICE_ID                                   XPAR_TMRCTR_0_DEVICE_ID
#define TIMER_FREQ                                         XPAR_TMRCTR_0_CLOCK_FREQ_HZ

#define TIMER_CLKS_PER_US                                  (TIMER_FREQ / 1000000)

#define TIMER_CNTR_FAST                                    0
#define TIMER_CNTR_SLOW                                    1

#define	FAST_TIMER_DUR_US                                  64
#define	SLOW_TIMER_DUR_US                                  200000


//-----------------------------------------------
// Scheduler defines
//
#define SCHEDULE_FINE                                      TIMER_CNTR_FAST
#define SCHEDULE_COARSE                                    TIMER_CNTR_SLOW

// Special value for num_calls parameter of wlan_sched
#define SCHEDULE_REPEAT_FOREVER                            0xFFFFFFFF

// Reserved Schedule ID range
#define SCHEDULE_ID_RESERVED_MIN                           0xFFFFFF00
#define SCHEDULE_ID_RESERVED_MAX                           0xFFFFFFFF

// Defined Reserved Schedule IDs
#define SCHEDULE_FAILURE                                   0xFFFFFFFF


//-----------------------------------------------
// Macros
//
#define wlan_mac_schedule_event(scheduler_sel, delay, callback)      \
                   wlan_mac_schedule_event_repeated(scheduler_sel, delay, 1, callback)



/*********************** Global Structure Definitions ************************/

// Schedule structure for scheduled events
typedef struct {
    u32            id;
    u32            delay_us;
    u32            num_calls;
    u64            target_us;
    function_ptr_t callback;
} wlan_sched;



/*************************** Function Prototypes *****************************/

int      wlan_mac_schedule_init();
int      wlan_mac_schedule_setup_interrupt(XIntc* intc);

u32      wlan_mac_schedule_event_repeated(u8 scheduler_sel, u32 delay, u32 num_calls, void(*callback)());
void     wlan_mac_remove_schedule(u8 scheduler_sel, u32 id);


#endif /* WLAN_MAC_SCHEDULE_H_ */
