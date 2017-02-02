/*
 * w3_platform_common.h
 *
 *  Created on: Jan 27, 2017
 *      Author: chunter
 */

#ifndef W3_PLATFORM_COMMON_H_
#define W3_PLATFORM_COMMON_H_

#include "xparameters.h"

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

// IPC mailbox
//  Only CPU High uses the mailbox interrupt. INT_ID defined here
//  so mailbox_util.h can be shared between projects.
#define PLATFORM_DEV_ID_MAILBOX    XPAR_MBOX_0_DEVICE_ID

// System monitor
#define PLATFORM_BASEADDR_SYSMON   XPAR_SYSMON_0_BASEADDR

// CPU IDs
#ifdef XPAR_MB_HIGH_FREQ
#define WLAN_COMPILE_FOR_CPU_HIGH                          1
#define WLAN_COMPILE_FOR_CPU_LOW                           0
#endif

#ifdef XPAR_MB_LOW_FREQ
#define WLAN_COMPILE_FOR_CPU_HIGH                          0
#define WLAN_COMPILE_FOR_CPU_LOW                           1
#endif


// Mutex for Tx/Rx packet buffers
#define PLATFORM_DEV_ID_PKT_BUF_MUTEX	XPAR_MUTEX_0_DEVICE_ID


#endif /* W3_PLATFORM_COMMON_H_ */
