#include "wlan_mac_high_sw_config.h"
#include "wlan_platform_high.h"
#include "include/w3_high.h"
#include "include/w3_eth.h"
#include "include/w3_uart.h"

static const platform_high_dev_info_t w3_platform_high_dev_info = {
		.dlmb_baseaddr = DLMB_BASEADDR,
		.dlmb_size = DLMB_HIGHADDR - DLMB_BASEADDR + 1,
		.ilmb_baseaddr = ILMB_BASEADDR,
		.ilmb_size = ILMB_HIGHADDR - ILMB_BASEADDR + 1,
		.aux_bram_baseaddr = AUX_BRAM_BASEADDR,
		.aux_bram_size = ETH_BD_MEM_BASE - AUX_BRAM_BASEADDR,
		.dram_baseaddr = DRAM_BASEADDR,
		.dram_size = DRAM_HIGHADDR - DRAM_BASEADDR + 1,
		.intc_dev_id = PLATFORM_DEV_ID_INTC,
		.gpio_dev_id = PLATFORM_DEV_ID_USRIO_GPIO,
		.gpio_int_id = PLATFORM_INT_ID_USRIO_GPIO,
		.timer_dev_id = PLATFORM_DEV_ID_TIMER,
		.timer_int_id = PLATFORM_INT_ID_TIMER,
		.timer_freq = TIMER_FREQ,
		.cdma_dev_id = PLATFORM_DEV_ID_CMDA,
		.mailbox_int_id = PLATFORM_INT_ID_MAILBOX
};

platform_high_dev_info_t wlan_platform_high_get_dev_info(){
	return w3_platform_high_dev_info;
}

int wlan_platform_high_init(platform_high_config_t platform_config){
	int return_value = XST_SUCCESS;

	return_value = w3_wlan_platform_uart_init();
	w3_wlan_platform_uart_set_rx_callback(platform_config.uart_rx_callback);
	return_value |= w3_wlan_platform_uart_setup_interrupt(platform_config.intc);


#if WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE
	// Initialize Ethernet in wlan_platform
	return_value = w3_wlan_platform_ethernet_init();
	w3_wlan_platform_ethernet_set_rx_callback(platform_config.eth_rx_callback);
	return_value |= w3_wlan_platform_ethernet_setup_interrupt(platform_config.intc);
#endif //WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE

	return return_value;
}

void wlan_platform_free_queue_entry_notify(){
	w3_wlan_platform_ethernet_free_queue_entry_notify();
}

