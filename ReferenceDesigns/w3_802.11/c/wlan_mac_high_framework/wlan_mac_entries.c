/** @file wlan_mac_entries.c
 *  @brief Event log
 *
 *  This contains the code for accessing event log.
 *
 *  @copyright Copyright 2014, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *	@note
 *  This is the only code that the user should modify in order to add entries
 *  to the event log.  To add a new entry, please follow the template provided
 *  and create:
 *    1) A new entry type in wlan_mac_entries.h
 *    2) Wrapper function:  get_next_empty_*_entry()
 *    3) Update the print function so that it is easy to print the log to the
 *    terminal
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 *  @bug No known bugs.
 */


/***************************** Include Files *********************************/

// SDK includes
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "xil_types.h"

// WLAN includes
#include "wlan_mac_event_log.h"
#include "wlan_mac_entries.h"


/*************************** Constant Definitions ****************************/



/*********************** Global Variable Definitions *************************/



/*************************** Variable Definitions ****************************/



/*************************** Functions Prototypes ****************************/



/******************************** Functions **********************************/


/*****************************************************************************/
/**
* Get the next empty experiment info entry
*
* @param    size - Amount of space to be allocated for experiment info entry message
*
* @return	exp_info_entry *   - Pointer to the next "empty" Experiment info entry or NULL
*
* @note		None.
*
******************************************************************************/
exp_info_entry* get_next_empty_exp_info_entry(u16 size){

	// Get the next empty entry
	return (exp_info_entry *)event_log_get_next_empty_entry( ENTRY_TYPE_EXP_INFO, sizeof(exp_info_entry) + size - 4 );
}


/*****************************************************************************/
/**
* Get the next empty RX OFDM entry
*
* @param    None.
*
* @return	rx_entry *   - Pointer to the next "empty" RX entry or NULL
*
* @note		None.
*
******************************************************************************/
rx_ofdm_entry* get_next_empty_rx_ofdm_entry(){

	// Get the next empty entry
	return (rx_ofdm_entry *)event_log_get_next_empty_entry( ENTRY_TYPE_RX_OFDM, sizeof(rx_ofdm_entry) );
}

/*****************************************************************************/
/**
* Get the next empty RX DSSS entry
*
* @param    None.
*
* @return	rx_entry *   - Pointer to the next "empty" RX entry or NULL
*
* @note		None.
*
******************************************************************************/
rx_dsss_entry* get_next_empty_rx_dsss_entry(){

	// Get the next empty entry
	return (rx_dsss_entry *)event_log_get_next_empty_entry( ENTRY_TYPE_RX_DSSS, sizeof(rx_dsss_entry) );
}

/*****************************************************************************/
/**
* Get the next empty TX entry
*
* @param    None.
*
* @return	tx_entry *   - Pointer to the next "empty" TX entry or NULL
*
* @note		None.
*
******************************************************************************/
tx_entry* get_next_empty_tx_entry(){

	// Get the next empty entry
	return (tx_entry *)event_log_get_next_empty_entry( ENTRY_TYPE_TX, sizeof(tx_entry) );

}

