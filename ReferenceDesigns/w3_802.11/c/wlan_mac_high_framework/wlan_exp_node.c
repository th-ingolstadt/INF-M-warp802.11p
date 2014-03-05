/** @file wlan_exp_node.c
 *  @brief Experiment Framework
 *
 *  This contains the code for WARPnet Experimental Framework.
 *
 *  @copyright Copyright 2014, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 *  @bug No known bugs.
 */
/***************************** Include Files *********************************/

#include "wlan_exp_common.h"
#include "wlan_exp_node.h"
#include "wlan_exp_transport.h"

#ifdef USE_WARPNET_WLAN_EXP

#include <xparameters.h>
#include <xil_io.h>
#include <xio.h>
#include <stdlib.h>

#ifdef XPAR_XSYSMON_NUM_INSTANCES
#include <xsysmon_hw.h>
#endif

// WARP Includes
#include "w3_userio.h"

// WLAN includes
#include "wlan_mac_event_log.h"
#include "wlan_mac_entries.h"
#include "wlan_mac_ltg.h"




/*************************** Constant Definitions ****************************/

// #define _DEBUG_

#ifdef XPAR_XSYSMON_NUM_INSTANCES
#define SYSMON_BASEADDR		           XPAR_SYSMON_0_BASEADDR
#endif


/*********************** Global Variable Definitions *************************/

extern int                 sock_unicast; // UDP socket for unicast traffic to / from the board
extern struct sockaddr_in  addr_unicast;

extern int                 sock_bcast; // UDP socket for broadcast traffic to the board
extern struct sockaddr_in  addr_bcast;

extern int                 sock_async; // UDP socket for async transmissions from the board
extern struct sockaddr_in  addr_async;


// Declared in each of the AP / STA
extern u8                  default_tx_gain_target;
extern u8                  default_unicast_rate;

extern dl_list		       association_table;


/*************************** Variable Definitions ****************************/

wn_node_info       node_info;
wn_tag_parameter   node_parameters[NODE_MAX_PARAMETER];

wn_function_ptr_t  node_process_callback;
extern function_ptr_t check_queue_callback;

u32                   async_pkt_enable;
u32                   async_eth_dev_num;
pktSrcInfo            async_pkt_dest;
wn_transport_header   async_pkt_hdr;


/*************************** Functions Prototypes ****************************/

void node_init_system_monitor(void);
int  node_init_parameters( u32 *info );
int  node_processCmd(const wn_cmdHdr* cmdHdr,const void* cmdArgs, wn_respHdr* respHdr,void* respArgs, void* pktSrc, unsigned int eth_dev_num);

void node_ltg_cleanup(u32 id, void* callback_arg);

#ifdef _DEBUG_
void print_wn_node_info( wn_node_info * info );
void print_wn_parameters( wn_tag_parameter *param, int num_params );
#endif


// Functions implemented in AP / STA
void reset_station_statistics();
void purge_all_data_tx_queue();




/******************************** Functions **********************************/


/*****************************************************************************/
/**
* Node Processes Null Callback
*
* This function is part of the callback system for processing node commands.
* If there are no additional node commands, then this will return appropriate values.
*
* To processes additional node commands, please set the node_process_callback
*
* @param    void * param  - Parameters for the callback
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
int wlan_exp_null_process_callback(unsigned int cmdID, void* param){

	xil_printf("Unknown node command: %d\n", cmdID);

	return NO_RESP_SENT;
};



/*****************************************************************************/
/**
* Node Transport Processing
*
* This function is part of the callback system for the Ethernet transport.
* Based on the Command Group field in the header, it will call the appropriate
* processing function.
*
* @param    Message to Node   - WARPNet Host Message to the node
*           Message from Node - WARPNet Host Message from the node
*           Packet Source          - Ethernet Packet Source
*           Ethernet Device Number - Indicates which Ethernet device packet came from
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void node_rxFromTransport(wn_host_message* toNode, wn_host_message* fromNode, void* pktSrc, unsigned int eth_dev_num){
	unsigned char cmd_grp;

	unsigned int respSent;

#ifdef _DEBUG_
	xil_printf("In node_rxFromTransport() \n");
#endif

	//Helper struct pointers to interpret the received packet contents
	wn_cmdHdr* cmdHdr;
	void * cmdArgs;

	//Helper struct pointers to form a response packet
	wn_respHdr* respHdr;
	void * respArgs;

	cmdHdr  = (wn_cmdHdr*)(toNode->payload);
	cmdArgs = (toNode->payload) + sizeof(wn_cmdHdr);

	//Endian swap the command header (this is the first place we know what/where it is)
	cmdHdr->cmd     = Xil_Ntohl(cmdHdr->cmd);
	cmdHdr->length  = Xil_Ntohs(cmdHdr->length);
	cmdHdr->numArgs = Xil_Ntohs(cmdHdr->numArgs);

	//Outgoing response header must be endian swapped as it's filled in
	respHdr  = (wn_respHdr*)(fromNode->payload);
	respArgs = (fromNode->payload) + sizeof(wn_cmdHdr);

	cmd_grp = WN_CMD_TO_GRP(cmdHdr->cmd);
	switch(cmd_grp){
		case WARPNET_GRP:
		case NODE_GRP:
			respSent = node_processCmd(cmdHdr,cmdArgs,respHdr,respArgs, pktSrc, eth_dev_num);
		break;
		case TRANS_GRP:
			respSent = transport_processCmd(cmdHdr,cmdArgs,respHdr,respArgs, pktSrc, eth_dev_num);
		break;
		default:
			xil_printf("Unknown command group\n");
		break;
	}

	if(respSent == NO_RESP_SENT)	fromNode->length += (respHdr->length + sizeof(wn_cmdHdr));


	//Endian swap the response header before returning
	// Do it here so the transport sender doesn't have to understand any payload contents
	respHdr->cmd     = Xil_Ntohl(respHdr->cmd);
	respHdr->length  = Xil_Ntohs(respHdr->length);
	respHdr->numArgs = Xil_Ntohs(respHdr->numArgs);

	return;
}



/*****************************************************************************/
/**
* Node Send Early Response
*
* Allows a node to send a response back to the host before the command has
* finished being processed.  This is to minimize the latency between commands
* since the node is able to finish processing the command during the time
* it takes to communicate to the host and receive another command.
*
* @param    Response Header        - WARPNet Response Header
*           Packet Source          - Ethernet Packet Source
*           Ethernet Device Number - Indicates which Ethernet device packet came from
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void node_sendEarlyResp(wn_respHdr* respHdr, void* pktSrc, unsigned int eth_dev_num){
	/* This function is used to send multiple command responses back to the host
	 * under the broader umbrella of a single command exchange. The best example
	 * of this functionality is a 'readIQ' command where a single packet from
	 * the host results in many response packets returning from the board.
	 *
	 * A key assumption in the use of this function is that the underlying command
	 * from the host does not raise the transport-level ACK flag in the transport
	 * header. Furthermore, this function exploits the fact that wn_node can determine
	 * the beginning of the overall send buffer from the location of the response to
	 * be sent.
	 */

	 wn_host_message nodeResp;

#ifdef _DEBUG_
	 xil_printf("In node_sendEarlyResp() \n");
#endif

	 nodeResp.payload = (void*) respHdr;
	 nodeResp.buffer  = (void*) respHdr - ( PAYLOAD_OFFSET + sizeof(wn_transport_header) );
	 nodeResp.length  = PAYLOAD_PAD_NBYTES + respHdr->length + sizeof(wn_cmdHdr); //Extra 2 bytes is for alignment

	//Endian swap the response header before before transport sends it
	// Do it here so the transport sender doesn't have to understand any payload contents
	respHdr->cmd     = Xil_Ntohl(respHdr->cmd);
	respHdr->length  = Xil_Ntohs(respHdr->length);
	respHdr->numArgs = Xil_Ntohs(respHdr->numArgs);

