#include "rftap.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "xil_cache.h"
#include "xstatus.h"

#include "wlan_platform_common.h"
#include "wlan_mac_common.h"
#include "wlan_exp_ip_udp.h"
#include "wlan_exp_ip_udp_device.h"

void rftap_init(unsigned int device_num) {
	int ret;

	// Get hardware info
    wlan_mac_hw_info_t *hw_info = get_mac_hw_info();

    if (ret = (transport_check_device(device_num) != XST_SUCCESS)) {
    	xil_printf("Error: transport_check_device() failed: %d\n", ret);
    	return;
    }

	// Enable UDP framework
	if ((ret = wlan_exp_ip_udp_init()) != XST_SUCCESS) {
		xil_printf("Error: wlan_exp_ip_udp_init() failed: %d\n", ret);
		return;
	}

	// Initialize Device
	if ((ret = eth_init(device_num, hw_info->hw_addr_wlan_exp, RFTAP_IP_DEFAULT, 0)) != XST_SUCCESS) {
		xil_printf("Error: eth_init() failed: %d\n", ret);
		return;
	}

	// Start Device
	if ((ret = eth_start_device(device_num)) != XST_SUCCESS) {
		xil_printf("Error: eth_start_device() failed: %d\n", ret);
		return;
	}

	// Success!
	xil_printf("Successfully initialized RFTap interface\n");
}

void rftap_send(void) {
	uint8_t packet[] = {
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x62, 0x81, 0x09, 0x8b, 0x00, 0x08, 0x06, 0x00, 0x01,
			0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0xf0, 0x62, 0x81, 0x09, 0x8b, 0x00, 0x0a, 0x54, 0x3f, 0xfe,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x54, 0x38, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	memcopy(TX_PKT_BUF_TO_ADDR(3), packet, sizeof(packet));
	int ret = 0;
	if ((ret = wlan_platform_ethernet_send(TX_PKT_BUF_TO_ADDR(3), sizeof(packet))) != XST_SUCCESS) {
		xil_printf("Error: wlan_platform_ethernet_send() failed: %d\n", ret);
	}
}
