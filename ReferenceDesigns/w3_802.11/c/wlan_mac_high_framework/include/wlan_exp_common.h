/** @file wlan_exp_common.h
 *  @brief Experiment Framework (Common)
 *
 *  This contains the code for WLAN Experimental Framework.
 *
 *  @copyright Copyright 2013-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *                See LICENSE.txt included in the design archive or
 *                at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */


/*************************** Constant Definitions ****************************/
#ifndef WLAN_EXP_COMMON_H_
#define WLAN_EXP_COMMON_H_

/***************************** Include Files *********************************/
#include "wlan_mac_high_sw_config.h"
#include "xil_types.h"
#include "wlan_common_types.h"
#include "wlan_high_types.h"



// **********************************************************************
// WLAN Experiment Controls
//
//   NOTE:  These are the most common parameters that would be modified by a user:
//       1) Debug print level
//       2) DDR initialization
//       3) Ethernet controls
//       4) Timeouts
//


// 1) Choose the default debug print level
//
//    Values (see below for more information):
//        WLAN_EXP_PRINT_NONE      - Print WLAN_EXP_PRINT_NONE messages
//        WLAN_EXP_PRINT_ERROR     - Print WLAN_EXP_PRINT_ERROR and WLAN_EXP_PRINT_NONE messages
//        WLAN_EXP_PRINT_WARNING   - Print WLAN_EXP_PRINT_WARNING, WLAN_EXP_PRINT_ERROR and WLAN_EXP_PRINT_NONE messages
//        WLAN_EXP_PRINT_INFO      - Print WLAN_EXP_PRINT_INFO, WLAN_EXP_PRINT_WARNING, WLAN_EXP_PRINT_ERROR and WLAN_EXP_PRINT_NONE messages
//        WLAN_EXP_PRINT_DEBUG     - Print WLAN_EXP_PRINT_DEBUG, WLAN_EXP_PRINT_INFO, WLAN_EXP_PRINT_WARNING, WLAN_EXP_PRINT_ERROR and WLAN_EXP_PRINT_NONE messages
//
#define WLAN_EXP_DEFAULT_DEBUG_PRINT_LEVEL                 WLAN_EXP_PRINT_WARNING

// 3) Ethernet controls
//
//    a) Choose the Ethernet device and set the base address for the subnet and speed of the device:
//
//       Values for WLAN_EXP_DEFAULT_IP_ADDR:
//           0xAABBCC00      - Hexadecimal representation of an IP subnet:  AA.BB.CC.00
//                             where AA, BB, and CC are hexadecimal numbers.
//
//           NOTE:  IP subnet should match the host networking setup
//
//       Values for WLAN_EXP_DEFAULT_SPEED:
//           1000            - 1000 Mbps Ethernet (ie 1Gbps)
//           100             - 100 Mbps Ethernet
//           10              - 10 Mbps Ethernet
//
#define WLAN_EXP_DEFAULT_IP_ADDR                           0x0a000000     // 10.0.0.x
#define WLAN_EXP_DEFAULT_SPEED                             1000
#define WLAN_EXP_DEFAULT_UDP_UNICAST_PORT                  9500
#define WLAN_EXP_DEFAULT_UDP_MULTICAST_PORT                9750

//    b) Wait for the WLAN Exp Ethernet interface to be ready before continuing boot
//
//       Values:
//           1               - Wait for Ethernet device to be ready
//           0               - Do not wait for Ethernet device to be ready
//
//    NOTE:  During boot, this parameter will cause the node to wait for Ethernet B to be ready.
//
#define WLAN_EXP_WAIT_FOR_ETH                              0


//    c) Allow WLAN Exp Ethernet interface link speed to be negotiated
//
//       Values of WLAN_EXP_NEGOTIATE_ETH_LINK_SPEED:
//           1               - Auto-negotiate the Ethernet link speed
//           0               - Do not auto-negotiate the Ethernet link speed.  Speed is set
//                             by the WLAN_EXP_DEFAULT_SPEED defined above.
//
//     NOTE:  Based on initial testing, auto-negotiation of the link speed will add around 3
//            seconds to the boot time of the node.  To bypass auto-negotiation but use a
//            different default link speed, please adjust the WLAN_EXP_DEFAULT_SPEED define
//            above.
//
#define WLAN_EXP_NEGOTIATE_ETH_LINK_SPEED                  0


//    d) Transmit MTU size for packets generated by the node (aka. async packets)
//
//       Values of WLAN_EXP_TX_ASYNC_PACKET_MTU_SIZE:
//           [0 - 9008]      - Size of packet in bytes
//
//   NOTE:  Unfortunately, it is difficult to determine the size of the largest packet supported
//       by the host to which packets are being sent.  Therefore, the framework assumes that an
//       "async" packet will be the standard MTU size of 1514 bytes.  Hence, the buffer needed
//       to hold a standard MTU will be 1516 bytes (ie 1514 bytes rounded up for 32 bit alignment).
//
#define WLAN_EXP_TX_ASYNC_PACKET_BUFFER_SIZE               1516


//    e) Default maximum words supported by the packet
//
//   NOTE:  By default, the node will only use ~60% of a standard MTU packet when issuing buffer
//       commands or other commands which require potentially large amounts of data.  The
//       CMDID_TRANSPORT_PAYLOAD_SIZE_TEST command can be used to override the default value in
//       the framework.  This value was chosen as a "sane default" in the case that the payload
//       size test command is not run.
//
#define WLAN_EXP_DEFAULT_MAX_PACKET_WORDS                  240


