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
           'StatsGetTxRx', 'StatsAddTxRxToLog', 
           'LTGConfigure', 'LTGStart', 'LTGStop', 'LTGRemove',
           'NodeResetState', 'NodeProcTime', 'NodeProcChannel', 'NodeProcTxPower', 
           'NodeProcTxRate', 'NodeProcTxAntMode', 'NodeProcRxAntMode', 'NodeGetStationInfo',
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

NODE_WRITE                             = 0x00000000
NODE_READ                              = 0x00000001
NODE_WRITE_DEFAULT                     = 0x00000002
NODE_READ_DEFAULT                      = 0x00000004

NODE_RSVD                              = 0xFFFFFFFF 

NODE_SUCCESS                           = 0x00000000
NODE_ERROR                             = 0xFF000000

NODE_RESET_LOG                         = 0x00000001
NODE_RESET_TXRX_STATS                  = 0x00000002
NODE_RESET_LTG                         = 0x00000004
NODE_RESET_TX_DATA_QUEUE               = 0x00000008

TIME_ADD_TO_LOG                        = 0x00000002
RSVD_TIME                              = 0xFFFFFFFF

NODE_TX_POWER_MAX_DBM                  = 19
NODE_TX_POWER_MIN_DBM                  = -12

NODE_UNICAST                           = 0x00000000
NODE_MULTICAST                         = 0x00000001


# LTG commands and defined values
CMD_LTG_CONFIG                         = 40
CMD_LTG_START                          = 41
CMD_LTG_STOP                           = 42
CMD_LTG_REMOVE                         = 43

LTG_ERROR                              = 0xFFFFFFFF

LTG_CONFIG_FLAG_RESTART                = 0x00000001


# Log commands and defined values
CMD_LOG_CONFIG                         = 50
CMD_LOG_GET_STATUS                     = 51
CMD_LOG_GET_CAPACITY                   = 52
CMD_LOG_GET_ENTRIES                    = 53
CMD_LOG_ADD_ENTRY                      = 54
CMD_LOG_ENABLE_ENTRY                   = 55
CMD_LOG_STREAM_ENTRIES                 = 56

