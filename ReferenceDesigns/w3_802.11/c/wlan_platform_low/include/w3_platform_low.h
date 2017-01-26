#ifndef W3_PLATFORM_LOW_H_
#define W3_PLATFORM_LOW_H_

// ****************************************************************************
// Define standard macros for base addresses and device IDs
//     XPAR_ names will change with instance names in hardware
//
#define CLK_BASEADDR         XPAR_W3_CLOCK_CONTROLLER_BASEADDR
#define RC_BASEADDR          XPAR_RADIO_CONTROLLER_BASEADDR
#define AD_BASEADDR          XPAR_W3_AD_CONTROLLER_BASEADDR

// Uncomment this macro to enable software support for RF C and D interfaces on the FMC-RF-2X245 module
//  Do not use a 4-radio hardware project on a kit with a different FMC module
//#define WLAN_4RF_EN

#ifdef WLAN_4RF_EN
    #define RC_ALL_RF  (RC_RFA | RC_RFB | RC_RFC | RC_RFD)
    #define AD_ALL_RF  (RFA_AD_CS | RFB_AD_CS | RFC_AD_CS | RFD_AD_CS)
#else
    #define RC_ALL_RF  (RC_RFA | RC_RFB)
    #define AD_ALL_RF  (RFA_AD_CS | RFB_AD_CS)
#endif

int w3_node_init();
void w3_radio_init();
inline u32 w3_wlan_chan_to_rc_chan(u32 mac_channel);
int w3_agc_init();


// AGC register renames
#define WLAN_AGC_REG_RESET           XPAR_WLAN_AGC_MEMMAP_RESET
#define WLAN_AGC_REG_TIMING_AGC      XPAR_WLAN_AGC_MEMMAP_TIMING_AGC
#define WLAN_AGC_REG_TIMING_DCO      XPAR_WLAN_AGC_MEMMAP_TIMING_DCO
#define WLAN_AGC_REG_TARGET          XPAR_WLAN_AGC_MEMMAP_TARGET
#define WLAN_AGC_REG_CONFIG          XPAR_WLAN_AGC_MEMMAP_CONFIG
#define WLAN_AGC_REG_RSSI_PWR_CALIB  XPAR_WLAN_AGC_MEMMAP_RSSI_PWR_CALIB
#define WLAN_AGC_REG_IIR_COEF_B0     XPAR_WLAN_AGC_MEMMAP_IIR_COEF_B0
#define WLAN_AGC_REG_IIR_COEF_A1     XPAR_WLAN_AGC_MEMMAP_IIR_COEF_A1
#define WLAN_AGC_TIMING_RESET        XPAR_WLAN_AGC_MEMMAP_TIMING_RESET


#endif /* W3_PLATFORM_LOW_H_ */
