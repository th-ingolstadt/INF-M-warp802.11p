# -*- coding: utf-8 -*-
"""
.. ------------------------------------------------------------------------------
.. WLAN Experiment Node - Access Point (AP)
.. ------------------------------------------------------------------------------
.. Authors:   Chris Hunter (chunter [at] mangocomm.com)
..            Patrick Murphy (murphpo [at] mangocomm.com)
..            Erik Welsh (welsh [at] mangocomm.com)
.. License:   Copyright 2014-2015, Mango Communications. All rights reserved.
..            Distributed under the WARP license (http://warpproject.org/license)
.. ------------------------------------------------------------------------------
.. MODIFICATION HISTORY:
..
.. Ver   Who  Date     Changes
.. ----- ---- -------- -----------------------------------------------------
.. 1.00a ejw  1/23/14  Initial release
.. ------------------------------------------------------------------------------

"""

import wlan_exp.node as node
import wlan_exp.cmds as cmds


__all__ = ['WlanExpNodeAp']


class WlanExpNodeAp(node.WlanExpNode):
    """802.11 Access Point (AP) functionality for a WLAN Experiment node.
    
    Args:
        network_config (transport.NetworkConfiguration) : Network configuration of the node
        mac_type (int)                                  : CPU Low MAC type
    """

    #-------------------------------------------------------------------------
    # Override WLAN Exp Node Commands
    #-------------------------------------------------------------------------
    def configure_bss(self, bssid=False, ssid=None, channel=None, beacon_interval=False):
        """Configure the BSS information of the node
        
        Each node is either a member of no BSS (colloquially "unassociated") 
        or a member of one BSS.  A node requires a minimum valid bss_info to 
        be a member of a BSS. The minimum valid bss_info has:
            #. BSSID: 48-bit MAC address (must be either None or AP's wlan_mac_address)
            #. Channel: Logical channel for Tx/Rx by BSS members
            #. SSID: Variable length string (ie the name of the network)
            #. Beacon Interval:  Interval (in TUs) for beacons
            
        This method is used to manipulate node parameters that affect BSS state
        
        Args:
            ssid (str):  SSID string (Must be 32 characters or less)
            channel (int): Channel number on which the BSS operates
            beacon_interval (int): Integer number of beacon Time Units in [10, 65535]
                (http://en.wikipedia.org/wiki/TU_(Time_Unit); a TU is 1024 microseconds);
                A value of None will disable beacons;  A value of False will not 
                update the current beacon interval.
        
        ..note::  For the AP, the bssid is not configurable and will always be
            the MAC address of the node.
        """
        if bssid is not None:
            if bssid:
                if (bssid != self.wlan_mac_address):
                    raise AttributeError("BSSID must be either None or the wlan_mac_address of the node.")
        
        self.send_cmd(cmds.NodeConfigBSS(bssid=bssid, ssid=ssid, channel=channel, beacon_interval=beacon_interval))
        
        
    def enable_beacon_mac_time_update(self, enable):
        """Enable / Disable MAC time update from beacons
        
        ..note:: Raises NotImplementedError().  Current AP implementation does 
            not support updating MAC time from beacon receptions
            
        Args:
            enable (bool):  True - enable MAC time updates from beacons
                            False - disable MAC time updates from beacons
        
        """
        raise NotImplementedError("Current AP implementation does not support updating MAC time from beacon receptions")
        

        
    #-------------------------------------------------------------------------
    # Override Internal WLAN Exp Node methods
    #-------------------------------------------------------------------------
    def _check_allowed_rate(self, mcs, phy_mode, verbose=False):
        """Check that rate parameters are allowed

        Args:
            mcs (int):           Modulation and coding scheme (MCS) index
            phy_mode (str, int): PHY mode (from util.phy_modes)

        Returns:
            valid (bool):  Are all parameters valid?
        """

        # TODO: implement AP-specific rate checking here
        #  Allow all supported rates for now

        return self._check_supported_rate(mcs, phy_mode, verbose)



    #-------------------------------------------------------------------------
    # AP specific WLAN Exp Commands 
    #-------------------------------------------------------------------------
    def enable_DTIM_multicast_buffering(self, enable):
        """Enable / Disable DTIM buffering of multicast data

        The Delivery Traffic Indication Map (DTIM) keeps track of STA sleep
        states and will buffer traffic for the node based on those sleep 
        states.  When an AP is configured with enable_DTIM_multicast_buffering(False), 
        it will include the multicast queue in the normal polling of queues, 
        independent of any STA sleep states.

        Args:
            enable (bool):  True - enable DTIM multicast buffering
                            False - disable DTIM multicast buffering
                            (Default value on Node: True)
        """
        self.send_cmd(cmds.NodeAPConfigure(dtim_multicast_buffering=enable))
    

    def set_authentication_address_filter(self, allow):
        """Command to set the authentication address filter on the node.
        
        Args:
            allow (list of tuple) List of (address, mask) tuples that will be used to filter 
                addresses on the node.
    
        .. note::  For the mask, bits that are 0 are treated as "any" and bits that are 1 are 
            treated as "must equal".  For the address, locations of one bits in the mask 
            must match the incoming addresses to pass the filter.
        """
        filters = []

        if (type(allow) is not list):
            allow = [allow]
        
        for value in allow:
            if type(value[0]) in [int, long]:
                filters.append(value)
            elif type(value[0]) is str:
                try:
                    import wlan_exp.util as util                    
                    filters.append((util.str_to_mac_addr(value[0]), value[1]))
                    
                except TypeError:
                    raise TypeError("MAC address is not valid")
            else:
                raise TypeError("MAC address is not valid")
        
        self.send_cmd(cmds.NodeAPSetAuthAddrFilter(filters))


    def add_association(self, device_list, allow_timeout=None):
        """Command to add an association to each device in the device list.
        
        Args:
            device_list (list of WlanExpNode / WlanDevice):  List of 802.11 devices 
                or single 802.11 device to add to the association table
            allow_timeout (bool, optional):  Allow the association to timeout if inactive
        
        .. note::  This adds an association with the default tx/rx params.  If
            allow_timeout is not specified, the default on the node is to 
            not allow timeout of the association.

        .. note::  If the device is a WlanExpNodeSta, then this method will also
            add the association to that device.
        
        .. note::  The add_association method will bypass any association address filtering
            on the node.  One caveat is that if a device sends a de-authentication packet
            to the AP, the AP will honor it and completely remove the device from the 
            association table.  If the association address filtering is such that the
            device is not allowed to associate, then the device will not be allowed back
            on the AP even though at the start of the experiment the association was 
            explicitly added.
        """
        ret_val = []
        ret_list = True
        
        # Convert entries to a list if it is not already one
        if type(device_list) is not list:
            device_list = [device_list]
            ret_list    = False

        # Get the AP's current channel, SSID
        bss_info = self.get_bss_info()
        
        if bss_info is None:
            msg  = "\n    Cannot add association:  BSS not configured on AP."
            msg += "\n    Please configure the BSS using configure_bss() before adding associations."
            raise Exception(msg)
        
        bssid    = bss_info['bssid']
        channel  = bss_info['channel']
        ssid     = bss_info['ssid']
        
        for device in device_list:
            ret_val.append(self._add_association(device=device, bssid=bssid, channel=channel, ssid=ssid, allow_timeout=allow_timeout))

        # Need to return a single value and not a list
        if not ret_list:
            ret_val = ret_val[0]
        
        return ret_val
        


    #-------------------------------------------------------------------------
    # Internal AP methods
    #-------------------------------------------------------------------------
    def _add_association(self, device, bssid, channel, ssid, allow_timeout):
        """Internal command to add an association."""
        ret_val = False

        import wlan_exp.node_ibss as node_ibss
        
        if isinstance(device, node_ibss.WlanExpNodeIBSS):
            print("WARNING:  Could not add association for IBSS node '{0}'".format(device.description))
            return ret_val

        aid = self.send_cmd(cmds.NodeAPAddAssociation(device, allow_timeout))
        
        if (aid != cmds.CMD_PARAM_ERROR):
            import wlan_exp.node_sta as node_sta

            if isinstance(device, node_sta.WlanExpNodeSta):
                device.configure_bss(bssid=bssid, ssid=ssid, channel=channel)
                device.set_aid(aid=aid)
                ret_val = True
            else:
                msg  = "\nWARNING:  Could not add association to non-STA node '{0}'\n".format(device.description) 
                msg += "    Please add the association to the AP manually on the device.\n"
                print(msg)
                ret_val = True
            
        return ret_val



    #-------------------------------------------------------------------------
    # Misc methods for the Node
    #-------------------------------------------------------------------------
    def __str__(self):
        """Pretty print WlanExpNodeAp object"""
        msg = ""

        if self.serial_number is not None:
            from wlan_exp.util import mac_addr_to_str
            msg += "WLAN EXP AP Node:\n"
            msg += "    WLAN MAC addr :  {0}\n".format(mac_addr_to_str(self.wlan_mac_address))
            msg += "    Node ID       :  {0}\n".format(self.node_id)
            msg += "    Serial #      :  {0}\n".format(self.sn_str)
            msg += "    HW version    :  WARP v{0}\n".format(self.hw_ver)
        else:
            msg += "Node not initialized."

        if self.transport is not None:
            msg += "WLAN EXP "
            msg += str(self.transport)

        return msg


    def __repr__(self):
        """Return node name and description"""
        msg = super(WlanExpNodeAp, self).__repr__()
        msg = "WLAN EXP AP   " + msg
        return msg

# End Class WlanExpNodeAp
