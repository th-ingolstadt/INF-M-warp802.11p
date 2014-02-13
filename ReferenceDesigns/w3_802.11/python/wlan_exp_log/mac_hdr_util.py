# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment MAC Header Utilities
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
1.00a pom  1/27/14  Initial release

------------------------------------------------------------------------------

This module provides utility functions when dealing with MAC headers within
802.11.

Functions:
    addr_is_bcast() -- Is the address the broadcast address
    addr_is_unicast() -- Is the address a unicast address
    mac_hdr_addr1() -- Return the address 1 field from the MAC header
    mac_hdr_addr2() -- Return the address 2 field from the MAC header
    mac_hdr_wireless_bcast() -- Is the MAC header wireless broadcast
    mac_hdr_wireless_unicast() -- Is the MAC header wirelss unicast

See this link for more information:
http://technet.microsoft.com/en-us/library/cc757419%28v=ws.10%29.aspx

"""

def addr_is_bcast(addr):
	return addr == 6*[255,]

def addr_is_unicast(addr):
	return addr != 6*[255,]

def get_addr1(hdr):
	return list(hdr)[4:10]

def get_addr2(hdr):
	return list(hdr)[10:16]

def dest_is_wireless_bcast(hdr):
	return addr_is_bcast(mac_hdr_addr1(hdr))

def dest_is_wireless_unicast(hdr):
	return not addr_is_bcast(mac_hdr_addr1(hdr))