// 4) Timeouts
//
//    a) Timeout when requesting data from CPU Low
//
//       Values of WLAN_EXP_CPU_LOW_DATA_REQ_TIMEOUT (in microseconds):
//           [1 - 1000000]   - Number of microseconds before timing out on a data request from CPU Low
//
//     NOTE:  By default the host transport timeout is 1 second so the value of this timeout should be
//         somewhere between 1 usec and 1 sec.  The reference design will set this timeout to be 0.5 sec
//         by default.  This is used when requesting data from CPU low through:  wlan_mac_high_read_low_mem()
//         and wlan_mac_high_read_low_param().
//
#define WLAN_EXP_CPU_LOW_DATA_REQ_TIMEOUT                  500000



// **********************************************************************
// WLAN Exp print levels
//
#define WLAN_EXP_PRINT_NONE                      0U
#define WLAN_EXP_PRINT_ERROR                     1U
#define WLAN_EXP_PRINT_WARNING                   2U
#define WLAN_EXP_PRINT_INFO                      3U
#define WLAN_EXP_PRINT_DEBUG                     4U


#define wlan_exp_printf(level, type, format, args...) \
            do {  \
                if ((u8)level <= (u8)wlan_exp_print_level) { \
                    wlan_exp_print_header(level, type, __FILE__, __LINE__); \
                    xil_printf(format, ##args); \
                } \
            } while (0)



extern u8       wlan_exp_print_level;
extern const char   * print_type_node;
extern const char   * print_type_transport;
extern const char   * print_type_event_log;
extern const char   * print_type_counts;
extern const char   * print_type_ltg;
extern const char   * print_type_queue;



// **********************************************************************
// WLAN Exp Common Defines
//
#define RESP_SENT                                          1
#define NO_RESP_SENT                                       0

#define SUCCESS                                            0
#define FAILURE                                           -1

#define CMD_TO_GROUP(x)                                  ((x) >> 24)
#define CMD_TO_CMDID(x)                                  ((x) & 0xFFFFFF)

#define WLAN_EXP_FALSE                                     0
#define WLAN_EXP_TRUE                                      1

#define WLAN_EXP_DISABLE                                   0
#define WLAN_EXP_ENABLE                                    1

#define WLAN_EXP_NO_TRANSMIT                               0
#define WLAN_EXP_TRANSMIT                                  1

#define WLAN_EXP_SILENT                                    0
#define WLAN_EXP_VERBOSE                                   1

#define WLAN_EXP_BUFFER_NUM_ARGS                           5
#define WLAN_EXP_BUFFER_HEADER_SIZE                        20


// **********************************************************************
// Command Group defines
//
#define GROUP_NODE                                         0x00
#define GROUP_TRANSPORT                                    0x10
#define GROUP_USER                                         0x20



/*********************** Global Structure Definitions ************************/
// 
// Message Structures
//

// Command / Response Header
//
typedef struct cmd_resp_hdr{
    u32                      cmd;
    u16                      length;
    u16                      num_args;
} cmd_resp_hdr;


// Command / Response data structure
//     This structure is used to keep track of pointers when decoding commands.
//
typedef struct cmd_resp{
    u32                      flags;                        // Flags for the command / response
                                                           //     [0] - Is the packet broadcast?  WLAN_EXP_TRUE / WLAN_EXP_FALSE
    void                   * buffer;                       // In general, assumed to be a (wlan_exp_ip_udp_buffer *)
    cmd_resp_hdr           * header;
    u32                    * args;
} cmd_resp;

// **********************************************************************
// WLAN Exp Tag Parameter Structure
//
typedef struct wlan_exp_tag_parameter{
    u8    reserved;
    u8    group;
    u16   length;
    u32   command;
    u32  *value;
} wlan_exp_tag_parameter;



/*************************** Function Prototypes *****************************/
#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP

// Initialization Functions

// Callbacks
int           wlan_exp_null_callback(void* param);

// Printing Functions
void          wlan_exp_print_header(u8 level, const char * type, char * filename, u32 line);
void          wlan_exp_print_mac_address(u8 level, u8 * mac_address);

void          wlan_exp_set_print_level(u8 level);

void          print_mac_address(u8 * mac_address);
void          print_timestamp();

// WLAN Exp specific functions
void          wlan_exp_get_mac_addr(u32 * src, u8 * dest);
void          wlan_exp_put_mac_addr(u8 * src, u32 * dest);

// Tag parameter functions
int           wlan_exp_init_parameters(wlan_exp_tag_parameter * parameters, u8 group, u32 num_parameters, u32 * values, u16 * lengths);
int           wlan_exp_get_parameters(wlan_exp_tag_parameter * parameters, u32 num_parameters, u32 * buffer, u32 max_resp_len, u8 values_only, u8 transmit);


#ifdef _DEBUG_
void          print_wlan_exp_parameters(wlan_exp_tag_parameter *param, int num_params);
#endif

#endif //WLAN_SW_CONFIG_ENABLE_WLAN_EXP

#endif /* WLAN_EXP_COMMON_H_ */
