# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment Default Constants
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
MODIFICATION HISTORY:

Ver   Who  Date     Changes
----- ---- -------- -----------------------------------------------------
1.00a ejw  1/23/14  Initial release

------------------------------------------------------------------------------

This module provides WLAN Exp default constants.

"""

# Default Variables
PACKAGE_NAME                      = 'wlan_exp'


# WARPNet Node Types
#   NOTE:  The C counterparts are found in wlan_exp_common.h
WLAN_EXP_BASE                     = 0x10000
WLAN_EXP_HIGH_AP                  = 0x00100
WLAN_EXP_HIGH_STA                 = 0x00200

WLAN_EXP_LOW_DCF                  = 0x00001

WLAN_EXP_AP_TYPE                  = WLAN_EXP_BASE + WLAN_EXP_HIGH_AP + WLAN_EXP_LOW_DCF
WLAN_EXP_AP_CLASS                 = 'wlan_exp.wlan_exp_node_ap.WlanExpNodeAp'

WLAN_EXP_STA_TYPE                 = WLAN_EXP_BASE + WLAN_EXP_HIGH_STA + WLAN_EXP_LOW_DCF
WLAN_EXP_STA_CLASS                = 'wlan_exp.wlan_exp_node_sta.WlanExpNodeSta'


# WLAN Exp Transmit rate constants
WLAN_MAC_RATE_6M                  = 1
WLAN_MAC_RATE_9M                  = 2
WLAN_MAC_RATE_12M                 = 3
WLAN_MAC_RATE_18M                 = 4
WLAN_MAC_RATE_24M                 = 5
WLAN_MAC_RATE_36M                 = 6
WLAN_MAC_RATE_48M                 = 7
WLAN_MAC_RATE_54M                 = 8





