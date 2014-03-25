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
* Get the next empty station info entry
*
* @param    None.
*
* @return	station_info_entry *   - Pointer to the next "empty" Station info entry or NULL
*
* @note		None.
*
******************************************************************************/
station_info_entry* get_next_empty_station_info_entry(){

	// Get the next empty entry
	return (station_info_entry *)event_log_get_next_empty_entry( ENTRY_TYPE_STATION_INFO, sizeof(station_info_entry) );
}

/*****************************************************************************/
/**
* Get the next empty WARPNet command entry
*
* @param    None.
*
* @return	wn_cmd_entry *   - Pointer to the next "empty" WARPNet command entry or NULL
*
* @note		None.
*
******************************************************************************/
wn_cmd_entry* get_next_empty_wn_cmd_entry(){

	// Get the next empty entry
	return (wn_cmd_entry *)event_log_get_next_empty_entry( ENTRY_TYPE_WN_CMD, sizeof(wn_cmd_entry) );
}

/*****************************************************************************/
/**
* Get the next empty time info entry
*
* @param    None.
*
* @return	time_info_entry *   - Pointer to the next "empty" time info entry or NULL
*
* @note		None.
*
******************************************************************************/
time_info_entry* get_next_empty_time_info_entry(){

	// Get the next empty entry
	return (time_info_entry *)event_log_get_next_empty_entry( ENTRY_TYPE_TIME_INFO, sizeof(time_info_entry) );
}

/*****************************************************************************/
/**
* Get the next empty RX OFDM entry
*
* @param    u32 payload_log_len
* 				- Number of bytes to set aside for payload.
* 				@note This needs to be 4-byte aligned.
*
* @return	rx_entry *   - Pointer to the next "empty" RX entry or NULL
*
* @note		None.
*
******************************************************************************/
rx_ofdm_entry* get_next_empty_rx_ofdm_entry(u32 payload_log_len){
	u32 extra_payload = (payload_log_len > MIN_MAC_PAYLOAD_LOG_LEN) ? (payload_log_len - MIN_MAC_PAYLOAD_LOG_LEN) : 0;
    return (rx_ofdm_entry *)event_log_get_next_empty_entry( ENTRY_TYPE_RX_OFDM, sizeof(rx_ofdm_entry) + extra_payload  );

}

/*****************************************************************************/
/**
* Get the next empty RX DSSS entry
*
* @param    u32 payload_log_len
* 				- Number of bytes to set aside for payload.
* 				@note This needs to be 4-byte aligned.
*
* @return	rx_entry *   - Pointer to the next "empty" RX entry or NULL
*
* @note		None.
*
******************************************************************************/
rx_dsss_entry* get_next_empty_rx_dsss_entry(u32 payload_log_len){
	u32 extra_payload = (payload_log_len > MIN_MAC_PAYLOAD_LOG_LEN) ? (payload_log_len - MIN_MAC_PAYLOAD_LOG_LEN) : 0;
	return (rx_dsss_entry *)event_log_get_next_empty_entry( ENTRY_TYPE_RX_DSSS, sizeof(rx_dsss_entry) + extra_payload );

}

/*****************************************************************************/
/**
* Get the next empty TX high entry
*
* @param    None.
*
* @return	tx_high_entry *   - Pointer to the next "empty" TX high entry or NULL
*
* @note		None.
*
******************************************************************************/
tx_high_entry* get_next_empty_tx_high_entry(u32 payload_log_len){
	u32 extra_payload = (payload_log_len > MIN_MAC_PAYLOAD_LOG_LEN) ? (payload_log_len - MIN_MAC_PAYLOAD_LOG_LEN) : 0;
	return (tx_high_entry *)event_log_get_next_empty_entry( ENTRY_TYPE_TX_HIGH, sizeof(tx_high_entry) + extra_payload );

}

