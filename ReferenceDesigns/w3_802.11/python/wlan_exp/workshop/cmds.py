# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment Workshop Commands
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
1.00a ejw  5/29/14  Initial release

------------------------------------------------------------------------------

This module provides class definitions for all Workshop WLAN Exp commands.  
"""

import wlan_exp.cmds as cmds

__all__ = []


# Low Param IDs -- in sync with wlan_mac_dcf_workshop.h
CMD_PARAM_LOW_PARAM_WORKSHOP_CONFIG              = 0x00000002


# Workshop Param defines
CMD_PARAM_DISABLE                                = 0x00
CMD_PARAM_ENABLE                                 = 0x01

CMD_PARAM_ENABLE_POS                             =  0
CMD_PARAM_FADING_POS                             =  8
CMD_PARAM_DIVERSITY_ORDER_POS                    = 16
CMD_PARAM_TX_POWER_POS                           = 24

CMD_PARAM_EXT_PKT_DETECT_EN_POS                  =  8
CMD_PARAM_RETRANS_SWITCH_THRESHOLD_POS           = 16

CMD_PARAM_ANT_DIVSRITY_ORDER_MIN                 =  1
CMD_PARAM_ANT_DIVSRITY_ORDER_MAX                 =  2

CMD_PARAM_TX_RETRANS_MIN                         =  0
CMD_PARAM_TX_RETRANS_MAX                         =  7


#-----------------------------------------------------------------------------
# Class Definitions for WLAN Exp Commands
#-----------------------------------------------------------------------------
class WorkshopModeConfig(object):
    """Class that contains a configuration for WLAN Exp Workshop modes
    
    Attributes:
        enable          -- Is the antenna on/off
        fading          -- Is fading for the antenna on/off
        diversity_order -- What is the diversity order of the fading
        tx_power        -- What is the mean Tx power of the antenna
    """
    ant_a_config     = None
    ant_b_config     = None

    def __init__(self, ant_a_config, ant_b_config):
        self.ant_a_config = ant_a_config
        self.ant_b_config = ant_b_config
    

    def serialize(self): 
        ret_val = []
        
        ret_val.append(self.ant_a_config.serialize())
        ret_val.append(self.ant_b_config.serialize())
        
        return ret_val

# End Class



class WorkshopModeAntennaConfig(object):
    """Class that contains an antenna configuration for WLAN Exp Workshop modes
    
    Attributes:
        enable          -- Is the antenna on/off
        fading          -- Is fading for the antenna on/off
        diversity_order -- What is the diversity order of the fading
        tx_power        -- What is the mean Tx power of the antenna
    """
    enable           = None
    fading           = None
    diversity_order  = None
    tx_power         = None

    def __init__(self, enable, fading, diversity_order, tx_power):
        self.enable = enable
        self.fading = fading

        # Check the diversity order argument
        if (diversity_order > CMD_PARAM_ANT_DIVSRITY_ORDER_MAX):        
            msg  = "WARNING:  Max antenna Tx diversity order is {0}, ".format(CMD_PARAM_ANT_DIVSRITY_ORDER_MAX)
            msg += "provided {1}.  Setting to max.".format(diversity_order)
            print(msg)
            diversity_order = CMD_PARAM_ANT_DIVSRITY_ORDER_MAX
            
        if (diversity_order < CMD_PARAM_ANT_DIVSRITY_ORDER_MIN):        
            msg  = "WARNING:  Min antenna Tx diversity order is {0}, ".format(CMD_PARAM_ANT_DIVSRITY_ORDER_MIN)
            msg += "provided {1}.  Setting to min.".format(diversity_order)
            print(msg)
            diversity_order = CMD_PARAM_ANT_DIVSRITY_ORDER_MIN

        self.diversity_order = diversity_order

        # Check the tx power argument
        if (tx_power > cmds.CMD_PARAM_NODE_TX_POWER_MAX_DBM):
            msg  = "WARNING:  Max Tx power is {0}, ".format(cmds.CMD_PARAM_NODE_TX_POWER_MAX_DBM)
            msg += "provided {1}.  Setting to max.".format(tx_power)
            print(msg)
            tx_power = cmds.CMD_PARAM_NODE_TX_POWER_MAX_DBM

        if (tx_power < cmds.CMD_PARAM_NODE_TX_POWER_MIN_DBM):
            msg  = "WARNING:  Min Tx power is {0}, ".format(cmds.CMD_PARAM_NODE_TX_POWER_MIN_DBM)
            msg += "provided {1}.  Setting to min.".format(tx_power)
            print(msg)
            tx_power = cmds.CMD_PARAM_NODE_TX_POWER_MIN_DBM

        self.tx_power = tx_power
    

    def serialize(self): 
        ret_val = 0
        
        if (self.enable):
            ret_val += (CMD_PARAM_ENABLE << CMD_PARAM_ENABLE_POS)
        
        if (self.fading):
            ret_val += (CMD_PARAM_ENABLE << CMD_PARAM_FADING_POS)

        ret_val += (self.diversity_order << CMD_PARAM_DIVERSITY_ORDER_POS)

        # Adjust the TX power so that it is [0, 31] instead of [-12, 19]
        adj_tx_power = self.tx_power - cmds.CMD_PARAM_NODE_TX_POWER_MIN_DBM

        ret_val += (adj_tx_power << CMD_PARAM_TX_POWER_POS)

        return ret_val

# End Class



        



