/** @file wlan_mac_sysmon_util.c
 *  @brief System Monitor Utility functions
 *
 *  @copyright Copyright 2013-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */
/***************************** Include Files *********************************/

// Xilinx / Standard library includes
#include "wlan_platform_common.h"
#include "w3_common.h"
#include <xil_io.h>

// Hardware includes
#include <xsysmon_hw.h>


/*************************** Constant Definitions ****************************/



/*********************** Global Variable Definitions *************************/

/*************************** Functions Prototypes ****************************/

/*************************** Variable Definitions ****************************/

/******************************** Functions **********************************/

/*****************************************************************************/
/**
 * Initialize the System Monitor
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void init_sysmon() {
    u32 value;

    // Reset the system monitor
    XSysMon_WriteReg(PLATFORM_BASEADDR_SYSMON, XSM_SRR_OFFSET, XSM_SRR_IPRST_MASK);

    // Disable the Channel Sequencer before configuring the Sequence registers.
    value = XSysMon_ReadReg(PLATFORM_BASEADDR_SYSMON, XSM_CFR1_OFFSET) & (~ XSM_CFR1_SEQ_VALID_MASK);
    XSysMon_WriteReg(PLATFORM_BASEADDR_SYSMON, XSM_CFR1_OFFSET, (value | XSM_CFR1_SEQ_SINGCHAN_MASK));

    // Setup the Averaging to be done for the channels in the Configuration 0 register as 16 samples
    value = XSysMon_ReadReg(PLATFORM_BASEADDR_SYSMON, XSM_CFR0_OFFSET) & (~XSM_CFR0_AVG_VALID_MASK);
    XSysMon_WriteReg(PLATFORM_BASEADDR_SYSMON, XSM_CFR0_OFFSET, (value | XSM_CFR0_AVG16_MASK));

    // Enable the averaging on the following channels in the Sequencer registers:
    //  - On-chip Temperature
    //  - On-chip VCCAUX supply sensor
    XSysMon_WriteReg(PLATFORM_BASEADDR_SYSMON, XSM_SEQ02_OFFSET, (XSM_SEQ_CH_TEMP | XSM_SEQ_CH_VCCAUX));

    // Enable the following channels in the Sequencer registers:
    //  - On-chip Temperature
    //  - On-chip VCCAUX supply sensor
    XSysMon_WriteReg(PLATFORM_BASEADDR_SYSMON, XSM_SEQ00_OFFSET, (XSM_SEQ_CH_TEMP | XSM_SEQ_CH_VCCAUX));

    // Set the ADCCLK frequency equal to 1/32 of System clock for the System Monitor/ADC
    //   in the Configuration Register 2.
    XSysMon_WriteReg(PLATFORM_BASEADDR_SYSMON, XSM_CFR2_OFFSET, (32 << XSM_CFR2_CD_SHIFT));

    // Enable the Channel Sequencer in continuous sequencer cycling mode.
    value = XSysMon_ReadReg(PLATFORM_BASEADDR_SYSMON, XSM_CFR1_OFFSET) & (~ XSM_CFR1_SEQ_VALID_MASK);
    XSysMon_WriteReg(PLATFORM_BASEADDR_SYSMON, XSM_CFR1_OFFSET, (value | XSM_CFR1_SEQ_CONTINPASS_MASK));

    // Wait till the End of Sequence occurs
    XSysMon_ReadReg(PLATFORM_BASEADDR_SYSMON, XSM_SR_OFFSET); /* Clear the old status */

#if 1
    // Initialization without a timeout
    //   This has never been an issue during boot.  If it ever does, then use the code below
    //
    while (((XSysMon_ReadReg(PLATFORM_BASEADDR_SYSMON, XSM_SR_OFFSET)) & XSM_SR_EOS_MASK) != XSM_SR_EOS_MASK);
#else
    // Initialization with a timeout
    //   Timeout is 100 ms (ie 100000 us)
    //
    u64 timestamp = get_system_time_usec();

    while (((XSysMon_ReadReg(PLATFORM_BASEADDR_SYSMON, XSM_SR_OFFSET)) & XSM_SR_EOS_MASK) != XSM_SR_EOS_MASK) {
        if ((get_system_time_usec() - timestamp) < 100000) { break; }
    }
#endif
}



/*****************************************************************************/
/**
 * Initialize the System Monitor
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
u32  wlan_platform_get_current_temp   ( void ) { return XSysMon_ReadReg(PLATFORM_BASEADDR_SYSMON, XSM_TEMP_OFFSET);     }
u32  wlan_platform_get_min_temp       ( void ) { return XSysMon_ReadReg(PLATFORM_BASEADDR_SYSMON, XSM_MIN_TEMP_OFFSET); }
u32  wlan_platform_get_max_temp       ( void ) { return XSysMon_ReadReg(PLATFORM_BASEADDR_SYSMON, XSM_MAX_TEMP_OFFSET); }


