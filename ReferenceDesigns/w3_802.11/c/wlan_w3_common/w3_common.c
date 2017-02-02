#include "stdlib.h"
#include "string.h"
#include "stdarg.h"

#include "xil_io.h"
#include "xstatus.h"
#include "xparameters.h"

#include "wlan_platform_common.h"
#include "w3_common.h"
#include "w3_userio_util.h"
#include "wlan_mac_common.h"
#include "w3_iic_eeprom.h"
#include "w3_sysmon_util.h"


static const platform_common_dev_info_t platform_common_dev_info = {
		.platform_id = PLATFORM_ID,
		.cpu_id = XPAR_CPU_ID,
#if WLAN_COMPILE_FOR_CPU_HIGH
		.is_cpu_high = 1,
		.is_cpu_low = 0,
#endif
#if WLAN_COMPILE_FOR_CPU_LOW
		.is_cpu_high = 0,
		.is_cpu_low = 1,
#endif
		.mailbox_dev_id = PLATFORM_DEV_ID_MAILBOX,
		.pkt_buf_mutex_dev_id = PLATFORM_DEV_ID_PKT_BUF_MUTEX,
		.tx_pkt_buf_baseaddr = XPAR_PKT_BUFF_TX_BRAM_CTRL_S_AXI_BASEADDR,
		.rx_pkt_buf_baseaddr = XPAR_PKT_BUFF_RX_BRAM_CTRL_S_AXI_BASEADDR
};

platform_common_dev_info_t wlan_platform_common_get_dev_info(){
	return platform_common_dev_info;
}

int wlan_platform_common_init(){
    u32 iter = 0;
    while (iic_eeprom_init(EEPROM_BASEADDR, 0x64, XPAR_CPU_ID) != IIC_EEPROM_SUCCESS){
    	iter++;
    };

    // ------------------------------------------
	// Initialize the System Monitor
	init_sysmon();

    return 0;
}

wlan_mac_hw_info_t wlan_platform_get_hw_info(){

	wlan_mac_hw_info_t mac_hw_info;

    // Set General Node information
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

    return mac_hw_info;
}

u32  wlan_platform_userio_get_state(){
	u32 return_value = 0;
	u32 user_io_inputs;

	user_io_inputs = userio_read_inputs(USERIO_BASEADDR);

	if(user_io_inputs & W3_USERIO_PB_U) return_value |= USERIO_INPUT_MASK_PB_0;
	if(user_io_inputs & W3_USERIO_PB_M) return_value |= USERIO_INPUT_MASK_PB_1;
	if(user_io_inputs & W3_USERIO_PB_D) return_value |= USERIO_INPUT_MASK_PB_2;

	if(user_io_inputs & W3_USERIO_DIPSW_0) return_value |= USERIO_INPUT_MASK_SW_0;
	if(user_io_inputs & W3_USERIO_DIPSW_1) return_value |= USERIO_INPUT_MASK_SW_1;
	if(user_io_inputs & W3_USERIO_DIPSW_2) return_value |= USERIO_INPUT_MASK_SW_2;
	if(user_io_inputs & W3_USERIO_DIPSW_3) return_value |= USERIO_INPUT_MASK_SW_3;

	return return_value;
}
