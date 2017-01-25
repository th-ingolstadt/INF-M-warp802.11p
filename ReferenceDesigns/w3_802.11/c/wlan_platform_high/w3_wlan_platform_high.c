#include "wlan_platform_high.h"
#include "wlan_mac_high_sw_config.h"

#include "w3_wlan_platform_ethernet.h"

//---------------------------------------
// Public Functions for WLAN MAC High Framework
//  These functions are intended to be called directly to the WLAN MAC High Framework
int wlan_platform_high_init(platform_config_t platform_config){
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
