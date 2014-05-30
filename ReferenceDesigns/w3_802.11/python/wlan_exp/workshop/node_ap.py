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
    WlanExpNodeWorkshopAp() -- Class for workshop WLAN Exp access point

"""

import wlan_exp.defaults as defaults
import wlan_exp.node_ap as node_ap

import wlan_exp.workshop.node as wksp_node
import wlan_exp.workshop.defaults as wksp_defaults


__all__ = ['WlanExpNodeWorkshopAp']


class WlanExpNodeWorkshopAp(wksp_node.WlanExpWorkshopNode, node_ap.WlanExpNodeAp):
    """Workshop 802.11 Access Point (AP) for WLAN Experiment node."""
    
    def __init__(self, host_config=None):
        super(WlanExpNodeWorkshopAp, self).__init__(host_config)

        # Set the correct WARPNet node type
        self.node_type = self.node_type + defaults.WLAN_EXP_HIGH_AP
        self.node_type = self.node_type + wksp_defaults.WLAN_EXP_LOW_DCF_WORKSHOP


    def configure_node(self, jumbo_frame_support=False):
        """Get remaining information from the node and set remaining parameters."""
        # Call WarpNetNode apply_configuration method
        super(WlanExpNodeWorkshopAp, self).configure_node(jumbo_frame_support)
        
        # Set description
        self.description = str("Workshop AP  " + self.description)


    #-------------------------------------------------------------------------
    # WLAN Exp Commands for the AP
    #-------------------------------------------------------------------------



    #-------------------------------------------------------------------------
    # Misc methods for the Node
    #-------------------------------------------------------------------------
    def __str__(self):
        """Pretty print WlanExpNodeAp object"""
        msg = super(WlanExpNodeWorkshopAp, self).__str__()
        msg = "Workshop WLAN Exp AP: \n" + msg
        return msg

    def __repr__(self):
        """Return node name and description"""
        msg = super(WlanExpNodeWorkshopAp, self).__repr__()
        msg = "Workshop WLAN EXP AP  " + msg
        return msg

# End Class