#ifdef _DEBUG_
	xil_printf("sendEarlyResp\n");
	xil_printf("payloadAddr = 0x%x, bufferAddr = 0x%x, len = %d\n",nodeResp.payload,nodeResp.buffer,nodeResp.length);
#endif

	 transport_send(sock_unicast, &nodeResp, pktSrc, eth_dev_num);

}



/*****************************************************************************/
/**
* Node Commands
*
* This function is part of the callback system for the Ethernet transport
* and will be executed when a valid node commands is recevied.
*
* @param    Command Header         - WARPNet Command Header
*           Command Arguments      - WARPNet Command Arguments
*           Response Header        - WARPNet Response Header
*           Response Arguments     - WARPNet Response Arguments
*           Packet Source          - Ethernet Packet Source
*           Ethernet Device Number - Indicates which Ethernet device packet came from
*
* @return	None.
*
* @note		See on-line documentation for more information about the ethernet
*           packet structure for WARPNet:  www.warpproject.org
*
******************************************************************************/
int node_processCmd(const wn_cmdHdr* cmdHdr,const void* cmdArgs, wn_respHdr* respHdr,void* respArgs, void* pktSrc, unsigned int eth_dev_num){
	//IMPORTANT ENDIAN NOTES:
	// -cmdHdr is safe to access directly (pre-swapped if needed)
	// -cmdArgs is *not* pre-swapped, since the framework doesn't know what it is
	// -respHdr will be swapped by the framework; user code should fill it normally
	// -respArgs will *not* be swapped by the framework, since only user code knows what it is
	//    Any data added to respArgs by the code below must be endian-safe (swapped on AXI hardware)

	int           status     = 0;
	const u32   * cmdArgs32  = cmdArgs;
	u32         * respArgs32 = respArgs;

	unsigned int  respIndex  = 0;
	unsigned int  respSent   = NO_RESP_SENT;
    unsigned int  max_words  = 320;                // Max number of u32 words that can be sent in the packet (~1400 bytes)
                                                   //   If we need more, then we will need to rework this to send multiple response packets

    unsigned int  temp, temp2, i;

    // Variables for functions
    u32           id;
    u32           flags;
    u32           serial_number;
	u32           start_address;
	u32           curr_address;
	u32           next_address;
	u32           ip_address;
	u32           size;
	u32           evt_log_size;
	u32           transfer_size;
	u32           bytes_per_pkt;
	u32           num_bytes;
	u32           num_pkts;
	u64           time;

	u8            mac_addr[6];

	station_info* curr_station_info;


	unsigned int  cmdID;
    
	cmdID = WN_CMD_TO_CMDID(cmdHdr->cmd);
    
	respHdr->cmd     = cmdHdr->cmd;
	respHdr->length  = 0;
	respHdr->numArgs = 0;

#ifdef _DEBUG_
	xil_printf("In node_processCmd():  ID = %d \n", cmdID);
#endif

	wlan_mac_high_cdma_finish_transfer();

	switch(cmdID){

	    //---------------------------------------------------------------------
        case WARPNET_TYPE:
        	// Return the WARPNet Type
            respArgs32[respIndex++] = Xil_Htonl( node_info.type );    

#ifdef _DEBUG_
            xil_printf("WARPNet Type = %d \n", node_info.type);
#endif

            respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
        break;
        
    
	    //---------------------------------------------------------------------
		case NODE_INFO:
			// Return the info about the WLAN_EXP_NODE
            
            // Send node parameters
            temp = node_get_parameters( &respArgs32[respIndex], max_words, WN_TRANSMIT);
            respIndex += temp;
            max_words -= temp;
            if ( max_words <= 0 ) { xil_printf("No more space left in NODE_INFO packet \n"); };
            
            // Send transport parameters
            temp = transport_get_parameters( eth_dev_num, &respArgs32[respIndex], max_words, WN_TRANSMIT);
            respIndex += temp;
            max_words -= temp;
            if ( max_words <= 0 ) { xil_printf("No more space left in NODE_INFO packet \n"); };

#ifdef _DEBUG_
            xil_printf("NODE INFO: \n");
            for ( i = 0; i < respIndex; i++ ) {
            	xil_printf("   [%2d] = 0x%8x \n", i, respArgs32[i]);
            }
            xil_printf("END NODE INFO \n");
#endif

            // --------------------------------
            // Future parameters go here
            // --------------------------------
                        
            // Finalize response
			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;
        

	    //---------------------------------------------------------------------
		case NODE_IDENTIFY:
			// Blink the HEX display LEDs
			//     The current blink time is 10 seconds (25 times at 0.4 sec per blink)
			//     Returns Null Response

            #define NODE_IDENTIFY_NUM_BLINKS         25
            #define NODE_IDENTIFY_BLINK_USEC_HALF    200000

			// Send the response early so that code does not time out while waiting for blinks
			//   The node is responsible for waiting until the LED blinking is done before issuing the
			//   node another command.
			node_sendEarlyResp(respHdr, pktSrc, eth_dev_num);
			respSent = RESP_SENT;

			temp = Xil_Ntohl(cmdArgs32[0]);

			u32  left_hex;
			u32  right_hex;

			// If parameter is not the magic number, then set the TX power
			if ( (temp == NODE_IDENTIFY_ALL) || (temp == node_info.serial_number) ) {
	            xil_printf("WARPNET Node: %d    IP Address: %d.%d.%d.%d \n", node_info.node, node_info.ip_addr[0], node_info.ip_addr[1],node_info.ip_addr[2],node_info.ip_addr[3]);

	            // Store the original value
            	left_hex  = userio_read_hexdisp_left(USERIO_BASEADDR);
            	right_hex = userio_read_hexdisp_right(USERIO_BASEADDR);

            	// Blink for 10 seconds
				for (i = 0; i < NODE_IDENTIFY_NUM_BLINKS; i++){
	                userio_write_control( USERIO_BASEADDR, ( userio_read_control( USERIO_BASEADDR ) & ( ~( W3_USERIO_HEXDISP_L_MAPMODE | W3_USERIO_HEXDISP_R_MAPMODE ) ) ) );
			        userio_write_hexdisp_left(USERIO_BASEADDR, 0x00);
			        userio_write_hexdisp_right(USERIO_BASEADDR, 0x00);
					usleep(NODE_IDENTIFY_BLINK_USEC_HALF);

					userio_write_control(USERIO_BASEADDR, userio_read_control(USERIO_BASEADDR) | (W3_USERIO_HEXDISP_L_MAPMODE | W3_USERIO_HEXDISP_R_MAPMODE));
	                userio_write_hexdisp_left( USERIO_BASEADDR, left_hex );
		            userio_write_hexdisp_right(USERIO_BASEADDR, right_hex );
					usleep(NODE_IDENTIFY_BLINK_USEC_HALF);
				}
			}
        break;


	    //---------------------------------------------------------------------
		case NODE_CONFIG_SETUP:
            // NODE_CONFIG_SETUP Packet Format:
            //   - Note:  All u32 parameters in cmdArgs32 are byte swapped so use Xil_Ntohl()
            //
            //   - cmdArgs32[0] - Serial Number
            //   - cmdArgs32[1] - Node ID
            //   - cmdArgs32[2] - IP Address
            //   - cmdArgs32[3] - Unicast Port
            //   - cmdArgs32[4] - Broadcast Port
            // 

			// Only update the parameters if the serial numbers match
            if ( node_info.serial_number ==  Xil_Ntohl(cmdArgs32[0]) ) {

            	// Only update the node if it has not been configured
            	if ( node_info.node == 0xFFFF ) {
                    xil_printf("\nReconfiguring ETH %c \n", wn_conv_eth_dev_num(eth_dev_num) );

                	node_info.node = Xil_Ntohl(cmdArgs32[1]) & 0xFFFF;

                    xil_printf("  New Node ID       : %d \n", node_info.node);
                    
                    // Grab New IP Address
                    node_info.ip_addr[0]     = (Xil_Ntohl(cmdArgs32[2]) >> 24) & 0xFF;
                    node_info.ip_addr[1]     = (Xil_Ntohl(cmdArgs32[2]) >> 16) & 0xFF;
                    node_info.ip_addr[2]     = (Xil_Ntohl(cmdArgs32[2]) >>  8) & 0xFF;
                    node_info.ip_addr[3]     = (Xil_Ntohl(cmdArgs32[2])      ) & 0xFF;
                    
                    // Grab new ports
                    node_info.unicast_port   = Xil_Ntohl(cmdArgs32[3]);
                    node_info.broadcast_port = Xil_Ntohl(cmdArgs32[4]);

                    xil_printf("  New IP Address    : %d.%d.%d.%d \n", node_info.ip_addr[0], node_info.ip_addr[1],node_info.ip_addr[2],node_info.ip_addr[3]);
                    xil_printf("  New Unicast Port  : %d \n", node_info.unicast_port);
                    xil_printf("  New Broadcast Port: %d \n", node_info.broadcast_port);

                    transport_set_hw_info( eth_dev_num, node_info.ip_addr, node_info.hw_addr);

                    status = transport_config_sockets(eth_dev_num, node_info.unicast_port, node_info.broadcast_port);

                    xil_printf("\n");
                    if(status != 0) {
        				xil_printf("Error binding transport...\n");
        			}
                } else {
                    xil_printf("NODE_CONFIG_SETUP Packet ignored.  Network already configured for node %d.\n", node_info.node);
                    xil_printf("    Use NODE_CONFIG_RESET command to reset network configuration.\n\n");
                }
            } else {
                xil_printf("NODE_CONFIG_SETUP Packet with Serial Number %d ignored.  My serial number is %d \n", Xil_Ntohl(cmdArgs32[0]), node_info.serial_number);
            }
		break;

        
	    //---------------------------------------------------------------------
		case NODE_CONFIG_RESET:
            // NODE_CONFIG_RESET Packet Format:
            //   - Note:  All u32 parameters in cmdArgs32 are byte swapped so use Xil_Ntohl()
            //
            //   - cmdArgs32[0] - Serial Number
            // 
            
            // Send the response early so that M-Code does not hang when IP address changes
			node_sendEarlyResp(respHdr, pktSrc, eth_dev_num);
			respSent = RESP_SENT;
            
			serial_number = Xil_Ntohl(cmdArgs32[0]);

            // Only update the parameters if the serial numbers match or this it is "all serial numbers"
            if ( (node_info.serial_number ==  serial_number) || (NODE_CONFIG_RESET_ALL == serial_number) ) {

            	if (node_info.node != 0xFFFF){

					// Reset node to 0xFFFF
					node_info.node = 0xFFFF;

					xil_printf("\n!!! Reseting Network Configuration !!! \n\n");

					// Reset transport;  This will update the IP Address back to default and rebind the sockets
					//   - See below for default IP address:  NODE_IP_ADDR_BASE + node
					node_info.ip_addr[0]      = (NODE_IP_ADDR_BASE >> 24) & 0xFF;
					node_info.ip_addr[1]      = (NODE_IP_ADDR_BASE >> 16) & 0xFF;
					node_info.ip_addr[2]      = (NODE_IP_ADDR_BASE >>  8) & 0xFF;
					node_info.ip_addr[3]      = (NODE_IP_ADDR_BASE      ) & 0xFF;  // IP ADDR = w.x.y.z

					node_info.unicast_port    = NODE_UDP_UNICAST_PORT_BASE;
					node_info.broadcast_port  = NODE_UDP_MCAST_BASE;

					transport_set_hw_info(eth_dev_num, node_info.ip_addr, node_info.hw_addr);
					transport_config_sockets(eth_dev_num, node_info.unicast_port, node_info.broadcast_port);

					// Update User IO
					xil_printf("\n!!! Waiting for Network Configuration !!! \n\n");
            	} else {
                    xil_printf("NODE_CONFIG_RESET Packet ignored.  Network already reset for node %d.\n", node_info.node);
                    xil_printf("    Use NODE_CONFIG_SETUP command to set the network configuration.\n\n");
            	}
            } else {
                xil_printf("NODE_CONFIG_RESET Packet with Serial Number %d ignored.  My serial number is %d \n", Xil_Ntohl(cmdArgs32[0]), node_info.serial_number);
            }
		break;


	    //---------------------------------------------------------------------
		case NODE_TEMPERATURE:
            // NODE_TEMPERATURE
            //   - If the system monitor exists, return the current, min and max temperature of the node
            //
			respArgs32[respIndex++] = Xil_Htonl(wn_get_curr_temp());
			respArgs32[respIndex++] = Xil_Htonl(wn_get_min_temp());
			respArgs32[respIndex++] = Xil_Htonl(wn_get_max_temp());

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;


		// Case NODE_ASSN_GET_STATUS  is implemented in the child classes

		// Case NODE_ASSN_SET_TABLE   is implemented in the child classes

		// Case NODE_DISASSOCIATE     is implemented in the child classes


	    //---------------------------------------------------------------------
		// TODO:  THIS FUNCTION IS NOT COMPLETE
		case NODE_TX_GAIN:
			//TODO: !!! Replace with NODE_TX_POWER
#if 0
			// Set / Get node TX gain
			temp = Xil_Ntohl(cmdArgs32[0]);

			// If parameter is not the magic number, then set the TX power
			if ( temp != NODE_TX_GAIN_RSVD_VAL ) {

				if (temp <  0) {
					default_tx_gain_target = 0;
				}
				else  if (temp > 63) {
					default_tx_gain_target = 63;
				}
				else {
					default_tx_gain_target = temp;
				}

			    xil_printf("Setting TX gain = %d\n", temp);
			}

			// Send response of current power
            respArgs32[respIndex++] = Xil_Htonl( default_tx_gain_target );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
#endif
		break;


		// Case NODE_TX_RATE is implemented in the child classes
	    //---------------------------------------------------------------------
		case NODE_TX_RATE:
            // NODE_TX_RATE Packet Format:
			//   - cmdArgs32[0 - 1]  - MAC Address (All 0xF means all nodes)
			//   - cmdArgs32[2]      - Rate

			// Get MAC Address
        	wlan_exp_get_mac_addr(&((u32 *)cmdArgs32)[0], &mac_addr[0]);
        	id = wlan_exp_get_aid_from_ADDR(&mac_addr[0]);

			// Local variables
			u32  rate;

        	// Get node TX rate and adjust so that it is a legal value
			rate = Xil_Ntohl(cmdArgs32[2]);

			if(rate < WLAN_MAC_RATE_6M ){ rate = WLAN_MAC_RATE_6M;  }
			if(rate > WLAN_MAC_RATE_54M){ rate = WLAN_MAC_RATE_54M; }

			// If parameter is not the magic number, then set the TX rate
			if ( rate != NODE_TX_RATE_RSVD_VAL ) {
				// If the ID is not for all nodes, configure the node
				if ( id != NODE_CONFIG_ALL_ASSOCIATED ) {
                    // Set the rate of the station
					curr_station_info = (station_info*)(association_table.first);
					for(i=0; i < association_table.length; i++){
						if (curr_station_info->AID == id){
							curr_station_info->tx.rate = rate;
							xil_printf("Setting TX rate on AID %d = %d Mbps\n", id, wlan_lib_mac_rate_to_mbps(rate));
							break;
						}
						curr_station_info = (station_info*)((curr_station_info->entry).next);
					}
				} else {
                    // Set the rate of all stations
					default_unicast_rate = rate;

					curr_station_info = (station_info*)(association_table.first);
					for(i=0; i < association_table.length; i++){
						curr_station_info->tx.rate = default_unicast_rate;
						curr_station_info = (station_info*)((curr_station_info->entry).next);
					}

					xil_printf("Setting Default TX rate = %d Mbps\n", wlan_lib_mac_rate_to_mbps(default_unicast_rate));
				}
			} else {
				// If the ID is not for all nodes, configure the node
				if ( id != NODE_CONFIG_ALL_ASSOCIATED ) {
					// Get the rate of the station
					curr_station_info = (station_info*)(association_table.first);
					for(i=0; i < association_table.length; i++){
						if (curr_station_info->AID == id){
							rate = curr_station_info->tx.rate;
							break;
						}
						curr_station_info = (station_info*)((curr_station_info->entry).next);
					}
				} else {
					// Get the default rate
					rate = default_unicast_rate;
				}
			}

			// Send response of current rate
            respArgs32[respIndex++] = Xil_Htonl( rate );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;


		// Case NODE_CHANNEL is implemented in the child classes


	    //---------------------------------------------------------------------
		case NODE_TIME:
			// Set / Get node time
			//
			// Message format:
			//     cmdArgs32[0]   Time in microseconds - lower 32 bits (or 0x0000FFFF)
			//     cmdArgs32[1]   Time in microseconds - upper 32 bits (or 0x0000FFFF)
			//
			temp  = Xil_Ntohl(cmdArgs32[0]);
			temp2 = Xil_Ntohl(cmdArgs32[1]);

			// If parameter is not the magic number, then set the time on the node
			if ( ( temp != NODE_TIME_RSVD_VAL ) && ( temp2 != NODE_TIME_RSVD_VAL ) ) {

				time = (((u64)temp2)<<32) + ((u64)temp);

				wlan_mac_high_set_timestamp( time );

			    xil_printf("WARPNET:  Setting time = 0x%08x 0x%08x\n", temp2, temp);
			} else {
				time  = get_usec_timestamp();
				temp  = time & 0xFFFFFFFF;
				temp2 = (time >> 32) & 0xFFFFFFFF;
			}

			// Send response of current power
            respArgs32[respIndex++] = Xil_Htonl( temp );
            respArgs32[respIndex++] = Xil_Htonl( temp2 );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;


	    //---------------------------------------------------------------------
		case NODE_LTG_CONFIG:
            // NODE_LTG_START Packet Format:
			//   - cmdArgs32[0 - 1]  - MAC Address
			//   - cmdArgs32[2 - N]  - LTG Schedule (packed)
			//                         [0] - [31:16] Type    [15:0] Length
			//   - cmdArgs32[N+1 - M]- LTG Payload (packed)
			//                         [0] - [31:16] Type    [15:0] Length
			//
            //   - respArgs32[0] - 0           - Success
			//                     0xFFFF_FFFF - Failure

			// Get MAC Address
        	wlan_exp_get_mac_addr(&((u32 *)cmdArgs32)[0], &mac_addr[0]);
        	id = wlan_exp_get_aid_from_ADDR(&mac_addr[0]);

            // Local variables
			u32            s1, s2, t1, t2;
			void *         ltg_callback_arg;
        	void *         params;

        	// Check to see if LTG ID already exists
			if( ltg_sched_get_callback_arg( id, &ltg_callback_arg ) == 0 ) {
				// This LTG has already been configured. We need to free the old callback argument so we can create a new one.
				ltg_sched_stop( id );
				wlan_mac_high_free( ltg_callback_arg );
			}

			// Get Schedule
			params           = ltg_sched_deserialize( &(((u32 *)cmdArgs)[2]), &t1, &s1 );
			ltg_callback_arg = ltg_payload_deserialize( &(((u32 *)cmdArgs)[3 + s1]), &t2, &s2);

			if( (ltg_callback_arg != NULL) && (params != NULL) ) {
				memcpy(((ltg_pyld_hdr*)ltg_callback_arg)->addr_da,mac_addr,6);
				// Configure the LTG
				status = ltg_sched_configure( id, t1, params, ltg_callback_arg, &node_ltg_cleanup );

				xil_printf("LTG %d configured\n", id);

			} else {
				xil_printf("ERROR:  LTG - Error allocating memory for ltg_callback_arg\n");
				status = NODE_LTG_ERROR;
			}

			// Send response of current channel
            respArgs32[respIndex++] = Xil_Htonl( status );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;


	    //---------------------------------------------------------------------
		case NODE_LTG_START:
            // NODE_LTG_START Packet Format:
			//   - cmdArgs32[0 - 1]  - MAC Address
			//                         - 0xFFFF_FFFF_FFFF  -> Start all IDs
			//
            //   - respArgs32[0] - 0           - Success
			//                     0xFFFF_FFFF - Failure

			// Get MAC Address
        	wlan_exp_get_mac_addr(&((u32 *)cmdArgs32)[0], &mac_addr[0]);
        	id = wlan_exp_get_aid_from_ADDR(&mac_addr[0]);

			// If parameter is not the magic number, then start the LTG
			if ( id != NODE_CONFIG_ALL_ASSOCIATED ) {
                // Try to start the ID
		        status = ltg_sched_start( id );

		        if ( status != 0 ) {
					xil_printf("WARNING:  LTG - LTG %d failed to start.\n", id);
		        	status = NODE_LTG_ERROR;
		        } else {
					xil_printf("Starting LTG %d.\n", id);
			    }
			} else {
				// Start all LTGs
				status = ltg_sched_start_all();

				if ( status != 0 ) {
					xil_printf("WARNING:  LTG - Some LTGs failed to start.\n");
					status = NODE_LTG_ERROR;
				} else {
					xil_printf("Starting all LTGs.\n");
				}
			}

			// Send response of current rate
            respArgs32[respIndex++] = Xil_Htonl( status );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;


	    //---------------------------------------------------------------------
		case NODE_LTG_STOP:
            // NODE_LTG_STOP Packet Format:
			//   - cmdArgs32[0 - 1]  - MAC Address
			//                         - 0xFFFF_FFFF_FFFF  -> Stop all IDs
			//
            //   - respArgs32[0] - 0           - Success
			//                     0xFFFF_FFFF - Failure

			// Get MAC Address
        	wlan_exp_get_mac_addr(&((u32 *)cmdArgs32)[0], &mac_addr[0]);
        	id = wlan_exp_get_aid_from_ADDR(&mac_addr[0]);

			// If parameter is not the magic number, then stop the LTG
			if ( id != NODE_CONFIG_ALL_ASSOCIATED ) {
                // Try to start the ID
		        status = ltg_sched_stop( id );

		        if ( status != 0 ) {
					xil_printf("WARNING:  LTG - LTG %d failed to stop.\n", id);
		        	status = NODE_LTG_ERROR;
		        } else {
					xil_printf("Stopping LTG %d.\n", id);
			    }
			} else {
				// Start all LTGs
				status = ltg_sched_stop_all();

				if ( status != 0 ) {
					xil_printf("WARNING:  LTG - Some LTGs failed to stop.\n");
					status = NODE_LTG_ERROR;
				} else {
					xil_printf("Stopping all LTGs.\n");
				}
			}

			// Send response of current rate
            respArgs32[respIndex++] = Xil_Htonl( status );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;


	    //---------------------------------------------------------------------
		case NODE_LTG_REMOVE:
            // NODE_LTG_REMOVE Packet Format:
			//   - cmdArgs32[0 - 1]  - MAC Address
			//                         - 0xFFFF_FFFF_FFFF  -> Remove all IDs
			//
            //   - respArgs32[0] - 0           - Success
			//                     0xFFFF_FFFF - Failure

			// Get MAC Address
        	wlan_exp_get_mac_addr(&((u32 *)cmdArgs32)[0], &mac_addr[0]);
        	id = wlan_exp_get_aid_from_ADDR(&mac_addr[0]);

			// If parameter is not the magic number, then remove the LTG
			if ( id != NODE_CONFIG_ALL_ASSOCIATED ) {
                // Try to remove the ID
		        status = ltg_sched_remove( id );

		        if ( status != 0 ) {
					xil_printf("WARNING:  LTG - LTG %d failed to remove.\n", id);
		        	status = NODE_LTG_ERROR;
		        } else {
					xil_printf("Removing LTG %d.\n", id);
			    }
			} else {
				// Remove all LTGs
		        status = ltg_sched_remove( id );

		        if ( status != 0 ) {
					xil_printf("WARNING:  LTG - Failed to remove all LTGs.\n");
		        	status = NODE_LTG_ERROR;
		        } else {
					xil_printf("Removing All LTGs.\n");
			    }
			}

			// Send response of status
            respArgs32[respIndex++] = Xil_Htonl( status );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;


	    //---------------------------------------------------------------------
		case NODE_LOG_RESET:
			xil_printf("EVENT LOG:  Reset log\n");
			event_log_reset();
	    break;


	    //---------------------------------------------------------------------
		case NODE_LOG_CONFIG:
            // NODE_LOG_CONFIG Packet Format:
			//   - cmdArgs32[0]  - flags
			//                     [ 0] - Wrap = 1; No Wrap = 0;
			//
            //   - respArgs32[0] - 0           - Success
			//                     0xFFFF_FFFF - Failure

			// Set the return value
			status = 0;

			// Get flags
			temp = Xil_Ntohl(cmdArgs32[0]);

			// Configure the LTG based on the flag bits
			if ( ( temp & NODE_LOG_CONFIG_FLAG_WRAP ) == NODE_LOG_CONFIG_FLAG_WRAP ) {
				event_log_config_wrap( EVENT_LOG_WRAP_ENABLE );
			} else {
				event_log_config_wrap( EVENT_LOG_WRAP_DISABLE );
			}

			// Send response of status
            respArgs32[respIndex++] = Xil_Htonl( status );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
	    break;


	    //---------------------------------------------------------------------
		case NODE_LOG_GET_CURR_IDX:
			// Get the current index of the log
			temp = event_log_get_current_index();

			xil_printf("EVENT LOG:  Current index = %d\n", temp);

			// Send response of current index
            respArgs32[respIndex++] = Xil_Htonl( temp );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
	    break;


	    //---------------------------------------------------------------------
		case NODE_LOG_GET_OLDEST_IDX:
			// Get the current index of the log
			temp = event_log_get_oldest_entry_index();

			xil_printf("EVENT LOG:  Oldest index  = %d\n", temp);

			// Send response of oldest index
            respArgs32[respIndex++] = Xil_Htonl( temp );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
	    break;


	    //---------------------------------------------------------------------
		case NODE_LOG_GET_EVENTS:
            // NODE_GET_EVENTS Packet Format:
            //   - Note:  All u32 parameters in cmdArgs32 are byte swapped so use Xil_Ntohl()
            //
			//   - cmdArgs32[0] - buffer id
			//   - cmdArgs32[1] - flags
            //   - cmdArgs32[2] - start_address of transfer
			//   - cmdArgs32[3] - size of transfer (in bytes)
			//                      0xFFFF_FFFF  -> Get everything in the event log
			//
			//   Return Value:
			//     - wn_buffer
            //       - buffer_id  - uint32  - ID of the buffer
			//       - flags      - uint32  - Flags
			//       - start_byte - uint32  - Byte index of the first byte in this packet
			//       - size       - uint32  - Number of payload bytes in this packet
			//       - byte[]     - uint8[] - Array of payload bytes
			//
			// NOTE:  The address passed via the command is the address relative to the current
			//   start of the event log.  It is not an absolute address and should not be treated
			//   as such.
			//
			//     When you transferring "everything" in the event log, the command will take a
			//   snapshot of the size of the log at the time the command is received.  It will then
			//   only transfer those events and not any new events that are added to the log while
			//   we are transferring the current log.
            //

			id                = Xil_Ntohl(cmdArgs32[0]);
			flags             = Xil_Ntohl(cmdArgs32[1]);
			start_address     = Xil_Ntohl(cmdArgs32[2]);
            size              = Xil_Ntohl(cmdArgs32[3]);

            // Get the current size of the event log
            evt_log_size      = event_log_get_size();

            // Check if we should transfer everything or if the request was larger than the current log
            if ( ( size == 0xFFFFFFFF ) || ( size > evt_log_size ) ) {
                size = evt_log_size;
            }

            bytes_per_pkt     = max_words * 4;
            num_pkts          = size / bytes_per_pkt + 1;
            curr_address      = start_address;

#ifdef _DEBUG_
			xil_printf("WLAN EXP NODE_GET_EVENTS \n");
			xil_printf("    start_address    = 0x%8x\n    size             = %10d\n    num_pkts         = %10d\n", start_address, size, num_pkts);
#endif

            // Initialize constant parameters
            respArgs32[0] = Xil_Htonl( id );
            respArgs32[1] = Xil_Htonl( flags );

            // Iterate through all the packets
			for( i = 0; i < num_pkts; i++ ) {

				// Get the next address
				next_address  = curr_address + bytes_per_pkt;

				// Compute the transfer size (use the full buffer unless you run out of space)
				if( next_address > ( start_address + size ) ) {
                    transfer_size = (start_address + size) - curr_address;

				} else {
					transfer_size = bytes_per_pkt;
				}

				// Set response args that change per packet
	            respArgs32[2]   = Xil_Htonl( curr_address );
                respArgs32[3]   = Xil_Htonl( transfer_size );

                // Unfortunately, due to the byte swapping that occurs in node_sendEarlyResp, we need to set all 
                //   three command parameters for each packet that is sent.
	            respHdr->cmd     = cmdHdr->cmd;
	            respHdr->length  = 16 + transfer_size;
				respHdr->numArgs = 4;

				// Transfer data
				num_bytes = event_log_get_data( curr_address, transfer_size, (char *) &respArgs32[4] );

#ifdef _DEBUG_
				xil_printf("Packet %8d: \n", i);
				xil_printf("    transfer_address = 0x%8x\n    transfer_size    = %10d\n    num_bytes        = %10d\n", curr_address, transfer_size, num_bytes);
#endif

				// Check that we copied everything
				if ( num_bytes == transfer_size ) {
					// Send the packet
					node_sendEarlyResp(respHdr, pktSrc, eth_dev_num);
				} else {
					xil_printf("ERROR:  NODE_GET_EVENTS tried to get %d bytes, but only received %d @ 0x%x \n", transfer_size, num_bytes, curr_address );
				}

				// Update our current address
				curr_address = next_address;
			}

			respSent = RESP_SENT;
		break;


		//---------------------------------------------------------------------
		// TODO:  THIS FUNCTION IS NOT COMPLETE
		case NODE_LOG_ADD_EVENT:
			xil_printf("EVENT LOG:  Add Event not supported\n");
	    break;


		//---------------------------------------------------------------------
		// TODO:  THIS FUNCTION IS NOT COMPLETE
		case NODE_LOG_ENABLE_EVENT:
			xil_printf("EVENT LOG:  Enable Event not supported\n");
	    break;


	    //---------------------------------------------------------------------
		case NODE_LOG_STREAM_ENTRIES:
			// Stream entries from the log
			//
			// Message format:
			//     cmdArgs32[0]   Enable = 1 / Disable = 0
			//     cmdArgs32[1]   IP Address (32 bits)
			//     cmdArgs32[2]   Host ID (upper 16 bits); Port (lower 16 bits)
			//
			temp       = Xil_Ntohl(cmdArgs32[0]);
			ip_address = Xil_Ntohl(cmdArgs32[1]);
			temp2      = Xil_Ntohl(cmdArgs32[2]);

			// Check the enable
			if ( temp == 0 ) {
				xil_printf("EVENT LOG:  Disable streaming to %08x (%d)\n", ip_address, (temp2 & 0xFFFF) );
				async_pkt_enable = temp;
			} else {
				xil_printf("EVENT LOG:  Enable streaming to %08x (%d)\n", ip_address, (temp2 & 0xFFFF) );

				// Initialize all of the global async packet variables
				async_pkt_enable = temp;

				async_pkt_dest.srcIPAddr = ip_address;
				async_pkt_dest.destPort  = (temp2 & 0xFFFF);

				async_pkt_hdr.destID     = ((temp2 >> 16) & 0xFFFF);
				async_pkt_hdr.srcID      = node_info.node;
				async_pkt_hdr.pktType    = PKTTPYE_NTOH_MSG_ASYNC;
				async_pkt_hdr.length     = PAYLOAD_PAD_NBYTES + 4;
				async_pkt_hdr.seqNum     = 0;
				async_pkt_hdr.flags      = 0;

				status = transport_config_socket( eth_dev_num, &sock_async, &addr_async, ((temp2 >> 16) & 0xFFFF));
				if (status == FAILURE) {
					xil_printf("Failed to configure socket.\n");
				}

				// Transmit the Node Info
				add_node_info_entry(WN_TRANSMIT);
			}

			// Send response
			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
        break;


	    //---------------------------------------------------------------------
		case NODE_ADD_STATS_TO_LOG:
			// Add the current statistics to the log
			// TODO:  Add parameter to command to transmit stats
			temp = add_all_txrx_statistics_to_log(WN_NO_TRANSMIT);

			xil_printf("EVENT LOG:  Added %d statistics.\n", temp);

			// Send response of oldest index
            respArgs32[respIndex++] = Xil_Htonl( temp );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
        break;


		//---------------------------------------------------------------------
		case NODE_GET_STATS:
            // NODE_GET_STATS Packet Format:
			//   - cmdArgs32[0 - 1]  - MAC Address (All 0xF means all stats)

			// Get MAC Address
        	wlan_exp_get_mac_addr(&((u32 *)cmdArgs32)[0], &mac_addr[0]);
        	id = wlan_exp_get_aid_from_ADDR(&mac_addr[0]);

        	// Local variables
        	txrx_stats_entry * stats_entry = (txrx_stats_entry *) &respArgs32[respIndex];
        	u32                entry_size = sizeof(txrx_stats_entry);
        	u32                stats_size = sizeof(statistics_txrx) - sizeof(dl_entry);

        	size = 0;

			// If parameter is not the magic number, then remove the LTG
			if ( id != NODE_CONFIG_ALL_ASSOCIATED ) {
				// Find the station info
				curr_station_info = (station_info*)(association_table.first);
				for(i=0; i < association_table.length; i++){
					if (curr_station_info->AID == id){ size = entry_size; break; }
					curr_station_info = (station_info*)((curr_station_info->entry).next);
				}

				if (size != 1) {
					stats_entry->timestamp = get_usec_timestamp();

					// Copy the statistics to the log entry
					//   NOTE:  This assumes that the statistics entry in wlan_mac_entries.h has a contiguous piece of memory
					//          equivalent to the statistics structure in wlan_mac_high.h (without the dl_entry)
					memcpy( (void *)(&stats_entry->last_timestamp), (void *)(&curr_station_info->stats->last_timestamp), stats_size );

					xil_printf("Getting Statistics for AID = %d \n", id);
                    // print_entry(0, ENTRY_TYPE_TXRX_STATS, stats_entry);

				} else {
					xil_printf("Could not find specified node:  AID = %d", id);
				}
			} else {
				xil_printf("Command not supported.");
			}

			respHdr->length += size;
			respHdr->numArgs = respIndex;
			break;


		//---------------------------------------------------------------------
		case NODE_RESET_STATS:
			xil_printf("Reseting Statistics\n");

			reset_station_statistics();
		break;


		//---------------------------------------------------------------------
		case NODE_QUEUE_TX_DATA_PURGE_ALL:
			xil_printf("Purging All Data Transmit Queues\n");

			purge_all_data_tx_queue();
		break;


		// Case NODE_CONFIG_DEMO is implemented in the child classes


        //---------------------------------------------------------------------
		default:
			// Call standard function in child class to parse parameters implmented there
			respSent = node_process_callback( cmdID, cmdHdr, cmdArgs, respHdr, respArgs, pktSrc, eth_dev_num);
		break;
	}

	return respSent;
}



