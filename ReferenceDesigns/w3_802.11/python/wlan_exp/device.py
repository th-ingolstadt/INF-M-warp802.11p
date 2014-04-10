# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Device
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

This module provides class definition for WLAN Device.

Functions (see below for more information):
    WlanDevice()        -- Base class for WLAN Devices

Integer constants:
    TBD

"""
__all__ = ['WlanDevice']



class WlanDevice(object):
    """Base Class for WLAN Device.
    
    The WLAN device represents one 802.11 device in a WLAN network.  
    
    Attributes:
        device_type       -- Unique type of the Wlan Device
        name              -- String description of this node (user generated)

        wlan_mac_address  -- MAC Address of WLAN Device
    """
    device_type           = None
    name                  = None
    
    wlan_mac_address      = None
    
    def __init__(self, mac_address, name=None):
        self.wlan_mac_address = mac_address
        self.name             = name


    #-------------------------------------------------------------------------
    # WLAN Commands for the Device
    #-------------------------------------------------------------------------

    def get_wlan_mac_address(self):
        """Get the wireless mac address of the 802.11 device."""
        return self.wlan_mac_address


    def get_name(self):
        """Get the name of the 802.11 device."""
        return self.host_name
    

# End Class WlanDevice