/*****************************************************************************/
/**
* Get the next empty TX low entry
*
* @param    None.
*
* @return	tx_low_entry *   - Pointer to the next "empty" TX low entry or NULL
*
* @note		None.
*
******************************************************************************/
tx_low_entry* get_next_empty_tx_low_entry(){

	// Get the next empty entry
	return (tx_low_entry *)event_log_get_next_empty_entry( ENTRY_TYPE_TX_LOW, sizeof(tx_low_entry) );

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
	wn_cmd_entry       * wn_cmd_entry_log_item;
	time_info_entry    * time_info_entry_log_item;
	txrx_stats_entry   * txrx_stats_entry_log_item;
	rx_common_entry    * rx_common_log_item;
	tx_high_entry      * tx_high_entry_log_item;
	tx_low_entry       * tx_low_entry_log_item;

	switch( entry_type ){
        case ENTRY_TYPE_NODE_INFO:
        	node_info_entry_log_item = (node_info_entry*) entry;
			xil_printf("%d: - Log Info entry\n", entry_number );
			xil_printf("   Type        :   %d\n",     node_info_entry_log_item->type);
			xil_printf("   ID          :   0x%4x\n",  node_info_entry_log_item->id);
			xil_printf("   HW Gen      :   %d\n",     node_info_entry_log_item->hw_gen);
			xil_printf("   WN Ver      :   0x%08x\n", node_info_entry_log_item->wn_ver);
			xil_printf("   FPGA DNA    :   0x%08x  0x%08x\n", (u32)(node_info_entry_log_item->fpga_dna >>32), (u32)(node_info_entry_log_item->fpga_dna) );
			xil_printf("   Serial Num  :   %d\n",     node_info_entry_log_item->serial_number);
			xil_printf("   WLAN Exp Ver:   0x%08x\n", node_info_entry_log_item->wlan_exp_ver);
			xil_printf("   Max assn    :   %d\n",     node_info_entry_log_item->wlan_max_assn);
			xil_printf("   Log size    :   %d\n",     node_info_entry_log_item->wlan_event_log_size);
			xil_printf("   Max stats   :   %d\n",     node_info_entry_log_item->wlan_max_stats);
		break;

        case ENTRY_TYPE_EXP_INFO:
        	exp_info_entry_log_item = (exp_info_entry*) entry;
			xil_printf("%d: - Experiment Info entry\n", entry_number );
			xil_printf("   Timestamp:  %d\n", (u32)(exp_info_entry_log_item->timestamp));
			xil_printf("   Info Type:  %d\n",       exp_info_entry_log_item->info_type);
			xil_printf("   Message  :  \n");
			for( i = 0; i < exp_info_entry_log_item->length; i++) {
				xil_printf("        ");
				for( j = 0; j < 16; j++){
					xil_printf("0x%02x ", (exp_info_entry_log_item->msg)[16*i + j]);
				}
				xil_printf("\n");
			}
		break;

        case ENTRY_TYPE_WN_CMD:
        	wn_cmd_entry_log_item = (wn_cmd_entry*) entry;
			xil_printf("%d: - WARPNet Command entry\n", entry_number );
			xil_printf("   Timestamp:  %d\n", (u32)(wn_cmd_entry_log_item->timestamp));
			xil_printf("   Command  :  0x%08x\n",    wn_cmd_entry_log_item->command);
			xil_printf("   Args[%02d] :  \n",       wn_cmd_entry_log_item->num_args);
			for( i = 0; i < wn_cmd_entry_log_item->num_args; i++) {
				if (i == 10) break;
				xil_printf("        0x%08x \n", (wn_cmd_entry_log_item->args)[i]);
			}
        break;

        case ENTRY_TYPE_TIME_INFO:
        	time_info_entry_log_item = (time_info_entry*) entry;
			xil_printf("%d: - Time Info entry\n", entry_number );
			xil_printf("   Timestamp:  %d\n", (u32)(time_info_entry_log_item->timestamp));
			xil_printf("   Abs time :  $d\n", (u32)(time_info_entry_log_item->abs_time));
			xil_printf("   New time :  %d\n", (u32)(time_info_entry_log_item->new_time));
			xil_printf("   Reason   :  %d\n",       time_info_entry_log_item->reason);
        break;

		case ENTRY_TYPE_TXRX_STATS:
			txrx_stats_entry_log_item = (txrx_stats_entry*) entry;
			xil_printf("%d: - Statistics Event\n", entry_number );
			xil_printf("   Last timestamp :        %d\n",        (u32)(txrx_stats_entry_log_item->stats.last_rx_timestamp));
			xil_printf("   Address        :        %02x",             (txrx_stats_entry_log_item->stats.addr)[0]);
			for( i = 1; i < 6; i++) { xil_printf(":%02x",         (txrx_stats_entry_log_item->stats.addr)[i]); }
			xil_printf("\n");
			xil_printf("   Is associated  :        %d\n",              txrx_stats_entry_log_item->stats.is_associated);
			xil_printf("   # Tx High Data MPDUs:   %d (%d successful)\n", txrx_stats_entry_log_item->stats.data.tx_num_packets_total, txrx_stats_entry_log_item->stats.data.tx_num_packets_success);
			xil_printf("   # Tx High Data bytes:   %d (%d successful)\n", (u32)(txrx_stats_entry_log_item->stats.data.tx_num_bytes_total), (u32)(txrx_stats_entry_log_item->stats.data.tx_num_bytes_success));
			xil_printf("   # Tx Low Data MPDUs:    %d\n", txrx_stats_entry_log_item->stats.data.tx_num_packets_low);
			xil_printf("   # Tx High Mgmt MPDUs:   %d (%d successful)\n", txrx_stats_entry_log_item->stats.mgmt.tx_num_packets_total, txrx_stats_entry_log_item->stats.mgmt.tx_num_packets_success);
			xil_printf("   # Tx High Mgmt bytes:   %d (%d successful)\n", (u32)(txrx_stats_entry_log_item->stats.mgmt.tx_num_bytes_total), (u32)(txrx_stats_entry_log_item->stats.mgmt.tx_num_bytes_success));
			xil_printf("   # Tx Low Mgmt MPDUs:    %d\n", txrx_stats_entry_log_item->stats.mgmt.tx_num_packets_low);
			xil_printf("   # Rx Data MPDUs:        %d\n", txrx_stats_entry_log_item->stats.data.rx_num_packets);
			xil_printf("   # Rx Data Bytes:        %d\n", txrx_stats_entry_log_item->stats.data.rx_num_bytes);
			xil_printf("   # Rx Mgmt MPDUs:        %d\n", txrx_stats_entry_log_item->stats.mgmt.rx_num_packets);
			xil_printf("   # Rx Mgmt Bytes:        %d\n", txrx_stats_entry_log_item->stats.mgmt.rx_num_bytes);
		break;

		case ENTRY_TYPE_RX_OFDM:
			rx_common_log_item = (rx_common_entry*) entry;
			xil_printf("%d: - Rx OFDM Event\n", entry_number );
#ifdef WLAN_MAC_ENTRIES_LOG_CHAN_EST
			xil_printf("   Channel Estimates:\n");

			for( i = 0; i < 16; i++) {
				xil_printf("        ");
				for( j = 0; j < 4; j++){
					xil_printf("0x%8x ", (((rx_ofdm_entry*)rx_common_log_item)->channel_est)[4*i + j]);
				}
				xil_printf("\n");
			}
#endif
			xil_printf("   Time:     %d\n",		(u32)(rx_common_log_item->timestamp));
			xil_printf("   FCS:      %d\n",     rx_common_log_item->fcs_status);
			xil_printf("   Pow:      %d\n",     rx_common_log_item->power);
			xil_printf("   Rate:     %d\n",     rx_common_log_item->rate);
			xil_printf("   Length:   %d\n",     rx_common_log_item->length);
			xil_printf("   Pkt Type: 0x%x\n",   rx_common_log_item->pkt_type);
			xil_printf("   Channel:  %d\n",     rx_common_log_item->chan_num);
		break;

		case ENTRY_TYPE_RX_DSSS:
			rx_common_log_item = (rx_common_entry*) entry;
			xil_printf("%d: - Rx DSSS Event\n", entry_number );
			xil_printf("   Time:     %d\n",		(u32)(rx_common_log_item->timestamp));
			xil_printf("   FCS:      %d\n",     rx_common_log_item->fcs_status);
			xil_printf("   Pow:      %d\n",     rx_common_log_item->power);
			xil_printf("   Rate:     %d\n",     rx_common_log_item->rate);
			xil_printf("   Length:   %d\n",     rx_common_log_item->length);
			xil_printf("   Pkt Type: 0x%x\n",   rx_common_log_item->pkt_type);
			xil_printf("   Channel:  %d\n",     rx_common_log_item->chan_num);
		break;

		case ENTRY_TYPE_TX_HIGH:
			tx_high_entry_log_item = (tx_high_entry*) entry;
			xil_printf("%d: - Tx High Event\n", entry_number);
			xil_printf("   Creation Time:    %d\n",		(u32)(tx_high_entry_log_item->timestamp_create));
			xil_printf("   Accept Delay:     %d\n",		(u32)(tx_high_entry_log_item->delay_accept));
			xil_printf("   Done Delay:       %d\n",		(u32)(tx_high_entry_log_item->delay_done));
			xil_printf("   Tx Power:         %d\n",     tx_high_entry_log_item->power);
			xil_printf("   Rate:             %d\n",     tx_high_entry_log_item->rate);
			xil_printf("   Length:           %d\n",     tx_high_entry_log_item->length);
			xil_printf("   Channel:          %d\n",     tx_high_entry_log_item->chan_num);
			xil_printf("   Result:           %d\n",     tx_high_entry_log_item->result);
			xil_printf("   Pkt Type:         0x%x\n",   tx_high_entry_log_item->pkt_type);
			xil_printf("   Num Tx:           %d\n",     tx_high_entry_log_item->num_tx);
		break;

		case ENTRY_TYPE_TX_LOW:
			tx_low_entry_log_item = (tx_low_entry*) entry;
			xil_printf("%d: - Tx Low Event\n", entry_number);
			xil_printf("   Tx Start Time:    %d\n",		(u32)(tx_low_entry_log_item->timestamp_send));
			xil_printf("   Tx Count:         %d\n",		tx_low_entry_log_item->transmission_count);
			xil_printf("   Power:            %d\n",     tx_low_entry_log_item->phy_params.power);
			xil_printf("   Rate:             %d\n",     tx_low_entry_log_item->phy_params.rate);
			xil_printf("   Length:           %d\n",     tx_low_entry_log_item->length);
			xil_printf("   Channel:          %d\n",     tx_low_entry_log_item->chan_num);
			xil_printf("   Pkt Type:         0x%x\n",   tx_low_entry_log_item->pkt_type);
			xil_printf("   Antenna Mode:     %d\n",     tx_low_entry_log_item->phy_params.antenna_mode);
			xil_printf("   # of BO Slots     %d\n",     tx_low_entry_log_item->num_slots);
		break;

		default:
			xil_printf("%d: - Unknown Event\n", entry_number);
		break;
	}

}



