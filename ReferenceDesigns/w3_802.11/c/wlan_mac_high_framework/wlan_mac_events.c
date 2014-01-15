/** @file wlan_mac_events.c
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
 *  This is the only code that the user should modify in order to add events
 *  to the event log.  To add a new event, please follow the template provided
 *  and create:
 *    1) A new event type in wlan_mac_events.h
 *    2) Wrapper function:  get_next_empty_*_event()
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
#include "wlan_mac_events.h"


/*************************** Constant Definitions ****************************/



/*********************** Global Variable Definitions *************************/



/*************************** Variable Definitions ****************************/



/*************************** Functions Prototypes ****************************/



/******************************** Functions **********************************/


/*****************************************************************************/
/**
* Get the next empty log info event
*
* @param    None.
*
* @return	log_info_event *   - Pointer to the next "empty" log info event or NULL
*
* @note		None.
*
******************************************************************************/
log_info_event* get_next_empty_log_info_event(){

	// Get the next empty event
	return (log_info_event *)event_log_get_next_empty_entry( EVENT_TYPE_LOG_INFO, sizeof(log_info_event) );
}


/*****************************************************************************/
/**
* Get the next empty experiment info event
*
* @param    size - Amount of space to be allocated for experiment info event message
*
* @return	exp_info_event *   - Pointer to the next "empty" Experiment info event or NULL
*
* @note		None.
*
******************************************************************************/
exp_info_event* get_next_empty_exp_info_event(u16 size){

	// Get the next empty event
	return (exp_info_event *)event_log_get_next_empty_entry( EVENT_TYPE_EXP_INFO, sizeof(exp_info_event) + size - 4 );
}


/*****************************************************************************/
/**
* Get the next empty statistics event
*
* @param    None.
*
* @return	statistics_event *   - Pointer to the next "empty" statistics event or NULL
*
* @note		None.
*
******************************************************************************/
statistics_event* get_next_empty_statistics_event(){

	// Get the next empty event
	return (statistics_event *)event_log_get_next_empty_entry( EVENT_TYPE_STATISTICS, sizeof(statistics_event) );
}


/*****************************************************************************/
/**
* Get the next empty RX OFDM event
*
* @param    None.
*
* @return	rx_event *   - Pointer to the next "empty" RX event or NULL
*
* @note		None.
*
******************************************************************************/
rx_ofdm_event* get_next_empty_rx_ofdm_event(){

	// Get the next empty event
	return (rx_ofdm_event *)event_log_get_next_empty_entry( EVENT_TYPE_RX_OFDM, sizeof(rx_ofdm_event) );
}

/*****************************************************************************/
/**
* Get the next empty RX DSSS event
*
* @param    None.
*
* @return	rx_event *   - Pointer to the next "empty" RX event or NULL
*
* @note		None.
*
******************************************************************************/
rx_dsss_event* get_next_empty_rx_dsss_event(){

	// Get the next empty event
	return (rx_dsss_event *)event_log_get_next_empty_entry( EVENT_TYPE_RX_DSSS, sizeof(rx_dsss_event) );
}

/*****************************************************************************/
/**
* Get the next empty TX event
*
* @param    None.
*
* @return	tx_event *   - Pointer to the next "empty" TX event or NULL
*
* @note		None.
*
******************************************************************************/
tx_event* get_next_empty_tx_event(){

	// Get the next empty event
	return (tx_event *)event_log_get_next_empty_entry( EVENT_TYPE_TX, sizeof(tx_event) );

}

