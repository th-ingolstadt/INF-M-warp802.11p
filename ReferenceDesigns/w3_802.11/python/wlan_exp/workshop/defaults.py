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
import wlan_exp.defaults as defaults


# Default Variables
PACKAGE_NAME                      = 'wlan_exp.workshop'


# WARPNet Node Types
#   NOTE:  The C counterparts are found in wlan_mac_dcf_workshop.h
WLAN_EXP_LOW_DCF_WORKSHOP         = 0x00002


WLAN_EXP_WORKSHOP_AP_TYPE         = defaults.WLAN_EXP_BASE + defaults.WLAN_EXP_HIGH_AP + WLAN_EXP_LOW_DCF_WORKSHOP
WLAN_EXP_WORKSHOP_AP_CLASS        = 'node_ap.WlanExpNodeWorkshopAp'
WLAN_EXP_WORKSHOP_AP_DESCRIPTION  = 'Workshop WLAN Exp (AP/DCF) '

WLAN_EXP_WORKSHOP_STA_TYPE        = defaults.WLAN_EXP_BASE + defaults.WLAN_EXP_HIGH_STA + WLAN_EXP_LOW_DCF_WORKSHOP
WLAN_EXP_WORKSHOP_STA_CLASS       = 'node_sta.WlanExpNodeWorkshopSta'
WLAN_EXP_WORKSHOP_STA_DESCRIPTION = 'Workshop WLAN Exp (STA/DCF)'




