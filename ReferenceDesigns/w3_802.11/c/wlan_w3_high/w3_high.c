#include "wlan_mac_high_sw_config.h"

#include "stdarg.h"

#include "wlan_platform_high.h"
#include "w3_high.h"
#include "w3_eth.h"
#include "w3_uart.h"
#include "w3_userio_util.h"

#include "wlan_mac_high.h"

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

	return_value = w3_uart_init();
	w3_uart_set_rx_callback(platform_config.uart_rx_callback);
	return_value |= w3_uart_setup_interrupt(platform_config.intc);

	return_value |= w3_high_userio_init();
	w3_high_userio_set_inputs_callback(platform_config.userio_inputs_callback);
	return_value |= w3_high_userio_setup_interrupt(platform_config.intc);


#if WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE
	// Initialize Ethernet in wlan_platform
	return_value |= w3_wlan_platform_ethernet_init();
	w3_wlan_platform_ethernet_set_rx_callback(platform_config.eth_rx_callback);
	return_value |= w3_wlan_platform_ethernet_setup_interrupt(platform_config.intc);
#endif //WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE

	return return_value;
}

void wlan_platform_free_queue_entry_notify(){
	w3_wlan_platform_ethernet_free_queue_entry_notify();
}


void wlan_platform_userio_disp_status(userio_disp_status_t status, ...){
   va_list valist;

   static application_role_t application_role = APPLICATION_ROLE_UNKNOWN;

   /* initialize valist for num number of arguments */
   va_start(valist, status);

   switch(status){


   	   case USERIO_DISP_STATUS_IDENTIFY: {
   		   blink_hex_display(10, 5);
   	   } break;

   	   case USERIO_DISP_STATUS_APPLICATION_ROLE: {
   		   application_role = va_arg(valist, application_role_t);

   		   if(application_role == APPLICATION_ROLE_AP){
				// Set Periodic blinking of hex display (period of 500 with min of 2 and max of 400)
				set_hex_pwm_period(500);
				set_hex_pwm_min_max(2, 400);
				enable_hex_pwm();
   		   }
   	   } break;

   	   case USERIO_DISP_STATUS_MEMBER_LIST_UPDATE: {
   		   	u32 val = va_arg(valist, u32);

			if(application_role == APPLICATION_ROLE_AP){
				write_hex_display_with_pwm((u8)val);
			} else {
				write_hex_display((u8)val);
			}

   	   } break;

   	   case USERIO_DISP_STATUS_WLAN_EXP_CONFIGURE: {
   		   u32 wlan_exp_is_configured = va_arg(valist, u32);

   		   if( wlan_exp_is_configured ){
   			   set_hex_display_right_dp(1);
   		   } else {
   			   set_hex_display_right_dp(0);
   		   }
   	   } break;

   	   case USERIO_DISP_STATUS_CPU_ERROR: {

   		   u32 error_code = va_arg(valist, u32);
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

   	   } break;

   	   default:
	   break;
   }


   /* clean memory reserved for valist */
   va_end(valist);

   return;

}

