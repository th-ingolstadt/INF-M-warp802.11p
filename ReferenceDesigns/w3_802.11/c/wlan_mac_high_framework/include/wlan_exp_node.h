/** @file wlan_exp_node.h
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
 */


/***************************** Include Files *********************************/
#include "wlan_exp_common.h"


// WLAN MAC includes for common functions
#include "wlan_mac_high.h"


/*************************** Constant Definitions ****************************/
#ifndef WLAN_EXP_NODE_H_
#define WLAN_EXP_NODE_H_



// ****************************************************************************
// Define WLAN Exp Node Commands
//
#define NODE_INFO                       1
#define NODE_IDENTIFY                   2

#define NODE_IDENTIFY_ALL              0xFFFF

#define NODE_CONFIG_SETUP               3
#define NODE_CONFIG_RESET               4
#define NODE_TEMPERATURE                5

#define NODE_ASSN_GET_STATUS           10
#define NODE_ASSN_SET_TABLE            11

#define NODE_DISASSOCIATE              20

#define NODE_TX_GAIN                   30
#define NODE_TX_RATE                   31
#define NODE_CHANNEL                   32
#define NODE_TIME                      33

#define NODE_TX_GAIN_RSVD_VAL          0xFFFF
#define NODE_TX_RATE_RSVD_VAL          0xFFFF
#define NODE_CHANNEL_RSVD_VAL          0xFFFF
#define NODE_TIME_RSVD_VAL             0xFFFF

#define NODE_LTG_CONFIG                40
#define NODE_LTG_START                 41
#define NODE_LTG_STOP                  42
#define NODE_LTG_REMOVE                43

#define NODE_LTG_ERROR                 0xFFFFFFFF

#define NODE_LOG_RESET                 50
#define NODE_LOG_CONFIG                51
#define NODE_LOG_GET_CURR_IDX          52
#define NODE_LOG_GET_OLDEST_IDX        53
#define NODE_LOG_GET_EVENTS            54
#define NODE_LOG_ADD_EVENT             55
#define NODE_LOG_ENABLE_EVENT          56
#define NODE_LOG_STREAM_ENTRIES        57

#define NODE_LOG_CONFIG_FLAG_WRAP      0x00000001

#define NODE_ADD_STATS_TO_LOG          60
#define NODE_GET_STATS                 61
#define NODE_RESET_STATS               62

#define NODE_CONFIG_DEMO               90


#define NODE_CONFIG_ALL_ASSOCIATED     0xFFFFFFFF

// ****************************************************************************
// Define Node Parameters
//   - NOTE:  To add another parameter, add the define before "NODE_MAX_PARAMETER"
//     and then change the value of "NODE_MAX_PARAMETER" to be the largest value
//     in the list so it is easy to iterate over all parameters
//
#define NODE_TYPE                      0
#define NODE_ID                        1
#define NODE_HW_GEN                    2
#define NODE_DESIGN_VER                3
#define NODE_SERIAL_NUM                4
#define NODE_FPGA_DNA                  5
#define NODE_WLAN_MAC_ADDR             6
#define NODE_WLAN_MAX_ASSN             7
#define NODE_WLAN_EVENT_LOG_SIZE       8
#define NODE_WLAN_MAX_STATS            9
#define NODE_MAX_PARAMETER            10



/*********************** Global Structure Definitions ************************/

// **********************************************************************
// WARPNet Node Info Structure
//
typedef struct {

    u32   type;                             // Type of WARPNet node
    u32   node;                             // Only first 16 bits are valid
    u32   hw_generation;                    // Only first  8 bits are valid
	u32   design_ver;                       // Only first 24 bits are valid

	u32   serial_number;
	u32   fpga_dna[FPGA_DNA_LEN];

    u32   wlan_hw_addr[2];                  // WLAN Exp - Wireless MAC address
	u32   wlan_max_assn;                    // WLAN Exp - Max Associations
	u32   wlan_event_log_size;              // WLAN Exp - Event Log Size
	u32   wlan_max_stats;                   // WLAN Exp - Max number of promiscuous statistic entries

    u32   eth_device;
    u8    hw_addr[ETH_ADDR_LEN];
    u8    ip_addr[IP_VERSION];
    u32   unicast_port;
    u32   broadcast_port;

} wn_node_info;



/*************************** Function Prototypes *****************************/

// WLAN Exp node commands
//
int  wlan_exp_node_init( u32 type, u32 serial_number, u32 *fpga_dna, u32 eth_dev_num, u8 *hw_addr );

void node_set_process_callback(void(*callback)());
int  node_get_parameters(u32 * buffer, unsigned int max_words, unsigned char network);
int  node_get_parameter_values(u32 * buffer, unsigned int max_words);

void node_info_set_wlan_hw_addr  ( u8 * hw_addr  );
void node_info_set_max_assn      ( u32 max_assn  );
void node_info_set_event_log_size( u32 log_size  );
void node_info_set_max_stats     ( u32 max_stats );

u32  wn_get_node_id       ( void );
u32  wn_get_serial_number ( void );
u32  wn_get_curr_temp     ( void );
u32  wn_get_min_temp      ( void );
u32  wn_get_max_temp      ( void );

#endif /* WLAN_EXP_NODE_H_ */
