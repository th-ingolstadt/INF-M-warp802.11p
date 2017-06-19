/** @file wlan_phy_util.h
 *  @brief Physical Layer Utility
 *
 *  This contains code for configuring low-level parameters in the PHY and hardware.
 *
 *  @copyright Copyright 2013-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *                See LICENSE.txt included in the design archive or
 *                at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/*************************** Constant Definitions ****************************/
#ifndef WLAN_PHY_UTIL_H_
#define WLAN_PHY_UTIL_H_

#include "xil_types.h"

//Forward declarations
enum phy_samp_rate_t;

// ****************************************************************************
// RATE field values for SIGNAL/L-SIG in PHY preamble (IEEE 802.11-2012 18.3.4.2)
//  DSSS 1M rate code is non-standard, used by our code to indicate DSSS Rx
#define WLAN_PHY_RATE_DSSS_1M  0x1

// ****************************************************************************
// Data bytes per OFDM symbol
//     NOTE:  Values from Table 17-3 of 2007 IEEE 802.11
//
#define N_DBPS_R6    24
#define N_DBPS_R9    36
#define N_DBPS_R12   48
#define N_DBPS_R18   72
#define N_DBPS_R24   96
#define N_DBPS_R36  144
#define N_DBPS_R48  192
#define N_DBPS_R54  216

/**************************** Macro Definitions ******************************/

#define REG_CLEAR_BITS(addr, mask) Xil_Out32(addr, (Xil_In32(addr) & ~(mask)))
#define REG_SET_BITS(addr, mask)   Xil_Out32(addr, (Xil_In32(addr) | (mask)))

// Macros to calculate the SIGNAL / L-SIG field in the PHY preamble (IEEE 802.11-2012 18.3.4)
#define WLAN_TX_SIGNAL_CALC(rate, length) (((rate) & 0xF) | (((length) & 0xFFF) << 5) | (WLAN_TX_SIGNAL_PARITY_CALC(rate,length)))
#define WLAN_TX_SIGNAL_PARITY_CALC(rate, length) ((0x1 & (ones_in_chars[(rate)] + ones_in_chars[(length) & 0xFF] + ones_in_chars[(length) >> 8])) << 17)

/*********************** Global Variable Definitions *************************/

extern const u8    ones_in_chars[256];

/*************************** Function Prototypes *****************************/

// PHY commands
void write_phy_preamble(u8 pkt_buf, u8 phy_mode, u8 mcs, u16 length);

// Calculate transmit times
u16 wlan_ofdm_calc_txtime(u16 length, u8 mcs, u8 phy_mode, enum phy_samp_rate_t phy_samp_rate);
u16 wlan_ofdm_calc_num_payload_syms(u16 length, u8 mcs, u8 phy_mode);

#endif /* WLAN_PHY_UTIL_H_ */
