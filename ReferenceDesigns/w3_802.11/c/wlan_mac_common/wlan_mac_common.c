/** @file wlan_mac_ipc_util.c
 *  @brief Inter-processor Communication Framework
 *
 *  This contains code common to both CPU_LOW and CPU_HIGH that allows them
 *  to pass messages to one another.
 *
 *  @copyright Copyright 2013-2015, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *                See LICENSE.txt included in the design archive or
 *                at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

/***************************** Include Files *********************************/

#include "stdlib.h"
#include "string.h"

#include "xil_io.h"
#include "xstatus.h"
#include "xparameters.h"

#include "w3_iic_eeprom.h"

#include "wlan_mac_common.h"
#include "wlan_mac_userio_util.h"


/*********************** Global Variable Definitions *************************/

#define EEPROM_BASEADDR                                    XPAR_W3_IIC_EEPROM_ONBOARD_BASEADDR


/*************************** Variable Definitions ****************************/

static wlan_mac_hw_info_t         mac_hw_info;
static wlan_mac_low_config_t      mac_low_config;


/*************************** Functions Prototypes ****************************/


/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * Null Callback
 *
 * This function will always return XST_SUCCESS and should be used to initialize
 * callbacks.  All input parameters will be ignored.
 *
 * @param   param            - Void pointer for parameters
 *
 * @return  int              - Status:
 *                                 XST_SUCCESS - Command completed successfully
 *****************************************************************************/
int wlan_null_callback(void* param) {
    return XST_SUCCESS;
};



/*****************************************************************************/
/**
 * Verify channel is supported
 *
 * @param   channel          - Channel to verify
 *
 * @return  int              - Channel supported?
 *                                 XST_SUCCESS - Channel supported
 *                                 XST_FAILURE - Channel not supported
 *****************************************************************************/
int wlan_verify_channel(u32 channel) {
    int return_value;

    // The 802.11 reference design allows a subset of 2.4 and 5 GHz channels
    //     Channel number follows 802.11 conventions:
    //         https://en.wikipedia.org/wiki/List_of_WLAN_channels
    //
    switch (channel) {
        // 2.4GHz channels
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        // 5GHz channels
        case 36:
        case 40:
        case 44:
        case 48:
            return_value = XST_SUCCESS;
        break;
        default:
            return_value = XST_FAILURE;
        break;
    }

    return return_value;
}



/*****************************************************************************/
/**
 * Halt CPU on Error
 *
 * @param   error_code       - Error code to display on WARP hex display
 *****************************************************************************/
void cpu_error_halt(u32 error_code) {

    if (error_code != WLAN_ERROR_CPU_STOP) {
        // Print error message
        xil_printf("\n\nERROR:  CPU is halting with error code: E%X\n\n", (error_code & 0xF));

        // Set the error code on the hex display
        set_hex_display_error_status(error_code & 0xF);

        // Enter infinite loop blinking the hex display
        blink_hex_display(0, 250000);
    } else {
        // Stop execution
        while (1) {};
    }
}




/*****************************************************************************/
/**
 * Initialize the MAC Hardware Info
 *
 * This function will initialize the MAC hardware information structure for
 * the CPU based on information contained in the EEPROM and the wlan_exp_type
 * provided.  This function should only be called after the EEPROM has been
 * initialized.
 *
 * @param   wlan_exp_type    - WLAN Exp type of the CPU
 *                                 See wlan_exp.h or WLAN Exp types
 *
 *****************************************************************************/
void init_mac_hw_info(u32 wlan_exp_type) {
    // Initialize the wlan_mac_hw_info_t structure to all zeros
    memset((void*)(&mac_hw_info), 0x0, sizeof(wlan_mac_hw_info_t));

    // Set General Node information
    mac_hw_info.wlan_exp_type = wlan_exp_type;
    mac_hw_info.serial_number = w3_eeprom_read_serial_num(EEPROM_BASEADDR);
    mac_hw_info.fpga_dna[1]   = w3_eeprom_read_fpga_dna(EEPROM_BASEADDR, 1);
    mac_hw_info.fpga_dna[0]   = w3_eeprom_read_fpga_dna(EEPROM_BASEADDR, 0);

    // Set HW Addresses
    //   - The w3_eeprom_read_eth_addr() function handles the case when the WARP v3
    //     hardware does not have a valid Ethernet address
    //
    // Use address 0 for the WLAN interface, address 1 for the WLAN Exp interface
    //
    w3_eeprom_read_eth_addr(EEPROM_BASEADDR, 0, mac_hw_info.hw_addr_wlan);
    w3_eeprom_read_eth_addr(EEPROM_BASEADDR, 1, mac_hw_info.hw_addr_wlan_exp);
}