LOG_CONFIG_FLAG_LOGGING                = 0x00000001
LOG_CONFIG_FLAG_WRAP                   = 0x00000002
LOG_CONFIG_FLAG_LOG_PAYLOADS           = 0x00000004
LOG_CONFIG_FLAG_LOG_WN_CMDS            = 0x00000008

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
    def __init__(self, log_enable=None, log_wrap_enable=None, 
                       log_full_payloads=None, log_warpnet_commands=None):
        super(LogConfigure, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_LOG_CONFIG

        flags = 0
        mask  = 0

        if log_enable is not None:
            mask += LOG_CONFIG_FLAG_LOGGING
            if log_enable:
                flags += LOG_CONFIG_FLAG_LOGGING
        
        if log_wrap_enable is not None:
            mask += LOG_CONFIG_FLAG_WRAP
            if log_wrap_enable:
                flags += LOG_CONFIG_FLAG_WRAP

        if log_full_payloads is not None:
            mask += LOG_CONFIG_FLAG_LOG_PAYLOADS
            if log_full_payloads:
                flags += LOG_CONFIG_FLAG_LOG_PAYLOADS

        if log_warpnet_commands is not None:
            mask += LOG_CONFIG_FLAG_LOG_WN_CMDS
            if log_warpnet_commands:
                flags += LOG_CONFIG_FLAG_LOG_WN_CMDS
        
        self.add_args(flags)
        self.add_args(mask)
    
    def process_resp(self, resp):
        pass

# End Class


class LogGetStatus(wn_message.Cmd):
    """Command to get the state information about the log."""
    def __init__(self):
        super(LogGetStatus, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_LOG_GET_STATUS
    
    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=4):
            args = resp.get_args()
            return (args[0], args[1], args[2], args[3])
        else:
            return (0,0,0,0)

# End Class


class LogGetCapacity(wn_message.Cmd):
    """Command to get the log capacity and current use."""
    def __init__(self):
        super(LogGetCapacity, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_LOG_GET_CAPACITY
    
    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=2):
            args = resp.get_args()
            return (args[0], args[1])
        else:
            return (0,0)

# End Class


class LogStreamEntries(wn_message.Cmd):
    """Command to configure the node log streaming."""
    def __init__(self, enable, host_id, ip_address, port):
        super(LogStreamEntries, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_LOG_STREAM_ENTRIES
        
        if (type(ip_address) is str):
            addr = wn_transport.ip_to_int(ip_address)
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
    
    A value of STATS_RSVD_CONFIG will return a read of the flags.
    """
    def __init__(self, flags):
        super(LogConfigure, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_STATS_CONFIG_TXRX
        self.add_args(flags)
    
    def process_resp(self, resp):
        pass

# End Class


class StatsGetTxRx(wn_message.BufferCmd):
    """Command to get the statistics from the node for a given node."""
    def __init__(self, node=None):
        super(StatsGetTxRx, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_STATS_GET_TXRX

        if node is not None:
            mac_address = node.wlan_mac_address
        else:
            mac_address = 0xFFFFFFFFFFFFFFFF            

        self.add_args(((mac_address >> 32) & 0xFFFF))
        self.add_args((mac_address & 0xFFFFFFFF))

    def process_resp(self, resp):
        # Contains a WARPNet Buffer of all stats entries.  Need to convert to 
        #   a list of statistics dictionaries.
        import warpnet.wlan_exp_log.log_entries as log
        
        index   = 0
        data    = resp.get_bytes()
        ret_val = log.entry_txrx_stats.deserialize(data[index:])

        return ret_val

# End Class


class StatsAddTxRxToLog(wn_message.Cmd):
    """Command to add the current statistics to the Event log"""
    def __init__(self):
        super(StatsAddTxRxToLog, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_STATS_ADD_TXRX_TO_LOG
    
    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=1):
            args = resp.get_args()
            return args[0]
        else:
            return 0

# End Class


#--------------------------------------------
# Local Traffic Generation (LTG) Commands
#--------------------------------------------
class LTGCommon(wn_message.Cmd):
    """Common code for LTG Commands."""
    def __init__(self, node=None):
        super(LTGCommon, self).__init__()
        
        if not node is None:
            mac_address = node.wlan_mac_address
            self.add_args(((mac_address >> 32) & 0xFFFF))
            self.add_args((mac_address & 0xFFFFFFFF))
        else:
            self.add_args(0xFFFFFFFF)
            self.add_args(0xFFFFFFFF)

    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=1):
            args = resp.get_args()
            return args[0]
        else:
            return LTG_ERROR
        
# End Class


class LTGConfigure(LTGCommon):
    """Command to configure an LTG with the given traffic flow to the 
    specified node.
    """
    def __init__(self, node, traffic_flow, restart=False):
        super(LTGConfigure, self).__init__(node)
        self.command = _CMD_GRPID_NODE + CMD_LTG_CONFIG

        flags = 0
        
        if restart:
            flags += LTG_CONFIG_FLAG_RESTART
        
        self.add_args(flags)
        
        for arg in traffic_flow.serialize():
            self.add_args(arg)
    
# End Class


class LTGStart(LTGCommon):
    """Command to start a configured LTG to the given node.
    
    NOTE:  By providing no node argument, this command will start all 
    configured LTGs on the node.
    """
    def __init__(self, node=None):
        super(LTGStart, self).__init__(node)
        self.command = _CMD_GRPID_NODE + CMD_LTG_START

# End Class


class LTGStop(LTGCommon):
    """Command to stop a configured LTG to the given node.
    
    NOTE:  By providing no node argument, this command will stop all 
    configured LTGs on the node.
    """
    def __init__(self, node=None):
        super(LTGStop, self).__init__(node)
        self.command = _CMD_GRPID_NODE + CMD_LTG_STOP
    
# End Class


class LTGRemove(LTGCommon):
    """Command to remove a configured LTG to the given node.
    
    NOTE:  By providing no node argument, this command will remove all 
    configured LTGs on the node.
    """
    def __init__(self, node=None):
        super(LTGRemove, self).__init__(node)
        self.command = _CMD_GRPID_NODE + CMD_LTG_REMOVE
    
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
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       NODE_READ
                       NODE_WRITE
                       TIME_ADD_TO_LOG
        node_time -- Time as either an integer number of microseconds or 
                       a floating point number in seconds.
        time_id   -- ID to use identify the time command in the log.
    """
    time_factor = 6
    time_type   = None
    time_cmd    = None
    
    def __init__(self, cmd, node_time, time_id=None):
        super(NodeProcTime, self).__init__()
        self.command  = _CMD_GRPID_NODE + CMD_NODE_TIME
        self.time_cmd = cmd

        # Read the time as a float
        if (cmd == NODE_READ):
            self.time_type = 0
            self.add_args(RSVD_TIME)
            self.add_args(RSVD_TIME)
            self.add_args(RSVD_TIME)
            self.add_args(RSVD_TIME)
            self.add_args(RSVD_TIME)
            self.add_args(RSVD_TIME)

        # Write the time / Add time to log
        else:
            import time

            # By default set the time_id to a random number between [0, 2^32)
            if time_id is None:
                import random
                time_id = 2**32 * random.random()

            if (cmd == NODE_WRITE):
                self.add_args(NODE_WRITE)

                # Format the node_time appropriately
                if   (type(node_time) is float):
                    time_to_send   = int(round(node_time, self.time_factor) * (10**self.time_factor))
                    self.time_type = 0
                elif (type(node_time) is int):
                    time_to_send   = node_time
                    self.time_type = 1
                else:
                    raise TypeError("Time must be either a float or int")
            else:
                self.add_args(TIME_ADD_TO_LOG)

                # Send the reserved value
                time_to_send = (RSVD_TIME << 32) + RSVD_TIME

            # Get the current time on the host
            now = int(round(time.time(), self.time_factor) * (10**self.time_factor))
            
            self.add_args(time_id)
            self.add_args((time_to_send & 0xFFFFFFFF))
            self.add_args(((time_to_send >> 32) & 0xFFFFFFFF))
            self.add_args((now & 0xFFFFFFFF))
            self.add_args(((now >> 32) & 0xFFFFFFFF))


    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=3, status_errors=[NODE_ERROR], name='Time command'):
            args = resp.get_args()
            time = (2**32 * args[2]) + args[1]
        else:
            time = 0

        ret_val = 0
        
        if   (self.time_type == 0):
            ret_val = float(time / (10**self.time_factor))
        elif (self.time_type == 1):
            ret_val = time
            
        return ret_val

# End Class


class NodeProcChannel(wn_message.Cmd):
    """Command to get / set the channel of the node.
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       NODE_READ
                       NODE_WRITE
        channel   -- 802.11 Channel for the node.  Should be a value between
                       0 and 11.  Checking is done on the node and the current
                       channel will always be returned by the node.  
    """
    def __init__(self, cmd, channel):
        super(NodeProcChannel, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_NODE_CHANNEL

        self.add_args(cmd)
        self.add_args(channel)
    
    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=2, status_errors=[NODE_ERROR], name='Channel command'):
            args = resp.get_args()
            return args[1]
        else:
            return NODE_ERROR

# End Class


class NodeProcTxPower(wn_message.Cmd):
    """Command to get / set the transmit power of the node.
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       NODE_READ
                       NODE_WRITE
        power     -- Transmit power for the WARP node (in dBm).
    """
    def __init__(self, cmd, power):
        super(NodeProcTxPower, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_NODE_TX_POWER

        self.add_args(cmd)

        if (cmd == NODE_READ):
            self.add_args(power)

        if (cmd == NODE_WRITE):
            if (power > NODE_TX_POWER_MAX_DBM):
                msg  = "WARNING:  Requested power too high.\n"
                msg += "    Adjusting transmit power from {0} to {1}".format(power, NODE_TX_POWER_MAX_DBM)
                print(msg)
                power = NODE_TX_POWER_MAX_DBM

            if (power < NODE_TX_POWER_MIN_DBM):
                msg  = "WARNING:  Requested power too low. \n"
                msg += "    Adjusting transmit power from {0} to {1}".format(power, NODE_TX_POWER_MIN_DBM)
                print(msg)
                power = NODE_TX_POWER_MIN_DBM

            # Shift the value so that there are only positive integers over the wire
            self.add_args(power - NODE_TX_POWER_MIN_DBM)
    
    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=5, status_errors=[NODE_ERROR], name='Power command'):
            args = resp.get_args()
            # Shift values back to the original range
            args = [x + NODE_TX_POWER_MIN_DBM for x in args]
            return args[1:]
        else:
            return []

# End Class


class NodeProcTxRate(wn_message.Cmd):
    """Command to get / set the transmit rate of the node.
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       NODE_READ
                       NODE_WRITE
                       NODE_WRITE_DEFAULT
                       NODE_READ_DEFAULT 
        node_type -- Is this for unicast transmit or multicast transmit.
        rate      -- 802.11 transmit rate for the node.  Should be an entry
                     from the rates table in wlan_exp.util.  Checking is
                     done on the node and the current rate will always be 
                     returned by the node.
        device    -- 802.11 device for which the rate is being set.  
    """
    rate     = None
    dev_name = None
    
    def __init__(self, cmd, node_type, rate=None, device=None):
        super(NodeProcTxRate, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_NODE_TX_RATE

        self.add_args(cmd)

        if ((node_type == NODE_UNICAST) or (node_type == NODE_MULTICAST)):
            self.add_args(node_type)
        else:
            msg  = "The type must be either the define NODE_UNICAST or NODE_MULTICAST"
            raise ValueError(msg)
        
        if rate is not None:
            try:
                self.rate = rate['index']
                self.add_args(self.rate)
            except (KeyError, TypeError):
                msg  = "The TX rate must be an entry from the rates table in wlan_exp.util"
                raise ValueError(msg)
        else:
            self.add_args(0)

        if device is not None:
            mac_address   = device.wlan_mac_address
            self.dev_name = device.name
            self.add_args(((mac_address >> 32) & 0xFFFF))
            self.add_args((mac_address & 0xFFFFFFFF))
        else:
            self.add_args(0xFFFFFFFF)
            self.add_args(0xFFFFFFFF)
    
    def process_resp(self, resp):
        import warpnet.wlan_exp.util as wlan_exp_util
        
        if resp.resp_is_valid(num_args=2, status_errors=[NODE_ERROR], name='Tx rate command'):
            args = resp.get_args()
            if self.rate is not None:
                if (args[1] != self.rate):
                    msg  = "WARNING: Device {0} rate mismatch.\n".format(self.dev_name)
                    msg += "    Tried to set rate to {0}\n".format(wlan_exp_util.tx_rate_index_to_str(self.rate))
                    msg += "    Actually set rate to {0}\n".format(wlan_exp_util.tx_rate_index_to_str(args[1]))
                    print(msg)
            return wlan_exp_util.find_tx_rate_by_index(args[1])
        else:
            return None

# End Class


class NodeProcTxAntMode(wn_message.Cmd):
    """Command to get / set the transmit antenna mode of the node.
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       NODE_READ
                       NODE_WRITE
                       NODE_WRITE_DEFAULT
                       NODE_READ_DEFAULT 
        node_type -- Is this for unicast transmit or multicast transmit.
        ant_mode  -- Transmit antenna mode for the node.  Checking is
                     done both in the command and on the node.  The current
                     antenna mode will be returned by the node.  
        device    -- 802.11 device for which the rate is being set.  
    """
    node_type = None    
    
    def __init__(self, cmd, node_type, ant_mode=None, device=None):
        super(NodeProcTxAntMode, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_NODE_TX_ANT_MODE

        self.add_args(cmd)
        
        if ((node_type == NODE_UNICAST) or (node_type == NODE_MULTICAST)):
            self.node_type = node_type
            self.add_args(node_type)
        else:
            msg  = "The type must be either the define NODE_UNICAST or NODE_MULTICAST"
            raise ValueError(msg)
        
        if ant_mode is not None:
            self.add_args(self.check_ant_mode(ant_mode))
        else:
            self.add_args(0)

        if device is not None:
            mac_address = device.wlan_mac_address
            self.add_args(((mac_address >> 32) & 0xFFFF))
            self.add_args((mac_address & 0xFFFFFFFF))
        else:
            self.add_args(0xFFFFFFFF)
            self.add_args(0xFFFFFFFF)


    def check_ant_mode(self, ant_mode):
        """Check the antenna mode to see if it is valid."""
        try:
            return ant_mode['index']
        except KeyError:
            msg  = "The antenna mode must be an entry from the wlan_tx_ant_mode\n"
            msg += "    list in wlan_exp.util\n"
            raise ValueError(msg)
    
    def process_resp(self, resp):
        import warpnet.wlan_exp.util as wlan_exp_util
        
        if resp.resp_is_valid(num_args=2, status_errors=[NODE_ERROR], name='Tx antenna mode command'):
            args = resp.get_args()
            if   (self.node_type == NODE_UNICAST):
                return wlan_exp_util.find_tx_ant_mode_by_index(args[1])
            elif (self.node_type == NODE_MULTICAST):
                return [wlan_exp_util.find_tx_ant_mode_by_index((args[1] >> 16) & 0xFFFF),
                        wlan_exp_util.find_tx_ant_mode_by_index(args[1] & 0xFFFF)]
            else:
                return NODE_ERROR
        else:
            return NODE_ERROR

# End Class


class NodeProcRxAntMode(wn_message.Cmd):
    """Command to get / set the receive antenna mode of the node.
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       NODE_READ
                       NODE_WRITE
        ant_mode  -- Receive antenna mode for the node.  Checking is
                     done both in the command and on the node.  The current
                     antenna mode will be returned by the node.  
    """
    def __init__(self, cmd, ant_mode=None):
        super(NodeProcRxAntMode, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_NODE_RX_ANT_MODE
        
        self.add_args(cmd)
        if ant_mode is not None:
            self.add_args(self.check_ant_mode(ant_mode))
        else:
            self.add_args(NODE_RSVD)


    def check_ant_mode(self, ant_mode):
        """Check the antenna mode to see if it is valid."""
        try:
            return ant_mode['index']
        except KeyError:
            msg  = "The antenna mode must be an entry from the wlan_rx_ant_mode\n"
            msg += "    list in wlan_exp.util\n"
            raise ValueError(msg)
    
    def process_resp(self, resp):
        import warpnet.wlan_exp.util as wlan_exp_util
        
        if resp.resp_is_valid(num_args=2, status_errors=[NODE_ERROR], name='Rx antenna mode command'):
            args = resp.get_args()
            return wlan_exp_util.find_rx_ant_mode_by_index(args[1])
        else:
            return NODE_ERROR

# End Class


class NodeGetStationInfo(wn_message.BufferCmd):
    """Command to get the station info for a given node."""
    def __init__(self, node=None):
        super(NodeGetStationInfo, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_GET_STATION_INFO

        if node is not None:
            mac_address = node.wlan_mac_address
        else:
            mac_address = 0xFFFFFFFFFFFFFFFF            

        self.add_args(((mac_address >> 32) & 0xFFFF))
        self.add_args((mac_address & 0xFFFFFFFF))

    def process_resp(self, resp):
        # Contains a WWARPNet Buffer of all station info entries.  Need to 
        #   convert to a list of station info dictionaries.
        import warpnet.wlan_exp_log.log_entries as log

        index   = 0
        data    = resp.get_bytes()
        ret_val = log.entry_station_info.deserialize(data[index:])

        return ret_val

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
