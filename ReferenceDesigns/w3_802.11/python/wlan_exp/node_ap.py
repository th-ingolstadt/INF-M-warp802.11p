# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment Node - Access Point (AP)
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

This module provides class definition for the Access Point (AP) WLAN Exp Node

Functions (see below for more information):
    WlanExpNodeAp() -- Class for WLAN Exp access point

"""

import wlan_exp.defaults as defaults
import wlan_exp.node as node
import wlan_exp.cmds as cmds


__all__ = ['WlanExpNodeAp']


class WlanExpNodeAp(node.WlanExpNode):
    """802.11 Access Point (AP) for WLAN Experiment node."""
    
    def __init__(self, network_config=None):
        super(WlanExpNodeAp, self).__init__(network_config)

        # Set the correct WARPNet node type
        self.node_type = self.node_type + defaults.WLAN_EXP_HIGH_AP
        self.node_type = self.node_type + defaults.WLAN_EXP_LOW_DCF

        self.device_type = self.node_type


    def configure_node(self, jumbo_frame_support=False):
        """Get remaining information from the node and set remaining parameters."""
        # Call WarpNetNode apply_configuration method
        super(WlanExpNodeAp, self).configure_node(jumbo_frame_support)
        
        # Set description
        self.description = str("AP  " + self.description)


    #-------------------------------------------------------------------------
    # WLAN Exp Commands for the AP
    #-------------------------------------------------------------------------
    def set_authentication_address_filter(self, allow):
        """Command to set the authentication address filter on the node.
        
        Attributes:
            allow  -- List of (address, mask) tuples that will be used to filter addresses
                      on the node.
    
        NOTE:  For the mask, bits that are 0 are treated as "any" and bits that are 1 are 
        treated as "must equal".  For the address, locations of one bits in the mask 
        must match the incoming addresses to pass the filter.
        """
        if (type(allow) is list):
            self.send_cmd(cmds.NodeAPSetAuthAddrFilter(allow))
        else:
            self.send_cmd(cmds.NodeAPSetAuthAddrFilter([allow]))


    def get_ssid(self):
        """Command to get the SSID of the AP."""
        return self.send_cmd(cmds.NodeAPProcSSID())


    def set_ssid(self, ssid):
        """Command to set the SSID of the AP."""
        return self.send_cmd(cmds.NodeAPProcSSID(ssid))


    def add_association(self, device_list, allow_timeout=None):
        """Command to add an association to each device in the device list.
        
        Attributes:
            device_list   -- Device(s) to add to the association table
            allow_timeout -- Allow the association to timeout if inactive
        
        NOTE:  This adds an association with the default tx/rx params.  If
            allow_timeout is not specified, the default on the node is to 
            not allow timeout of the association.

        NOTE:  If the device is a WlanExpNodeSta, then this method will also
            add the association to that device.
        
        NOTE:  The add_association method will bypass any association address filtering
            on the node.  One caveat is that if a device sends a de-authentication packet
            to the AP, the AP will honor it and completely remove the device from the 
            association table.  If the association address filtering is such that the
            device is not allowed to associate, then the device will not be allowed back
            on the AP even though at the start of the experiment the association was 
            explicitly added.
        """
        ret_val = []
        
        try:
            for device in device_list:
                ret_val.append(self._add_association(device, allow_timeout))
        except TypeError:
            ret_val.append(self._add_association(device_list, allow_timeout))
        
        return ret_val
        

    def _add_association(self, device, allow_timeout):
        """Internal command to add an association."""
        ret_val = False

        aid = self.send_cmd(cmds.NodeAPAssociate(device))
        
        if (aid != cmds.CMD_PARAM_ERROR):
            import wlan_exp.node_sta as node_sta

            if isinstance(device, node_sta.WlanExpNodeSta):
                if device.send_cmd(cmds.NodeSTAAssociate(self, aid)):
                    ret_val = True
            else:
                ret_val = True
            
        return ret_val



    #-------------------------------------------------------------------------
    # Misc methods for the Node
    #-------------------------------------------------------------------------
    def __str__(self):
        """Pretty print WlanExpNodeAp object"""
        msg = super(WlanExpNodeAp, self).__str__()
        msg = "WLAN Exp AP: \n" + msg
        return msg

    def __repr__(self):
        """Return node name and description"""
        msg = super(WlanExpNodeAp, self).__repr__()
        msg = "WLAN EXP AP  " + msg
        return msg

# End Class WlanExpNodeAp
