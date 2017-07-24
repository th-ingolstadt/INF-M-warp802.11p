#include "wlan_mac_high_sw_config.h"

#include "stdarg.h"

#include "wlan_platform_high.h"
#include "w3_high.h"
#include "w3_eth.h"
#include "w3_uart.h"
#include "w3_high_userio.h"
#include "w3_userio_util.h"
#include "w3_iic_eeprom.h"

#include "wlan_mac_high.h"
#include "wlan_exp_common.h"
#include "wlan_exp_node.h"

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
		.mailbox_int_id = PLATFORM_INT_ID_MAILBOX,
		.wlan_exp_eth_mac_dev_id = WLAN_EXP_ETH_MAC_ID,
		.wlan_exp_eth_dma_dev_id = WLAN_EXP_ETH_DMA_ID,
		.wlan_exp_phy_addr = 0x7
};

platform_high_dev_info_t wlan_platform_high_get_dev_info(){
	return w3_platform_high_dev_info;
}

int wlan_platform_high_init(XIntc* intc){
	int return_value = XST_SUCCESS;

	return_value = w3_uart_init();
	return_value |= w3_uart_setup_interrupt(intc);

	return_value |= w3_high_userio_init();
	return_value |= w3_high_userio_setup_interrupt(intc);


#if WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE
	// Initialize Ethernet in wlan_platform
	return_value |= w3_wlan_platform_ethernet_init();
	return_value |= w3_wlan_platform_ethernet_setup_interrupt(intc);
#endif //WLAN_SW_CONFIG_ENABLE_ETH_BRIDGE

	return return_value;
}

void wlan_platform_free_queue_entry_notify(){
	w3_wlan_platform_ethernet_free_queue_entry_notify();
}


