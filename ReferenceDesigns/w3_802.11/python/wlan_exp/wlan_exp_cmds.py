# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment Commands
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

This module provides class definitions for all WLAN Exp commands.  

Functions (see below for more information):
    WlanExpCmdLogGetEvents()
    WlanExpCmdResetLog()
    WlanExpCmdGetLogCurrIdx() 
    WlanExpCmdGetLogOldestIdx()
    WlanExpCmdAddStatsToLog()
    WlanExpCmdStreamLogEntries()    
    WlanExpCmdNodeTime()
    WlanExpCmdNodeChannel()

Integer constants:
    None

Many other constants may be defined; please do not use these values when 
defining other sub-classes of WnCmd and WnBufferCmd.

"""

import warpnet.wn_cmds as wn_cmds
import warpnet.wn_message as wn_message
import warpnet.wn_transport_eth_udp as wn_transport


__all__ = ['WlanExpCmdLogGetEvents', 'WlanExpCmdResetLog', 
           'WlanExpCmdGetLogCurrIdx', 'WlanExpCmdGetLogOldestIdx', 
           'WlanExpCmdAddStatsToLog', 'WlanExpCmdStreamLogEntries', 
           'WlanExpCmdNodeTime', 'WlanExpCmdNodeChannel']


# WLAN Exp Command IDs (Extension of WARPNet Command IDs)
#   NOTE:  The C counterparts are found in wlan_exp_node.h
CMD_ASSN_GET_STATUS          = 10
CMD_ASSN_SET_TABLE           = 11
CMD_ASSN_RESET_STATS         = 12

CMD_DISASSOCIATE             = 20

CMD_TX_POWER                 = 30
CMD_TX_RATE                  = 31
CMD_CHANNEL                  = 32
CMD_TIME                     = 33

CMD_LTG_CONFIG_CBF           = 40
CMD_LTG_START                = 41
CMD_LTG_STOP                 = 42
CMD_LTG_REMOVE               = 43

CMD_LOG_RESET                = 50
CMD_LOG_CONFIG               = 51
CMD_LOG_GET_CURR_IDX         = 52
CMD_LOG_GET_OLDEST_IDX       = 53
CMD_LOG_GET_EVENTS           = 54
CMD_LOG_ADD_EVENT            = 55
CMD_LOG_ENABLE_EVENT         = 56
CMD_LOG_STREAM_ENTRIES       = 57

CMD_STATS_ADD_TO_LOG         = 60
CMD_STATS_GET_STATS          = 61

CMD_CONFIG_DEMO              = 90


# Be careful that new commands added to WlanExpNode do not collide with child commands


#-----------------------------------------------------------------------------
# Class Definitions for WLAN Exp Commands
#-----------------------------------------------------------------------------

class WlanExpCmdLogGetEvents(wn_message.WnBufferCmd):
    """Command to get the WLAN Exp log events of the node"""
    def __init__(self, size, start_byte=0):
        command = (wn_cmds.GRPID_NODE << 24) | CMD_LOG_GET_EVENTS
        super(WlanExpCmdLogGetEvents, self).__init__(
                command=command, buffer_id=0, flags=0, start_byte=start_byte, size=size)

    def process_resp(self, resp):
        return resp

# End Class


class WlanExpCmdResetLog(wn_message.WnCmd):
    """Command to reset the Event log"""
    def __init__(self):
        super(WlanExpCmdResetLog, self).__init__()
        self.command = (wn_cmds.GRPID_NODE << 24) | CMD_LOG_RESET
    
    def process_resp(self, resp):
        pass

# End Class


class WlanExpCmdGetLogCurrIdx(wn_message.WnCmd):
    """Command to reset the Event log"""
    def __init__(self):
        super(WlanExpCmdGetLogCurrIdx, self).__init__()
        self.command = (wn_cmds.GRPID_NODE << 24) | CMD_LOG_GET_CURR_IDX
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]

# End Class


class WlanExpCmdGetLogOldestIdx(wn_message.WnCmd):
    """Command to reset the Event log"""
    def __init__(self):
        super(WlanExpCmdGetLogOldestIdx, self).__init__()
        self.command = (wn_cmds.GRPID_NODE << 24) | CMD_LOG_GET_OLDEST_IDX
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]

# End Class


class WlanExpCmdAddStatsToLog(wn_message.WnCmd):
    """Command to add the current statistics to the Event log"""
    def __init__(self):
        super(WlanExpCmdAddStatsToLog, self).__init__()
        self.command = (wn_cmds.GRPID_NODE << 24) | CMD_STATS_ADD_TO_LOG
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]

# End Class


class WlanExpCmdStreamLogEntries(wn_message.WnCmd):
    """Command to configure the node log streaming."""
    def __init__(self, enable, host_id, ip_address, port):
        super(WlanExpCmdStreamLogEntries, self).__init__()
        self.command = (wn_cmds.GRPID_NODE << 24) | CMD_LOG_STREAM_ENTRIES
        
        if (type(ip_address) is str):
            addr = wn_transport.ip2int(ip_address)
        elif (type(ip_address) is int):
            addr = ip_address
        else:
            raise TypeError("IP Address must be either a str or int")

        arg = (2**16 * int(host_id)) + (int(port) & 0xFFFF)

        self.add_args(enable)
        self.add_args(addr)
        self.add_args(arg)
    
    def process_resp(self, resp):
        pass

# End Class


class WlanExpCmdNodeTime(wn_message.WnCmd):
    """Command to get / set the time on the node.
    
    NOTE:  Python time functions operate on floating point numbers in 
        seconds, while the WnNode operates on microseconds.  In order
        to be more flexible, this class can be initialized with either
        type of input.  However, it will only return an integer number
        of microseconds.
    
    Attributes:
        time -- Time as either an integer number of microseconds or 
                  a floating point number in seconds.
    """
    time_factor = 6
    
    def __init__(self, time):
        super(WlanExpCmdNodeTime, self).__init__()
        self.command = (wn_cmds.GRPID_NODE << 24) | CMD_TIME
        
        if   (type(time) is float):
            time_to_send = int(round(time, self.time_factor) * (10**self.time_factor))
        elif (type(time) is int):
            time_to_send = time
        else:
            raise TypeError("Time must be either a float or int")
            
        self.add_args((time_to_send & 0xFFFFFFFF))
        self.add_args(((time_to_send >> 32) & 0xFFFFFFFF))
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 2:
            print("Invalid response.")
            print(resp)
        return ( (2**32 * args[1]) + args[0] )

# End Class


class WlanExpCmdNodeChannel(wn_message.WnCmd):
    """Command to get / set the channel of the node.
    
    Attributes:
        channel -- 802.11 Channel for the node.  Should be a value between
                   0 and 11.  Checking is done on the node and the current
                   channel will always be returned by the node.  A value 
                   of 0xFFFF will only return the channel.
    """
    def __init__(self, channel):
        super(WlanExpCmdNodeChannel, self).__init__()
        self.command = (wn_cmds.GRPID_NODE << 24) | CMD_CHANNEL        

        self.add_args((channel & 0xFFFF))
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]

# End Class


class WlanExpCmdConfigDemo(wn_message.WnCmd):
    """Command to configure the demo on the node.
    
    Attributes:
        flags - Flags to pass to the demo.  Defined Flags:
                  DEMO_CONFIG_FLAGS_EN = 1
        wait_time - Inter-packet sleep time (usec)
    """
    def __init__(self, flags, sleep_time):
        super(WlanExpCmdConfigDemo, self).__init__()
        self.command = (wn_cmds.GRPID_NODE << 24) | CMD_CONFIG_DEMO        

        self.add_args(flags)
        self.add_args(sleep_time)
    
    def process_resp(self, resp):
        pass

# End Class


