#include "wlan_mac_high_sw_config.h"
#include "wlan_platform_high.h"
#include "include/w3_platform_high.h"
#include "include/w3_eth.h"

static platform_high_dev_info_t w3_platform_high_dev_info;

platform_high_dev_info_t wlan_platform_high_get_dev_info(){

	w3_platform_high_dev_info.dlmb_baseaddr = DLMB_BASEADDR;
	w3_platform_high_dev_info.dlmb_size = DLMB_HIGHADDR - DLMB_BASEADDR + 1;
	w3_platform_high_dev_info.ilmb_baseaddr = ILMB_BASEADDR;
	w3_platform_high_dev_info.ilmb_size = ILMB_HIGHADDR - ILMB_BASEADDR + 1;
	w3_platform_high_dev_info.aux_bram_baseaddr = AUX_BRAM_BASEADDR;
	w3_platform_high_dev_info.aux_bram_size = ETH_BD_MEM_BASE - AUX_BRAM_BASEADDR;
	w3_platform_high_dev_info.dram_baseaddr = DRAM_BASEADDR;
	w3_platform_high_dev_info.dram_size = DRAM_HIGHADDR - DRAM_BASEADDR + 1;
	w3_platform_high_dev_info.intc_dev_id = PLATFORM_DEV_ID_INTC;
	w3_platform_high_dev_info.uart_dev_id = PLATFORM_DEV_ID_UART;
	w3_platform_high_dev_info.uart_int_id = PLATFORM_INT_ID_UART;
	w3_platform_high_dev_info.gpio_dev_id = PLATFORM_DEV_ID_USRIO_GPIO;
	w3_platform_high_dev_info.gpio_int_id = PLATFORM_INT_ID_USRIO_GPIO;
	w3_platform_high_dev_info.timer_dev_id = PLATFORM_DEV_ID_TIMER;
	w3_platform_high_dev_info.timer_int_id = PLATFORM_INT_ID_TIMER;
	w3_platform_high_dev_info.timer_freq = TIMER_FREQ;
	w3_platform_high_dev_info.cdma_dev_id = PLATFORM_DEV_ID_CMDA;

	return w3_platform_high_dev_info;
}

int wlan_platform_high_init(platform_high_config_t platform_config){
	int return_value = XST_SUCCESS;

#if WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE
	// Initialize Ethernet in wlan_platform
	return_value = w3_wlan_platform_ethernet_init();
	return_value |= w3_wlan_platform_ethernet_setup_interrupt(platform_config.intc);
	w3_wlan_platform_ethernet_set_rx_callback(platform_config.eth_rx_callback);
#endif //WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE

	return return_value;
}

void wlan_platform_free_queue_entry_notify(){
	w3_wlan_platform_ethernet_free_queue_entry_notify();
}

