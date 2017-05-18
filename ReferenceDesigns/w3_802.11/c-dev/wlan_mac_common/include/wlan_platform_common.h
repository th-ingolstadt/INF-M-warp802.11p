#ifndef WLAN_PLATFORM_COMMON_H_
#define WLAN_PLATFORM_COMMON_H_

#include "xil_types.h"
#include "wlan_mac_common_types.h"

// Common platform init functions
platform_common_dev_info_t wlan_platform_common_get_dev_info();
int wlan_platform_common_init();
wlan_mac_hw_info_t wlan_platform_get_hw_info();

// User IO functions
void wlan_platform_userio_disp_status(userio_disp_status_t status, ...);
u32  wlan_platform_userio_get_state();

// Temperature functions
u32 wlan_platform_get_current_temp();
u32 wlan_platform_get_min_temp();
u32 wlan_platform_get_max_temp();

// MAC time functions
volatile u64       get_mac_time_usec();
void               set_mac_time_usec(u64 new_time);
void               apply_mac_time_delta_usec(s64 time_delta);
volatile u64       get_system_time_usec();
void               wlan_usleep(u64 delay);

#endif /* WLAN_PLATFORM_COMMON_H_ */
