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
    def configure_bss(self, bssid=False, ssid=None, channel=None, beacon_interval=False, ht_capable=None):
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
            bssid (int, str):  48-bit ID of the BSS either None or 
                the wlan_mac_address of the node
            ssid (str):  SSID string (Must be 32 characters or less)
            channel (int): Channel number on which the BSS operates
            beacon_interval (int): Integer number of beacon Time Units in [10, 65535]
                (http://en.wikipedia.org/wiki/TU_(Time_Unit); a TU is 1024 microseconds);
                A value of None will disable beacons;  A value of False will not 
                update the current beacon interval.
            ht_capable (bool):  Is the PHY mode HTMF (True) or NONHT (False)?
        
        ..note::  For the AP, the bssid is not configurable and should always be
            the wlan_mac_address of the node.
        """
        if bssid is not None:
            if bssid is not False:
                # User supplied a not-None BSSID argument
                error = False
                
                if type(bssid) in [int, long]:
                    if (bssid != self.wlan_mac_address):
                        error = True
                elif type(bssid) is str:
                    import wlan_exp.util as util
                    try:
                        if (util.str_to_mac_addr(bssid) != self.wlan_mac_address):
                            error = True
                    except:
                        error = True
            
                if (error):
                    raise AttributeError("BSSID must be either None or the wlan_mac_address of the node.")
            else:
                # User did not provide a BSSID argument - use the AP's MAC address
                bssid = self.wlan_mac_address
        
        self.send_cmd(cmds.NodeConfigBSS(bssid=bssid, ssid=ssid, channel=channel, 
                                         beacon_interval=beacon_interval, ht_capable=ht_capable))
        
        
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

        This command will reset the current address filter and then set the 
        address filter to the values in the allow list.  The filter only affects
        over-the-air associations.  Assocaitions created by WLAN Exp will 
        bypass the filter.
        
        Clients will be allowed to associate if they pass any of the filters
        that are set.
        
        Args:
            allow (list of tuple):  List of (address, mask) tuples that will be 
                used to filter addresses on the node.  A tuple can be substituted 
                with a predefined string:  "NONE", "ALL", or "MANGO-W3"
    
        .. note::  For the mask, bits that are 0 are treated as "any" and bits 
            that are 1 are treated as "must equal".  For the address, locations 
            of one bits in the mask must match the incoming addresses to pass the 
            filter.
        
        **Examples:**
        
        * **Allow a single client**:
            To only accept a single client with a given MAC address, say '01:23:45:67:89:AB'.
            ::        
                n_ap.set_authentication_address_filter(allow=('01:23:45:67:89:AB', 'FF:FF:FF:FF:FF:FF'))
                n_ap.set_authentication_address_filter(allow=(0x0123456789AB, 0xFFFFFFFFFFFF))

        * **Allow a range of clients**:
            To only accept clients with MAC address starint with '01:23:45'.
            ::        
                n_ap.set_authentication_address_filter(allow=('01:23:45:00:00:00', 'FF:FF:FF:00:00:00'))
                n_ap.set_authentication_address_filter(allow=(0x012345000000, 0xFFFFFF000000))
        
        * **Allow multiple ranges of clients**:
            To only accept clients with MAC address starint with '01:23:45' and '40'
            ::        
                n_ap.set_authentication_address_filter(allow=[('01:23:45:00:00:00', 'FF:FF:FF:00:00:00'),
                                                              ('40:00:00:00:00:00', 'FF:00:00:00:00:00')])
                n_ap.set_authentication_address_filter(allow=[(0x012345000000, 0xFFFFFF000000),
                                                              (0x400000000000, 0xFF0000000000)])
        
        * **Pre-defined filters**:
            Pre-defined strings can be used instead of specific ranges.
            ::        
                n_ap.set_authentication_address_filter(allow='NONE')     # Same as allow=(0x000000000000, 0xFFFFFFFFFFFF)
                n_ap.set_authentication_address_filter(allow='ALL')      # Same as allow=(0x000000000000, 0x000000000000)
                n_ap.set_authentication_address_filter(allow='MANGO-W3') # Same as allow=(0x40d855402000, 0xFFFFFFFFF000)

        """
        filters = []

        if (type(allow) is not list):
            allow = [allow]
        
        for value in allow:
            # Process pre-defined strings
            if type(value) is str:
                if   (value == 'NONE'):
                    filters.append((0x000000000000, 0xFFFFFFFFFFFF))
                elif (value == 'ALL'):
                    filters.append((0x000000000000, 0x000000000000))
                elif (value == 'MANGO-W3'):
                    filters.append((0x40d855402000, 0xFFFFFFFFF000))
                else:
                    msg  = "\n    String '{0}' not recognized.".format(value)
                    msg += "\n    Please use 'NONE', 'ALL', 'MANGO-W3' or a (address, mask) tuple"
                    raise AttributeError(msg)
            
            elif type(value) is tuple:
                import wlan_exp.util as util

                # Process address                
                if type(value[0]) in [int, long]:
                    address = value[0]
                elif type(value[0]) is str:
                    try:
                        address = util.str_to_mac_addr(value[0])
                    except:
                        raise AttributeError("Address {0} is not valid".format(value[0]))
                else:
                    raise AttributeError("Address type {0} is not valid".format(type(value[0])))

                # Process mask
                if type(value[1]) in [int, long]:
                    mask = value[1]
                elif type(value[1]) is str:
                    try:
                        mask = util.str_to_mac_addr(value[1])
                    except:
                        raise AttributeError("Mask {0} is not valid".format(value[1]))
                else:
                    raise AttributeError("Mask type {0} is not valid".format(type(value[1])))
                
                filters.append((address, mask))
                
            else:
                msg  = "\n    Value {0} with type {1} not recognized.".format(value, type(value))
                msg += "\n    Please use 'NONE', 'ALL', 'MANGO-W3' or a (address, mask) tuple"
                raise AttributeError(msg)
        
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
