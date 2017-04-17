/** @file wlan_phy_util.c
 *  @brief Physical Layer Utility
 *
 *  This contains code for configuring low-level parameters in the PHY and hardware.
 *
 *  @copyright Copyright 2013-2017, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */
/***************************** Include Files *********************************/

// Xilinx SDK includes
#include "stdio.h"
#include "stdarg.h"
#include "xio.h"
#include "string.h"
#include "xparameters.h"

// WLAN includes
#include "wlan_platform_low.h"
#include "wlan_platform_common.h"
#include "wlan_mac_mailbox_util.h"
#include "wlan_phy_util.h"
#include "wlan_mac_low.h"
#include "wlan_mac_common.h"

// LUT of number of ones in each byte (used to calculate PARITY in SIGNAL)
const u8 ones_in_chars[256] = {
         0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,   // 0x00 - 0x0F
         1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,   // 0x10 - 0x1F
         1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,   // 0x20 - 0x2F
         2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,   // 0x30 - 0x3F
         1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,   // 0x40 - 0x4F
         2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,   // 0x50 - 0x5F
         2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,   // 0x60 - 0x6F
         3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,   // 0x70 - 0x7F
         1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,   // 0x80 - 0x8F
         2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,   // 0x90 - 0x9F
         2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,   // 0xA0 - 0xAF
         3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,   // 0xB0 - 0xBF
         2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,   // 0xC0 - 0xCF
         3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,   // 0xD0 - 0xDF
         3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,   // 0xE0 - 0xEF
         4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};  // 0xF0 - 0xFF


// Common Platform Device Info
extern platform_common_dev_info_t	 platform_common_dev_info;


/*****************************************************************************/
/**
 * Calculates the PHY preamble (SIGNAL for 11a, L-SIG/HT-SIG for 11n) and writes
 * the preamble bytes to the specified packet buffer. The PHY preamble must be
 * written to the packet buffer for each transmission.
 *
 * @param   pkt_buf          - Packet buffer number
 * @param   phy_mode         - Sets waveform format; must be PHY_MODE_NON_HT (11a) or PHY_MODE_HTMF (11n)
 * @param   mcs              - MCS (modulation/coding scheme) index
 * @param   length           - Length of the packet being transmitted
 *
 * @return  None
 *
 *****************************************************************************/
const u8 sig_rate_vals[8] = {0xB, 0xF, 0xA, 0xE, 0x9, 0xD, 0x8, 0xC};

void write_phy_preamble(u8 pkt_buf, u8 phy_mode, u8 mcs, u16 length) {

	u8* htsig_ptr;
	u16 lsig_length;
	u8* phy_hdr_ptr;

	phy_hdr_ptr = (u8*)(CALC_PKT_BUF_ADDR(platform_common_dev_info.tx_pkt_buf_baseaddr, pkt_buf) + PHY_TX_PKT_BUF_PHY_HDR_OFFSET);

	// RATE field values for SIGNAL/L-SIG in PHY preamble (IEEE 802.11-2012 18.3.4.2)
	//  RATE field in SIGNAL/L-SIG is one of 8 4-bit values indicating modulation scheme and coding rate
	//  For 11a (NONHT) transmissions we map mcs index to SIGNAL.RATE directly
	//  For 11n (HTMF) transmissions the L-SIG.RATE field is always the lowest (BSPK 1/2)

	if((phy_mode & PHY_MODE_NONHT) == PHY_MODE_NONHT) {
		//11a mode - write SIGNAL (3 bytes)

		//Zero-out any stale header, also properly sets SERVICE and reserved bytes to 0
		//Old: takes longer than necessary. We only really need the SERVICE bits 0.
		//bzero((u32*)phy_hdr_ptr, PHY_TX_PKT_BUF_PHY_HDR_SIZE);

		// Set SERVICE to 0.
		// Unfortunately, SERVICE spans a 32-bit boundary so we need two write 2 words.
		Xil_Out32(phy_hdr_ptr + 8, 0);
		Xil_Out32(phy_hdr_ptr + 8, 4);

		//Set SIGNAL with actual rate/length
	    Xil_Out32((u32*)(phy_hdr_ptr), WLAN_TX_SIGNAL_CALC(sig_rate_vals[mcs], length));

	} else if((phy_mode & PHY_MODE_HTMF) == PHY_MODE_HTMF) {
		//11n mode - write L-SIG (3 bytes) and HT-SIG (6 bytes)

		//Zero-out any stale header, also properly sets SERVICE, reserved bytes, and auto-filled bytes of HT-SIG to 0
		//Old: takes longer than necessary. We only really need the SERVICE bits 0.
		//bzero((u32*)(TX_PKT_BUF_TO_ADDR(pkt_buf) + PHY_TX_PKT_BUF_PHY_HDR_OFFSET), PHY_TX_PKT_BUF_PHY_HDR_SIZE);

		// Set SERVICE to 0.
		Xil_Out32(phy_hdr_ptr + 8, 0);


	    // L-SIG is same format as 11a SIGNAL, but with RATE always 6Mb and LENGTH
	    //  set such that LENGTH/6Mb matches duration of HT transmission
	    //  Using equation from IEEE 802.11-2012 9.23.4
	    //   L-SIG.LENGTH = (3*ceil( (TXTIME - 6 - 20) / 4) - 3)
	    //  where TXTIME is actual duration of the HT transmission
		//   The ceil((TXTIME - 6 - 20)/4) term represents the number of OFDM symbols after the L-SIG symbol
		//  (-6-20) are (T_EXT-T_NONHT_PREAMBLE); (-3) accounts for service/tail

		// Calc (3*(num_payload_syms+num_ht_preamble_syms) = (3*(num_payload_syms+4))

		lsig_length = 3*wlan_ofdm_calc_num_payload_syms(length, mcs, phy_mode) + 12 - 3;

		

		Xil_Out32((u32*)(phy_hdr_ptr), WLAN_TX_SIGNAL_CALC(sig_rate_vals[0], lsig_length));

		//Assign pointer to first byte of HTSIG (PHY header base + 3 for sizeof(L-SIG))
	    htsig_ptr = (u8*)(phy_hdr_ptr + 3);

		//Set HTSIG bytes
	    // PHY logic fills in bytes 4 and 5; ok to ignore here
	    // Going byte-by-byte for now - maybe optimize to (unaligned) 32-bit write later
	    htsig_ptr[0] = mcs & 0x3F; //MSB is channel bandwidth; 0=20MHz
	    htsig_ptr[1] = length & 0xFF;
	    htsig_ptr[2] = (length >> 8) & 0xFF;
	    htsig_ptr[3] = 0x7; //smoothing=1, not-sounding=1, reserved=1, aggregation=STBC=FEC=short_GI=0
	}


    return;
}

