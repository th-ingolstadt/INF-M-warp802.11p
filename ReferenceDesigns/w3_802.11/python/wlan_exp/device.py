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
    def create_bss_info(self, ssid, channel, bssid=None, beacon_interval=1, ibss_status=False):
        """Create a Basic Service Set (BSS) information structure.
        
        This method will create a dictionary that contains all necessary information 
        for a BSS for the device.

        Attributes:
            ssid              -- String of the SSID
            bssid             -- 40-bit ID of the BSS
            channel           -- Channel on which the BSS operates 
            beacon_interval   -- Integer number of time units for beacon intervals
                                 (a time unit is 1024 microseconds)    
            ibss_status       -- True  => Capabilities field = 0x2 (BSS info is for IBSS)
                                 False => Capabilities field = 0x1 (BSS info is for BSS)
        """
        import wlan_exp.util as util
        
        bss_dict = {}
        channel_error = False

        bss_dict['name']             = self.name        
        bss_dict['wlan_mac_address'] = self.wlan_mac_address

        # Check SSID
        if type(ssid) is not str:
            raise ValueError("The SSID must be a string.")

        bss_dict['ssid'] = ssid

        # Check BSSID
        if bssid is not None:
            bss_dict['bssid'] = bssid
        else:
            bss_dict['bssid'] = self.wlan_mac_address

        # Check Channel
        if type(channel) is int:
            channel = util.find_channel_by_channel_number(channel)

            if channel is None: channel_error = True                

        elif type(channel) is dict:
            pass

        else:
            channel_error = True

        if not channel_error:
            bss_dict['channel'] = channel
        else:        
            msg  = "The channel must either be a channel number or a wlan_exp.util.wlan_channel entry."
            raise ValueError(msg)

        # Check beacon interval
        if type(beacon_interval) is not int:
            beacon_interval = int(beacon_interval)
            print("WARNING:  Beacon interval must be an interger number of time units rounding to {0}".format(beacon_interval))

        bss_dict['beacon_interval'] = beacon_interval

        # Check IBSS status
        if type(ibss_status) is not bool:
            raise ValueError("The ibss_status must be a boolean.")

        bss_dict['ibss_status'] = ibss_status

        
        return bss_dict


# End Class WlanDevice