/*****************************************************************************/
/**
* This will initialize the WARPNet WLAN_EXP node with the appropriate information
* and set up the node to communicate with WARPNet on the given ethernet device. 
*
* @param    None.
*
* @return	 0 - Success
*           -1 - Failure
*
* @note		This function will print to the terminal but is not able to control any of the LEDs
*
******************************************************************************/
int wlan_exp_node_init( u32 type, u32 serial_number, u32 *fpga_dna, u32 eth_dev_num, u8 *hw_addr ) {

    int i;
	int status = SUCCESS;

	xil_printf("WARPNet WLAN EXP v%d.%d.%d (compiled %s %s)\n", WARPNET_VER_MAJOR, WARPNET_VER_MINOR, WARPNET_VER_REV, __DATE__, __TIME__);

	// Initialize Global variables
	//   Node must be configured using the WARPNet nodesConfig
	//   HW must be WARP v3
	//   IP Address should be NODE_IP_ADDR_BASE
    node_info.type            = type;
    node_info.node            = 0xFFFF;
    node_info.hw_generation   = WARP_HW_VERSION;
    node_info.design_ver      = (WARPNET_VER_MAJOR<<16)|(WARPNET_VER_MINOR<<8)|(WARPNET_VER_REV);

    node_info.serial_number   = serial_number;
    
    for( i = 0; i < FPGA_DNA_LEN; i++ ) {
        node_info.fpga_dna[i] = fpga_dna[i];
    }
    
    // WLAN Exp Parameters are assumed to be initialize already
    //    node_info.wlan_hw_addr
    //    node_info.wlan_max_assn
    //    node_info.wlan_event_log_size
    //    node_info.wlan_max_stats

    node_info.eth_device      = eth_dev_num;
    
	node_info.ip_addr[0]      = (NODE_IP_ADDR_BASE >> 24) & 0xFF;
	node_info.ip_addr[1]      = (NODE_IP_ADDR_BASE >> 16) & 0xFF;
	node_info.ip_addr[2]      = (NODE_IP_ADDR_BASE >>  8) & 0xFF;
	node_info.ip_addr[3]      = (NODE_IP_ADDR_BASE      ) & 0xFF;  // IP ADDR = w.x.y.z
    
    for ( i = 0; i < ETH_ADDR_LEN; i++ ) {
        node_info.hw_addr[i]  = hw_addr[i];
    }
    
    node_info.unicast_port    = NODE_UDP_UNICAST_PORT_BASE;
    node_info.broadcast_port  = NODE_UDP_MCAST_BASE;


    // Set up callback for process function
    node_process_callback     = (wn_function_ptr_t)wlan_exp_null_process_callback;

    // Initialize the System Monitor
    node_init_system_monitor();
    
    // Initialize Tag parameters
    node_init_parameters( (u32*)&node_info );
    

#ifdef _DEBUG_
    print_wn_node_info( &node_info );
    print_wn_parameters( (wn_tag_parameter *)&node_parameters, NODE_MAX_PARAMETER );
#endif


    // Initialize Global variables for async packet sending
    async_pkt_enable = 0;
    async_eth_dev_num = eth_dev_num;
    bzero((void *)&async_pkt_dest, sizeof(pktSrcInfo));
    bzero((void *)&async_pkt_hdr, sizeof(wn_transport_header));

    
    // Transport initialization
	//   NOTE:  These errors are fatal and status error will be displayed
	//       on the hex display.  Also, please attach a USB cable for
	//       terminal debug messages.
	status = transport_init(node_info.node, node_info.ip_addr, node_info.hw_addr, node_info.unicast_port, node_info.broadcast_port, node_info.eth_device);
	if(status != 0) {
        xil_printf("  Error in transport_init()! Exiting...\n");
        return FAILURE;
	}

#ifdef 	WLAN_EXP_WAIT_FOR_ETH

	xil_printf("  Waiting for Ethernet link ... ");
	while( transport_linkStatus( eth_dev_num ) != 0 );
	xil_printf("  Initialization Successful\n");

#else

	xil_printf("  Not waiting for Ethernet link.  Current status is: ");

	if ( transport_linkStatus( eth_dev_num ) == LINK_READY ) {
		xil_printf("ready.\n");
	} else {
		xil_printf("not ready.\n");
		xil_printf("    Make sure link is ready before using WARPNet.\n");
	}

#endif

	//Assign the new packet callback
	// IMPORTANT: must be called after transport_init()
	transport_setReceiveCallback( (void *)node_rxFromTransport );

	// If you are in configure over network mode, then indicate that to the user
	if ( node_info.node == 0xFFFF ) {
		xil_printf("  !!! Waiting for Network Configuration !!! \n");
	}

	xil_printf("End WARPNet WLAN Exp initialization\n");
	return status;
}



