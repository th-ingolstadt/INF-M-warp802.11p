#ifndef W3_PLATFORM_LOW_H_
#define W3_PLATFORM_LOW_H_

// ****************************************************************************
// Define standard macros for base addresses and device IDs
//     XPAR_ names will change with instance names in hardware
//
#define CLK_BASEADDR         XPAR_W3_CLOCK_CONTROLLER_BASEADDR
#define DRAM_BASEADDR        XPAR_DDR3_2GB_SODIMM_MPMC_BASEADDR
#define RC_BASEADDR          XPAR_RADIO_CONTROLLER_BASEADDR
#define AD_BASEADDR          XPAR_W3_AD_CONTROLLER_BASEADDR

// ****************************************************************************
// Uncomment this macro to enable software support for RF C and D interfaces on the FMC-RF-2X245 module
//
// IMPORTANT: Do not use a 4-radio hardware project on a kit with a different FMC module
//
//#define WLAN_4RF_EN

#ifdef WLAN_4RF_EN
    #define RC_ALL_RF  (RC_RFA | RC_RFB | RC_RFC | RC_RFD)
    #define AD_ALL_RF  (RFA_AD_CS | RFB_AD_CS | RFC_AD_CS | RFD_AD_CS)
#else
    #define RC_ALL_RF  (RC_RFA | RC_RFB)
    #define AD_ALL_RF  (RFA_AD_CS | RFB_AD_CS)
#endif



#endif /* W3_PLATFORM_LOW_H_ */
