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
    FlowConfigCBR() -- Constant Bit Rate (CBR) LTG flow config
    FlowConfigRandomRandom() -- Random (size, period) LTG flow config 

Integer constants:
    LTG_REMOVE_ALL -- Constant to remove all LTG flows on a given node.
    LTG_START_ALL  -- Constant to start all LTG flows on a given node.
    LTG_STOP_ALL   -- Constant to stop all LTG flows on a given node.

It is assumed that users will extend these classes to create their own
LTG flow configurations.  When an LTG component is serialized, it follows 
the following structure:
  Word        Bits           Function   
   [0]        [31:16]        type
   [0]        [15:0]         length (number of 32 bit words)
   [1:length]                parameters

When a LTG flow configuration is serialized, the schedule is serialized 
first and then the payload.

"""


__all__ = ['Schedule', 'SchedulePeriodic', 'ScheduleUniformRandom',
           'Payload', 'PayloadFixed', 'PayloadUniformRandom',
           'FlowConfig', 'FlowConfigCBR', 'FlowConfigRandomRandom']



# LTG Schedules
#   NOTE:  The C counterparts are found in *_ltg.h
LTG_SCHED_TYPE_PERIODIC                = 1
LTG_SCHED_TYPE_UNIFORM_RAND            = 2


# LTG Payloads
#   NOTE:  The C counterparts are found in *_ltg.h
LTG_PYLD_TYPE_FIXED                    = 1
LTG_PYLD_TYPE_UNIFORM_RAND             = 2

# LTG Payload Min/Max
#
#   These defines are a place-holder for now; In the future, the min/max will
# change.  Currently, the minimum payload size is determined by the size
# of the LTG header in the payload (as of 0.8, the LTG header only contained
# the LLC header).  Currently, the maximum payload size is 1500 bytes.  This
# is a relatively arbitrary amount chosen because 1) in 0.8 the 802.11 phy
# cannot transmit more than 1600 bytes total per packet; 2) 1500 bytes is 
# about the size of a standard Ethernet MTU.  If sizes outside the range are
# requested, the functions will print a warning and adjust the value to the
# appropriate boundary.
#
LTG_PYLD_MIN                           = 8
LTG_PYLD_MAX                           = 1500


# LTG Constants
#   NOTE:  The C counterparts are found in *_ltg.h
LTG_REMOVE_ALL                         = 0xFFFFFFFF
LTG_START_ALL                          = 0xFFFFFFFF
LTG_STOP_ALL                           = 0xFFFFFFFF


#-----------------------------------------------------------------------------
# LTG Schedules
#-----------------------------------------------------------------------------
class Schedule(object):
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


class SchedulePeriodic(Schedule):
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
    

class ScheduleUniformRandom(Schedule):
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
class Payload(object):
    """Base class for LTG Payloads."""
    ltg_type = None
    
    def get_type(self):
        """Return the type of the LTG Payload."""
        return self.ltg_type

    def get_params(self): 
        """Returns a list of parameters of the LTG Payload."""
        raise NotImplementedError

    def validate_length(self, length):
        """Returns a valid LTG Payload length and prints warnings if 
        length was invalid.
        """
        msg  = "WARNING:  Adjusting LTG Payload length from {0} ".format(length)

        if (length < LTG_PYLD_MIN):
            msg += "to {0} (min).".format(LTG_PYLD_MIN)
            print(msg)
            length = LTG_PYLD_MIN

        if (length > LTG_PYLD_MAX):
            msg += "to {0} (max).".format(LTG_PYLD_MAX)
            print(msg)
            length = LTG_PYLD_MAX
        
        return length


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

# End Class Payload


class PayloadFixed(Payload):
    """LTG payload that will generate a fixed payload of the given length."""    
    length = None

    def __init__(self, length):
        self.ltg_type = LTG_PYLD_TYPE_FIXED
        self.length   = self.validate_length(length)
        
    def get_params(self):
        return [self.length]

# End Class PayloadFixed
    

class PayloadUniformRandom(Payload):
    """LTG payload that will generate a payload with uniformly random size 
    between min_length and max_length bytes.    
    """
    min_length = None
    max_length = None

    def __init__(self, min_length, max_length):
        self.ltg_type     = LTG_PYLD_TYPE_UNIFORM_RAND
        self.min_length = self.validate_length(min_length)
        self.max_length = self.validate_length(max_length)
        
    def get_params(self):
        return [self.min_length, self.max_length]

# End Class PayloadUniformRand


#-----------------------------------------------------------------------------
# LTG Flow Configurations
#-----------------------------------------------------------------------------
class FlowConfig(object):
    """Base class for LTG Flow Configurations."""
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

# End Class FlowConfig


class FlowConfigCBR(FlowConfig):
    """Class to implement a Constant Bit Rate LTG flow configuration."""
    def __init__(self, size, interval):
        self.ltg_schedule = SchedulePeriodic(interval)
        self.ltg_payload  = PayloadFixed(size)

# End Class FlowConfigCBR


class FlowConfigRandomRandom(FlowConfig):
    """Class to implement an LTG flow configuration with random period 
    and random sized payload.
    """
    def __init__(self, min_length, max_length, min_interval, max_interval):
        self.ltg_schedule = ScheduleUniformRandom(min_interval, max_interval)
        self.ltg_payload  = PayloadUniformRandom(min_length, max_length)

# End Class FlowConfigRandom































