/** @file wlan_mac_nomac.c
 *  @brief Simple MAC that does nothing but transmit and receive
 *
 *  @copyright Copyright 2014-2015, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */


/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_NOMAC_H_
#define WLAN_MAC_NOMAC_H_


//-----------------------------------------------
// WLAN Exp low parameter defines (NOMAC)
//     NOTE:  Need to make sure that these values do not conflict with any of the LOW PARAM
//     callback defines
//
// #define LOW_PARAM_NOMAC_                                  0x20000001


/*********************** Global Structure Definitions ************************/


/*************************** Function Prototypes *****************************/
int  main();

int  frame_transmit(u8 pkt_buf, u8 rate, u16 length, wlan_mac_low_tx_details* low_tx_details);
u32  frame_receive(u8 rx_pkt_buf, phy_rx_details* phy_details);

int  wlan_nomac_process_low_param(u8 mode, u32* payload);

#endif /* WLAN_MAC_NOMAC_H_ */
