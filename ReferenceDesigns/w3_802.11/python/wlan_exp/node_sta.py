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

This module provides class definition for the Station (STA) WLAN Exp Node

Functions (see below for more information):
    WlanExpNodeSta() -- Class for WLAN Exp station

"""

from . import defaults
from . import node


__all__ = ['WlanExpNodeSta']


class WlanExpNodeSta(node.WlanExpNode):
    """802.11 Station (STA) for WLAN Experiment node."""
    
    def __init__(self, host_config=None):
        super(WlanExpNodeSta, self).__init__(host_config)

        # Set the correct WARPNet node type
        self.node_type = self.node_type + defaults.WLAN_EXP_HIGH_AP 
        self.node_type = self.node_type + defaults.WLAN_EXP_LOW_DCF


    def configure_node(self, jumbo_frame_support=False):
        """Get remaining information from the node and set remaining parameters."""
        # Call WarpNetNode apply_configuration method
        super(WlanExpNodeSta, self).configure_node(jumbo_frame_support)
        
        # Set description
        self.description = str("STA " + self.description)


    #-------------------------------------------------------------------------
    # WLAN Exp Commands for the Node
    #-------------------------------------------------------------------------


    #-------------------------------------------------------------------------
    # Misc methods for the Node
    #-------------------------------------------------------------------------
    def __str__(self):
        """Pretty print WlanExpNodeAp object"""
        msg = super(WlanExpNodeSta, self).__str__()
        msg = "WLAN Exp STA: \n" + msg
        return msg

    def __repr__(self):
        """Return node name and description"""
        msg = super(WlanExpNodeSta, self).__repr__()
        msg = "WLAN EXP STA " + msg
        return msg

# End Class WlanExpNodeSta
