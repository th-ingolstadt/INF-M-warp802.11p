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

// CPU IDs
#define CPU_ID	         XPAR_CPU_ID
#define CPU_ID_MB_LOW	 0
#define CPU_ID_MB_HIGH	 1

#define IS_CPU_HIGH	    (CPU_ID == CPU_ID_MB_HIGH)

// --------------------------------------------------------------------
// Peripherals accessible by both CPUs

// IIC EEPROMs - WARP v3 on-board and FMC-RF-2X245 EEPROMs
#define EEPROM_BASEADDR			XPAR_W3_IIC_EEPROM_ONBOARD_BASEADDR
#define FMC_EEPROM_BASEADDR  	XPAR_W3_IIC_EEPROM_FMC_BASEADDR

// IPC mailbox
//  Only CPU High uses the mailbox interrupt. INT_ID defined here
//  so mailbox_util.h can be shared between projects.
#define PLATFORM_DEV_ID_MAILBOX    XPAR_MBOX_0_DEVICE_ID
#define PLATFORM_INT_ID_MAILBOX    XPAR_INTC_0_MBOX_0_VEC_ID

// System monitor
#define PLATFORM_BASEADDR_SYSMON   XPAR_SYSMON_0_BASEADDR

#ifdef XPAR_XSYSMON_NUM_INSTANCES
 #define PLATFORM_SYSMON_PRESENT    1
#else
 #define PLATFORM_SYSMON_PRESENT    0
#endif


#include "wlan_mac_common.h"

void wlan_platform_init();
wlan_mac_hw_info_t wlan_platform_get_hw_info();
void wlan_platform_userio_disp_status(userio_disp_status_t status, ...);

#endif /* WLAN_PLATFORM_COMMON_H_ */