/*****************************************************************************/
/**
* Set the node process callback
*
* @param    Pointer to the callback
*
* @return	None.
*
* @note     None.
*
******************************************************************************/
void node_set_process_callback(void(*callback)()){
	node_process_callback = (wn_function_ptr_t)callback;
}



/*****************************************************************************/
/**
* Initialize the System Monitor if it exists
*
* @param    None
*
* @return	None
*
* @note     None
*
******************************************************************************/
void node_init_system_monitor(void) {

#ifdef XPAR_XSYSMON_NUM_INSTANCES
	u32 RegValue;

    // Reset the system monitor
    XSysMon_WriteReg(SYSMON_BASEADDR, XSM_SRR_OFFSET, XSM_SRR_IPRST_MASK);

    // Disable the Channel Sequencer before configuring the Sequence registers.
    RegValue = XSysMon_ReadReg(SYSMON_BASEADDR, XSM_CFR1_OFFSET) & (~ XSM_CFR1_SEQ_VALID_MASK);
    XSysMon_WriteReg(SYSMON_BASEADDR, XSM_CFR1_OFFSET,	RegValue | XSM_CFR1_SEQ_SINGCHAN_MASK);

    // Setup the Averaging to be done for the channels in the Configuration 0
    //   register as 16 samples:
    RegValue = XSysMon_ReadReg(SYSMON_BASEADDR, XSM_CFR0_OFFSET) & (~XSM_CFR0_AVG_VALID_MASK);
    XSysMon_WriteReg(SYSMON_BASEADDR, XSM_CFR0_OFFSET, RegValue | XSM_CFR0_AVG16_MASK);

    // Enable the averaging on the following channels in the Sequencer registers:
    //  - On-chip Temperature
    //  - On-chip VCCAUX supply sensor
    XSysMon_WriteReg(SYSMON_BASEADDR,XSM_SEQ02_OFFSET, XSM_SEQ_CH_TEMP | XSM_SEQ_CH_VCCAUX);

    // Enable the following channels in the Sequencer registers:
    //  - On-chip Temperature
    //  - On-chip VCCAUX supply sensor
    XSysMon_WriteReg(SYSMON_BASEADDR, XSM_SEQ00_OFFSET, XSM_SEQ_CH_TEMP | XSM_SEQ_CH_VCCAUX);

    // Set the ADCCLK frequency equal to 1/32 of System clock for the System Monitor/ADC
    //   in the Configuration Register 2.
    XSysMon_WriteReg(SYSMON_BASEADDR, XSM_CFR2_OFFSET, 32 << XSM_CFR2_CD_SHIFT);

    // Enable the Channel Sequencer in continuous sequencer cycling mode.
    RegValue = XSysMon_ReadReg(SYSMON_BASEADDR, XSM_CFR1_OFFSET) & (~ XSM_CFR1_SEQ_VALID_MASK);
    XSysMon_WriteReg(SYSMON_BASEADDR, XSM_CFR1_OFFSET,	RegValue | XSM_CFR1_SEQ_CONTINPASS_MASK);

    // Wait till the End of Sequence occurs
    XSysMon_ReadReg(SYSMON_BASEADDR, XSM_SR_OFFSET); /* Clear the old status */
    while (((XSysMon_ReadReg(SYSMON_BASEADDR, XSM_SR_OFFSET)) & XSM_SR_EOS_MASK) != XSM_SR_EOS_MASK);

    // TODO:  Do we need a timeout for this while loop?

#endif

}



