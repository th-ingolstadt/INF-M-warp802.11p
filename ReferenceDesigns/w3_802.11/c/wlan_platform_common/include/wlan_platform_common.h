#include "stdarg.h"

#include "wlan_mac_common.h"

#ifndef WLAN_PLATFORM_COMMON_H_
#define WLAN_PLATFORM_COMMON_H_

/*********************************************************************
 * Mapping macros from xparameters.h
 *
 * The BSP generates xparameters.h to define macros for hardware-specific
 * parameters, including peripheral memory addresses, peripheral IDs,
 * interrupt IDs, etc.
 *
 * The macros below map the BSP-generated macros ("XPAR_...") to the
 * hardware-agnostic names used by the 802.11 Ref Design C code. Each
 * platform must implement this mapping in wlan_platform_common.h.
 *
 **********************************************************************/


// --------------------------------------------------------------------
// Peripherals accessible by both CPUs

// IIC EEPROMs - WARP v3 on-board and FMC-RF-2X245 EEPROMs
#define EEPROM_BASEADDR			XPAR_W3_IIC_EEPROM_ONBOARD_BASEADDR
#define FMC_EEPROM_BASEADDR  	XPAR_W3_IIC_EEPROM_FMC_BASEADDR





void wlan_platform_init();
wlan_mac_hw_info_t wlan_platform_get_hw_info();
void wlan_platform_userio_disp_status(userio_disp_status_t status, ...);

#endif /* WLAN_PLATFORM_COMMON_H_ */