/*****************************************************************************/
/**
* Prints an entry
*
* @param    event_number     - Index of event in the log
*           event_type       - Type of event
*           timestamp        - Lower 32 bits of the timestamp
*           event            - Pointer to the event
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void print_entry( u32 entry_number, u32 entry_type, void * event ){
	u32 i, j;

	log_info_event     * log_info_event_log_item;
	exp_info_event     * exp_info_event_log_item;
	statistics_event   * statistics_event_log_item;
	rx_ofdm_event      * rx_ofdm_event_log_item;
	rx_dsss_event      * rx_dsss_event_log_item;
	tx_event           * tx_event_log_item;

	switch( entry_type ){
        case EVENT_TYPE_LOG_INFO:
        	log_info_event_log_item = (log_info_event*) event;
			xil_printf("%d: - Log Info Event\n", entry_number );
			xil_printf("   Type        :   %d\n",	  log_info_event_log_item->node_type);
			xil_printf("   ID          :   %d\n",     log_info_event_log_item->node_id);
			xil_printf("   HW Gen      :   %d\n",     log_info_event_log_item->node_hw_gen);
			xil_printf("   Design Ver  :   %x\n",     log_info_event_log_item->node_design_ver);
			xil_printf("   Serial Num  :   %d\n",     log_info_event_log_item->node_serial_number);
			xil_printf("   Max assn    :   %d\n",     log_info_event_log_item->node_wlan_max_assn);
			xil_printf("   Log size    :   %d\n",     log_info_event_log_item->node_wlan_event_log_size);
		break;

        case EVENT_TYPE_EXP_INFO:
        	exp_info_event_log_item = (exp_info_event*) event;
			xil_printf("%d: - Experiment Info Event\n", entry_number );
			xil_printf("   Timestamp:  %d\n", (u32)(exp_info_event_log_item->timestamp));
			xil_printf("   Reason   :  %d\n",       exp_info_event_log_item->reason);
			xil_printf("   Message  :  \n");
			for( i = 0; i < exp_info_event_log_item->length; i++) {
				xil_printf("        ");
				for( j = 0; j < 16; j++){
					xil_printf("0x%02x ", (exp_info_event_log_item->msg)[16*i + j]);
				}
				xil_printf("\n");
			}
		break;

		case EVENT_TYPE_STATISTICS:
			statistics_event_log_item = (statistics_event*) event;
			xil_printf("%d: - Statistics Event\n", entry_number );
			xil_printf("   Last timestamp :    %d\n",        (u32)(statistics_event_log_item->last_timestamp));
			xil_printf("   Address        :    %02x",             (statistics_event_log_item->addr)[0]);
			for( i = 1; i < 6; i++) { xil_printf(":%02x",         (statistics_event_log_item->addr)[i]); }
			xil_printf("\n");
			xil_printf("   Is associated  :    %d\n",              statistics_event_log_item->is_associated);
			xil_printf("   Tx total       :    %d (%d success)\n", statistics_event_log_item->num_tx_total,
					                                               statistics_event_log_item->num_tx_success);
			xil_printf("   Tx retry       :    %d\n",              statistics_event_log_item->num_retry);
			xil_printf("   Rx total       :    %d (%d bytes)\n",   statistics_event_log_item->num_rx_success,
					                                               statistics_event_log_item->num_rx_bytes);
		break;

		case EVENT_TYPE_RX_OFDM:
			rx_ofdm_event_log_item = (rx_ofdm_event*) event;
			xil_printf("%d: - Rx OFDM Event\n", entry_number );
			xil_printf("   Time:     %d\n",		(u32)(rx_ofdm_event_log_item->timestamp));
			xil_printf("   FCS:      %d\n",     rx_ofdm_event_log_item->fcs_status);
			xil_printf("   Pow:      %d\n",     rx_ofdm_event_log_item->power);
			xil_printf("   Rate:     %d\n",     rx_ofdm_event_log_item->rate);
			xil_printf("   Length:   %d\n",     rx_ofdm_event_log_item->length);
			xil_printf("   Pkt Type: 0x%x\n",   rx_ofdm_event_log_item->pkt_type);
			xil_printf("   Channel:  %d\n",     rx_ofdm_event_log_item->chan_num);
#ifdef WLAN_MAC_EVENTS_LOG_CHAN_EST
			xil_printf("   Channel Estimates:\n");

			for( i = 0; i < 16; i++) {
				xil_printf("        ");
				for( j = 0; j < 4; j++){
					xil_printf("0x%8x ", (rx_ofdm_event_log_item->channel_est)[4*i + j]);
				}
				xil_printf("\n");
			}
#endif
		break;

		case EVENT_TYPE_RX_DSSS:
			rx_dsss_event_log_item = (rx_dsss_event*) event;
			xil_printf("%d: - Rx DSSS Event\n", entry_number );
			xil_printf("   Time:     %d\n",		(u32)(rx_dsss_event_log_item->timestamp));
			xil_printf("   FCS:      %d\n",     rx_dsss_event_log_item->fcs_status);
			xil_printf("   Pow:      %d\n",     rx_dsss_event_log_item->power);
			xil_printf("   Rate:     %d\n",     rx_dsss_event_log_item->rate);
			xil_printf("   Length:   %d\n",     rx_dsss_event_log_item->length);
			xil_printf("   Pkt Type: 0x%x\n",   rx_dsss_event_log_item->pkt_type);
			xil_printf("   Channel:  %d\n",     rx_dsss_event_log_item->chan_num);
		break;

		case EVENT_TYPE_TX:
			tx_event_log_item = (tx_event*) event;
			xil_printf("%d: - Tx Event\n", entry_number);
			xil_printf("   Creation Time:    %d\n",		(u32)(tx_event_log_item->timestamp_create));
			xil_printf("   Accept Delay:     %d\n",		(u32)(tx_event_log_item->delay_accept));
			xil_printf("   Done Delay:       %d\n",		(u32)(tx_event_log_item->delay_done));
			xil_printf("   Tx Gain Target:   %d\n",     tx_event_log_item->gain_target);
			xil_printf("   Rate:             %d\n",     tx_event_log_item->rate);
			xil_printf("   Length:           %d\n",     tx_event_log_item->length);
			xil_printf("   Channel:          %d\n",     tx_event_log_item->chan_num);
			xil_printf("   Result:           %d\n",     tx_event_log_item->result);
			xil_printf("   Pkt Type:         0x%x\n",   tx_event_log_item->pkt_type);
			xil_printf("   Retry:            %d\n",     tx_event_log_item->retry_count);
		break;

		default:
			xil_printf("%d: - Unknown Event\n", entry_number);
		break;
	}

}