/*****************************************************************************/
/**
* Initialize the TAG parameters structure
*
* @param    Pointer to info structure from which to pull all the tag parameter values
*
* @return	Total number of bytes of the TAG parameter structure
*
* @note     Please make sure that the *_info structure and the parameter values
*           maintain the same order
*
******************************************************************************/
int node_init_parameters( u32 *info ) {

	int              i;
	int              length;
	int              size;
	wn_tag_parameter temp_param;

    unsigned int       num_params = NODE_MAX_PARAMETER;
    wn_tag_parameter * parameters = (wn_tag_parameter *)&node_parameters;

	// Initialize variables
	length = 0;
	size   = sizeof(wn_tag_parameter);

    for( i = 0; i < num_params; i++ ) {

    	// Set reserved space to 0xFF
    	temp_param.reserved = 0xFF;

    	// Common parameter settings
    	temp_param.group    = NODE_GRP;
    	temp_param.command  = i;

    	// Any parameter specific code
    	switch ( i ) {
            case NODE_FPGA_DNA:
            case NODE_WLAN_MAC_ADDR:
    		    temp_param.length = 2;
    		break;

            default:
            	temp_param.length = 1;
    	    break;
    	}

    	// Set pointer to parameter values in info structure
        temp_param.value = &info[length];

        // Increment length so that we get the correct index in to info structure
        length += temp_param.length;

        // Copy the temp parameter to the tag parameter array
        memcpy( &parameters[i], &temp_param, size );
    }

    return ( ( size * i ) + ( length * 4 ) ) ;
}



