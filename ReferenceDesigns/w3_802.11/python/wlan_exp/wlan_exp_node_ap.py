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

from . import wlan_exp_defaults
from . import wlan_exp_node


__all__ = ['WlanExpNodeAp']


class WlanExpNodeAp(wlan_exp_node.WlanExpNode):
    """802.11 Access Point (AP) for WLAN Experiment node."""
    
    def __init__(self):
        super(WlanExpNodeAp, self).__init__()

        # Set the correct WARPNet node type
        self.node_type = self.node_type + wlan_exp_defaults.WLAN_EXP_AP


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
        if not self.serial_number is None:
            print("WLAN Exp AP:")
            print("Node '{0}' with ID {1}:".format(self.name, self.node_id))
            print("    Desc    :  {0}".format(self.description))
            print("    Serial #:  W3-a-{0:05d}".format(self.serial_number))
        else:
            print("Node not initialized.")
        if not self.transport is None:
            print(self.transport)


    def __repr__(self):
        """Return node name and description"""
        return str("WLAN EXP AP  " + 
                   "W3-a-{0:05d}: ID {1:5d} ({2})".format(self.serial_number,
                                                          self.node_id,
                                                          self.name))


# End Class WlanExpNodeAp
