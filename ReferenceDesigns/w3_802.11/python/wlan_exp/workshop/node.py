# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment Node
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

This module provides class definition for WLAN Exp Node.

Functions (see below for more information):
    WlanExpNode() -- Base class for WLAN Exp node
    WlanExpNodeFactory() -- Base class for creating WLAN Exp nodes

Integer constants:
    NODE_WLAN_MAX_ASSN, NODE_WLAN_EVENT_LOG_SIZE, NODE_WLAN_MAX_STATS
        -- Node hardware parameter constants 

If additional hardware parameters are needed for sub-classes of WlanExpNode, 
pleasemake sure that the values of these hardware parameters are not reused.

"""

import wlan_exp.workshop.defaults as defaults
import wlan_exp.workshop.cmds as cmds
import wlan_exp.node as node


__all__ = ['WlanExpWorkshopNode', 'WlanExpWorkshopNodeFactory']


class WlanExpWorkshopNode(node.WlanExpNode):
    """Base Class for Workshop WLAN Experiment node.
    
    New Attributes:
        None
    """
    def __init__(self, host_config=None):
        super(WlanExpWorkshopNode, self).__init__(host_config)


    def configure_node(self, jumbo_frame_support=False):
        """Get remaining information from the node and set remaining parameters."""
        # Call WarpNetNode apply_configuration method
        super(WlanExpWorkshopNode, self).configure_node(jumbo_frame_support)
        
        # Set description
        self.description = str("Workshop " + self.description)


    #-------------------------------------------------------------------------
    # WLAN Exp Commands for the Node
    #-------------------------------------------------------------------------

    #--------------------------------------------
    # Configure Node Attribute Commands
    # Deprecated - I'm leaving this in for code compatibility with what is already written
    #--------------------------------------------
    def set_workshop_params(self, enable, ext_pkt_detect_en, retrans_switch_thresh, mode_1, mode_2):
        """Sets the physical carrier sense threshold of the node."""
        values = []

        # Create top level config word
        temp   = 0
        
        if (enable):
            temp += (cmds.CMD_PARAM_ENABLE << cmds.CMD_PARAM_ENABLE_POS)
        
        if (ext_pkt_detect_en):
            temp += (cmds.CMD_PARAM_ENABLE << cmds.CMD_PARAM_EXT_PKT_DETECT_EN_POS)

        # Check the retransmission switch threshold argument
        if (retrans_switch_thresh > cmds.CMD_PARAM_TX_RETRANS_MAX):
            msg  = "WARNING:  Max Tx retransmissions is {0}, ".format(cmds.CMD_PARAM_TX_RETRANS_MAX)
            msg += "provided {1}.  Setting to max.".format(retrans_switch_thresh)
            print(msg)
            retrans_switch_thresh = cmds.CMD_PARAM_TX_RETRANS_MAX
            
        if (retrans_switch_thresh < cmds.CMD_PARAM_TX_RETRANS_MIN):        
            msg  = "WARNING:  Min Tx retransmissions is {0}, ".format(cmds.CMD_PARAM_TX_RETRANS_MIN)
            msg += "provided {1}.  Setting to min.".format(retrans_switch_thresh)
            print(msg)
            retrans_switch_thresh = cmds.CMD_PARAM_TX_RETRANS_MIN

        temp += (retrans_switch_thresh << cmds.CMD_PARAM_RETRANS_SWITCH_THRESHOLD_POS)
            
        values.append(temp)
        values = values + mode_1.serialize()
        values = values + mode_2.serialize()
        
        self._set_low_param(cmds.CMD_PARAM_LOW_PARAM_WORKSHOP_CONFIG, values)
        
    #--------------------------------------------
    # Configure Node Attribute Commands
    #--------------------------------------------
    def set_workshop_params_v2(self, wkshpmode, ext_pkt_detect_en, ant, coop):
        """."""
        values = []

        # Create top level config word
        temp   = 0
        
        if (wkshpmode == cmds.CMD_PARAM_MIMO):
            temp += (cmds.CMD_PARAM_MIMO << cmds.CMD_PARAM_ENABLE_POS)

        if (wkshpmode == cmds.CMD_PARAM_COOP):
            temp += (cmds.CMD_PARAM_COOP << cmds.CMD_PARAM_ENABLE_POS)            
        
        if (ext_pkt_detect_en):
            temp += (cmds.CMD_PARAM_MIMO << cmds.CMD_PARAM_EXT_PKT_DETECT_EN_POS)
            
        values.append(temp)
        values = values + ant.serialize()
        values = values + coop.serialize()
        
        self._set_low_param(cmds.CMD_PARAM_LOW_PARAM_WORKSHOP_CONFIG, values)


    #--------------------------------------------
    # Internal helper methods to configure node attributes
    #--------------------------------------------


# End Class WlanExpNode




class WlanExpWorkshopNodeFactory(node.WlanExpNodeFactory):
    """Sub-class of WLAN Exp node factory used to help with node configuration 
    and setup.
        
    Attributes (inherited):
        wn_dict -- Dictionary of WARPNet Node Types to class names
    """
    def __init__(self, host_config=None):
        super(WlanExpWorkshopNodeFactory, self).__init__(host_config)
        
        # Add default classes to the factory
        self.node_add_class(defaults.WLAN_EXP_WORKSHOP_AP_TYPE, 
                            defaults.WLAN_EXP_WORKSHOP_AP_CLASS,
                            defaults.WLAN_EXP_WORKSHOP_AP_DESCRIPTION)

        self.node_add_class(defaults.WLAN_EXP_WORKSHOP_STA_TYPE, 
                            defaults.WLAN_EXP_WORKSHOP_STA_CLASS, 
                            defaults.WLAN_EXP_WORKSHOP_STA_DESCRIPTION)

    
    def node_eval_class(self, node_class, host_config):
        """Evaluate the node_class string to create a node.  
        
        NOTE:  This should be overridden in each sub-class with the same
        overall structure but a different import.  Please call the super
        class so that the calls will propagate to catch all node types.
        """
        import wlan_exp.workshop.node_ap as node_ap
        import wlan_exp.workshop.node_sta as node_sta
        
        node = None

        try:
            full_node_class = node_class + "(host_config)"
            node = eval(full_node_class, globals(), locals())
        except:
            pass
        
        if node is None:
            return super(WlanExpWorkshopNodeFactory, self).node_eval_class(node_class, 
                                                                           host_config)
        else:
            return node


# End Class WlanExpNodeFactory
