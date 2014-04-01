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



WLAN_DEVICE_FLAG_DISABLE_ASSOC_CHECK             = 0x0001
WLAN_DEVICE_FLAG_NEVER_REMOVE                    = 0x0002


class WlanDevice(object):
    """Base Class for WLAN Device.
    
    The WLAN device represents one 802.11 device in a WLAN network.  
    
    Attributes:
        device_type       -- Unique type of the Wlan Device
        description       -- String description of this node (user generated)

        wlan_mac_address  -- MAC Address of WLAN Device
        host_name         -- Host name for this node (auto-populated from station info)
        assocations       -- List of WlanDevices that are associated with this device
    """
    device_type           = None
    description           = None
    
    wlan_mac_address      = None
    host_name             = None
    associations          = None
    
    def __init__(self, station_info=None, mac_address=None, host_name=None):
        pass


    #-------------------------------------------------------------------------
    # WLAN Commands for the Device
    #-------------------------------------------------------------------------

    def get_wlan_mac_address(self):
        """Get the wireless mac address of the 802.11 device."""
        return self.wlan_mac_address


    def get_host_name(self):
        """Get the host name of the 802.11 device."""
        return self.host_name


    def update_from_station_info(self, station_info):
        """Update the fields of the device from the station info."""
        pass


    def create_station_info(self, disable_association_check=False, never_remove=False):
        """Create a station info from the given WlanDevice."""
        pass
    
    

# End Class WlanDevice