/*****************************************************************************/
/**
*
* This function will populate a buffer with tag parameter information
*
* @param    eth_dev_num is an int that specifies the Ethernet interface to use
*           buffer is a u32 pointer to store the tag parameter information
*           max_words is a integer to specify the max number of u32 words in the buffer
*
* @return	number_of_words is number of words used of the buffer for the tag
*             parameter information
*
* @note		The tag parameters must be initialized before this function will be
*       called.  The user can modify the file to add additional functionality
*       to the WARPNet Transport.
*
******************************************************************************/
int node_get_parameters(u32 * buffer, unsigned int max_words, u8 transmit) {

    int i, j;
    int num_total_words;
    int num_param_words;

    u32 length;
    u32 temp_word;

    // NOTE:  This code is mostly portable between WARPNet components.
    //        Please modify  if you are copying this function for other WARPNet extensions    
    unsigned int       num_params = NODE_MAX_PARAMETER;
    wn_tag_parameter * parameters = (wn_tag_parameter *) &node_parameters;
    
    
    // Initialize the total number of words used
    num_total_words = 0;
    
    // Iterate through all tag parameters
    for( i = 0; i < num_params; i++ ) {

        length = parameters[i].length;
    
        // The number of words in a tag parameter is the number of value words + 2 header words
        num_param_words = length + 2;
    
        // Make sure we have space in the buffer to put the parameter
        if ( ( num_total_words + num_param_words ) <= max_words ) {
    
            temp_word = ( ( parameters[i].reserved << 24 ) | 
                          ( parameters[i].group    << 16 ) |
                          ( length                       ) );
            
            if ( transmit == WN_TRANSMIT ) {

				buffer[num_total_words]     = Xil_Htonl( temp_word );
				buffer[num_total_words + 1] = Xil_Htonl( parameters[i].command );

				for( j = 0; j < length; j++ ) {
					buffer[num_total_words + 2 + j] = Xil_Htonl( parameters[i].value[j] );
				}

            } else {
            
				buffer[num_total_words]     = temp_word;
				buffer[num_total_words + 1] = parameters[i].command;

				for( j = 0; j < length; j++ ) {
					buffer[num_total_words + 2 + j] = parameters[i].value[j];
				}
            }

            num_total_words += num_param_words;
            
        } else {
            // Exit the loop because there is no more space
            break;
        }
    }
    
    return num_total_words;
}



