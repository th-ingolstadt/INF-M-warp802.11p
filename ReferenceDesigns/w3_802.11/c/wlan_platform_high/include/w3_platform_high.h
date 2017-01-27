#ifndef W3_PLATFORM_HIGH_H_
#define W3_PLATFORM_HIGH_H_

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

// Aux BRAM set aside for w3 platform usage
#define AUX_BRAM_ETH_BD_MEM_SIZE	(240 * 64)

/********************************************************************
 * Ethernet TX Aux. Space
 *
 * Space is set aside for the use of wlan_platform's Ethernet
 * implementation
 ********************************************************************/
#define ETH_BD_MEM_BASE         (AUX_BRAM_HIGHADDR - AUX_BRAM_ETH_BD_MEM_SIZE + 1)



// Off-chip DRAM
#define DRAM_BASEADDR           XPAR_DDR3_SODIMM_S_AXI_BASEADDR
#define DRAM_HIGHADDR           XPAR_DDR3_SODIMM_S_AXI_HIGHADDR

//---------------------------------------
// Peripherals accessible by CPU High
//  Each core has a device ID (used by Xilinx drivers to initialize config structs)
//  Cores that generate interrupts have interrupt IDs (identifies interrupt source for connecting ISRs)

// Interrupt controller
#define PLATFORM_DEV_ID_INTC                 XPAR_INTC_0_DEVICE_ID              ///< XParameters rename of interrupt controller device ID

// GPIO for user I/O inputs
#define PLATFORM_DEV_ID_USRIO_GPIO           XPAR_MB_HIGH_SW_GPIO_DEVICE_ID     ///< XParameters rename of device ID of GPIO
#define PLATFORM_INT_ID_USRIO_GPIO           XPAR_INTC_0_GPIO_0_VEC_ID          ///< XParameters rename of GPIO interrupt ID

// Timer
#define PLATFORM_DEV_ID_TIMER                XPAR_TMRCTR_0_DEVICE_ID
#define TIMER_FREQ                           XPAR_TMRCTR_0_CLOCK_FREQ_HZ
#define PLATFORM_INT_ID_TIMER                XPAR_INTC_0_TMRCTR_0_VEC_ID        ///< XParameters rename of timer interrupt ID

// Central DMA (CMDA)
#define PLATFORM_DEV_ID_CMDA				 XPAR_AXI_CDMA_0_DEVICE_ID

#endif /* W3_PLATFORM_HIGH_H_ */
