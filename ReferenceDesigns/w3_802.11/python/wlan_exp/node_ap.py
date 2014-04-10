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

import wlan_exp.defaults   as defaults
import wlan_exp.node       as node


__all__ = ['WlanExpNodeAp']


class WlanExpNodeAp(node.WlanExpNode):
    """802.11 Access Point (AP) for WLAN Experiment node."""
    
    def __init__(self, host_config=None):
        super(WlanExpNodeAp, self).__init__(host_config)        

        # Set the correct WARPNet node type
        self.node_type = self.node_type + defaults.WLAN_EXP_HIGH_AP
        self.node_type = self.node_type + defaults.WLAN_EXP_LOW_DCF


    def configure_node(self, jumbo_frame_support=False):
        """Get remaining information from the node and set remaining parameters."""
        # Call WarpNetNode apply_configuration method
        super(WlanExpNodeAp, self).configure_node(jumbo_frame_support)
        
        # Set description
        self.description = str("AP  " + self.description)


    #-------------------------------------------------------------------------
    # WLAN Exp Commands for the AP
    #-------------------------------------------------------------------------


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
