#include "stdlib.h"
#include "string.h"
#include "stdarg.h"

#include "xil_io.h"
#include "xstatus.h"
#include "xparameters.h"

#include "wlan_platform_common.h"
#include "include/w3_common.h"
#include "include/w3_userio_util.h"
#include "wlan_mac_common.h"
#include "w3_iic_eeprom.h"


static const platform_common_dev_info_t platform_common_dev_info = {
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
		.eeprom_baseaddr = EEPROM_BASEADDR,
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


void wlan_platform_userio_disp_status(userio_disp_status_t status, ...){
   va_list valist;

#if WLAN_COMPILE_FOR_CPU_HIGH
   static application_role_t application_role = APPLICATION_ROLE_UNKNOWN;
#endif

#if WLAN_COMPILE_FOR_CPU_LOW
   static u8 red_led_index = 0;
   static u8 green_led_index = 0;
#endif

   /* initialize valist for num number of arguments */
   va_start(valist, status);

   switch(status){

#if WLAN_COMPILE_FOR_CPU_HIGH
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
#endif

#if WLAN_COMPILE_FOR_CPU_LOW
   	   case USERIO_DISP_STATUS_GOOD_FCS_EVENT: {
           green_led_index = (green_led_index + 1) % 4;
           userio_write_leds_green(USERIO_BASEADDR, (1<<green_led_index));
   	   } break;

   	   case USERIO_DISP_STATUS_BAD_FCS_EVENT: {
           red_led_index = (red_led_index + 1) % 4;
           userio_write_leds_red(USERIO_BASEADDR, (1<<red_led_index));
   	   } break;
#endif

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