void wlan_platform_high_userio_disp_status(userio_disp_high_status_t status, ...){
   va_list valist;

   static application_role_t application_role = APPLICATION_ROLE_UNKNOWN;

   /* initialize valist for num number of arguments */
   va_start(valist, status);

   switch(status){

   	   case USERIO_DISP_STATUS_IDENTIFY: {
   		   blink_hex_display(25, 200000);
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

int wlan_platform_wlan_exp_process_node_cmd(u8* cmd_processed, u32 cmd_id, int socket_index, void* from, cmd_resp* command, cmd_resp* response, u32 max_resp_len){
	//
	// IMPORTANT ENDIAN NOTES:
	//     - command
	//         - header - Already endian swapped by the framework (safe to access directly)
	//         - args   - Must be endian swapped as necessary by code (framework does not know the contents of the command)
	//     - response
	//         - header - Will be endian swapped by the framework (safe to write directly)
	//         - args   - Must be endian swapped as necessary by code (framework does not know the contents of the response)
	//

	// Standard variables
	u32 resp_sent = NO_RESP_SENT;

	u32* cmd_args_32 = command->args;

	cmd_resp_hdr* resp_hdr = response->header;
	u32* resp_args_32 = response->args;
	u32 resp_index = 0;

	//
	// NOTE: Response header cmd, length, and num_args fields have already been initialized.
	//

	*cmd_processed = 1;

	switch(cmd_id){
		default:
			*cmd_processed = 0;
		break;
		//---------------------------------------------------------------------
		case CMDID_DEV_EEPROM: {


			// Read / Write values from / to EEPROM
			//
			// Write Message format:
			//     cmd_args_32[0]      Command == CMD_PARAM_WRITE_VAL
			//     cmd_args_32[1]      EEPROM (0 = ON_BOARD / 1 = FMC)
			//     cmd_args_32[2]      Address
			//     cmd_args_32[3]      Length (Number of u8 bytes to write)
			//     cmd_args_32[4:]     Values to write (Length u32 values each containing a single byte to write)
			// Response format:
			//     resp_args_32[0]     Status
			//
			// Read Message format:
			//     cmd_args_32[0]      Command == CMD_PARAM_READ_VAL
			//     cmd_args_32[1]      EEPROM Device (1 = ON_BOARD / 0 = FMC)
			//     cmd_args_32[2]      Address
			//     cmd_args_32[3]      Length (number of u8 bytes to read)
			// Response format:
			//     resp_args_32[0]     Status
			//     resp_args_32[1]     Length (Number of u8 bytes read)
			//     resp_args_32[2:]    EEPROM values (Length u32 values each containing a single byte read)
			//
			#define EEPROM_BASEADDR                        XPAR_W3_IIC_EEPROM_ONBOARD_BASEADDR
			#define FMC_EEPROM_BASEADDR                    XPAR_W3_IIC_EEPROM_FMC_BASEADDR

			u32 eeprom_idx;
			int eeprom_status;
			u8 byte_to_write;
			u32 status = CMD_PARAM_SUCCESS;
			u32 msg_cmd = Xil_Ntohl(cmd_args_32[0]);
			u32 eeprom_device = Xil_Ntohl(cmd_args_32[1]);
			u32 eeprom_addr = (Xil_Ntohl(cmd_args_32[2]) & 0xFFFF);
			u32 eeprom_length = Xil_Ntohl(cmd_args_32[3]);
			u32 use_default_resp = WLAN_EXP_TRUE;
			u32 eeprom_ba = EEPROM_BASEADDR;

			// Select EEPROM device
			if (eeprom_device) {
				eeprom_ba = EEPROM_BASEADDR;
			} else {
				#if FMC_EEPROM_BASEADDR
					eeprom_ba = FMC_EEPROM_BASEADDR;
				#else
					wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "FMC EEPROM not supported\n");
					msg_cmd = CMD_PARAM_RSVD;
				#endif
			}

			switch (msg_cmd) {
				case CMD_PARAM_WRITE_VAL:
					wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Write EEPROM:\n");
					wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Addr: 0x%08x\n", eeprom_addr);
					wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Len:  %d\n", eeprom_length);

					// Don't bother if length is clearly bogus
					if(eeprom_length < max_resp_len) {
						for (eeprom_idx = 0; eeprom_idx < eeprom_length; eeprom_idx++) {
							// Endian swap payload and extract the byte to write
							byte_to_write = (Xil_Ntohl(cmd_args_32[eeprom_idx + 4]) & 0xFF);

							// Write the byte and break if there was an EEPROM failure
							eeprom_status = iic_eeprom_write_byte(eeprom_ba, (eeprom_addr + eeprom_idx), byte_to_write, XPAR_CPU_ID);

							if (eeprom_status == IIC_EEPROM_FAILURE) {
								wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "CMDID_DEV_EEPROM write failed at byte %d\n", eeprom_idx);
								status = CMD_PARAM_ERROR;
								break;
							}
						}
					} else {
						wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "CMDID_DEV_EEPROM write longer than %d bytes\n", max_resp_len);
						status = CMD_PARAM_ERROR;
					}
				break;

				case CMD_PARAM_READ_VAL:
					wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Read EEPROM:\n");
					wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Addr: 0x%08x\n", eeprom_addr);
					wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Len:  %d\n", eeprom_length);

					if (eeprom_length < max_resp_len) {
						// Don't set the default response
						use_default_resp = WLAN_EXP_FALSE;

						for (eeprom_idx = 0; eeprom_idx < eeprom_length; eeprom_idx++) {
							// Read the byte and break if there was an EEPROM failure
							eeprom_status = iic_eeprom_read_byte(eeprom_ba, (eeprom_addr + eeprom_idx), XPAR_CPU_ID);

							if (eeprom_status == IIC_EEPROM_FAILURE) {
								wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "CMDID_DEV_EEPROM write failed at byte %d\n", eeprom_idx);
								status = CMD_PARAM_ERROR;
								break;
							}

							// Add the byte read and Endian swap the payload
							//     - This modified the output Ethernet packet but does not update the resp_index variable
							resp_args_32[resp_index + eeprom_idx + 2] = Xil_Htonl(eeprom_status & 0xFF);
						}

						// Add length argument to response
						resp_args_32[resp_index++] = Xil_Htonl(status);
						resp_args_32[resp_index++] = Xil_Htonl(eeprom_idx);
						resp_index        += eeprom_idx;                       // Update response index for all EEPROM bytes
						resp_hdr->length  += (resp_index * sizeof(resp_args_32));
						resp_hdr->num_args = resp_index;

					} else {
						wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "CMDID_DEV_EEPROM read longer than %d bytes\n", max_resp_len);
						status = CMD_PARAM_ERROR;
					}
				break;

				case CMD_PARAM_RSVD:
					status = CMD_PARAM_ERROR;
				break;

				default:
					wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
					status = CMD_PARAM_ERROR;
				break;
			}

			if (use_default_resp) {
				// Send default response
				resp_args_32[resp_index++] = Xil_Htonl(status);
				resp_hdr->length  += (resp_index * sizeof(resp_args_32));
				resp_hdr->num_args = resp_index;
			}

		}

		break;

	}

	return resp_sent;

}

int wlan_platform_wlan_exp_eth_init(XAxiEthernet* eth_ptr){
	return 0;
}

