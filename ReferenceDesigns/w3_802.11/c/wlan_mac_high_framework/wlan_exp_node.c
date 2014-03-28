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


// Declared in wlan_mac_high.c
extern u8                  promiscuous_stats_enabled;


// Declared in each of the AP / STA
extern tx_params           default_unicast_data_tx_params;

extern dl_list		       association_table;


/*************************** Variable Definitions ****************************/

wn_node_info          node_info;
wn_tag_parameter      node_parameters[NODE_MAX_PARAMETER];

wn_function_ptr_t     node_process_callback;
extern function_ptr_t check_queue_callback;

u32                   async_pkt_enable;
u32                   async_eth_dev_num;
pktSrcInfo            async_pkt_dest;
wn_transport_header   async_pkt_hdr;

u32                   wlan_exp_enable_logging = 0;


/*************************** Functions Prototypes ****************************/

void node_init_system_monitor(void);
int  node_init_parameters( u32 *info );
int  node_processCmd(const wn_cmdHdr* cmdHdr,const void* cmdArgs, wn_respHdr* respHdr,void* respArgs, void* pktSrc, unsigned int eth_dev_num);

void node_ltg_cleanup(u32 id, void* callback_arg);

void create_wn_cmd_log_entry(wn_cmdHdr* cmdHdr, void * cmdArgs, u16 src_id);

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
void node_rxFromTransport(wn_host_message* toNode, wn_host_message* fromNode,
		                  void* pktSrc, u16 src_id, unsigned int eth_dev_num){
	unsigned char cmd_grp;

	unsigned int respSent = RESP_SENT;

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

	// Create a log entry if logging is enabled
	if (wlan_exp_enable_logging == 1) {
		create_wn_cmd_log_entry(cmdHdr, cmdArgs, src_id);
	}

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

    unsigned int  temp, temp2, i, j;

    // Variables for functions
    u32           id;
    u32           flags;
    u32           serial_number;
	u32           start_index;
	u32           curr_index;
	u32           next_index;
	u32           bytes_remaining;
	u32           ip_address;
	u32           size;
	u32           evt_log_size;
	u32           transfer_size;
	u32           bytes_per_pkt;
	u32           num_bytes;
	u32           num_pkts;
	u64           time;
	u64           new_time;
	u64           abs_time;

	u32           entry_size;
	u32           entry_remaining;
	u32           total_entries;
	u32           entry_per_pkt;
	u32           transfer_entry_num;

	u8            mac_addr[6];

	dl_list     * curr_list;
	dl_entry	* curr_entry;
	station_info* curr_station_info;

	int           power;


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


		//---------------------------------------------------------------------
		case NODE_GET_STATION_INFO:
            // NODE_GET_STATION_INFO Packet Format:
			//   - cmdArgs32[0]   - buffer id
			//   - cmdArgs32[1]   - flags
            //   - cmdArgs32[2]   - start_address of transfer
			//   - cmdArgs32[3]   - size of transfer (in bytes)
			//   - cmdArgs32[4:5] - MAC Address (All 0xFF means all station info)
			//
			// Always returns a valid WARPNet Buffer (either 1 or more packets)
            //   - buffer_id       - uint32  - buffer_id
			//   - flags           - uint32  - 0
			//   - bytes_remaining - uint32  - Number of bytes remaining in the transfer
			//   - start_byte      - uint32  - Byte index of the first byte in this packet
			//   - size            - uint32  - Number of payload bytes in this packet
			//   - byte[]          - uint8[] - Array of payload bytes

			xil_printf("Get Station Info\n");

			// Get MAC Address
        	wlan_exp_get_mac_addr(&((u32 *)cmdArgs32)[4], &mac_addr[0]);
        	id = wlan_exp_get_aid_from_ADDR(&mac_addr[0]);

        	// Local variables
        	station_info_entry * info_entry;
			u32                  station_info_size = sizeof(station_info_base);

        	entry_size = sizeof(station_info_entry);

            // Initialize constant return values
        	respIndex     = 5;              // There will always be 5 return args
            respArgs32[0] = cmdArgs32[0];
            respArgs32[1] = 0;

            if ( id == 0 ) {
				// If we cannot find the MAC address, print a warning and return an empty buffer
				xil_printf("WARNING:  Could not find specified node: %02x", mac_addr[0]);
				for ( i = 1; i < ETH_ADDR_LEN; i++ ) { xil_printf(":%02x", mac_addr[i] ); } xil_printf("\n");

				respArgs32[2]    = 0;
				respArgs32[3]    = 0;
				respArgs32[4]    = 0;

            } else {
				// If parameter is not the magic number to return all Station Info structures
				if ( id != NODE_CONFIG_ALL_ASSOCIATED ) {
					// Find the station_info entry
					curr_entry = wlan_mac_high_find_station_info_ADDR( get_station_info_list(), &mac_addr[0]);

					if (curr_entry != NULL) {
						curr_station_info = (station_info*)(curr_entry->data);
						info_entry        = (station_info_entry *) &respArgs32[respIndex];

						info_entry->timestamp = get_usec_timestamp();

						// Copy the station info to the log entry
						//   NOTE:  This assumes that the station info entry in wlan_mac_entries.h has a contiguous piece of memory
						//          similar to the station info and tx params structures in wlan_mac_high.h
						memcpy( (void *)(&info_entry->info), (void *)(&curr_station_info), station_info_size );

						xil_printf("Getting Station Entry for node: %02x", mac_addr[0]);
						for ( i = 1; i < ETH_ADDR_LEN; i++ ) { xil_printf(":%02x", mac_addr[i] ); } xil_printf("\n");

						// Set the return args and increment the size
						respArgs32[2]    = Xil_Htonl( entry_size );
						respArgs32[3]    = 0;
						respArgs32[4]    = Xil_Htonl( entry_size );
						respHdr->length += entry_size;
					} else {
						// If we cannot find the MAC address, print a warning and return an empty buffer
						xil_printf("WARNING:  Could not find specified node: %02x", mac_addr[0]);
						for ( i = 1; i < ETH_ADDR_LEN; i++ ) { xil_printf(":%02x", mac_addr[i] ); } xil_printf("\n");

						respArgs32[2]    = 0;
						respArgs32[3]    = 0;
						respArgs32[4]    = 0;
					}
				} else {
					// Create a WARPNet buffer response to send all station_info entries

					// Get the list of TXRX Statistics
					curr_list      = get_station_info_list();
					total_entries  = curr_list->length;
					size           = entry_size * total_entries;

					if ( size != 0 ) {
						// Send the station_info as a series of WARPNet Buffers

						// Set loop variables
						entry_per_pkt     = (max_words * 4) / entry_size;
						bytes_per_pkt     = entry_per_pkt * entry_size;
						num_pkts          = size / bytes_per_pkt + 1;
						if ( (size % bytes_per_pkt) == 0 ){ num_pkts--; }    // Subtract the extra pkt if the division had no remainder

						entry_remaining   = total_entries;
						bytes_remaining   = size;
						curr_index        = 0;
						curr_entry        = curr_list->first;
						curr_station_info = (station_info*)(curr_entry->data);
						time              = get_usec_timestamp();

						// Iterate through all the packets
						for( i = 0; i < num_pkts; i++ ) {

							// Get the next index
							next_index  = curr_index + bytes_per_pkt;

							// Compute the transfer size (use the full buffer unless you run out of space)
							if( next_index > size ) {
								transfer_size = size - curr_index;
							} else {
								transfer_size = bytes_per_pkt;
							}

							if( entry_remaining < entry_per_pkt) {
								transfer_entry_num = entry_remaining;
							} else {
								transfer_entry_num = entry_per_pkt;
							}

							// Set response args that change per packet
							respArgs32[2]    = Xil_Htonl( bytes_remaining );
							respArgs32[3]    = Xil_Htonl( curr_index );
							respArgs32[4]    = Xil_Htonl( transfer_size );

							// Unfortunately, due to the byte swapping that occurs in node_sendEarlyResp, we need to set all
							//   three command parameters for each packet that is sent.
							respHdr->cmd     = cmdHdr->cmd;
							respHdr->length  = 20 + transfer_size;
							respHdr->numArgs = 5;

							// Transfer data
							info_entry = (station_info_entry *) &respArgs32[respIndex];

							for( j = 0; j < entry_remaining; j++ ){
								// Set the timestamp for the station_info entry
								info_entry->timestamp = time;

								// Copy the station info to the log entry
								//   NOTE:  This assumes that the station info entry in wlan_mac_entries.h has a contiguous piece of memory
								//          similar to the station info and tx params structures in wlan_mac_high.h
								memcpy( (void *)(&info_entry->info), (void *)(&curr_station_info), station_info_size );

								// Increment the pointers
								curr_entry         = dl_entry_next(curr_entry);
								curr_station_info  = (station_info*)(curr_entry->data);
								info_entry         = (station_info_entry *)(((void *)info_entry) + entry_size );
							}

							// Send the packet
							node_sendEarlyResp(respHdr, pktSrc, eth_dev_num);

							// Update our current address and bytes remaining
							curr_index       = next_index;
							bytes_remaining -= transfer_size;
							entry_remaining -= entry_per_pkt;
						}

						respSent = RESP_SENT;
					} else {
						// Set empty response args
						respArgs32[2]   = 0;
						respArgs32[3]   = 0;
						respArgs32[4]   = 0;
					}
				}
            }

			// Set the length and number of response args
			respHdr->length += (5 * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;


		// Case NODE_SET_STATION_INFO  is implemented in the child classes

		// Case NODE_DISASSOCIATE      is implemented in the child classes


	    //---------------------------------------------------------------------
		case NODE_RESET_STATE:
            // NODE_RESET_STATE Packet Format:
			//   - cmdArgs32[0]  - Flags
			//                     [0] - NODE_RESET_LOG
			//                     [1] - NODE_RESET_TXRX_STATS
			temp   = Xil_Ntohl(cmdArgs32[0]);
			status = 0;

			// Disable interrupts so no packets interrupt the reset
			wlan_mac_high_interrupt_stop();
			// Configure the LOG based on the flag bits
			if ( ( temp & NODE_RESET_LOG ) == NODE_RESET_LOG ) {
				xil_printf("EVENT LOG:  Reset log\n");
				event_log_reset();
			}

			if ( ( temp & NODE_RESET_TXRX_STATS ) == NODE_RESET_TXRX_STATS ) {
				xil_printf("Reseting Statistics\n");
				reset_station_statistics();
			}

			if ( ( temp & NODE_RESET_LTG ) == NODE_RESET_LTG ) {
				status = ltg_sched_remove( LTG_REMOVE_ALL );

				if ( status != 0 ) {
					xil_printf("WARNING:  LTG - Failed to remove all LTGs.\n");
					status = NODE_LTG_ERROR;
				} else {
					xil_printf("Removing All LTGs.\n");
				}
			}

			if ( ( temp & NODE_TX_DATA_QUEUE ) == NODE_TX_DATA_QUEUE ) {
				xil_printf("Purging All Data Transmit Queues\n");
				purge_all_data_tx_queue();
			}

			// Re-enable interrupts
			wlan_mac_high_interrupt_start();

			// Send response of success
            respArgs32[respIndex++] = Xil_Htonl( status );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;


	    //---------------------------------------------------------------------
		case NODE_TX_POWER:
            // NODE_TX_POWER Packet Format:
			//   - cmdArgs32[0]  - Power (shifted by TX_POWER_MIN_DBM)
			temp = Xil_Ntohl(cmdArgs32[0]);

			// If parameter is not the magic number, then set the TX power
			if ( temp != NODE_TX_POWER_RSVD_VAL ) {

				power = temp + TX_POWER_MIN_DBM;

				// Check that the power is within the specified bounds
		        if ((power >= TX_POWER_MIN_DBM) && (power <= TX_POWER_MAX_DBM)){

				    xil_printf("Setting TX power = %d\n", power);

		        	// Set the default power for new associations

		        	// Update the Tx power in each current association

		        	// Set the multicast power

		        	// Send IPC to CPU low to set the Tx power for control frames

		        } else {
					// Get default power for new associations
					power = temp + TX_POWER_MIN_DBM;
		        }
			} else {
				// Get default power for new associations
				power = temp + TX_POWER_MIN_DBM;
			}

			// Shift the return value so that we do not transmit negative numbers
			temp = power - TX_POWER_MIN_DBM;

			// Send response of current power
            respArgs32[respIndex++] = Xil_Htonl( temp );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;


	    //---------------------------------------------------------------------
		case NODE_TX_RATE:
            // NODE_TX_RATE Packet Format:
			//   - cmdArgs32[0 - 1]  - MAC Address (All 0xF means all nodes)
			//   - cmdArgs32[2]      - Type
			//   - cmdArgs32[3]      - Rate

			// TODO:  THIS NEEDS TO BE UPDATED FOR TYPE WHEN IMPLEMENTED IN FRAMEWORK

			// Get MAC Address
        	wlan_exp_get_mac_addr(&((u32 *)cmdArgs32)[0], &mac_addr[0]);
        	id = wlan_exp_get_aid_from_ADDR(&mac_addr[0]);

			// Local variables
			u32  rate;

        	// Get node TX rate and adjust so that it is a legal value
			rate = Xil_Ntohl(cmdArgs32[3]);

			if(rate < WLAN_MAC_RATE_6M ){ rate = WLAN_MAC_RATE_6M;  }
			if(rate > WLAN_MAC_RATE_54M){ rate = WLAN_MAC_RATE_54M; }

			curr_list = get_station_info_list();

			// If the ID is not for all nodes, configure the node
			if ( id != NODE_CONFIG_ALL_ASSOCIATED ) {
				// Set the rate of the station

				curr_entry = curr_list->first;

				for(i=0; i < curr_list->length; i++){
					curr_station_info = (station_info*)(curr_entry->data);
					if (curr_station_info->AID == id){
						// If parameter is not the magic number, then set the TX rate
						if ( rate != NODE_TX_RATE_RSVD_VAL ) {
							curr_station_info->tx.phy.rate = rate;
							xil_printf("Setting TX rate on AID %d = %d Mbps\n", id, wlan_lib_mac_rate_to_mbps(rate));
						} else {
							rate = curr_station_info->tx.phy.rate;
						}
						break;
					}
					curr_entry = dl_entry_next(curr_entry);
				}
			} else {
				// If parameter is not the magic number, then set the TX rate
				if ( rate != NODE_TX_RATE_RSVD_VAL ) {
					// Set the rate of all stations
					default_unicast_data_tx_params.phy.rate = rate;
					curr_entry = curr_list->first;

					for(i=0; i < curr_list->length; i++){
						curr_station_info = (station_info*)(curr_entry->data);
						curr_station_info->tx.phy.rate = default_unicast_data_tx_params.phy.rate;
						curr_entry        = dl_entry_next(curr_entry);
					}

					xil_printf("Setting Default TX rate = %d Mbps\n", wlan_lib_mac_rate_to_mbps(default_unicast_data_tx_params.phy.rate));
				} else {
					// Get the default rate
					rate = default_unicast_data_tx_params.phy.rate;
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
			//     cmdArgs32[0]   Read (NODE_TIME_RSVD_VAL) / Write (0)
			//     cmdArgs32[1]   New Time in microseconds - lower 32 bits (or NODE_TIME_RSVD_VAL)
			//     cmdArgs32[2]   New Time in microseconds - upper 32 bits (or NODE_TIME_RSVD_VAL)
			//     cmdArgs32[3]   Abs Time in microseconds - lower 32 bits (or NODE_TIME_RSVD_VAL)
			//     cmdArgs32[4]   Abs Time in microseconds - upper 32 bits (or NODE_TIME_RSVD_VAL)
			//
			// Response format:
            //     respArgs32[0]  Time on node in microseconds - lower 32 bits
			//     respArgs32[1]  Time on node in microseconds - upper 32 bits
			//
			temp  = Xil_Ntohl(cmdArgs32[0]);
			time  = get_usec_timestamp();

			time_info_entry * time_entry;

			// If parameter is not the magic number, then set the time on the node
			if ( temp != NODE_TIME_RSVD_VAL ) {

				// Get the new time
				temp     = Xil_Ntohl(cmdArgs32[1]);
				temp2    = Xil_Ntohl(cmdArgs32[2]);
				new_time = (((u64)temp2)<<32) + ((u64)temp);

				// If the time is not the reserved value; then update the time
				// Otherwise, get the current time to return to the host
				if ( (temp != NODE_TIME_RSVD_VAL) && (temp2 != NODE_TIME_RSVD_VAL) ) {
  				    wlan_mac_high_set_timestamp( new_time );
  				    xil_printf("WARPNET:  Setting time = 0x%08x 0x%08x\n", temp2, temp);
				} else {
					new_time = time;
				}

				// Get the absolute time
				temp     = Xil_Ntohl(cmdArgs32[3]);
				temp2    = Xil_Ntohl(cmdArgs32[4]);
				abs_time = (((u64)temp2)<<32) + ((u64)temp);

				// Create a time info log entry
				time_entry = get_next_empty_time_info_entry();

				if (time_entry != NULL) {
				    time_entry->timestamp = time;
				    time_entry->new_time  = new_time;
				    time_entry->abs_time  = abs_time;
				    time_entry->reason    = TIME_INFO_ENTRY_WN_SET_TIME;
				}
			} else {
				new_time = time;
			}

			temp  = new_time & 0xFFFFFFFF;
			temp2 = (new_time >> 32) & 0xFFFFFFFF;

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

        	status = NODE_LTG_ERROR;

        	if ((id != NODE_CONFIG_ALL_ASSOCIATED) || (id != 0)){
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
				}
        	} else {
				xil_printf("ERROR:  LTG ID = 0x%x is not usable for LTG_CONFIG\n", id);
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
        	status = NODE_LTG_ERROR;

        	if ( id != 0 ){
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
        	} else {
				xil_printf("ERROR:  Could not find ID for MAC address.\n");
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
			status = NODE_LTG_ERROR;

        	if ( id != 0 ){
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
        	} else {
				xil_printf("ERROR:  Could not find ID for MAC address.\n");
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
			status = NODE_LTG_ERROR;

        	if ( id != 0 ){
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
        	} else {
				xil_printf("ERROR:  Could not find ID for MAC address.\n");
        	}

			// Send response of status
            respArgs32[respIndex++] = Xil_Htonl( status );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;


	    //---------------------------------------------------------------------
		case NODE_LOG_CONFIG:
            // NODE_LOG_CONFIG Packet Format:
			//   - cmdArgs32[0]  - flags
			//                     [ 0] - Logging Enabled = 1; Logging Disabled = 0;
			//                     [ 1] - Wrap = 1; No Wrap = 0;
			//                     [ 2] - Full Payloads Enabled = 1; Full Payloads Disabled = 0;
			//                     [ 3] - Log WN Cmds Enabled = 1; Log WN Cmds Disabled = 0;
			//   - cmdArgs32[0]  - mask for flags
			//
            //   - respArgs32[0] - 0           - Success
			//                     0xFFFF_FFFF - Failure

			// Set the return value
			status = 0;

			// Get flags
			temp  = Xil_Ntohl(cmdArgs32[0]);
			temp2 = Xil_Ntohl(cmdArgs32[1]);

			xil_printf("EVENT LOG:  Configure flags = 0x%08x  mask = 0x%08x\n", temp, temp2);

			// Configure the LOG based on the flag bit / mask
			if ( ( temp2 & NODE_LOG_CONFIG_FLAG_LOGGING ) == NODE_LOG_CONFIG_FLAG_LOGGING ) {
				if ( ( temp & NODE_LOG_CONFIG_FLAG_LOGGING ) == NODE_LOG_CONFIG_FLAG_LOGGING ) {
					event_log_config_logging( EVENT_LOG_LOGGING_ENABLE );
				} else {
					event_log_config_logging( EVENT_LOG_LOGGING_DISABLE );
				}
			}

			if ( ( temp2 & NODE_LOG_CONFIG_FLAG_WRAP ) == NODE_LOG_CONFIG_FLAG_WRAP ) {
				if ( ( temp & NODE_LOG_CONFIG_FLAG_WRAP ) == NODE_LOG_CONFIG_FLAG_WRAP ) {
					event_log_config_wrap( EVENT_LOG_WRAP_ENABLE );
				} else {
					event_log_config_wrap( EVENT_LOG_WRAP_DISABLE );
				}
			}

			if ( ( temp2 & NODE_LOG_CONFIG_FLAG_PAYLOADS ) == NODE_LOG_CONFIG_FLAG_PAYLOADS ) {
				if ( ( temp & NODE_LOG_CONFIG_FLAG_PAYLOADS ) == NODE_LOG_CONFIG_FLAG_PAYLOADS ) {
					set_mac_payload_log_len( MAX_MAC_PAYLOAD_LOG_LEN );
				} else {
					set_mac_payload_log_len( MIN_MAC_PAYLOAD_LOG_LEN );
				}
			}

			if ( ( temp2 & NODE_LOG_CONFIG_FLAG_WN_CMDS ) == NODE_LOG_CONFIG_FLAG_WN_CMDS ) {
				if ( ( temp & NODE_LOG_CONFIG_FLAG_WN_CMDS ) == NODE_LOG_CONFIG_FLAG_WN_CMDS ) {
					wlan_exp_enable_logging = 1;
				} else {
					wlan_exp_enable_logging = 0;
				}
			}

			// Send response of status
            respArgs32[respIndex++] = Xil_Htonl( status );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
	    break;


	    //---------------------------------------------------------------------
		case NODE_LOG_GET_INFO:
            // NODE_LOG_GET_INFO Packet Format:
            //   - respArgs32[0] - Next empty entry index
            //   - respArgs32[1] - Oldest empty entry index
            //   - respArgs32[2] - Number of wraps
            //   - respArgs32[3] - Flags

			xil_printf("EVENT LOG:  Get Info\n");

			temp = event_log_get_next_entry_index();
            respArgs32[respIndex++] = Xil_Htonl( temp );
			xil_printf("    Next Index   = %10d\n", temp);

			temp = event_log_get_oldest_entry_index();
            respArgs32[respIndex++] = Xil_Htonl( temp );
			xil_printf("    Oldest Index = %10d\n", temp);

			temp = event_log_get_num_wraps();
            respArgs32[respIndex++] = Xil_Htonl( temp );
			xil_printf("    Num Wraps    = %10d\n", temp);

			temp = event_log_get_flags();
            respArgs32[respIndex++] = Xil_Htonl( temp );
			xil_printf("    Flags        = 0x%08x\n", temp);

			// Send response of current info
			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
	    break;


	    //---------------------------------------------------------------------
		case NODE_LOG_GET_CAPACITY:
            // NODE_LOG_GET_CAPACITY Packet Format:
            //   - respArgs32[0] - Max log size
            //   - respArgs32[1] - Current log size

			xil_printf("EVENT LOG:  Get Capacity\n");

			temp = event_log_get_capacity();
            respArgs32[respIndex++] = Xil_Htonl( temp );
			xil_printf("    Capacity = %10d\n", temp);

			temp = event_log_get_total_size();
            respArgs32[respIndex++] = Xil_Htonl( temp );
			xil_printf("    Size     = %10d\n", temp);

			// Send response of current info
			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
	    break;


	    //---------------------------------------------------------------------
		case NODE_LOG_GET_ENTRIES:
            // NODE_LOG_GET_ENTRIES Packet Format:
            //   - Note:  All u32 parameters in cmdArgs32 are byte swapped so use Xil_Ntohl()
            //
			//   - cmdArgs32[0] - buffer id
			//   - cmdArgs32[1] - flags
            //   - cmdArgs32[2] - start_address of transfer
			//   - cmdArgs32[3] - size of transfer (in bytes)
			//                      0xFFFF_FFFF  -> Get everything in the event log
			//   - cmdArgs32[4] - bytes_per_pkt
			//
			//   Return Value:
			//     - wn_buffer
            //       - buffer_id       - uint32  - ID of the buffer
			//       - flags           - uint32  - Flags
			//       - bytes_remaining - uint32  - Number of bytes remaining in the transfer
			//       - start_byte      - uint32  - Byte index of the first byte in this packet
			//       - size            - uint32  - Number of payload bytes in this packet
			//       - byte[]          - uint8[] - Array of payload bytes
			//
			// NOTE:  The address passed via the command is the address relative to the current
			//   start of the event log.  It is not an absolute address and should not be treated
			//   as such.
			//
			//     When you transferring "everything" in the event log, the command will take a
			//   snapshot of the size of the log to the "end" at the time the command is received
			//   (ie either the next_entry_index or the end of the log before it wraps).  It will then
			//   only transfer those events.  It will not any new events that are added to the log while
			//   we are transferring the current log as well as transfer any events after a wrap.
            //

			id                = Xil_Ntohl(cmdArgs32[0]);
			flags             = Xil_Ntohl(cmdArgs32[1]);
			start_index       = Xil_Ntohl(cmdArgs32[2]);
            size              = Xil_Ntohl(cmdArgs32[3]);

            // Get the size of the log to the "end"
            evt_log_size      = event_log_get_size(start_index);

            // Check if we should transfer everything or if the request was larger than the current log
            if ( ( size == NODE_LOG_GET_ALL_ENTRIES ) || ( size > evt_log_size ) ) {
                size = evt_log_size;
            }

            bytes_per_pkt     = max_words * 4;
            num_pkts          = (size / bytes_per_pkt) + 1;
            if ( (size % bytes_per_pkt) == 0 ){ num_pkts--; }    // Subtract the extra pkt if the division had no remainder
            curr_index        = start_index;
            bytes_remaining   = size;

			xil_printf("EVENT LOG: Get Entries \n");
			xil_printf("    curr_index       = 0x%8x\n", curr_index);
			xil_printf("    size             = %10d\n", size);
			xil_printf("    num_pkts         = %10d\n", num_pkts);

            // Initialize constant parameters
            respArgs32[0] = Xil_Htonl( id );
            respArgs32[1] = Xil_Htonl( flags );

            // Iterate through all the packets
			for( i = 0; i < num_pkts; i++ ) {

				// Get the next address
				next_index  = curr_index + bytes_per_pkt;

				// Compute the transfer size (use the full buffer unless you run out of space)
				if( next_index > ( start_index + size ) ) {
                    transfer_size = (start_index + size) - curr_index;
				} else {
					transfer_size = bytes_per_pkt;
				}

				// Set response args that change per packet
				respArgs32[2]   = Xil_Htonl( bytes_remaining );
	            respArgs32[3]   = Xil_Htonl( curr_index );
                respArgs32[4]   = Xil_Htonl( transfer_size );

                // Unfortunately, due to the byte swapping that occurs in node_sendEarlyResp, we need to set all 
                //   three command parameters for each packet that is sent.
	            respHdr->cmd     = cmdHdr->cmd;
	            respHdr->length  = 20 + transfer_size;
				respHdr->numArgs = 5;

				// Transfer data
				num_bytes = event_log_get_data( curr_index, transfer_size, (char *) &respArgs32[5] );

#ifdef _DEBUG_
				xil_printf("Packet %8d: \n", i);
				xil_printf("    transfer_index = 0x%8x\n    transfer_size    = %10d\n    num_bytes        = %10d\n", curr_index, transfer_size, num_bytes);
#endif

				// Check that we copied everything
				if ( num_bytes == transfer_size ) {
					// Send the packet
					node_sendEarlyResp(respHdr, pktSrc, eth_dev_num);
				} else {
					xil_printf("ERROR:  NODE_GET_EVENTS tried to get %d bytes, but only received %d @ 0x%x \n", transfer_size, num_bytes, curr_index );
				}

				// Update our current address and bytes remaining
				curr_index       = next_index;
				bytes_remaining -= transfer_size;
			}

			respSent = RESP_SENT;
		break;


		//---------------------------------------------------------------------
		// TODO:  THIS FUNCTION IS NOT COMPLETE
		case NODE_LOG_ADD_ENTRY:
			xil_printf("EVENT LOG:  Add Event not supported\n");
	    break;


		//---------------------------------------------------------------------
		// TODO:  THIS FUNCTION IS NOT COMPLETE
		case NODE_LOG_ENABLE_ENTRY:
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
		case NODE_STATS_CONFIG_TXRX:
            // NODE_STATS_CONFIG_TXRX Packet Format:
			//   - cmdArgs32[0]  - flags
			//                     [ 0] - Promiscuous stats collected = 1
			//                            Promiscuous stats not collected = 0
			//
			//   If the value is NODE_STATS_CONFIG_RSVD_VAL, then the flags will
			//   not be modified.
			//
            //   - respArgs32[0] - Value of flags
			//

			// Get flags
			temp = Xil_Ntohl(cmdArgs32[0]);

			if (temp != NODE_STATS_CONFIG_RSVD_VAL){
				// Configure the LOG based on the flag bits
				if ( ( temp & NODE_STATS_CONFIG_FLAG_PROMISC ) == NODE_STATS_CONFIG_FLAG_PROMISC ) {
					promiscuous_stats_enabled = 1;
				} else {
					promiscuous_stats_enabled = 0;
				}
			}
			// Set the return value
			status = 0;

			// Send response of status
            respArgs32[respIndex++] = Xil_Htonl( status );

			respHdr->length += (respIndex * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
	    break;


	    //---------------------------------------------------------------------
		case NODE_STATS_ADD_TXRX_TO_LOG:
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
		case NODE_STATS_GET_TXRX:
            // NODE_GET_STATS Packet Format:
			//   - cmdArgs32[0]   - buffer id
			//   - cmdArgs32[1]   - flags
            //   - cmdArgs32[2]   - start_address of transfer
			//   - cmdArgs32[3]   - size of transfer (in bytes)
			//   - cmdArgs32[4:5] - MAC Address (All 0xFF means all stats)
			// Always returns a valid WARPNet Buffer (either 1 or more packets)
            //   - buffer_id       - uint32  - buffer_id
			//   - flags           - uint32  - 0
			//   - bytes_remaining - uint32  - Number of bytes remaining in the transfer
			//   - start_byte      - uint32  - Byte index of the first byte in this packet
			//   - size            - uint32  - Number of payload bytes in this packet
			//   - byte[]          - uint8[] - Array of payload bytes

			xil_printf("Get TXRX Statistics\n");

			// Get MAC Address
        	wlan_exp_get_mac_addr(&((u32 *)cmdArgs32)[4], &mac_addr[0]);
        	id = wlan_exp_get_aid_from_ADDR(&mac_addr[0]);

        	// Local variables
        	statistics_txrx  * stats;
        	txrx_stats_entry * stats_entry;
        	u32                stats_size  = sizeof(statistics_txrx) - sizeof(dl_entry);

        	entry_size = sizeof(txrx_stats_entry);

            // Initialize constant return values
        	respIndex     = 5;              // There will always be 5 return args
            respArgs32[0] = cmdArgs32[0];
            respArgs32[1] = 0;

            if ( id == 0 ) {
				// If we cannot find the MAC address, print a warning and return an empty buffer
				xil_printf("WARNING:  Could not find specified node: %02x", mac_addr[0]);
				for ( i = 1; i < ETH_ADDR_LEN; i++ ) { xil_printf(":%02x", mac_addr[i] ); } xil_printf("\n");

				respArgs32[2]    = 0;
				respArgs32[3]    = 0;
				respArgs32[4]    = 0;

            } else {
				// If parameter is not the magic number to return all statistics structures
    			if ( id != NODE_CONFIG_ALL_ASSOCIATED ) {
    				// Find the statistics entry
    				curr_entry = wlan_mac_high_find_statistics_ADDR( get_statistics(), &mac_addr[0]);

    				if (curr_entry != NULL) {
    					stats       = (statistics_txrx *)(curr_entry->data);
    					stats_entry = (txrx_stats_entry *) &respArgs32[respIndex];

    					stats_entry->timestamp = get_usec_timestamp();

    					// Copy the statistics to the log entry
    					//   NOTE:  This assumes that the statistics entry in wlan_mac_entries.h has a contiguous piece of memory
    					//          equivalent to the statistics structure in wlan_mac_high.h (without the dl_entry)
    					memcpy( (void *)(&stats_entry->stats), (void *)(&stats), stats_size );

    					xil_printf("Getting Statistics for node: %02x", mac_addr[0]);
    					for ( i = 1; i < ETH_ADDR_LEN; i++ ) { xil_printf(":%02x", mac_addr[i] ); } xil_printf("\n");

						// Set the return args and increment the size
						respArgs32[2]    = Xil_Htonl( entry_size );
						respArgs32[3]    = 0;
						respArgs32[4]    = Xil_Htonl( entry_size );
						respHdr->length += entry_size;
    				} else {
						// If we cannot find the MAC address, print a warning and return an empty buffer
						xil_printf("WARNING:  Could not find specified node: %02x", mac_addr[0]);
						for ( i = 1; i < ETH_ADDR_LEN; i++ ) { xil_printf(":%02x", mac_addr[i] ); } xil_printf("\n");

						respArgs32[2]    = 0;
						respArgs32[3]    = 0;
						respArgs32[4]    = 0;
    				}
    			} else {
    				// Create a WARPNet buffer response to send all stats entries

    	            // Initialize constant parameters
    	            respArgs32[0] = 0xFFFFFFFF;
    	            respArgs32[1] = 0;

                    // Get the list of TXRX Statistics
    	            curr_list     = get_statistics();
                    total_entries = curr_list->length;
    	            size          = entry_size * total_entries;

    	            if ( size != 0 ) {
                        // Send the stats as a series of WARPNet Buffers

    	            	// Set loop variables
    	            	entry_per_pkt     = (max_words * 4) / entry_size;
    	            	bytes_per_pkt     = entry_per_pkt * entry_size;
    	            	num_pkts          = size / bytes_per_pkt + 1;
    		            if ( (size % bytes_per_pkt) == 0 ){ num_pkts--; }    // Subtract the extra pkt if the division had no remainder

    		            entry_remaining   = total_entries;
    		            bytes_remaining   = size;
    					curr_index        = 0;
    					curr_entry        = curr_list->first;
    					stats             = (statistics_txrx*)(curr_entry->data);
    					time              = get_usec_timestamp();

    					// Iterate through all the packets
    					for( i = 0; i < num_pkts; i++ ) {

    						// Get the next index
    						next_index  = curr_index + bytes_per_pkt;

    						// Compute the transfer size (use the full buffer unless you run out of space)
    						if( next_index > size ) {
    							transfer_size = size - curr_index;
    						} else {
    							transfer_size = bytes_per_pkt;
    						}

    						if( entry_remaining < entry_per_pkt) {
    						    transfer_entry_num = entry_remaining;
    						} else {
    						    transfer_entry_num = entry_per_pkt;
    						}

    						// Set response args that change per packet
    						respArgs32[2]    = Xil_Htonl( bytes_remaining );
    						respArgs32[3]    = Xil_Htonl( curr_index );
    						respArgs32[4]    = Xil_Htonl( transfer_size );

    						// Unfortunately, due to the byte swapping that occurs in node_sendEarlyResp, we need to set all
    						//   three command parameters for each packet that is sent.
    						respHdr->cmd     = cmdHdr->cmd;
    						respHdr->length  = 20 + transfer_size;
    						respHdr->numArgs = 5;

    						// Transfer data
    						stats_entry      = (txrx_stats_entry *) &respArgs32[respIndex];

                            for( j = 0; j < transfer_entry_num; j++ ){
                                // Set the timestamp for the stats entry
                            	stats_entry->timestamp = time;

        						// Copy the statistics to the log entry
        						//   NOTE:  This assumes that the statistics entry in wlan_mac_entries.h has a contiguous piece of memory
        						//          equivalent to the statistics structure in wlan_mac_high.h (without the dl_entry)
        						memcpy( (void *)(&stats_entry->stats), (void *)(&stats), stats_size );

                                // Increment the pointers
        						curr_entry  = dl_entry_next(curr_entry);
    							stats       = (statistics_txrx *)(curr_entry->data);
    							stats_entry = (txrx_stats_entry *)(((void *)stats_entry) + entry_size );
                            }

    						// Send the packet
    						node_sendEarlyResp(respHdr, pktSrc, eth_dev_num);

    						// Update our current address and bytes remaining
    						curr_index       = next_index;
    						bytes_remaining -= transfer_size;
    						entry_remaining -= entry_per_pkt;
    					}

    					respSent = RESP_SENT;
    	            } else {
						// Set empty response args
						respArgs32[2]   = 0;
						respArgs32[3]   = 0;
						respArgs32[4]   = 0;
    	            }
    			}
            }

			// Set the length and number of response args
			respHdr->length += (5 * sizeof(respArgs32));
			respHdr->numArgs = respIndex;
		break;


		//---------------------------------------------------------------------
		case NODE_QUEUE_TX_DATA_PURGE_ALL:
			xil_printf("Purging All Data Transmit Queues\n");

			purge_all_data_tx_queue();
		break;


        //---------------------------------------------------------------------
		default:
			// Call standard function in child class to parse parameters implmented there
			respSent = node_process_callback( cmdID, cmdHdr, cmdArgs, respHdr, respArgs, pktSrc, eth_dev_num);
		break;
	}

	return respSent;
}


#if 0

/*****************************************************************************/
/**
* Initial attempt to collapse 'get station info' and 'get statistics' in to a
* single function.  This does not currently work and needs to be fixed base on
* updates that are being made to dl_list / dl_entry.
*
* @param
*
* @return    Number of response arguments
*
* @note
*
******************************************************************************/
u32 wlan_exp_get_info_cmd_helper( u32 command, u32 * cmdArgs, u32 * respArgs,
		                          wn_respHdr* respHdr, void* pktSrc, u32 eth_dev_num, u32 max_words ) {

	// Common Variables
	u32                      i, j;
    u32                      ret_val             = 0;
    u32                      cmd_id              = WN_CMD_TO_CMDID(command);

	u8                       mac_addr[6];
	u32                      id;

	dl_list                * list;
	dl_entry               * list_item;
	void                   * data;
	void                   * entry;

	u32                      transfer_size;
	u32                      entry_size;
	u32                      data_size;
	u32                      pkt_size;

	u32                      bytes_remaining;
	u32                      entry_remaining;

	u32                      num_pkts;
	u32                      bytes_per_pkt;
	u32                      entry_per_pkt;

	u32                      total_entries;
	u32                      transfer_entry_num;

	u32                      curr_index;
	u32                      next_index;

	u64                      time;

	// Station Info specific variables
	station_info_entry     * info_entry;

	// Statistics specific variables
	txrx_stats_entry       * stats_entry;


	xil_printf("Get Info\n");

	// Get MAC Address
	wlan_exp_get_mac_addr(&cmdArgs[0], &mac_addr[0]);
	id = wlan_exp_get_aid_from_ADDR(&mac_addr[0]);

    // Initialize variables based on command
	switch(cmd_id){
        case NODE_GET_STATION_INFO:
        	entry_size = sizeof(station_info_entry);
        	list       = get_station_info_list();
        break;

		case NODE_STATS_GET_TXRX:
        	entry_size = sizeof(txrx_stats_entry);
        	list       = get_statistics();
        break;

        default:
        	xil_printf("   Command not supported:  %d\n", cmd_id);
        	return ret_val;
	}

	// If parameter is not the magic number
	if ( id != NODE_CONFIG_ALL_ASSOCIATED ) {
#if 0
		// Find the station_info entry
		curr_station_info = wlan_mac_high_find_station_info_ADDR( get_station_info_list(), &mac_addr[0]);

		if (curr_station_info != NULL) {
			info_entry = (station_info_entry *) &respArgs[respIndex];

			info_entry->timestamp = get_usec_timestamp();

			// Copy the station info to the log entry
			//   NOTE:  This assumes that the station info entry in wlan_mac_entries.h has a contiguous piece of memory
			//          similar to the station info and tx params structures in wlan_mac_high.h
			memcpy( (void *)(&info_entry->addr), (void *)(&curr_station_info->addr), station_info_size );
			memcpy( (void *)(&info_entry->rate), (void *)(&curr_station_info->tx.phy.rate), tx_params_size );

			xil_printf("Getting Station Entry for node: %02x", mac_addr[0]);
			for ( i = 1; i < ETH_ADDR_LEN; i++ ) { xil_printf(":%02x", mac_addr[i] ); } xil_printf("\n");
		} else {
			xil_printf("Could not find specified node: %02x", mac_addr[0]);
			for ( i = 1; i < ETH_ADDR_LEN; i++ ) { xil_printf(":%02x", mac_addr[i] ); } xil_printf("\n");
		}
#endif

	} else {
		// Create a WARPNet buffer response to send all entries in the list

	    // Initialize constant parameters
		respArgs[0] = 0xFFFFFFFF;
		respArgs[1] = 0;

	    // Get the number of
	    total_entries     = list->length;
	    transfer_size     = entry_size * total_entries;

	    if ( transfer_size != 0 ) {
	        // Send the data as a series of WARPNet Buffers

	    	// Set loop variables
	    	entry_per_pkt     = (max_words * 4) / entry_size;
	    	bytes_per_pkt     = entry_per_pkt * entry_size;
	    	num_pkts          = size / bytes_per_pkt + 1;
	        if ( (size % bytes_per_pkt) == 0 ){ num_pkts--; }    // Subtract the extra pkt if the division had no remainder

	        entry_remaining   = total_entries;
	        bytes_remaining   = size;
			curr_index        = 0;
			time              = get_usec_timestamp();
			list_item         = list->first;

			// Iterate through all the packets
			for( i = 0; i < num_pkts; i++ ) {

				// Get the next index
				next_index  = curr_index + bytes_per_pkt;

				// Compute the transfer size (use the full buffer unless you run out of space)
				if( next_index > size ) {
					pkt_size = size - curr_index;
				} else {
					pkt_size = bytes_per_pkt;
				}

				if( entry_remaining < entry_per_pkt) {
				    transfer_entry_num = entry_remaining;
				} else {
				    transfer_entry_num = entry_per_pkt;
				}

				// Set response args that change per packet
				respArgs[2]    = Xil_Htonl( bytes_remaining );
				respArgs[3]    = Xil_Htonl( curr_index );
				respArgs[4]    = Xil_Htonl( pkt_size );

				// Unfortunately, due to the byte swapping that occurs in node_sendEarlyResp, we need to set all
				//   three command parameters for each packet that is sent.
				respHdr->cmd     = command;
				respHdr->length  = 20 + pkt_size;
				respHdr->numArgs = 5;

				// Transfer data
				entry = (void *) &respArgs[5];

	            for( j = 0; j < transfer_entry_num; j++ ){

					// Copy the data to the packet
					memcpy( entry, info->data, entry_size );

	                // Set the timestamp for the station_info entry
	            	entry->timestamp = time;

	            	// Increment the pointers
					info               = info->next;
					entry              = (void *)(entry + entry_size);
	            }

				// Send the packet
				node_sendEarlyResp(respHdr, pktSrc, eth_dev_num);

				// Update our current address and bytes remaining
				curr_index       = next_index;
				bytes_remaining -= transfer_size;
				entry_remaining -= entry_per_pkt;
			}
	    } else {
			// Set empty response args
	    	respArgs[2]      = 0;
	    	respArgs[3]      = 0;
	    	respArgs[4]      = 0;
	    	ret_val          = 5;             // 5 response words

	    	respHdr->length += (ret_val * sizeof(u32));
	    }
	}

    return ret_val;
}

#endif


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
    node_info.type                = type;
    node_info.node                = 0xFFFF;
    node_info.hw_generation       = WARP_HW_VERSION;
    node_info.warpnet_design_ver  = REQ_WARPNET_HW_VER;
    
    for( i = 0; i < FPGA_DNA_LEN; i++ ) {
        node_info.fpga_dna[i]     = fpga_dna[i];
    }

    node_info.serial_number       = serial_number;
    node_info.wlan_exp_design_ver = REQ_WLAN_EXP_HW_VER;
    
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

#if WLAN_EXP_WAIT_FOR_ETH

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
	// Note:  Doing processing this way so that when the structure is copied and parsed in the log
	//   we do not need to mangle the address.
    node_info.wlan_hw_addr[0] = (hw_addr[2]<<24) | (hw_addr[3]<<16) | (hw_addr[4]<<8) | hw_addr[5];
    node_info.wlan_hw_addr[1] = (hw_addr[0]<<8)  |  hw_addr[1];
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



/*****************************************************************************/
/**
* WLAN Mapping of MAC Addr to AID
*
* This function contains the mapping of MAC address to AID within a node.
*
* @param    MAC Address
*
* @return	AID associated with that MAC address
*
* @note		None.
*
******************************************************************************/
u32  wlan_exp_get_aid_from_ADDR(u8 * mac_addr) {
	u32            id;
	dl_entry*	   entry;
	station_info * info;

	if ( wlan_addr_eq(mac_addr, bcast_addr) ) {
		id = 0xFFFFFFFF;
	} else {
		entry = wlan_mac_high_find_station_info_ADDR(&association_table, mac_addr);

		if (entry != NULL) {
			info = (station_info*)(entry->data);
            id = info->AID;
		} else {
			xil_printf("ERROR:  Could not find MAC address = %02x:%02x:%02x:%02x:%02x:%02x\n",
                       mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
			id = 0;
		}
	}

	return id;
}



/*****************************************************************************/
/**
* Create WN Command Log Entry
*
* This function creates a WN Command Log Entry
*
* @param    MAC Address
*
* @return	AID associated with that MAC address
*
* @note		None.
*
******************************************************************************/
void create_wn_cmd_log_entry(wn_cmdHdr* cmdHdr, void * cmdArgs, u16 src_id) {
    // Create a new log entry for each WARPNet command and copy up to the first 10 args
	//
	u32 i;
	wn_cmd_entry* entry      = get_next_empty_wn_cmd_entry();
	u32         * cmdArgs32  = cmdArgs;
	u32           num_args   = (cmdHdr->numArgs > 10) ? 10 : cmdHdr->numArgs;

	if (entry != NULL) {
		entry->timestamp = get_usec_timestamp();
		entry->command   = cmdHdr->cmd;
		entry->src_id    = src_id;
		entry->num_args  = num_args;

		// Add arguments to the entry
		for (i = 0; i < num_args; i++) {
			(entry->args)[i] = Xil_Ntohl(cmdArgs32[i]);
		}
		// Zero out any other arguments in the entry
		for (i = num_args; i < 10; i++) {
			(entry->args)[i] = 0;
		}

#ifdef _DEBUG_
		print_entry( 0, ENTRY_TYPE_WN_CMD, (void *) entry );
#endif
	}
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
    xil_printf("  WARPNet HW Ver:     0x%x \n",    info->warpnet_design_ver);
    
    xil_printf("  FPGA DNA:           ");
    
    for( i = 0; i < FPGA_DNA_LEN; i++ ) {
        xil_printf("0x%8x  ", info->fpga_dna[i]);
    }
	xil_printf("\n");

	xil_printf("  Serial Number:      0x%x \n",    info->serial_number);
    xil_printf("  WLAN Exp HW Ver:    0x%x \n",    info->wlan_exp_design_ver);
        
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
