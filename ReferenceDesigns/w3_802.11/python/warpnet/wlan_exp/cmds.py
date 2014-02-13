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
    LogGetEvents()
    ResetLog()
    GetLogCurrIdx() 
    GetLogOldestIdx()

    AddStatsToLog()
    ResetStats()

    ConfigureLTG()
    StartLTG()
    StopLTG()
    RemoveLTG()

    ProcNodeTime()
    ProcNodeChannel()
    ProcNodeTxRate()
    ProcNodeTxGain()

Integer constants:
    None

Many other constants may be defined; please do not use these values when 
defining other sub-classes of WARPNet Cmd and BufferCmd.

"""

import warpnet.wn_cmds as wn_cmds
import warpnet.wn_message as wn_message
import warpnet.wn_transport_eth_udp as wn_transport



__all__ = ['GetLogEvents', 'ResetLog', 'GetLogCurrIdx', 'GetLogOldestIdx',
           'AddStatsToLog', 'ResetStats', 
           'ConfigureLTG', 'StartLTG', 'StopLTG', 'RemoveLTG',
           'ProcNodeTime', 'ProcNodeChannel', 'ProcNodeTxRate', 'ProcNodeTxGain']


# WLAN Exp Command IDs (Extension of WARPNet Command IDs)
#   NOTE:  The C counterparts are found in wlan_exp_node.h
CMD_ASSN_GET_STATUS          = 10
CMD_ASSN_SET_TABLE           = 11

CMD_DISASSOCIATE             = 20

CMD_TX_GAIN                  = 30
CMD_TX_RATE                  = 31
CMD_CHANNEL                  = 32
CMD_TIME                     = 33

RSVD_TX_GAIN                 = 0xFFFF
RSVD_TX_RATE                 = 0xFFFF
RSVD_CHANNEL                 = 0xFFFF
RSVD_TIME                    = 0x0000FFFF0000FFFF

CMD_LTG_CONFIG               = 40
CMD_LTG_START                = 41
CMD_LTG_STOP                 = 42
CMD_LTG_REMOVE               = 43

LTG_ERROR                    = 0xFFFFFFFF

CMD_LOG_RESET                = 50
CMD_LOG_CONFIG               = 51
CMD_LOG_GET_CURR_IDX         = 52
CMD_LOG_GET_OLDEST_IDX       = 53
CMD_LOG_GET_EVENTS           = 54
CMD_LOG_ADD_EVENT            = 55
CMD_LOG_ENABLE_EVENT         = 56
CMD_LOG_STREAM_ENTRIES       = 57

LOG_CONFIG_FLAG_WRAP         = 0x00000001

CMD_STATS_ADD_TO_LOG         = 60
CMD_STATS_GET_STATS          = 61
CMD_STATS_RESET              = 62

CMD_CONFIG_DEMO              = 90


# Be careful that new commands added to WlanExpNode do not collide with child commands


# Local Constants
_CMD_GRPID_NODE              = (wn_cmds.GRPID_NODE << 24)


#-----------------------------------------------------------------------------
# Class Definitions for WLAN Exp Commands
#-----------------------------------------------------------------------------

#--------------------------------------------
# Log Commands
#--------------------------------------------

class GetLogEvents(wn_message.BufferCmd):
    """Command to get the WLAN Exp log events of the node"""
    def __init__(self, size, start_byte=0):
        command = _CMD_GRPID_NODE + CMD_LOG_GET_EVENTS
        super(GetLogEvents, self).__init__(
                command=command, buffer_id=0, flags=0, start_byte=start_byte, size=size)

    def process_resp(self, resp):
        return resp

# End Class


class ResetLog(wn_message.Cmd):
    """Command to reset the Event log"""
    def __init__(self):
        super(ResetLog, self).__init__()
        self.command = _CMD_GRPID_NODE +  CMD_LOG_RESET
    
    def process_resp(self, resp):
        pass

# End Class


class ConfigureLog(wn_message.Cmd):
    """Command to configure the Event log.
    
    Attributes:
        flags - [0] Enable wrapping (1 - Enabled / 0 - Disabled (default))    
    """
    def __init__(self, flags):
        super(ConfigureLog, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_LOG_CONFIG
        self.add_args(flags)
    
    def process_resp(self, resp):
        pass

# End Class


class GetLogCurrIdx(wn_message.Cmd):
    """Command to reset the Event log"""
    def __init__(self):
        super(GetLogCurrIdx, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_LOG_GET_CURR_IDX
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]

# End Class


class GetLogOldestIdx(wn_message.Cmd):
    """Command to reset the Event log"""
    def __init__(self):
        super(GetLogOldestIdx, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_LOG_GET_OLDEST_IDX
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]

# End Class


class StreamLogEntries(wn_message.Cmd):
    """Command to configure the node log streaming."""
    def __init__(self, enable, host_id, ip_address, port):
        super(StreamLogEntries, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_LOG_STREAM_ENTRIES
        
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


#--------------------------------------------
# Stats Commands
#--------------------------------------------
class GetAllStats(wn_message.BufferCmd):
    """Command to get the statistics from the node"""
    def __init__(self, size, start_byte=0):
        raise NotImplementedError
        # command = _CMD_GRPID_NODE + CMD_STATS_GET_STATS
        # super(GetAllStats, self).__init__(
        #         command=command, buffer_id=0, flags=0, start_byte=start_byte, size=size)

    def process_resp(self, resp):
        return resp

# End Class


class GetStats(wn_message.Cmd):
    """Command to get the statistics from the node for a given node."""
    def __init__(self, node):
        super(GetStats, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_STATS_GET_STATS

        mac_address = node.wlan_mac_address
        self.add_args(((mac_address >> 32) & 0xFFFF))
        self.add_args((mac_address & 0xFFFFFFFF))

    def process_resp(self, resp):
        import wlan_exp_log.log_entries as log
        
        # TODO:  This works but needs to be fixed
        val = log.log_entry_txrx_stats.deserialize(resp.raw_data[8:])
        
        return val

# End Class


class AddStatsToLog(wn_message.Cmd):
    """Command to add the current statistics to the Event log"""
    def __init__(self):
        super(AddStatsToLog, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_STATS_ADD_TO_LOG
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]

# End Class


class ResetStats(wn_message.Cmd):
    """Command to reset the current statistics on the node."""
    def __init__(self):
        super(ResetStats, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_STATS_RESET
    
    def process_resp(self, resp):
        pass

# End Class


#--------------------------------------------
# Local Traffic Generation (LTG) Commands
#--------------------------------------------
class ConfigureLTG(wn_message.Cmd):
    """Command to configure an LTG with the given traffic flow to the 
    specified node.
    """
    def __init__(self, node, traffic_flow):
        super(ConfigureLTG, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_LTG_CONFIG
        
        mac_address = node.wlan_mac_address
        self.add_args(((mac_address >> 32) & 0xFFFF))
        self.add_args((mac_address & 0xFFFFFFFF))
        for arg in traffic_flow.serialize():
            self.add_args(arg)

    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]

# End Class

class StartLTG(wn_message.Cmd):
    """Command to start a configured LTG to the given node.
    
    NOTE:  By providing no node argument, this command will start all 
    configured LTGs on the node.
    """
    def __init__(self, node=None):
        super(StartLTG, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_LTG_START

        if not node is None:
            mac_address = node.wlan_mac_address
            self.add_args(((mac_address >> 32) & 0xFFFF))
            self.add_args((mac_address & 0xFFFFFFFF))
        else:
            self.add_args(0xFFFFFFFF)
            self.add_args(0xFFFFFFFF)


    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]

# End Class


class StopLTG(wn_message.Cmd):
    """Command to stop a configured LTG to the given node.
    
    NOTE:  By providing no node argument, this command will start all 
    configured LTGs on the node.
    """
    def __init__(self, node=None):
        super(StopLTG, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_LTG_STOP

        if not node is None:
            mac_address = node.wlan_mac_address
            self.add_args(((mac_address >> 32) & 0xFFFF))
            self.add_args((mac_address & 0xFFFFFFFF))
        else:
            self.add_args(0xFFFFFFFF)
            self.add_args(0xFFFFFFFF)

    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]

# End Class


class RemoveLTG(wn_message.Cmd):
    """Command to remove a configured LTG to the given node.
    
    NOTE:  By providing no node argument, this command will start all 
    configured LTGs on the node.
    """
    def __init__(self, node=None):
        super(RemoveLTG, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_LTG_REMOVE

        if not node is None:
            mac_address = node.wlan_mac_address
            self.add_args(((mac_address >> 32) & 0xFFFF))
            self.add_args((mac_address & 0xFFFFFFFF))
        else:
            self.add_args(0xFFFFFFFF)
            self.add_args(0xFFFFFFFF)

    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]

# End Class


#--------------------------------------------
# Configure Node Attribute Commands
#--------------------------------------------
class ProcNodeTime(wn_message.Cmd):
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
        super(ProcNodeTime, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_TIME
        
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


class ProcNodeChannel(wn_message.Cmd):
    """Command to get / set the channel of the node.
    
    Attributes:
        channel -- 802.11 Channel for the node.  Should be a value between
                   0 and 11.  Checking is done on the node and the current
                   channel will always be returned by the node.  A value 
                   of 0xFFFF will only return the channel.
    """
    def __init__(self, channel):
        super(ProcNodeChannel, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_CHANNEL        

        self.add_args((channel & 0xFFFF))
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]

# End Class


class ProcNodeTxRate(wn_message.Cmd):
    """Command to get / set the transmit rate of the node.
    
    Attributes:
        rate    -- 802.11 transmit rate for the node.  Should be an entry
                   from the rates table in wlan_exp_util.  Checking is
                   done on the node and the current rate will always be 
                   returned by the node.  A value of 0xFFFF will only 
                   return the rate.
    """
    def __init__(self, rate, node=None):
        super(ProcNodeTxRate, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_TX_RATE
        
        if not node is None:
            mac_address = node.wlan_mac_address
            self.add_args(((mac_address >> 32) & 0xFFFF))
            self.add_args((mac_address & 0xFFFFFFFF))
        else:
            self.add_args(0xFFFFFFFF)
            self.add_args(0xFFFFFFFF)

        self.add_args((rate['index'] & 0xFFFF))
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]

# End Class


class ProcNodeTxGain(wn_message.Cmd):
    """Command to get / set the transmit gain of the node.
    
    Attributes:
        rate    -- Transmit gain for the WARP node.  Should be an integer
                   value between 0 and 63.  Checking is done on the node 
                   and the current gain will always be returned by the node.
                   A value of 0xFFFF will only return the gain.
    """
    def __init__(self, gain):
        super(ProcNodeTxGain, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_TX_GAIN

        self.add_args((gain & 0xFFFF))
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]

# End Class


#--------------------------------------------
# Misc Commands
#--------------------------------------------
class ConfigDemo(wn_message.Cmd):
    """Command to configure the demo mode on the node.
    
    Attributes:
        flags - Flags to pass to the demo.  Defined Flags:
                  DEMO_CONFIG_FLAGS_EN = 1
        wait_time - Inter-packet sleep time (usec)
    """
    def __init__(self, flags, sleep_time):
        super(ConfigDemo, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_CONFIG_DEMO        

        self.add_args(flags)
        self.add_args(sleep_time)
    
    def process_resp(self, resp):
        pass

# End Class


