# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment Local Traffic Generation (LTG)
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
1.00a ejw  02/07/14 Initial release

------------------------------------------------------------------------------

This module provides definitions for Local Traffic Generation (LTG) on 
WLAN Exp nodes.

Functions (see below for more information):
    WlanExpLTGFlowCBR() -- Constant Bit Rate (CBR) LTG flow
    WlanExpLTGFlowRandom() -- Random (size, period) LTG flow 

Integer constants:
    LTG_REMOVE_ALL -- Constant to remove all LTG flows on a given node.

It is assumed that users will extend this structure to create their own
LTG flows.  When an LTG component is serialized, it follows the following
structure:
  Word        Bits           Function   
   [0]        [31:16]        type
   [0]        [15:0]         length (number of 32 bit words)
   [1:length]                parameters

"""


__all__ = ['WlanExpLTGSchedule', 'WlanExpLTGSchedPeriodic', 'WlanExpLTGSchedUniformRand',
           'WlanExpLTGPayload', 'WlanExpLTGPayloadFixed', 'WlanExpLTGPayloadUniformRand',
           'WlanExpLTGFlow', 'WlanExpLTGFlowCBR', 'WlanExpLTGFlowRandom']



# LTG Schedules
#   NOTE:  The C counterparts are found in *_ltg.h
LTG_SCHED_TYPE_PERIODIC                = 1
LTG_SCHED_TYPE_UNIFORM_RAND            = 2


# LTG Payloads
#   NOTE:  The C counterparts are found in *_ltg.h
LTG_PYLD_TYPE_FIXED                    = 1
LTG_PYLD_TYPE_UNIFORM_RAND             = 2


# LTG Constants
#   NOTE:  The C counterparts are found in *_ltg.h
LTG_REMOVE_ALL                         = 0xFFFFFFFF
LTG_START_ALL                          = 0xFFFFFFFF
LTG_STOP_ALL                           = 0xFFFFFFFF


# LTG WLAN Exp Constants
#   NOTE:  The C counterparts are found in wlan_exp_node.h
LTG_ERROR                              = 0xFFFFFFFF


#-----------------------------------------------------------------------------
# LTG Schedules
#-----------------------------------------------------------------------------
class WlanExpLTGSchedule(object):
    """Base class for LTG Schedules."""
    ltg_type = None
    
    def get_type(self):
        """Return the type of the LTG Schedule."""
        return self.ltg_type

    def get_params(self):
        """Returns a list of parameters of the LTG Schedule."""
        raise NotImplementedError

    def serialize(self):
        """Returns a list of 32 bit intergers that can be added as arguments
        to a WnCmd.
        """
        args = []
        params = self.get_params()
        args.append(int((self.ltg_type << 16) + len(params)))
        for param in params:
            args.append(int(param))
        return args

# End Class WlanExpLTGSchedule


class WlanExpLTGSchedPeriodic(WlanExpLTGSchedule):
    """LTG Schedule that will generate a payload every 'interval' 
    microseconds.    
    """
    interval = None

    def __init__(self, interval):
        self.ltg_type = LTG_SCHED_TYPE_PERIODIC
        self.interval = interval
        
    def get_params(self):
        return [self.interval]

# End Class WlanExpLTGSchedPeriodic
    

class WlanExpLTGSchedUniformRand(WlanExpLTGSchedule):
    """LTG Schedule that will generate a payload a uniformly random time 
    between min_interval and max_interval microseconds.    
    """
    min_interval = None
    max_interval = None

    def __init__(self, min_interval, max_interval):
        self.ltg_type     = LTG_PYLD_TYPE_UNIFORM_RAND
        self.min_interval = min_interval
        self.max_interval = max_interval
        
    def get_params(self):
        return [self.min_interval, self.max_interval]

# End Class WlanExpLTGSchedUniformRand


#-----------------------------------------------------------------------------
# LTG Payloads
#-----------------------------------------------------------------------------
class WlanExpLTGPayload(object):
    """Base class for LTG Payloads."""
    ltg_type = None
    
    def get_type(self):
        """Return the type of the LTG Payload."""
        return self.ltg_type

    def get_params(self): 
        """Returns a list of parameters of the LTG Payload."""
        raise NotImplementedError

    def serialize(self):
        """Returns a list of 32 bit intergers that can be added as arguments
        to a WnCmd.
        """
        args = []
        params = self.get_params()
        args.append(int((self.ltg_type << 16) + len(params)))
        for param in params:
            args.append(int(param))
        return args

# End Class WlanExpLTGPayload


class WlanExpLTGPayloadFixed(WlanExpLTGPayload):
    """LTG paylod that will generate a fixed payload of the given length."""    
    length = None

    def __init__(self, length):
        self.ltg_type = LTG_PYLD_TYPE_FIXED
        self.length   = length
        
    def get_params(self):
        return [self.length]

# End Class WlanExpLTGSchedPeriodic
    

class WlanExpLTGPayloadUniformRand(WlanExpLTGPayload):
    """LTG payload that will generate a payload with uniformly random size 
    between min_length and max_length bytes.    
    """
    min_length = None
    max_length = None

    def __init__(self, min_length, max_length):
        self.ltg_type     = LTG_PYLD_TYPE_UNIFORM_RAND
        self.min_length = min_length
        self.max_length = max_length
        
    def get_params(self):
        return [self.min_length, self.max_length]

# End Class WlanExpLTGSchedUniformRand


#-----------------------------------------------------------------------------
# LTG Flows
#-----------------------------------------------------------------------------
class WlanExpLTGFlow(object):
    """Base class for LTG Flows."""
    ltg_schedule = None
    ltg_payload  = None
    
    def get_schedule(self):  return self.ltg_schedule
    def get_payload(self):   return self.ltg_payload

    def serialize(self):
        args = []

        for arg in self.ltg_schedule.serialize():
            args.append(arg)

        for arg in self.ltg_payload.serialize():       
            args.append(arg)

        return args

# End Class WlanExpLTGFlow


class WlanExpLTGFlowCBR(WlanExpLTGFlow):
    """Class to implement a Constant Bit Rate LTG flow."""
    def __init__(self, size, interval):
        self.ltg_schedule = WlanExpLTGSchedPeriodic(interval)
        self.ltg_payload  = WlanExpLTGPayloadFixed(size)

# End Class WlanExpLTGFlowCBR


class WlanExpLTGFlowRandom(WlanExpLTGFlow):
    """Class to implement an LTG flow with random period and random 
    sized payload.
    """
    def __init__(self, min_length, max_length, min_interval, max_interval):
        self.ltg_schedule = WlanExpLTGSchedUniformRand(min_interval, max_interval)
        self.ltg_payload  = WlanExpLTGPayloadUniformRand(min_length, max_length)

# End Class WlanExpLTGFlowRandom































