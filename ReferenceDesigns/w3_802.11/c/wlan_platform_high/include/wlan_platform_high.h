/** @file wlan_platform_high.h
 *  @brief WARP v3 High Platform Defines for 802.11 Ref design
 *
 *  @copyright Copyright 2014-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *                See LICENSE.txt included in the design archive or
 *                at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

#ifndef WLAN_PLATFORM_HIGH_H_
#define WLAN_PLATFORM_HIGH_H_

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

// CPU High local memory (ILMB, DLMB)
//  On WARP v3 the I and D LMB are a common memory space. The ref design
//   implements the 128KB LMB area as 2 64KB blocks. The blocks have
//   adjacent address spaces, hence the BASE=_0_ and HIGH=_1_ below
#define DLMB_BASEADDR			XPAR_MB_HIGH_DLMB_BRAM_CNTLR_0_BASEADDR
#define DLMB_HIGHADDR			XPAR_MB_HIGH_DLMB_BRAM_CNTLR_1_HIGHADDR

#define ILMB_BASEADDR			XPAR_MB_HIGH_ILMB_BRAM_CNTLR_0_BASEADDR
#define ILMB_HIGHADDR			XPAR_MB_HIGH_ILMB_BRAM_CNTLR_1_HIGHADDR


// On-chip Aux BRAM
#define AUX_BRAM_BASEADDR       XPAR_MB_HIGH_AUX_BRAM_CTRL_S_AXI_BASEADDR
#define AUX_BRAM_HIGHADDR       XPAR_MB_HIGH_AUX_BRAM_CTRL_S_AXI_HIGHADDR

// Off-chip DRAM
#define DRAM_BASEADDR           XPAR_DDR3_SODIMM_S_AXI_BASEADDR
#define DRAM_HIGHADDR           XPAR_DDR3_SODIMM_S_AXI_HIGHADDR

//---------------------------------------
// Peripherals accessible by CPU High
//  Each core has a device ID (used by Xilinx drivers to initialize config structs)
//  Cores that generate interrupts have interrupt IDs (identifies interrupt source for connecting ISRs)

// Interrupt controller
#define PLATFORM_DEV_ID_INTC                 XPAR_INTC_0_DEVICE_ID              ///< XParameters rename of interrupt controller device ID

// UART
#define PLATFORM_DEV_ID_UART                 XPAR_UARTLITE_0_DEVICE_ID          ///< XParameters rename for UART
#define PLATFORM_INT_ID_UART                 XPAR_INTC_0_UARTLITE_0_VEC_ID      ///< XParameters rename of UART interrupt ID

// GPIO for user I/O inputs
#define PLATFORM_DEV_ID_USRIO_GPIO           XPAR_MB_HIGH_SW_GPIO_DEVICE_ID     ///< XParameters rename of device ID of GPIO
#define PLATFORM_INT_ID_USRIO_GPIO           XPAR_INTC_0_GPIO_0_VEC_ID               ///< XParameters rename of GPIO interrupt ID

#define GPIO_USERIO_INPUT_CHANNEL            1                                  ///< Channel used as input user IO inputs (buttons, DIP switch)
#define GPIO_USERIO_INPUT_IR_CH_MASK         XGPIO_IR_CH1_MASK                  ///< Mask for enabling interrupts on GPIO input

// Timer
#define PLATFORM_DEV_ID_TIMER                XPAR_TMRCTR_0_DEVICE_ID
#define TIMER_FREQ                           XPAR_TMRCTR_0_CLOCK_FREQ_HZ
#define PLATFORM_INT_ID_TIMER                XPAR_INTC_0_TMRCTR_0_VEC_ID        ///< XParameters rename of timer interrupt ID

// Central DMA (CMDA)
#define PLATFORM_DEV_ID_CMDA				 XPAR_AXI_CDMA_0_DEVICE_ID



#endif /* WLAN_PLATFORM_HIGH_H_ */