/*****************************************************************************/
/**
 * Get the MAC Hardware Info
 *
 * Return the MAC hardware information structure.  This should only be used
 * after the structure is initialized.
 *
 * @return  wlan_mac_hw_info_t *  - Pointer to HW info structure
 *
 *****************************************************************************/
wlan_mac_hw_info_t * get_mac_hw_info()          { return &mac_hw_info; }
u8                 * get_mac_hw_addr_wlan()     { return mac_hw_info.hw_addr_wlan; }
u8                 * get_mac_hw_addr_wlan_exp() { return mac_hw_info.hw_addr_wlan_exp; }



/*****************************************************************************/
/**
 * Initialize the MAC Low config
 *
 * This function will initialize the MAC Low configuration parameters.  All
 * parameters are stored as u32 but will be interpreted as the correct data
 * type by CPU Low.
 *
 * @param   wlan_exp_type    - WLAN Exp type of the CPU
 *                                 See wlan_exp.h or WLAN Exp types
 *
 *****************************************************************************/
void init_mac_low_config(u32 channel, s8 tx_ctrl_pow, u8 rx_ant_mode, u32 rx_filter_mode) {
    // Initialize the wlan_mac_low_config_t structure to all zeros
    memset((void*)(&mac_low_config), 0x0, sizeof(wlan_mac_low_config_t));

    // Set configuration parameters
    mac_low_config.channel        = channel;
    mac_low_config.tx_ctrl_pow    = (u32) tx_ctrl_pow;
    mac_low_config.rx_ant_mode    = (u32) rx_ant_mode;
    mac_low_config.rx_filter_mode = rx_filter_mode;
}



/*****************************************************************************/
/**
 * Get the MAC Low config
 *
 * Return the MAC configuration parameter.  These methods should only be used
 * after the structure is initialized.
 *
 *****************************************************************************/
wlan_mac_low_config_t * get_mac_low_config()          { return &mac_low_config; }
u32                     get_mac_low_channel()         { return mac_low_config.channel; }
s8                      get_mac_low_tx_ctrl_pow()     { return (s8) mac_low_config.tx_ctrl_pow; }
u8                      get_mac_low_rx_ant_mode()     { return (u8) mac_low_config.rx_ant_mode; }
u32                     get_mac_low_rx_filter_mode()  { return mac_low_config.rx_filter_mode; }



#ifdef _DEBUG_

/*****************************************************************************/
/**
 * @brief Print Hardware Information
 *
 * @param wlan_mac_hw_info_t *    - Pointer to the hardware info struct
 *
 *****************************************************************************/
void wlan_print_hw_info(wlan_mac_hw_info_t * info) {
	int i;

	xil_printf("WLAN MAC HW INFO:  \n");
	xil_printf("  CPU Low Type     :  0x%8x\n", info->wlan_exp_type);
	xil_printf("  Serial Number    :  %d\n",    info->serial_number);
	xil_printf("  FPGA DNA         :  0x%8x  0x%8x\n", info->fpga_dna[1], info->fpga_dna[0]);
	xil_printf("  WLAN EXP HW Addr :  %02x",    info->hw_addr_wlan_exp[0]);

	for (i = 1; i < WLAN_MAC_ETH_ADDR_LEN; i++) {
		xil_printf(":%02x", info->hw_addr_wlan_exp[i]);
	}
	xil_printf("\n");

	xil_printf("  WLAN HW Addr     :  %02x",    info->hw_addr_wlan[0]);
	for (i = 1; i < WLAN_MAC_ETH_ADDR_LEN; i++) {
		xil_printf(":%02x", info->hw_addr_wlan[i]);
	}
	xil_printf("\n");
}



#endif