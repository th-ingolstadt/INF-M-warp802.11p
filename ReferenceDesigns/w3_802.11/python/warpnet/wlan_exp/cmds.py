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
    LogConfigure()
    LogGetStatus() 
    LogGetCapacity()

    StatsGetTxRx()
    StatsGetAllTxRx()
    StatsAddTxRxToLog()

    LTGConfigure()
    LTGStart()
    LTGStop()
    LTGRemove()

    NodeResetState()
    NodeProcTime()
    NodeProcChannel()
    NodeProcTxPower()
    NodeProcTxRate()
    NodeProcTxAntMode()
    NodeProcRxAntMode()
    
    QueueTxDataPurgeAll()
    

Integer constants:
    None

Many other constants may be defined; please do not use these values when 
defining other sub-classes of WARPNet Cmd and BufferCmd.

"""

import warpnet.wn_cmds as wn_cmds
import warpnet.wn_message as wn_message
import warpnet.wn_transport_eth_udp as wn_transport



__all__ = ['LogGetEvents', 'LogConfigure', 'LogGetStatus', 'LogGetCapacity',
           'StatsGetTxRx', 'StatsGetAllTxRx', 'StatsAddTxRxToLog', 
           'LTGConfigure', 'LTGStart', 'LTGStop', 'LTGRemove',
           'NodeResetState', 'NodeProcTime', 'NodeProcChannel', 'NodeProcTxPower', 
           'NodeProcTxRate', 'NodeProcTxAntMode', 'NodeProcRxAntMode',
           'QueueTxDataPurgeAll']


# WLAN Exp Command IDs (Extension of WARPNet Command IDs)
#   NOTE:  The C counterparts are found in wlan_exp_node.h
CMD_GET_STATION_INFO                   = 10
CMD_SET_STATION_INFO                   = 11

CMD_DISASSOCIATE                       = 20


# Node commands and defined values
CMD_NODE_RESET_STATE                   = 30
CMD_NODE_TIME                          = 31
CMD_NODE_CHANNEL                       = 32
CMD_NODE_TX_POWER                      = 33
CMD_NODE_TX_RATE                       = 34
CMD_NODE_TX_ANT_MODE                   = 35
CMD_NODE_RX_ANT_MODE                   = 36

NODE_RESET_LOG                         = 0x00000001
NODE_RESET_TXRX_STATS                  = 0x00000002

RSVD_TIME                              = 0x0000FFFF0000FFFF
RSVD_CHANNEL                           = 0xFFFF
RSVD_TX_POWER                          = 0xFFFF
RSVD_TX_RATE                           = 0xFFFF
RSVD_TX_ANT_MODE                       = 0xFFFF
RSVD_RX_ANT_MODE                       = 0xFFFF

NODE_TX_POWER_MAX_DBM                  = 19
NODE_TX_POWER_MIN_DBM                  = -12

NODE_RX_ANT_MODE_SISO_ANTA             = 0x1
NODE_RX_ANT_MODE_SISO_ANTB             = 0x2
NODE_RX_ANT_MODE_SISO_ANTC             = 0x3
NODE_RX_ANT_MODE_SISO_ANTD             = 0x4
NODE_RX_ANT_MODE_SISO_SELDIV_2ANT      = 0x5
NODE_RX_ANT_MODE_SISO_SELDIV_4ANT      = 0x6

NODE_TX_ANT_MODE_SISO_ANTA             = 0x10
NODE_TX_ANT_MODE_SISO_ANTB             = 0x20
NODE_TX_ANT_MODE_SISO_ANTC             = 0x30
NODE_TX_ANT_MODE_SISO_ANTD             = 0x40

NODE_UNICAST                           = 0x0000
NODE_MULTICAST                         = 0x0001


# LTG commands and defined values
CMD_LTG_CONFIG                         = 40
CMD_LTG_START                          = 41
CMD_LTG_STOP                           = 42
CMD_LTG_REMOVE                         = 43

LTG_ERROR                              = 0xFFFFFFFF


# Log commands and defined values
CMD_LOG_CONFIG                         = 50
CMD_LOG_GET_STATUS                     = 51
CMD_LOG_GET_CAPACITY                   = 52
CMD_LOG_GET_ENTRIES                    = 53
CMD_LOG_ADD_ENTRY                      = 54
CMD_LOG_ENABLE_ENTRY                   = 55
CMD_LOG_STREAM_ENTRIES                 = 56

LOG_CONFIG_FLAG_WRAP                   = 0x00000001
LOG_CONFIG_FLAG_LOGGING                = 0x00000002

LOG_GET_ALL_ENTRIES                    = 0xFFFFFFFF


# Statistics commands and defined values
CMD_STATS_CONFIG_TXRX                  = 60
CMD_STATS_ADD_TXRX_TO_LOG              = 61
CMD_STATS_GET_TXRX                     = 62

STATS_CONFIG_FLAG_PROMISC              = 0x00000001

STATS_RSVD_CONFIG                      = 0xFFFFFFFF


# Queue commands and defined values
CMD_QUEUE_TX_DATA_PURGE_ALL            = 70


# Be careful that new commands added to WlanExpNode do not collide with child commands


# Local Constants
_CMD_GRPID_NODE              = (wn_cmds.GRPID_NODE << 24)


#-----------------------------------------------------------------------------
# Class Definitions for WLAN Exp Commands
#-----------------------------------------------------------------------------

#--------------------------------------------
# Log Commands
#--------------------------------------------
class LogGetEvents(wn_message.BufferCmd):
    """Command to get the WLAN Exp log events of the node"""
    def __init__(self, size, start_byte=0):
        command = _CMD_GRPID_NODE + CMD_LOG_GET_ENTRIES
        
        if (size == LOG_GET_ALL_ENTRIES):
            size = wn_message.CMD_BUFFER_GET_SIZE_FROM_DATA
        
        super(LogGetEvents, self).__init__(
                command=command, buffer_id=0, flags=0, start_byte=start_byte, size=size)

        if (False):
            print("LogGetEvents:")
            print("    size       = {0}".format(size))
            print("    start_byte = {0}".format(start_byte))

    def process_resp(self, resp):
        return resp

# End Class


class LogConfigure(wn_message.Cmd):
    """Command to configure the Event log.
    
    Attributes:
        flags - [0] Enable wrapping (1 - Enabled / 0 - Disabled (default))    
                [1] Log events (1 - Enabled (default) / 0 - Disabled)
    """
    def __init__(self, flags):
        super(LogConfigure, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_LOG_CONFIG
        self.add_args(flags)
    
    def process_resp(self, resp):
        pass

# End Class


class LogGetStatus(wn_message.Cmd):
    """Command to get the state information about the log."""
    def __init__(self):
        super(LogGetStatus, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_LOG_GET_STATUS
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 4:
            print("Invalid response.")
            print(resp)
        return (args[0], args[1], args[2], args[3])

# End Class


class LogGetCapacity(wn_message.Cmd):
    """Command to get the log capacity and current use."""
    def __init__(self):
        super(LogGetCapacity, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_LOG_GET_CAPACITY
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 2:
            print("Invalid response.")
            print(resp)
        return (args[0], args[1])

# End Class


class LogStreamEntries(wn_message.Cmd):
    """Command to configure the node log streaming."""
    def __init__(self, enable, host_id, ip_address, port):
        super(LogStreamEntries, self).__init__()
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
class StatsConfigure(wn_message.Cmd):
    """Command to configure the Statistics collection.
    
    Attributes:
        flags - [0] Enable promiscuous statisitics
                    (1 - Enabled (default) / 0 - Disabled)
    
    A value of 0xFFFFFFFF will return a read of the flags.
    """
    def __init__(self, flags):
        super(LogConfigure, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_STATS_CONFIG_TXRX
        self.add_args(flags)
    
    def process_resp(self, resp):
        pass

# End Class


class StatsGetAllTxRx(wn_message.BufferCmd):
    """Command to get the statistics from the node"""
    def __init__(self):
        command = _CMD_GRPID_NODE + CMD_STATS_GET_TXRX
        super(StatsGetAllTxRx, self).__init__(
                command=command, buffer_id=0xFFFFFFFF, flags=0xFFFFFFFF, start_byte=0, size=0)

        # TODO:  This is using a bastardization of the WARPNet Buffer.
        #        We should really be able to independently specify that a 
        #        command requires a buffer as a return parameter instead
        #        of having to send a fixed set of arguments.  Otherwise,
        #        we should use a separate command

    def process_resp(self, resp):
        # Contains a WN_Buffer of all stats entries.  Need to convert to 
        # a list of statistics dictionaries.
        import warpnet.wlan_exp_log.log_entries as log

        index   = 0
        data    = resp.get_bytes()
        ret_val = log.entry_txrx_stats.deserialize(data[index:])

        return ret_val

# End Class


class StatsGetTxRx(wn_message.Cmd):
    """Command to get the statistics from the node for a given node."""
    def __init__(self, node):
        super(StatsGetTxRx, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_STATS_GET_TXRX

        mac_address = node.wlan_mac_address
        self.add_args(((mac_address >> 32) & 0xFFFF))
        self.add_args((mac_address & 0xFFFFFFFF))

    def process_resp(self, resp):
        import warpnet.wlan_exp_log.log_entries as log
        
        index   = 28                     # Offset after response arguments
        data    = resp.get_bytes()
        ret_val = log.entry_txrx_stats.deserialize(data[index:])

        if ret_val:
            return ret_val[0]
        else:
            return None

# End Class


class StatsAddTxRxToLog(wn_message.Cmd):
    """Command to add the current statistics to the Event log"""
    def __init__(self):
        super(StatsAddTxRxToLog, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_STATS_ADD_TXRX_TO_LOG
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]

# End Class


#--------------------------------------------
# Local Traffic Generation (LTG) Commands
#--------------------------------------------
class LTGConfigure(wn_message.Cmd):
    """Command to configure an LTG with the given traffic flow to the 
    specified node.
    """
    def __init__(self, node, traffic_flow):
        super(LTGConfigure, self).__init__()
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

class LTGStart(wn_message.Cmd):
    """Command to start a configured LTG to the given node.
    
    NOTE:  By providing no node argument, this command will start all 
    configured LTGs on the node.
    """
    def __init__(self, node=None):
        super(LTGStart, self).__init__()
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


class LTGStop(wn_message.Cmd):
    """Command to stop a configured LTG to the given node.
    
    NOTE:  By providing no node argument, this command will start all 
    configured LTGs on the node.
    """
    def __init__(self, node=None):
        super(LTGStop, self).__init__()
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


class LTGRemove(wn_message.Cmd):
    """Command to remove a configured LTG to the given node.
    
    NOTE:  By providing no node argument, this command will start all 
    configured LTGs on the node.
    """
    def __init__(self, node=None):
        super(LTGRemove, self).__init__()
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
class NodeResetState(wn_message.Cmd):
    """Command to reset the state of a portion of the node defined by the flags.
    
    Attributes:
        flags -- [0] NODE_RESET_LOG
                 [1] NODE_RESET_TXRX_STATS
    """
    def __init__(self, flags):
        super(NodeResetState, self).__init__()
        self.command = _CMD_GRPID_NODE +  CMD_NODE_RESET_STATE        
        self.add_args(flags)
    
    def process_resp(self, resp):
        pass

# End Class


class NodeProcTime(wn_message.Cmd):
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
        super(NodeProcTime, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_NODE_TIME
        
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


class NodeProcChannel(wn_message.Cmd):
    """Command to get / set the channel of the node.
    
    Attributes:
        channel -- 802.11 Channel for the node.  Should be a value between
                   0 and 11.  Checking is done on the node and the current
                   channel will always be returned by the node.  A value 
                   of RSVD_CHANNEL will only return the channel.
    """
    def __init__(self, channel):
        super(NodeProcChannel, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_NODE_CHANNEL

        self.add_args((channel & 0xFFFF))
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]

# End Class


class NodeProcTxPower(wn_message.Cmd):
    """Command to get / set the transmit power of the node.
    
    Attributes:
        power -- Transmit power for the WARP node (in dBm).  Should be an 
                 integer value between NODE_TX_POWER_MIN_DBM and 
                 NODE_TX_POWER_MAX_DBM.  A value of RSVD_TX_POWER will return 
                 the gain without setting it.
    """
    def __init__(self, power):
        super(NodeProcTxPower, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_NODE_TX_POWER

        if (power == RSVD_TX_POWER):
            self.add_args(power)
        else:
            if ((power >= NODE_TX_POWER_MIN_DBM) and (power <= NODE_TX_POWER_MAX_DBM)):
                # Shift the value so that there are only positive integers over the wire
                self.add_args(power - NODE_TX_POWER_MIN_DBM)
            else:
                msg  = "Transmit power must be a value in dBm between:  "
                msg += "{0} and {1}".format(NODE_TX_POWER_MIN_DBM, NODE_TX_POWER_MAX_DBM)
                raise ValueError(msg)
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]

# End Class


class NodeProcTxRate(wn_message.Cmd):
    """Command to get / set the transmit rate of the node.
    
    Attributes:
        node_type -- Is this for unicast transmit or multicast transmit.
        rate      -- 802.11 transmit rate for the node.  Should be an entry
                     from the rates table in wlan_exp.util.  Checking is
                     done on the node and the current rate will always be 
                     returned by the node.  A value of RSVD_TX_RATE will only 
                     return the rate.
        node      -- Node for which the rate is being set.  If node is 
                     not provided, then the default rate is set.
    """
    def __init__(self, node_type, rate, node=None):
        super(NodeProcTxRate, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_NODE_TX_RATE
        
        if not node is None:
            mac_address = node.wlan_mac_address
            self.add_args(((mac_address >> 32) & 0xFFFF))
            self.add_args((mac_address & 0xFFFFFFFF))
        else:
            self.add_args(0xFFFFFFFF)
            self.add_args(0xFFFFFFFF)

        if ((node_type == NODE_UNICAST) or (node_type == NODE_MULTICAST)):
            self.add_args(node_type)
        else:
            msg  = "The type must be either the define NODE_UNICAST or NODE_MULTICAST"
            raise ValueError(msg)
        
        try:
            self.add_args((rate['index'] & 0xFFFF))
        except (KeyError, TypeError):
            msg  = "The TX rate must be an entry from the rates table in wlan_exp.util"
            raise ValueError(msg)
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]

# End Class


class NodeProcTxAntMode(wn_message.Cmd):
    """Command to get / set the transmit antenna mode of the node.
    
    Attributes:
        node_type -- Is this for unicast transmit or multicast transmit.
        ant_mode  -- Transmit antenna mode for the node.  Checking is
                     done both in the command and on the node.  The current
                     antenna mode will be returned by the node.  A value of 
                     RSVD_TX_ANT_MODE will only return the antenna mode.
        node      -- Node for which the antenna mode is being set.  If node
                     is not provided, then the default antenna mode is set.
    """
    def __init__(self, node_type, ant_mode, node=None):
        super(NodeProcTxAntMode, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_NODE_TX_ANT_MODE
        
        if not node is None:
            mac_address = node.wlan_mac_address
            self.add_args(((mac_address >> 32) & 0xFFFF))
            self.add_args((mac_address & 0xFFFFFFFF))
        else:
            self.add_args(0xFFFFFFFF)
            self.add_args(0xFFFFFFFF)

        if ((node_type == NODE_UNICAST) or (node_type == NODE_MULTICAST)):
            self.add_args(node_type)
        else:
            msg  = "The type must be either the define NODE_UNICAST or NODE_MULTICAST"
            raise ValueError(msg)
        
        self.add_args(self.check_ant_mode(ant_mode))


    def check_ant_mode(self, ant_mode):
        """Check the antenna mode to see if it is valid."""
        if ((ant_mode == NODE_TX_ANT_MODE_SISO_ANTA) or
            (ant_mode == NODE_TX_ANT_MODE_SISO_ANTB) or
            (ant_mode == NODE_TX_ANT_MODE_SISO_ANTC) or
            (ant_mode == NODE_TX_ANT_MODE_SISO_ANTD)):
            return ant_mode
        else:
            msg  = "The antenna mode must be one of the following defines:\n"
            msg += "    NODE_TX_ANT_MODE_SISO_ANTA\n"
            msg += "    NODE_TX_ANT_MODE_SISO_ANTB\n"
            msg += "    NODE_TX_ANT_MODE_SISO_ANTC\n"
            msg += "    NODE_TX_ANT_MODE_SISO_ANTD\n"
            raise ValueError(msg)
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]

# End Class


class NodeProcRxAntMode(wn_message.Cmd):
    """Command to get / set the receive antenna mode of the node.
    
    Attributes:
        ant_mode  -- Receive antenna mode for the node.  Checking is
                     done both in the command and on the node.  The current
                     antenna mode will be returned by the node.  A value of 
                     RSVD_RX_ANT_MODE will only return the antenna mode.
    """
    def __init__(self, ant_mode):
        super(NodeProcRxAntMode, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_NODE_RX_ANT_MODE
        
        self.add_args(self.check_ant_mode(ant_mode))


    def check_ant_mode(self, ant_mode):
        """Check the antenna mode to see if it is valid."""
        if ((ant_mode == NODE_RX_ANT_MODE_SISO_ANTA) or
            (ant_mode == NODE_RX_ANT_MODE_SISO_ANTB) or
            (ant_mode == NODE_RX_ANT_MODE_SISO_ANTC) or
            (ant_mode == NODE_RX_ANT_MODE_SISO_ANTD) or
            (ant_mode == NODE_RX_ANT_MODE_SISO_SELDIV_2ANT) or
            (ant_mode == NODE_RX_ANT_MODE_SISO_SELDIV_4ANT)):
            return ant_mode
        else:
            msg  = "The antenna mode must be one of the following defines:\n"
            msg += "    NODE_RX_ANT_MODE_SISO_ANTA\n"
            msg += "    NODE_RX_ANT_MODE_SISO_ANTB\n"
            msg += "    NODE_RX_ANT_MODE_SISO_ANTC\n"
            msg += "    NODE_RX_ANT_MODE_SISO_ANTD\n"
            msg += "    NODE_RX_ANT_MODE_SISO_SELDIV_2ANT"
            msg += "    NODE_RX_ANT_MODE_SISO_SELDIV_4ANT"
            raise ValueError(msg)
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]

# End Class


class NodeGetAllStationInfo(wn_message.BufferCmd):
    """Command to get the statistics from the node"""
    def __init__(self):
        command = _CMD_GRPID_NODE + CMD_GET_STATION_INFO
        super(NodeGetAllStationInfo, self).__init__(
                command=command, buffer_id=0xFFFFFFFF, flags=0xFFFFFFFF, start_byte=0, size=0)

        # TODO:  This is using a bastardization of the WARPNet Buffer.
        #        We should really be able to independently specify that a 
        #        command requires a buffer as a return parameter instead
        #        of having to send a fixed set of arguments.  Otherwise,
        #        we should use a separate command

    def process_resp(self, resp):
        # Contains a WN_Buffer of all stats entries.  Need to convert to 
        # a list of statistics dictionaries.
        import warpnet.wlan_exp_log.log_entries as log

        index   = 0
        data    = resp.get_bytes()
        ret_val = log.entry_station_info.deserialize(data[index:])

        return ret_val

# End Class


class NodeGetStationInfo(wn_message.Cmd):
    """Command to get the station info for a given node."""
    def __init__(self, node):
        super(NodeGetStationInfo, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_GET_STATION_INFO

        mac_address = node.wlan_mac_address
        self.add_args(((mac_address >> 32) & 0xFFFF))
        self.add_args((mac_address & 0xFFFFFFFF))

    def process_resp(self, resp):
        import warpnet.wlan_exp_log.log_entries as log

        index   = 28                     # Offset after response arguments
        data    = resp.get_bytes()
        ret_val = log.entry_station_info.deserialize(data[index:])

        if ret_val:
            return ret_val[0]
        else:
            return None

# End Class


#--------------------------------------------
# Queue Commands
#--------------------------------------------
class QueueTxDataPurgeAll(wn_message.Cmd):
    """Command to purge all data transmit queues on the node."""
    def __init__(self):
        super(QueueTxDataPurgeAll, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_QUEUE_TX_DATA_PURGE_ALL
        
    def process_resp(self, resp):
        pass

# End Class