/*****************************************************************************/
/**
* This function will populate a buffer with tag parameter values
*
* @param    buffer is a u32 pointer to store the tag parameter information
*           max_words is a integer to specify the max number of u32 words in the buffer
*
* @return	number_of_words is number of words used of the buffer for the tag
*             parameter information
*
* @note		The tag parameters must be initialized before this function will be
*       called.  The user can modify the file to add additional functionality
*       to the WARPNet Transport.
*
******************************************************************************/
int node_get_parameter_values(u32 * buffer, unsigned int max_words) {

    int i, j;
    int num_total_words;

    u32 length;

    // NOTE:  This code is mostly portable between WARPNet components.
    //        Please modify  if you are copying this function for other WARPNet extensions
    unsigned int       num_params = NODE_MAX_PARAMETER;
    wn_tag_parameter * parameters = (wn_tag_parameter *) &node_parameters;

    // Initialize the total number of words used
    num_total_words = 0;

    // Iterate through all tag parameters
    for( i = 0; i < num_params; i++ ) {

        length = parameters[i].length;

        // Make sure we have space in the buffer to put the parameter
        if ( ( num_total_words + length ) <= max_words ) {

			for( j = 0; j < length; j++ ) {
				buffer[num_total_words + j] = parameters[i].value[j];
			}

            num_total_words += length;

        } else {
            // Exit the loop because there is no more space
            break;
        }
    }

    return num_total_words;
}



