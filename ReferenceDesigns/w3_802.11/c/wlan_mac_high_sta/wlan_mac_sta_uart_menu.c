/** @file wlan_mac_sta_uart_menu.c
 *  @brief Station UART Menu
 *
 *  This contains code for the 802.11 Station's UART menu.
 *
 *  @copyright Copyright 2013-2015, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *                See LICENSE.txt included in the design archive or
 *                at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

// Xilinx SDK includes
#include "xparameters.h"
#include "stdio.h"
#include "stdlib.h"
#include "xtmrctr.h"
#include "xio.h"
#include "string.h"
#include "xintc.h"

// WLAN includes
#include "wlan_mac_ipc_util.h"
#include "wlan_mac_misc_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_sta.h"
#include "wlan_mac_sta_join_fsm.h"
#include "ascii_characters.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_bss_info.h"


#ifndef WLAN_USE_UART_MENU

void uart_rx(u8 rxByte){ };

#else

#define MAX_NUM_CHARS                       31

extern u32                                  mac_param_chan;
extern tx_params                            default_unicast_data_tx_params;

extern bss_info*                            my_bss_info;
extern dl_list                              counts_table;

static volatile u8                          uart_mode            = UART_MODE_MAIN;
static volatile u32                         schedule_ID;
static volatile u8                          print_scheduled      = 0;



void uart_rx(u8 rxByte){

    u32            i;
    u32            tmp;
    dl_entry     * curr_dl_entry;
    bss_info     * curr_bss_info;
    station_info * access_point  = NULL;
    dl_list      * bss_info_list = wlan_mac_high_get_bss_info_list();

    if(my_bss_info != NULL){
        access_point = ((station_info*)(my_bss_info->associated_stations.first->data));
    }

    if(rxByte == ASCII_ESC){
        uart_mode = UART_MODE_MAIN;

        if(print_scheduled){
            wlan_mac_remove_schedule(SCHEDULE_COARSE, schedule_ID);
        }

        print_menu();

        ltg_sched_remove(LTG_REMOVE_ALL);

        return;
    }

    switch(uart_mode){
        case UART_MODE_MAIN:
            switch(rxByte){
                case ASCII_1:
                    uart_mode = UART_MODE_INTERACTIVE;
                    print_station_status(1);
                break;

                case ASCII_2:
                    print_all_observed_counts();
                break;

                case ASCII_e:
                    event_log_config_logging(EVENT_LOG_LOGGING_DISABLE);
                    print_event_log_size();
#ifdef _DEBUG_
                    print_event_log(0xFFFF);
                    print_event_log_size();
#endif
                    event_log_config_logging(EVENT_LOG_LOGGING_ENABLE);
                break;

                case ASCII_a:
                    print_bss_info();
                break;

                case ASCII_x:
                    reset_bss_info();
                break;

                case ASCII_j:
                    // NOTE:  This is a option that is not advertised because there is an issue:
                    //     Due to the way the character interrupt driven menu works, the STA must
                    //     print the BSS info right away and then wait for the selection.  However,
                    //     in the time it takes the user to make a selection, the BSS info list
                    //     might have changed (ie the order of the list could have changed; or
                    //     a BSS might have gone off-line).  Therefore, the selection might be
                    //     out of sync with the BSS that the user is trying to join.  There is no
                    //     obvious solution for this issue so this option will remain hidden.
                    //
                    uart_mode = UART_MODE_JOIN;
                    print_bss_info();

                    if (bss_info_list->length > 9) {
                        xil_printf("\nNOTE:  Can only join BSS between 0 and 9.\n");
                    }

                    xil_printf("\nSelect BSS to Join (0 - %d):  ", bss_info_list->length);
                break;

                case ASCII_r:
                    if((default_unicast_data_tx_params.phy.rate) > WLAN_MAC_MCS_6M){
                        (default_unicast_data_tx_params.phy.rate)--;
                    } else {
                        (default_unicast_data_tx_params.phy.rate) = WLAN_MAC_MCS_6M;
                    }

                    if(access_point != NULL) access_point->tx.phy.rate = (default_unicast_data_tx_params.phy.rate);

                    xil_printf("(-) Default Unicast Rate: %d Mbps\n", wlan_lib_mac_rate_to_mbps((default_unicast_data_tx_params.phy.rate)));
                break;

                case ASCII_R:
                    if((default_unicast_data_tx_params.phy.rate) < WLAN_MAC_MCS_54M){
                        (default_unicast_data_tx_params.phy.rate)++;
                    } else {
                        (default_unicast_data_tx_params.phy.rate) = WLAN_MAC_MCS_54M;
                    }

                    if(access_point != NULL) access_point->tx.phy.rate = (default_unicast_data_tx_params.phy.rate);

                    xil_printf("(+) Default Unicast Rate: %d Mbps\n", wlan_lib_mac_rate_to_mbps((default_unicast_data_tx_params.phy.rate)));
                break;
            }
        break;

        case UART_MODE_INTERACTIVE:
            switch(rxByte){
                case ASCII_r:
                    // Reset counts
                    reset_station_counts();
                break;
            }
        break;

        case UART_MODE_JOIN:
            xil_printf("%c\n", rxByte);

            if ((rxByte >= ASCII_0) && (rxByte <= ASCII_9)) {
                i             = 0;
                tmp           = rxByte - ASCII_0;
                curr_dl_entry = bss_info_list->last;
                curr_bss_info = NULL;

                while ((curr_dl_entry != NULL) && (i <= tmp)){
                    curr_bss_info = (bss_info*)(curr_dl_entry->data);

                    xil_printf("[%d]:  %s\n", i, curr_bss_info->ssid);

                    curr_dl_entry = dl_entry_prev(curr_dl_entry);
                    i++;
                }

                if (curr_bss_info != NULL) {
                    if(curr_bss_info->capabilities & CAPABILITIES_PRIVACY){
                        xil_printf("Cannot join a 'private' BSS.\n");
                    } else {
                        sta_disassociate();
                        xil_printf("Joining BSS:  %s\n", curr_bss_info->ssid);
                        wlan_mac_sta_scan_and_join(curr_bss_info->ssid, 10);
                    }
                } else {
                    xil_printf("Invalid selection.\n");
                }
            } else {
                xil_printf("Invalid selection.\n");
            }

            xil_printf("Returning to main menu.\n");
            usleep(5000000);

            uart_mode = UART_MODE_MAIN;
            print_menu();
        break;

        default:
            uart_mode = UART_MODE_MAIN;
            print_menu();
        break;
    }
}



void print_menu(){
    xil_printf("\f");
    xil_printf("********************** Station Menu **********************\n");
    xil_printf("[1]   - Interactive Station Status\n");
    xil_printf("[2]   - Print all Observed Counts\n");
    xil_printf("\n");
    xil_printf("[a]   - Display BSS information\n");
    xil_printf("[r/R] - change unicast rate\n");
    xil_printf("**********************************************************\n");
}



void print_station_status(u8 manual_call){

    u64 timestamp;
    dl_entry* access_point_entry = NULL;

    if(my_bss_info != NULL){
        access_point_entry = my_bss_info->associated_stations.first;
    }

    station_info* access_point = NULL;

    if(my_bss_info != NULL){
        access_point = ((station_info*)(access_point_entry->data));
    }
    counts_txrx* curr_counts;


    if(uart_mode == UART_MODE_INTERACTIVE){
        timestamp = get_system_time_usec();
        xil_printf("\f");
        xil_printf("---------------------------------------------------\n");

            if(my_bss_info != NULL){
                xil_printf(" AID: %02x -- MAC Addr: %02x:%02x:%02x:%02x:%02x:%02x\n", access_point->AID,
                            access_point->addr[0],access_point->addr[1],access_point->addr[2],access_point->addr[3],access_point->addr[4],access_point->addr[5]);

                curr_counts = access_point->counts;

                xil_printf("     - Last heard from         %d ms ago\n",((u32)(timestamp - (access_point->latest_activity_timestamp)))/1000);
                xil_printf("     - Last Rx Power:          %d dBm\n",access_point->rx.last_power);
                xil_printf("     - # of queued MPDUs:      %d\n", queue_num_queued(UNICAST_QID));
                xil_printf("     - # Tx High Data MPDUs:   %d (%d successful)\n", curr_counts->data.tx_num_packets_total, curr_counts->data.tx_num_packets_success);
                xil_printf("     - # Tx High Data bytes:   %d (%d successful)\n", (u32)(curr_counts->data.tx_num_bytes_total), (u32)(curr_counts->data.tx_num_bytes_success));
                xil_printf("     - # Tx Low Data MPDUs:    %d\n", curr_counts->data.tx_num_packets_low);
                xil_printf("     - # Tx High Mgmt MPDUs:   %d (%d successful)\n", curr_counts->mgmt.tx_num_packets_total, curr_counts->mgmt.tx_num_packets_success);
                xil_printf("     - # Tx High Mgmt bytes:   %d (%d successful)\n", (u32)(curr_counts->mgmt.tx_num_bytes_total), (u32)(curr_counts->mgmt.tx_num_bytes_success));
                xil_printf("     - # Tx Low Mgmt MPDUs:    %d\n", curr_counts->mgmt.tx_num_packets_low);
                xil_printf("     - # Rx Data MPDUs:        %d\n", curr_counts->data.rx_num_packets);
                xil_printf("     - # Rx Data Bytes:        %d\n", curr_counts->data.rx_num_bytes);
                xil_printf("     - # Rx Mgmt MPDUs:        %d\n", curr_counts->mgmt.rx_num_packets);
                xil_printf("     - # Rx Mgmt Bytes:        %d\n", curr_counts->mgmt.rx_num_bytes);
            }
        xil_printf("---------------------------------------------------\n");
        xil_printf("\n");
        xil_printf("[r] - reset counts\n\n");

        //Update display
        schedule_ID = wlan_mac_schedule_event(SCHEDULE_COARSE, 1000000, (void*)print_station_status);

    }
}



void print_all_observed_counts(){
    dl_entry     * curr_counts_entry;
    counts_txrx  * curr_counts;

    curr_counts_entry = counts_table.first;

    xil_printf("\nAll Counts:\n");
    while(curr_counts_entry != NULL){
        curr_counts = (counts_txrx*)(curr_counts_entry->data);
        xil_printf("---------------------------------------------------\n");
        xil_printf("%02x:%02x:%02x:%02x:%02x:%02x\n", curr_counts->addr[0], curr_counts->addr[1], curr_counts->addr[2],
                                                      curr_counts->addr[3], curr_counts->addr[4], curr_counts->addr[5]);
        xil_printf("     - Last timestamp:         %d usec\n", (u32)curr_counts->latest_txrx_timestamp);
        xil_printf("     - Associated?             %d\n", curr_counts->is_associated);
        xil_printf("     - # Tx High Data MPDUs:   %d (%d successful)\n", curr_counts->data.tx_num_packets_total, curr_counts->data.tx_num_packets_success);
        xil_printf("     - # Tx High Data bytes:   %d (%d successful)\n", (u32)(curr_counts->data.tx_num_bytes_total), (u32)(curr_counts->data.tx_num_bytes_success));
        xil_printf("     - # Tx Low Data MPDUs:    %d\n", curr_counts->data.tx_num_packets_low);
        xil_printf("     - # Tx High Mgmt MPDUs:   %d (%d successful)\n", curr_counts->mgmt.tx_num_packets_total, curr_counts->mgmt.tx_num_packets_success);
        xil_printf("     - # Tx High Mgmt bytes:   %d (%d successful)\n", (u32)(curr_counts->mgmt.tx_num_bytes_total), (u32)(curr_counts->mgmt.tx_num_bytes_success));
        xil_printf("     - # Tx Low Mgmt MPDUs:    %d\n", curr_counts->mgmt.tx_num_packets_low);
        xil_printf("     - # Rx Data MPDUs:        %d\n", curr_counts->data.rx_num_packets);
        xil_printf("     - # Rx Data Bytes:        %d\n", curr_counts->data.rx_num_bytes);
        xil_printf("     - # Rx Mgmt MPDUs:        %d\n", curr_counts->mgmt.rx_num_packets);
        xil_printf("     - # Rx Mgmt Bytes:        %d\n", curr_counts->mgmt.rx_num_bytes);
        curr_counts_entry = dl_entry_next(curr_counts_entry);
    }
}



#endif


