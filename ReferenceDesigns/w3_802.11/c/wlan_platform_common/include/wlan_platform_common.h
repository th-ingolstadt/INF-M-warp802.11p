#include "stdarg.h"

#include "wlan_mac_common.h"

#ifndef WLAN_PLATFORM_COMMON_H_
#define WLAN_PLATFORM_COMMON_H_

#define EEPROM_BASEADDR			XPAR_W3_IIC_EEPROM_ONBOARD_BASEADDR
#define FMC_EEPROM_BASEADDR  	XPAR_W3_IIC_EEPROM_FMC_BASEADDR

void wlan_platform_init();
wlan_mac_hw_info_t wlan_platform_get_hw_info();
void wlan_platform_userio_disp_status(userio_disp_status_t status, ...);

#endif /* WLAN_PLATFORM_COMMON_H_ */