/*****************************************************************************/
/**
* These are helper functions to set some node_info fields
*
* @param    field value
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void node_info_set_wlan_hw_addr  ( u8 * hw_addr  ) {
    node_info.wlan_hw_addr[0] = (hw_addr[0]<<8)  |  hw_addr[1];
    node_info.wlan_hw_addr[1] = (hw_addr[2]<<24) | (hw_addr[3]<<16) | (hw_addr[4]<<8) | hw_addr[5];
}

void node_info_set_max_assn      ( u32 max_assn  ) { node_info.wlan_max_assn       = max_assn;  }
void node_info_set_event_log_size( u32 log_size  ) { node_info.wlan_event_log_size = log_size;  }
void node_info_set_max_stats     ( u32 max_stats ) { node_info.wlan_max_stats      = max_stats; }



/*****************************************************************************/
/**
* These are helper functions to get some fields
*
* @param    None.
*
* @return	field value
*
* @note		None.
*
******************************************************************************/
u32  wn_get_node_id       ( void ) { return node_info.node; }
u32  wn_get_serial_number ( void ) { return node_info.serial_number; }

#ifdef XPAR_XSYSMON_NUM_INSTANCES
u32  wn_get_curr_temp     ( void ) { return XSysMon_ReadReg(SYSMON_BASEADDR, XSM_TEMP_OFFSET);     }
u32  wn_get_min_temp      ( void ) { return XSysMon_ReadReg(SYSMON_BASEADDR, XSM_MIN_TEMP_OFFSET); }
u32  wn_get_max_temp      ( void ) { return XSysMon_ReadReg(SYSMON_BASEADDR, XSM_MAX_TEMP_OFFSET); }
#else
u32  wn_get_curr_temp     ( void ) { return 0; }
u32  wn_get_min_temp      ( void ) { return 0; }
u32  wn_get_max_temp      ( void ) { return 0; }
#endif

/*****************************************************************************/
/**
* This is a helper function to clean up the LTGs owned by WLAN Exp
*
* @param    id            - LTG id
*           callback_arg  - Callback argument for LTG
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void node_ltg_cleanup(u32 id, void* callback_arg){
	wlan_mac_high_free( callback_arg );
}






#ifdef _DEBUG_

/*****************************************************************************/
/**
* Print Tag Parameters
*
* This function will print a list of wn_tag_parameter structures
*
* @param    param      - pointer to the wn_tag_parameter list
*           num_params - number of wn_tag_parameter structures in the list
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void print_wn_parameters( wn_tag_parameter *param, int num_params ) {

	int i, j;

	xil_printf("Node Parameters: \n");

    for( i = 0; i < num_params; i++ ){
    	xil_printf("  Parameter %d:\n", i);
    	xil_printf("    Group:            %d \n",   param[i].group);
    	xil_printf("    Length:           %d \n",   param[i].length);
    	xil_printf("    Command:          %d \n",   param[i].command);

    	for( j = 0; j < param[i].length; j++ ) {
    		xil_printf("    Value[%2d]:        0x%8x \n",   j, param[i].value[j]);
    	}
    }

    xil_printf("\n");
}



/*****************************************************************************/
/**
* Print Node Info
*
* This function will print a wn_node_info structure
*
* @param    info    - pointer to wn_node_info structure to print
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void print_wn_node_info( wn_node_info * info ) {
    int i;

	xil_printf("WARPNet Node Information: \n");
    xil_printf("  WARPNet Type:       0x%8x \n",   info->type);    
    xil_printf("  Node ID:            %d \n",      info->node);
    xil_printf("  HW Generation:      %d \n",      info->hw_generation);
    xil_printf("  HW Design Version:  0x%x \n",    info->design_ver);
    
    xil_printf("  Serial Number:      0x%x \n",    info->serial_number);
    xil_printf("  FPGA DNA:           ");
    
    for( i = 0; i < FPGA_DNA_LEN; i++ ) {
        xil_printf("0x%8x  ", info->fpga_dna[i]);
    }
	xil_printf("\n");
        
    xil_printf("  HW Address:         %02x",       info->hw_addr[0]);
                                              
    for( i = 1; i < ETH_ADDR_LEN; i++ ) {
        xil_printf(":%02x", info->hw_addr[i]);
    }
	xil_printf("\n");
                                                                                            
    xil_printf("  IP Address 0:       %d",         info->ip_addr[0]);
    
    for( i = 1; i < IP_VERSION; i++ ) {
        xil_printf(".%d", info->ip_addr[i]);
    }
	xil_printf("\n");

    xil_printf("  Unicast Port:       %d \n",      info->unicast_port);
    xil_printf("  Broadcast Port:     %d \n",      info->broadcast_port);
	xil_printf("\n");
    
}

#endif


// End USE_WARPNET_WLAN_EXP
#endif
