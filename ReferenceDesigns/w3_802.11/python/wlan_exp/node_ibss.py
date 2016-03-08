# -*- coding: utf-8 -*-
"""
.. ------------------------------------------------------------------------------
.. WLAN Experiment Node - IBSS Node
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


__all__ = ['WlanExpNodeIBSS']


class WlanExpNodeIBSS(node.WlanExpNode):
    """802.11 IBSS functionality for a WLAN Experiment node.
    
    Args:
        network_config (transport.NetworkConfiguration) : Network configuration of the node
        mac_type (int)                                  : CPU Low MAC type
    """

    #-------------------------------------------------------------------------
    # Override WLAN Exp Node Commands
    #-------------------------------------------------------------------------
    def counts_get_txrx(self, device_list=None, return_zeroed_counts_if_none=True):
        """Get the counts from the node.

        .. note:: This function has the same implementation as WlanExpNode but 
            different default values.
        
        Args:
            device_list (list of WlanExpNode, WlanExpNode, WlanDevice, optional): List of devices
                for which to get counts.  See note below for more information.
            return_zeroed_counts_if_none(bool, optional):  If no counts exist on the node for 
                the specified device(s), return a zeroed counts dictionary with proper timestamps
                instead of None.
        
        Returns:
            counts_dictionary (list of dictionaries, dictionary): Counts for the device(s) specified. 

        .. note:: If the device_list is a single device, then a single dictionary or 
            None is returned.  If the device_list is a list of devices, then a 
            list of dictionaries will be returned in the same order as the 
            devices in the list.  If any of the staistics are not there, 
            None will be inserted in the list.  If the device_list is not 
            specified, then all the counts on the node will be returned.
        """
        return super(WlanExpNodeIBSS, self).counts_get_txrx(device_list, return_zeroed_counts_if_none)


    def configure_bss(self, bssid=False, ssid=None, channel=None, beacon_interval=False):
        """Configure the BSS information of the node
        
        Each node is either a member of no BSS (colloquially "unassociated") 
        or a member of one BSS.  A node requires a minimum valid bss_info to 
        be a member of a BSS. The minimum valid bss_info has:
            #. BSSID: 48-bit MAC address
            #. Channel: Logical channel for Tx/Rx by BSS members
            #. SSID: Variable length string (ie the name of the network)
            #. Beacon Interval:  Interval (in TUs) for beacons
            
        This method is used to manipulate node parameters that affect BSS state
        
        Args:
            bssid (int, str):  48-bit ID of the BSS either as a integer or 
                colon delimited string of the form:  XX:XX:XX:XX:XX:XX
            ssid (str):  SSID string (Must be 32 characters or less)
            channel (int): Channel number on which the BSS operates
            beacon_interval (int): Integer number of beacon Time Units in [10, 65535]
                (http://en.wikipedia.org/wiki/TU_(Time_Unit); a TU is 1024 microseconds);
                A value of None will disable beacons;  A value of False will not 
                update the current beacon interval.
        
        ..note::  For the AP, the bssid is not configurable and will always be
            the MAC address of the node.
        """
        self.send_cmd(cmds.NodeConfigBSS(bssid=bssid, ssid=ssid, channel=channel, beacon_interval=beacon_interval))


    def disassociate(self, device_list=None):
        """Remove associations of devices within the device_list from the association table

        .. note:: This command is not supported by IBSS nodes.  Please use diassociate_all().
            This function will raise a NotImplementedError.
        
        Attributes:
            device_list (list of WlanExpNode / WlanDevice):  List of 802.11 devices or single 
                802.11 device for which to disassociate
        """
        msg = "ERROR:  disassociate(device_list) is not supported for IBSS nodes.  Please use disassociate_all()."
        raise NotImplementedError(msg)



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
        # TODO: implement IBSS-specific rate checking here
        #  Allow all supported rates for now

        return self._check_supported_rate(mcs, phy_mode, verbose)



    #-------------------------------------------------------------------------
    # IBSS specific WLAN Exp Commands 
    #-------------------------------------------------------------------------



    #-------------------------------------------------------------------------
    # Internal IBSS methods
    #-------------------------------------------------------------------------



    #-------------------------------------------------------------------------
    # Misc methods for the Node
    #-------------------------------------------------------------------------
    def __str__(self):
        """Pretty print WlanExpNodeIBSS object"""
        msg = ""

        if self.serial_number is not None:
            from wlan_exp.util import mac_addr_to_str
            msg += "WLAN EXP IBSS Node:\n"
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
        msg = super(WlanExpNodeIBSS, self).__repr__()
        msg = "WLAN EXP IBSS " + msg
        return msg

# End Class WlanExpNodeAp
