 /** @file wlan_platform_dbg_hdr.h
 *  @brief Debug Header Macros
 *
 *  This file should be included by any context that wishes to toggle bits
 *  on the hardware's debug header. This header is unique in that it is the
 *  only part of the platform code that is directly included. This is by
 *  design -- it is the only way to ensure near single-cycle write access
 *  to the registers that drive the debug header.
 *
 *  @copyright Copyright 2013-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

#ifndef WLAN_PLATFORM_COMMON_INCLUDE_WLAN_PLATFORM_DBG_HDR_H_
#define WLAN_PLATFORM_COMMON_INCLUDE_WLAN_PLATFORM_DBG_HDR_H_

#include "xparameters.h"
#include "xil_io.h"
#include "w3_userio.h"


// Wrapper macros for debug header IO
#define wlan_mac_set_dbg_hdr_dir(dir, pin_mask) userio_set_dbg_hdr_io_dir(USERIO_BASEADDR, (dir), (pin_mask))
#define wlan_mac_set_dbg_hdr_ctrlsrc(src, pin_mask) userio_set_dbg_hdr_out_ctrlsrc(USERIO_BASEADDR, (src), (pin_mask))
#define wlan_mac_set_dbg_hdr_out(pin_mask) 	 	userio_set_dbg_hdr_out(USERIO_BASEADDR, (pin_mask))
#define wlan_mac_clear_dbg_hdr_out(pin_mask) 	userio_clear_dbg_hdr_out(USERIO_BASEADDR, (pin_mask))
#define wlan_mac_write_dbg_hdr_out(val) 	 	userio_write_dbg_hdr_out(USERIO_BASEADDR, (val))
#define wlan_mac_read_dbg_hdr_io(val) 		 	userio_read_dbg_hdr_io(USERIO_BASEADDR)


#endif /* WLAN_PLATFORM_COMMON_INCLUDE_WLAN_PLATFORM_DBG_HDR_H_ */
