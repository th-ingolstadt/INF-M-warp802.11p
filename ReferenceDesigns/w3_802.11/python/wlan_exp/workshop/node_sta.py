# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment Node - Station (STA)
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

This module provides class definition for the Workshop Station (STA) WLAN Exp Node

Functions (see below for more information):
    WlanExpNodeWorkshopSta() -- Class for Workshop WLAN Exp station

"""

import wlan_exp.defaults as defaults
import wlan_exp.node_sta as node_sta

import wlan_exp.workshop.node as wksp_node
import wlan_exp.workshop.defaults as wksp_defaults


__all__ = ['WlanExpNodeWorkshopSta']


class WlanExpNodeWorkshopSta(wksp_node.WlanExpWorkshopNode, node_sta.WlanExpNodeSta):
    """Workshop 802.11 Station (STA) for WLAN Experiment node."""
    
    def __init__(self, host_config=None):
        super(WlanExpNodeWorkshopSta, self).__init__(host_config)

        # Set the correct WARPNet node type
        self.node_type = self.node_type + defaults.WLAN_EXP_HIGH_STA 
        self.node_type = self.node_type + wksp_defaults.WLAN_EXP_LOW_DCF_WORKSHOP


    def configure_node(self, jumbo_frame_support=False):
        """Get remaining information from the node and set remaining parameters."""
        # Call WarpNetNode apply_configuration method
        super(WlanExpNodeWorkshopSta, self).configure_node(jumbo_frame_support)
        
        # Set description
        self.description = str("Workshop STA " + self.description)


    #-------------------------------------------------------------------------
    # WLAN Exp Commands for the Node
    #-------------------------------------------------------------------------



    #-------------------------------------------------------------------------
    # Misc methods for the Node
    #-------------------------------------------------------------------------
    def __str__(self):
        """Pretty print WlanExpNodeAp object"""
        msg = super(WlanExpNodeWorkshopSta, self).__str__()
        msg = "Workshop WLAN Exp STA: \n" + msg
        return msg

    def __repr__(self):
        """Return node name and description"""
        msg = super(WlanExpNodeWorkshopSta, self).__repr__()
        msg = "Workshop WLAN Exp STA: \n" + msg
        return msg

# End Class