/*****************************************************************************/
/**
* Prints an entry
*
* @param    entry_number     - Index of entry in the log
*           entry_type       - Type of entry
*           timestamp        - Lower 32 bits of the timestamp
*           entry            - Pointer to the entry
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void print_entry( u32 entry_number, u32 entry_type, void * entry ){
	u32 i, j;

	node_info_entry    * node_info_entry_log_item;
	exp_info_entry     * exp_info_entry_log_item;
	statistics_entry   * statistics_entry_log_item;
	rx_ofdm_entry      * rx_ofdm_entry_log_item;
	rx_dsss_entry      * rx_dsss_entry_log_item;
	tx_entry           * tx_entry_log_item;

	switch( entry_type ){
        case ENTRY_TYPE_NODE_INFO:
        	node_info_entry_log_item = (node_info_entry*) entry;
			xil_printf("%d: - Log Info entry\n", entry_number );
			xil_printf("   Type        :   %d\n",     node_info_entry_log_item->type);
			xil_printf("   ID          :   0x%4x\n",     node_info_entry_log_item->id);
			xil_printf("   HW Gen      :   %d\n",     node_info_entry_log_item->hw_gen);
			xil_printf("   Design Ver  :   0x%08x\n",     node_info_entry_log_item->design_ver);
			xil_printf("   Serial Num  :   %d\n",     node_info_entry_log_item->serial_number);
			xil_printf("   FPGA DNA    :   0x%08x  0x%08x\n", (u32)(node_info_entry_log_item->fpga_dna >>32), (u32)(node_info_entry_log_item->fpga_dna) );
			xil_printf("   Max assn    :   %d\n",     node_info_entry_log_item->wlan_max_assn);
			xil_printf("   Log size    :   %d\n",     node_info_entry_log_item->wlan_event_log_size);
			xil_printf("   Max stats   :   %d\n",     node_info_entry_log_item->wlan_max_stats);
		break;

        case ENTRY_TYPE_EXP_INFO:
        	exp_info_entry_log_item = (exp_info_entry*) entry;
			xil_printf("%d: - Experiment Info entry\n", entry_number );
			xil_printf("   Timestamp:  %d\n", (u32)(exp_info_entry_log_item->timestamp));
			xil_printf("   Reason   :  %d\n",       exp_info_entry_log_item->reason);
			xil_printf("   Message  :  \n");
			for( i = 0; i < exp_info_entry_log_item->length; i++) {
				xil_printf("        ");
				for( j = 0; j < 16; j++){
					xil_printf("0x%02x ", (exp_info_entry_log_item->msg)[16*i + j]);
				}
				xil_printf("\n");
			}
		break;

		case ENTRY_TYPE_STATISTICS:
			statistics_entry_log_item = (statistics_entry*) entry;
			xil_printf("%d: - Statistics Event\n", entry_number );
			xil_printf("   Last timestamp :    %d\n",        (u32)(statistics_entry_log_item->last_timestamp));
			xil_printf("   Address        :    %02x",             (statistics_entry_log_item->addr)[0]);
			for( i = 1; i < 6; i++) { xil_printf(":%02x",         (statistics_entry_log_item->addr)[i]); }
			xil_printf("\n");
			xil_printf("   Is associated  :    %d\n",              statistics_entry_log_item->is_associated);
			xil_printf("   Tx total       :    %d (%d success)\n", statistics_entry_log_item->num_tx_total,
					                                               statistics_entry_log_item->num_tx_success);
			xil_printf("   Tx retry       :    %d\n",              statistics_entry_log_item->num_retry);
			xil_printf("   Rx total       :    %d (%d bytes)\n",   statistics_entry_log_item->num_rx_success,
					                                               statistics_entry_log_item->num_rx_bytes);
		break;

		case ENTRY_TYPE_RX_OFDM:
			rx_ofdm_entry_log_item = (rx_ofdm_entry*) entry;
			xil_printf("%d: - Rx OFDM Event\n", entry_number );
			xil_printf("   Time:     %d\n",		(u32)(rx_ofdm_entry_log_item->timestamp));
			xil_printf("   FCS:      %d\n",     rx_ofdm_entry_log_item->fcs_status);
			xil_printf("   Pow:      %d\n",     rx_ofdm_entry_log_item->power);
			xil_printf("   Rate:     %d\n",     rx_ofdm_entry_log_item->rate);
			xil_printf("   Length:   %d\n",     rx_ofdm_entry_log_item->length);
			xil_printf("   Pkt Type: 0x%x\n",   rx_ofdm_entry_log_item->pkt_type);
			xil_printf("   Channel:  %d\n",     rx_ofdm_entry_log_item->chan_num);
#ifdef WLAN_MAC_ENTRIES_LOG_CHAN_EST
			xil_printf("   Channel Estimates:\n");

			for( i = 0; i < 16; i++) {
				xil_printf("        ");
				for( j = 0; j < 4; j++){
					xil_printf("0x%8x ", (rx_ofdm_entry_log_item->channel_est)[4*i + j]);
				}
				xil_printf("\n");
			}
#endif
		break;

		case ENTRY_TYPE_RX_DSSS:
			rx_dsss_entry_log_item = (rx_dsss_entry*) entry;
			xil_printf("%d: - Rx DSSS Event\n", entry_number );
			xil_printf("   Time:     %d\n",		(u32)(rx_dsss_entry_log_item->timestamp));
			xil_printf("   FCS:      %d\n",     rx_dsss_entry_log_item->fcs_status);
			xil_printf("   Pow:      %d\n",     rx_dsss_entry_log_item->power);
			xil_printf("   Rate:     %d\n",     rx_dsss_entry_log_item->rate);
			xil_printf("   Length:   %d\n",     rx_dsss_entry_log_item->length);
			xil_printf("   Pkt Type: 0x%x\n",   rx_dsss_entry_log_item->pkt_type);
			xil_printf("   Channel:  %d\n",     rx_dsss_entry_log_item->chan_num);
		break;

		case ENTRY_TYPE_TX:
			tx_entry_log_item = (tx_entry*) entry;
			xil_printf("%d: - Tx Event\n", entry_number);
			xil_printf("   Creation Time:    %d\n",		(u32)(tx_entry_log_item->timestamp_create));
			xil_printf("   Accept Delay:     %d\n",		(u32)(tx_entry_log_item->delay_accept));
			xil_printf("   Done Delay:       %d\n",		(u32)(tx_entry_log_item->delay_done));
			xil_printf("   Tx Gain Target:   %d\n",     tx_entry_log_item->gain_target);
			xil_printf("   Rate:             %d\n",     tx_entry_log_item->rate);
			xil_printf("   Length:           %d\n",     tx_entry_log_item->length);
			xil_printf("   Channel:          %d\n",     tx_entry_log_item->chan_num);
			xil_printf("   Result:           %d\n",     tx_entry_log_item->result);
			xil_printf("   Pkt Type:         0x%x\n",   tx_entry_log_item->pkt_type);
			xil_printf("   Retry:            %d\n",     tx_entry_log_item->retry_count);
		break;

		default:
			xil_printf("%d: - Unknown Event\n", entry_number);
		break;
	}

}