/*****************************************************************************/
/**
 * Calculates duration of an OFDM waveform.
 *
 * This function assumes every OFDM symbol is the same duration. Duration
 * calculation for short guard interval (SHORT_GI) waveforms is not supported.
 *
 * @param   length         - Length of MAC payload in bytes
 * @param   mcs            - MCS index
 * @param   phy_mode       - PHY waveform mode - either PHY_MODE_NONHT (11a/g) or PHY_MODE_HTMF (11n)
 * @param   phy_samp_rate  - PHY sampling rate - one of (PHY_10M, PHY_20M, PHY_40M)
 *
 * @return  u16              - Duration of transmission in microseconds
 *                             (See IEEE 802.11-2012 18.4.3 and 20.4.3)
 *****************************************************************************/
inline u16 wlan_ofdm_calc_txtime(u16 length, u8 mcs, u8 phy_mode, phy_samp_rate_t phy_samp_rate) {

    u16 num_ht_preamble_syms, num_payload_syms;

    u16 t_preamble;
    u16 t_sym;

    // Note: the t_ext signal extension represent the value used in the standard, which in turn
    // is the value expected by other commercial WLAN devices. By default, the signal extensions
    // programmed into the PHY match this value.
    u16 t_ext = 6;

    // Set OFDM symbol duration in microseconds; only depends on PHY sampling rate
    switch(phy_samp_rate) {
        case PHY_40M:
            t_sym = 2;
        break;

        default:
        case PHY_20M:
            t_sym = 4;
        break;

        case PHY_10M:
            t_sym = 8;
        break;
    }

    // PHY preamble common to NONHT and HTMF waveforms consists of 5 OFDM symbols
    //  4 symbols for STF/LTF
    //  1 symbol for SIGNAL/L-SIG
    t_preamble = 5*t_sym;

    // Only HTMF waveforms have HT-SIG, HT-STF and HT-LTF symbols
    if(phy_mode == PHY_MODE_HTMF) {
    	num_ht_preamble_syms = 4;
    } else {
    	num_ht_preamble_syms = 0;
    }

    num_payload_syms = wlan_ofdm_calc_num_payload_syms(length, mcs, phy_mode);

    // Sum each duration and return
    return (t_preamble + (t_sym * (num_ht_preamble_syms + num_payload_syms)) + t_ext);
}


/*****************************************************************************/
/**
 * Calculates number of payload OFDM symbols in a packet. Implements
 *  ceil(payload_length_bits / num_bits_per_ofdm_sym)
 *   where num_bits_per_ofdm_sym is a function of the MCS index and PHY mode
 *
 * @param   length         - Length of MAC payload in bytes
 * @param   mcs            - MCS index
 * @param   phy_mode       - PHY waveform mode - either PHY_MODE_NONHT (11a/g) or PHY_MODE_HTMF (11n)
 *
 * @return  u16            - Number of OFDM symbols
 *****************************************************************************/
inline u16 wlan_ofdm_calc_num_payload_syms(u16 length, u8 mcs, u8 phy_mode) {
	u16 num_payload_syms;
	u32 num_payload_bits;
	u16 n_dbps;

    // Payload consists of:
    //  16-bit SERVICE field
    //  'length' byte MAC payload
    //  6-bit TAIL field
    num_payload_bits = 16 + (8 * length) + 6;

    // Num payload syms is ceil(num_payload_bits / N_DATA_BITS_PER_SYM). The ceil()
    //  operation implicitly accounts for any PAD bits in the waveform. The PHY inserts
    //  PAD bits to fill the final OFDM symbol. A waveform always consists of an integer
    //  number of OFDM symbols, so the actual number of PAD bits is irrelevant here
    n_dbps = wlan_mac_low_mcs_to_n_dbps(mcs, phy_mode);
    num_payload_syms = num_payload_bits / n_dbps;

	// Apply ceil()
	//  Integer div above implies floor(); increment result if floor() changed result
	if( (n_dbps * num_payload_syms) != num_payload_bits ) {
		num_payload_syms++;
	}

     return num_payload_syms;
}

