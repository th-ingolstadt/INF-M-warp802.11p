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

"""

import wlan_exp.warpnet.cmds as wn_cmds
import wlan_exp.warpnet.message as wn_message
import wlan_exp.warpnet.transport_eth_udp as wn_transport



__all__ = ['LogGetEvents', 'LogConfigure', 'LogGetStatus', 'LogGetCapacity',
           'StatsGetTxRx', 'StatsAddTxRxToLog', 
           'LTGConfigure', 'LTGStart', 'LTGStop', 'LTGRemove',
           'NodeResetState', 'NodeProcTime', 'NodeProcChannel', 'NodeProcTxPower', 
           'NodeProcTxRate', 'NodeProcTxAntMode', 'NodeProcRxAntMode', 'NodeGetStationInfo',
           'NodeSetLowToHighFilter', 'QueueTxDataPurgeAll']


# WLAN Exp Command IDs (Extension of WARPNet Command IDs)
#   NOTE:  The C counterparts are found in wlan_exp_node.h
#   NOTE:  All Command IDs (CMDID_*) must be unique 24-bit numbers

# Node commands and defined values
CMDID_NODE_RESET_STATE                           = 0x001000
CMDID_NODE_TIME                                  = 0x001001
CMDID_NODE_CHANNEL                               = 0x001002
CMDID_NODE_TX_POWER                              = 0x001003
CMDID_NODE_TX_RATE                               = 0x001004
CMDID_NODE_TX_ANT_MODE                           = 0x001005
CMDID_NODE_RX_ANT_MODE                           = 0x001006
CMDID_NODE_LOW_TO_HIGH_FILTER                    = 0x001007
CMDID_NODE_LOW_PARAM                             = 0x001008
CMDID_NODE_RANDOM_SEED                           = 0x001009

CMD_PARAM_WRITE                                  = 0x00000000
CMD_PARAM_READ                                   = 0x00000001
CMD_PARAM_WRITE_DEFAULT                          = 0x00000002
CMD_PARAM_READ_DEFAULT                           = 0x00000004

CMD_PARAM_SUCCESS                                = 0x00000000
CMD_PARAM_ERROR                                  = 0xFF000000

CMD_PARAM_UNICAST                                = 0x00000000
CMD_PARAM_MULTICAST                              = 0x00000001

CMD_PARAM_NODE_CONFIG_ALL                        = 0xFFFFFFFF 

CMD_PARAM_NODE_RESET_FLAG_LOG                    = 0x00000001
CMD_PARAM_NODE_RESET_FLAG_TXRX_STATS             = 0x00000002
CMD_PARAM_NODE_RESET_FLAG_LTG                    = 0x00000004
CMD_PARAM_NODE_RESET_FLAG_TX_DATA_QUEUE          = 0x00000008
CMD_PARAM_NODE_RESET_FLAG_ASSOCIATIONS           = 0x00000010

CMD_PARAM_TIME_ADD_TO_LOG                        = 0x00000002
CMD_PARAM_RSVD_TIME                              = 0xFFFFFFFF

CMD_PARAM_NODE_TX_POWER_MAX_DBM                  = 19
CMD_PARAM_NODE_TX_POWER_MIN_DBM                  = -12

CMD_PARAM_RX_FILTER_FCS_GOOD                     = 0x1000
CMD_PARAM_RX_FILTER_FCS_ALL                      = 0x2000
CMD_PARAM_RX_FILTER_FCS_NOCHANGE                 = 0xF000

CMD_PARAM_RX_FILTER_HDR_ADDR_MATCH_MPDU          = 0x0001
CMD_PARAM_RX_FILTER_HDR_ALL_MPDU                 = 0x0002
CMD_PARAM_RX_FILTER_HDR_ALL                      = 0x0003
CMD_PARAM_RX_FILTER_HDR_NOCHANGE                 = 0x0FFF

CMD_PARAM_RANDOM_SEED_VALID                      = 0x00000001
CMD_PARAM_RANDOM_SEED_RSVD                       = 0xFFFFFFFF

# Low Param IDs -- in sync with wlan_mac_low.h
CMD_PARAM_LOW_PARAM_PHYSICAL_CS_THRESH           = 0x00000001
CMD_PARAM_LOW_PARAM_CW_EXP_MIN                   = 0x00000002
CMD_PARAM_LOW_PARAM_CW_EXP_MAX                   = 0x00000003


# Association commands and defined values
CMDID_GET_STATION_INFO                           = 0x001080

CMDID_NODE_DISASSOCIATE                          = 0x001090
CMDID_NODE_ASSOCIATE                             = 0x001091

CMD_PARAM_ASSOCIATE_ALLOW_TIMEOUT                = 0x00000001


# LTG commands and defined values
CMDID_LTG_CONFIG                                 = 0x002000
CMDID_LTG_START                                  = 0x002001
CMDID_LTG_STOP                                   = 0x002002
CMDID_LTG_REMOVE                                 = 0x002003
CMDID_LTG_STATUS                                 = 0x002004

CMD_PARAM_LTG_ERROR                              = 0x000001

CMD_PARAM_LTG_ALL_LTGS                           = 0xFFFFFFFF

CMD_PARAM_LTG_CONFIG_FLAG_AUTOSTART              = 0x00000001

CMD_PARAM_LTG_RUNNING                            = 0x00000001
CMD_PARAM_LTG_STOPPED                            = 0x00000000


# Log commands and defined values
CMDID_LOG_CONFIG                                 = 0x003000
CMDID_LOG_GET_STATUS                             = 0x003001
CMDID_LOG_GET_CAPACITY                           = 0x003002
CMDID_LOG_GET_ENTRIES                            = 0x003003
CMDID_LOG_ADD_EXP_INFO_ENTRY                     = 0x003004
CMDID_LOG_ENABLE_ENTRY                           = 0x003005
CMDID_LOG_STREAM_ENTRIES                         = 0x003006


CMD_PARAM_LOG_GET_ALL_ENTRIES                    = 0xFFFFFFFF

CMD_PARAM_LOG_CONFIG_FLAG_LOGGING                = 0x00000001
CMD_PARAM_LOG_CONFIG_FLAG_WRAP                   = 0x00000002
CMD_PARAM_LOG_CONFIG_FLAG_LOG_PAYLOADS           = 0x00000004
CMD_PARAM_LOG_CONFIG_FLAG_LOG_WN_CMDS            = 0x00000008


# Statistics commands and defined values
CMDID_STATS_CONFIG_TXRX                          = 0x004000
CMDID_STATS_ADD_TXRX_TO_LOG                      = 0x004001
CMDID_STATS_GET_TXRX                             = 0x004002

CMD_PARAM_STATS_CONFIG_FLAG_PROMISC              = 0x00000001


# Queue commands and defined values
CMDID_QUEUE_TX_DATA_PURGE_ALL                    = 0x005000


# AP commands and defined values
CMDID_NODE_AP_SET_AUTHENTICATION_ADDR_FILTER     = 0x010000
CMDID_NODE_AP_SSID                               = 0x010001

CMD_PARAM_MAX_SSID_LEN                           = 32


# STA commands and defined values
CMDID_NODE_STA_CONFIG                            = 0x010000

CMD_PARAM_NODE_STA_BEACON_TS_UPDATE              = 0x00000001


# Developer commands and defined values
CMDID_DEV_MEM_HIGH                               = 0xFFF000
CMDID_DEV_MEM_LOW                                = 0xFFF001



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
        command = _CMD_GRPID_NODE + CMDID_LOG_GET_ENTRIES
        
        if (size == CMD_PARAM_LOG_GET_ALL_ENTRIES):
            size = wn_message.CMD_BUFFER_GET_SIZE_FROM_DATA
        
        super(LogGetEvents, self).__init__(
                command=command, buffer_id=0, flags=0, start_byte=start_byte, size=size)

    def process_resp(self, resp):
        return resp

# End Class


class LogConfigure(wn_message.Cmd):
    """Command to configure the Event log.
    
    Attributes (default state on the node is in CAPS):
        log_enable           -- Enable the event log (TRUE/False)
        log_warp_enable      -- Enable event log wrapping (True/FALSE)
        log_full_payloads    -- Record full Tx/Rx payloads in event log (True/FALSE)
        log_warpnet_commands -- Record WARPNet commands in event log (True/FALSE)        
    """
    def __init__(self, log_enable=None, log_wrap_enable=None, 
                       log_full_payloads=None, log_warpnet_commands=None):
        super(LogConfigure, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_LOG_CONFIG

        flags = 0
        mask  = 0

        if log_enable is not None:
            mask += CMD_PARAM_LOG_CONFIG_FLAG_LOGGING
            if log_enable:
                flags += CMD_PARAM_LOG_CONFIG_FLAG_LOGGING
        
        if log_wrap_enable is not None:
            mask += CMD_PARAM_LOG_CONFIG_FLAG_WRAP
            if log_wrap_enable:
                flags += CMD_PARAM_LOG_CONFIG_FLAG_WRAP

        if log_full_payloads is not None:
            mask += CMD_PARAM_LOG_CONFIG_FLAG_LOG_PAYLOADS
            if log_full_payloads:
                flags += CMD_PARAM_LOG_CONFIG_FLAG_LOG_PAYLOADS

        if log_warpnet_commands is not None:
            mask += CMD_PARAM_LOG_CONFIG_FLAG_LOG_WN_CMDS
            if log_warpnet_commands:
                flags += CMD_PARAM_LOG_CONFIG_FLAG_LOG_WN_CMDS
        
        self.add_args(flags)
        self.add_args(mask)
    
    def process_resp(self, resp):
        pass

# End Class


class LogGetStatus(wn_message.Cmd):
    """Command to get the state information about the log."""
    def __init__(self):
        super(LogGetStatus, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_LOG_GET_STATUS
    
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
        self.command = _CMD_GRPID_NODE + CMDID_LOG_GET_CAPACITY
    
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
        self.command = _CMD_GRPID_NODE + CMDID_LOG_STREAM_ENTRIES
        
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


class LogAddExpInfoEntry(wn_message.Cmd):
    """Command to write a EXP_INFO Log Entry to the node."""
    def __init__(self, info_type, message):
        super(LogAddExpInfoEntry, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_LOG_ADD_EXP_INFO_ENTRY

        self.add_args(info_type)

        if message is None:
            self.add_args(0)
        else:            
            import struct     
            data_len = len(message)
            data_buf = bytearray(message, 'UTF-8')
            
            # Add initial agruments
            self.add_args(data_len)
                
            # Zero pad so that the data buffer is 32-bit aligned
            if ((len(data_buf) % 4) != 0):
                data_buf += bytearray(4 - (len(data_buf) % 4))
            
            idx = 0
            while (idx < len(data_buf)):
                arg = struct.unpack_from('!I', data_buf[idx:idx+4])
                self.add_args(arg[0])
                idx += 4


    def process_resp(self, resp):
        pass

# End Class



#--------------------------------------------
# Stats Commands
#--------------------------------------------
class StatsConfigure(wn_message.Cmd):
    """Command to configure the Statistics collection.
    
    Attributes (default state on the node is in CAPS):
        promisc_stats        -- Enable promiscuous statistics collection (TRUE/False)
    """
    def __init__(self, promisc_stats=None):
        super(StatsConfigure, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_STATS_CONFIG_TXRX

        flags = 0
        mask  = 0

        if promisc_stats is not None:
            mask += CMD_PARAM_STATS_CONFIG_FLAG_PROMISC
            if promisc_stats:
                flags += CMD_PARAM_STATS_CONFIG_FLAG_PROMISC
                
        self.add_args(flags)
        self.add_args(mask)
    
    def process_resp(self, resp):
        pass

# End Class


class StatsGetTxRx(wn_message.BufferCmd):
    """Command to get the statistics from the node for a given node."""
    def __init__(self, node=None):
        super(StatsGetTxRx, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_STATS_GET_TXRX

        if node is not None:
            mac_address = node.wlan_mac_address
        else:
            mac_address = 0xFFFFFFFFFFFFFFFF            

        self.add_args(((mac_address >> 32) & 0xFFFF))
        self.add_args((mac_address & 0xFFFFFFFF))

    def process_resp(self, resp):
        # Contains a WARPNet Buffer of all stats entries.  Need to convert to 
        #   a list of statistics dictionaries.
        import wlan_exp.log.entry_types as entry_types
        
        index   = 0
        data    = resp.get_bytes()
        ret_val = entry_types.entry_txrx_stats.deserialize(data[index:])

        return ret_val

# End Class


class StatsAddTxRxToLog(wn_message.Cmd):
    """Command to add the current statistics to the Event log"""
    def __init__(self):
        super(StatsAddTxRxToLog, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_STATS_ADD_TXRX_TO_LOG
    
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
    name = None
    
    def __init__(self, ltg_id=None):
        super(LTGCommon, self).__init__()
        
        if ltg_id is not None:
            if type(ltg_id) is not int:
                raise TypeError("LTG ID must be an integer.")
            self.add_args(ltg_id)
        else:
            self.add_args(CMD_PARAM_LTG_ALL_LTGS)

    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=1, 
                              status_errors=[CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR], 
                              name='LTG {0} command'.format(self.name)):
            args = resp.get_args()
            return args[0]
        else:
            return CMD_PARAM_LTG_ERROR
        
# End Class


class LTGConfigure(wn_message.Cmd):
    """Command to configure an LTG with the given traffic flow to the 
    specified node.
    """
    name = 'configure'

    def __init__(self, traffic_flow, auto_start=False):
        super(LTGConfigure, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_LTG_CONFIG

        flags = 0
        
        if auto_start:
            flags += CMD_PARAM_LTG_CONFIG_FLAG_AUTOSTART
        
        self.add_args(flags)
        
        for arg in traffic_flow.serialize():
            self.add_args(arg)

    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=2, 
                              status_errors=[CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR], 
                              name='LTG {0} command'.format(self.name)):
            args = resp.get_args()
            return args[1]
        else:
            return CMD_PARAM_LTG_ERROR
    
# End Class


class LTGStart(LTGCommon):
    """Command to start a configured LTG to the given node.
    
    NOTE:  By providing no node argument, this command will start all 
    configured LTGs on the node.
    """
    name = 'start'

    def __init__(self, ltg_id=None):
        super(LTGStart, self).__init__(ltg_id)
        self.command = _CMD_GRPID_NODE + CMDID_LTG_START

# End Class


class LTGStop(LTGCommon):
    """Command to stop a configured LTG to the given node.
    
    NOTE:  By providing no node argument, this command will stop all 
    configured LTGs on the node.
    """
    name = 'stop'

    def __init__(self, ltg_id=None):
        super(LTGStop, self).__init__(ltg_id)
        self.command = _CMD_GRPID_NODE + CMDID_LTG_STOP
    
# End Class


class LTGRemove(LTGCommon):
    """Command to remove a configured LTG to the given node.
    
    NOTE:  By providing no node argument, this command will remove all 
    configured LTGs on the node.
    """
    name = 'remove'

    def __init__(self, ltg_id=None):
        super(LTGRemove, self).__init__(ltg_id)
        self.command = _CMD_GRPID_NODE + CMDID_LTG_REMOVE
    
# End Class


class LTGStatus(wn_message.Cmd):
    """Command to get the status of the LTG."""
    time_factor = 6
    name        = 'status'

    def __init__(self, ltg_id):
        super(LTGStatus, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_LTG_STATUS

        if type(ltg_id) is not int:
            raise TypeError("LTG ID must be an integer.")
        self.add_args(ltg_id)

    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=6, 
                              status_errors=[CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR], 
                              name='LTG {0} command'.format(self.name)):
            args = resp.get_args()

            start_timestamp = (2**32 * args[3]) + args[2]
            stop_timestamp  = (2**32 * args[5]) + args[4]
            
            return (True, (args[1] == CMD_PARAM_LTG_RUNNING), start_timestamp, stop_timestamp)
        else:
            return (False, False, 0, 0)
    
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
        self.command = _CMD_GRPID_NODE +  CMDID_NODE_RESET_STATE        
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
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
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
        self.command  = _CMD_GRPID_NODE + CMDID_NODE_TIME
        self.time_cmd = cmd

        # Read the time as a float
        if (cmd == CMD_PARAM_READ):
            self.time_type = 0
            self.add_args(CMD_PARAM_READ)
            self.add_args(CMD_PARAM_RSVD_TIME)             # Reads do not need a time_id
            self.add_args(CMD_PARAM_RSVD_TIME)
            self.add_args(CMD_PARAM_RSVD_TIME)
            self.add_args(CMD_PARAM_RSVD_TIME)
            self.add_args(CMD_PARAM_RSVD_TIME)

        # Write the time / Add time to log
        else:
            import time

            # By default set the time_id to a random number between [0, 2^32)
            if time_id is None:
                import random
                time_id = 2**32 * random.random()

            if (cmd == CMD_PARAM_WRITE):
                self.add_args(CMD_PARAM_WRITE)

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
                self.add_args(CMD_PARAM_TIME_ADD_TO_LOG)

                # Send the reserved value
                time_to_send = (CMD_PARAM_RSVD_TIME << 32) + CMD_PARAM_RSVD_TIME

            # Get the current time on the host
            now = int(round(time.time(), self.time_factor) * (10**self.time_factor))
            
            self.add_args(int(time_id))
            self.add_args((time_to_send & 0xFFFFFFFF))
            self.add_args(((time_to_send >> 32) & 0xFFFFFFFF))
            self.add_args((now & 0xFFFFFFFF))
            self.add_args(((now >> 32) & 0xFFFFFFFF))


    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=3, status_errors=[CMD_PARAM_ERROR], name='Time command'):
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


class NodeSetLowToHighFilter(wn_message.Cmd):
    """Command to set the low to high filter on the node.
    
    Attributes:
        mac_header -- MAC header filter.  Values can be:
                        'MPDU_TO_ME' -- Pass any unicast-to-me or multicast data or 
                                        management packet
                        'ALL_MPDU'   -- Pass any data or management packet (no address filter)
                        'ALL'        -- Pass any packet (no type or address filters)
        FCS        -- FCS status filter.  Values can be:
                        'GOOD'       -- Pass only packets with good checksum result
                        'ALL'        -- Pass packets with any checksum result
    """    
    def __init__(self, cmd, mac_header=None, fcs=None):
        super(NodeSetLowToHighFilter, self).__init__()
        self.command  = _CMD_GRPID_NODE + CMDID_NODE_LOW_TO_HIGH_FILTER

        self.add_args(CMD_PARAM_WRITE)

        rx_filter = 0

        if mac_header is None:
            rx_filter += CMD_PARAM_RX_FILTER_HDR_NOCHANGE
        else:
            mac_header = str(mac_header)
            mac_header.upper()
            
            if   (mac_header == 'MPDU_TO_ME'):
                rx_filter += CMD_PARAM_RX_FILTER_HDR_ADDR_MATCH_MPDU
            elif (mac_header == 'ALL_MPDU'):
                rx_filter += CMD_PARAM_RX_FILTER_HDR_ALL_MPDU
            elif (mac_header == 'ALL'):
                rx_filter += CMD_PARAM_RX_FILTER_HDR_ALL
            else:
                msg  = "WARNING:  Not a valid mac_header value.\n"
                msg += "    Provided:  {0}\n".format(mac_header)
                msg += "    Requires:  ['MPDU_TO_ME', 'ALL_MPDU', 'ALL']"
                print(msg)
                rx_filter += CMD_PARAM_RX_FILTER_HDR_NOCHANGE

        if fcs is None:
            rx_filter += CMD_PARAM_RX_FILTER_FCS_NOCHANGE
        else:
            fcs = str(fcs)
            fcs.upper()

            if   (fcs == 'GOOD'):
                rx_filter += CMD_PARAM_RX_FILTER_FCS_GOOD
            elif (fcs == 'ALL'):
                rx_filter += CMD_PARAM_RX_FILTER_FCS_NOCHANGE
            else:
                msg  = "WARNING: Not a valid fcs value.\n"
                msg += "    Provided:  {0}\n".format(fcs)
                msg += "    Requires:  ['GOOD', 'ALL']"
                print(msg)
                rx_filter += CMD_PARAM_RX_FILTER_FCS_NOCHANGE

        self.add_args(rx_filter)


    def process_resp(self, resp):
        pass

# End Class


class NodeProcChannel(wn_message.Cmd):
    """Command to get / set the channel of the node.
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
        channel   -- 802.11 Channel for the node.  Should be a valid channel defined
                       in wlan_exp.util wlan_channel table.
    """
    channel  = None

    def __init__(self, cmd, channel=None):
        super(NodeProcChannel, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_CHANNEL

        self.add_args(cmd)
        
        if channel is not None:
            try:
                self.channel = channel['index']
            except (KeyError, TypeError):
                import wlan_exp.util as util
                
                tmp_chan = util.find_channel_by_channel_number(channel)
                
                if tmp_chan is not None:
                    self.channel = tmp_chan['index']
                else:
                    msg  = "Unknown channel:  {0}".format(channel)
                    raise ValueError(msg)                    

        if self.channel is not None:
            self.add_args(self.channel)
        else:
            self.add_args(0)

    
    def process_resp(self, resp):
        import wlan_exp.util as util
        
        if resp.resp_is_valid(num_args=2, status_errors=[CMD_PARAM_ERROR], name='Channel command'):
            args = resp.get_args()
            if self.channel is not None:
                if (args[1] != self.channel):
                    msg  = "WARNING: Channel mismatch.\n"
                    msg += "    Tried to set channel to {0}\n".format(util.channel_to_str(self.channel))
                    msg += "    Actually set channel to {0}\n".format(util.channel_to_str(args[1]))
                    print(msg)
            return util.find_channel_by_index(args[1])
        else:
            return None

# End Class


class NodeProcRandomSeed(wn_message.Cmd):
    """Command to set the random seed of the node.
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       CMD_PARAM_READ   (Not supported)
                       CMD_PARAM_WRITE
        high_seed -- Random number generator seed for CPU high
        low_seed  -- Random number generator seed for CPU low
    
    import random
    high_seed = random.randint(0, 0xFFFFFFFF)
    NOTE:  If a seed is not provided, then the seed is not updated.
    """
    def __init__(self, cmd, high_seed=None, low_seed=None):
        super(NodeProcRandomSeed, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_RANDOM_SEED

        if (cmd == CMD_PARAM_READ):
            raise AttributeError("Read not supported for NodeProcRandomSeed.")

        self.add_args(CMD_PARAM_WRITE)

        if high_seed is not None:
            self.add_args(CMD_PARAM_RANDOM_SEED_VALID)
            self.add_args(high_seed)
        else:
            self.add_args(CMD_PARAM_RANDOM_SEED_RSVD)
            self.add_args(CMD_PARAM_RANDOM_SEED_RSVD)
    
        if low_seed is not None:
            self.add_args(CMD_PARAM_RANDOM_SEED_VALID)
            self.add_args(high_seed)
        else:
            self.add_args(CMD_PARAM_RANDOM_SEED_RSVD)
            self.add_args(CMD_PARAM_RANDOM_SEED_RSVD)
    
    def process_resp(self, resp):
        pass

# End Class


class NodeLowParam(wn_message.Cmd):
    """Command to set parameter in CPU Low
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       CMD_PARAM_WRITE

        param     -- ID of parameter to modify

        values    -- When cmd==CMD_PARAM_WRITE, scalar or list of u32 values to write
                     When cmd==CMD_PARAM_READ, None

    """    
    def __init__(self, cmd, param, values):
        super(NodeLowParam, self).__init__()        
        
        self.command = _CMD_GRPID_NODE + CMDID_NODE_LOW_PARAM

        # Caluculate the size of the entire message to CPU Low [PARAM_ID, ARGS[]]
        size = len(values) + 1

        self.add_args(cmd)
        self.add_args(size)
        self.add_args(param)
        try:
            for v in values:
                self.add_args(v)
        except TypeError:
            self.add_args(values)
            
    def process_resp(self, resp):
        pass

# End Class
        

class NodeProcTxPower(wn_message.Cmd):
    """Command to get / set the transmit power of the node.
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
        power     -- Transmit power for the WARP node (in dBm).
    """
    def __init__(self, cmd, power=None):
        super(NodeProcTxPower, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_TX_POWER

        self.add_args(cmd)

        if (cmd == CMD_PARAM_WRITE):
            if power is None:
                raise ValueError("Must supply value to set Tx power.")
            
            if (power > CMD_PARAM_NODE_TX_POWER_MAX_DBM):
                msg  = "WARNING:  Requested power too high.\n"
                msg += "    Adjusting transmit power from {0} to {1}".format(power, CMD_PARAM_NODE_TX_POWER_MAX_DBM)
                print(msg)
                power = CMD_PARAM_NODE_TX_POWER_MAX_DBM

            if (power < CMD_PARAM_NODE_TX_POWER_MIN_DBM):
                msg  = "WARNING:  Requested power too low. \n"
                msg += "    Adjusting transmit power from {0} to {1}".format(power, CMD_PARAM_NODE_TX_POWER_MIN_DBM)
                print(msg)
                power = CMD_PARAM_NODE_TX_POWER_MIN_DBM

            # Shift the value so that there are only positive integers over the wire
            self.add_args(power - CMD_PARAM_NODE_TX_POWER_MIN_DBM)
    
    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=5, status_errors=[CMD_PARAM_ERROR], name='Power command'):
            args = resp.get_args()
            # Shift values back to the original range
            args = [x + CMD_PARAM_NODE_TX_POWER_MIN_DBM for x in args]
            return args[1:]
        else:
            return []

# End Class


class NodeProcTxRate(wn_message.Cmd):
    """Command to get / set the transmit rate of the node.
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
                       CMD_PARAM_WRITE_DEFAULT
                       CMD_PARAM_READ_DEFAULT 
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
        self.command = _CMD_GRPID_NODE + CMDID_NODE_TX_RATE

        self.add_args(cmd)

        if ((node_type == CMD_PARAM_UNICAST) or (node_type == CMD_PARAM_MULTICAST)):
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
        import wlan_exp.util as util
        
        if resp.resp_is_valid(num_args=2, status_errors=[CMD_PARAM_ERROR], name='Tx rate command'):
            args = resp.get_args()
            if self.rate is not None:
                if (args[1] != self.rate):
                    msg  = "WARNING: Device {0} rate mismatch.\n".format(self.dev_name)
                    msg += "    Tried to set rate to {0}\n".format(util.tx_rate_index_to_str(self.rate))
                    msg += "    Actually set rate to {0}\n".format(util.tx_rate_index_to_str(args[1]))
                    print(msg)
            return util.find_tx_rate_by_index(args[1])
        else:
            return None

# End Class


class NodeProcTxAntMode(wn_message.Cmd):
    """Command to get / set the transmit antenna mode of the node.
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
                       CMD_PARAM_WRITE_DEFAULT
                       CMD_PARAM_READ_DEFAULT 
        node_type -- Is this for unicast transmit or multicast transmit.
        ant_mode  -- Transmit antenna mode for the node.  Checking is
                     done both in the command and on the node.  The current
                     antenna mode will be returned by the node.  
        device    -- 802.11 device for which the rate is being set.  
    """
    node_type = None    
    
    def __init__(self, cmd, node_type, ant_mode=None, device=None):
        super(NodeProcTxAntMode, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_TX_ANT_MODE

        self.add_args(cmd)
        
        if ((node_type == CMD_PARAM_UNICAST) or (node_type == CMD_PARAM_MULTICAST)):
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
        import wlan_exp.util as util
        
        if resp.resp_is_valid(num_args=2, status_errors=[CMD_PARAM_ERROR], name='Tx antenna mode command'):
            args = resp.get_args()
            if   (self.node_type == CMD_PARAM_UNICAST):
                return util.find_tx_ant_mode_by_index(args[1])
            elif (self.node_type == CMD_PARAM_MULTICAST):
                return [util.find_tx_ant_mode_by_index((args[1] >> 16) & 0xFFFF),
                        util.find_tx_ant_mode_by_index(args[1] & 0xFFFF)]
            else:
                return CMD_PARAM_ERROR
        else:
            return CMD_PARAM_ERROR

# End Class


class NodeProcRxAntMode(wn_message.Cmd):
    """Command to get / set the receive antenna mode of the node.
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
        ant_mode  -- Receive antenna mode for the node.  Checking is
                     done both in the command and on the node.  The current
                     antenna mode will be returned by the node.  
    """
    def __init__(self, cmd, ant_mode=None):
        super(NodeProcRxAntMode, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_RX_ANT_MODE
        
        self.add_args(cmd)
        if ant_mode is not None:
            self.add_args(self.check_ant_mode(ant_mode))
        else:
            self.add_args(CMD_PARAM_NODE_CONFIG_ALL)


    def check_ant_mode(self, ant_mode):
        """Check the antenna mode to see if it is valid."""
        try:
            return ant_mode['index']
        except KeyError:
            msg  = "The antenna mode must be an entry from the wlan_rx_ant_mode\n"
            msg += "    list in wlan_exp.util\n"
            raise ValueError(msg)
    
    def process_resp(self, resp):
        import wlan_exp.util as util
        
        if resp.resp_is_valid(num_args=2, status_errors=[CMD_PARAM_ERROR], name='Rx antenna mode command'):
            args = resp.get_args()
            return util.find_rx_ant_mode_by_index(args[1])
        else:
            return CMD_PARAM_ERROR

# End Class


class NodeGetStationInfo(wn_message.BufferCmd):
    """Command to get the station info for a given node."""
    def __init__(self, node=None):
        super(NodeGetStationInfo, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_GET_STATION_INFO

        if node is not None:
            mac_address = node.wlan_mac_address
        else:
            mac_address = 0xFFFFFFFFFFFFFFFF            

        self.add_args(((mac_address >> 32) & 0xFFFF))
        self.add_args((mac_address & 0xFFFFFFFF))

    def process_resp(self, resp):
        # Contains a WWARPNet Buffer of all station info entries.  Need to 
        #   convert to a list of station info dictionaries.
        import wlan_exp.log.entry_types as entry_types

        index   = 0
        data    = resp.get_bytes()
        ret_val = entry_types.entry_station_info.deserialize(data[index:])

        return ret_val

# End Class



#--------------------------------------------
# Association Commands
#--------------------------------------------
class NodeDisassociate(wn_message.Cmd):
    """Command to remove associations from the association table."""
    name = None

    def __init__(self, device=None):
        super(NodeDisassociate, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_DISASSOCIATE

        if device is not None:
            self.name   = device.name
            mac_address = device.wlan_mac_address
        else:
            self.name   = "All nodes"
            mac_address = 0xFFFFFFFFFFFFFFFF            

        self.add_args(((mac_address >> 32) & 0xFFFF))
        self.add_args((mac_address & 0xFFFFFFFF))

    def process_resp(self, resp):
        resp.resp_is_valid(num_args=1, status_errors=[CMD_PARAM_ERROR], 
                           name='Node {0} Disassociate'.format(self.name))

# End Class



#--------------------------------------------
# Queue Commands
#--------------------------------------------
class QueueTxDataPurgeAll(wn_message.Cmd):
    """Command to purge all data transmit queues on the node."""
    def __init__(self):
        super(QueueTxDataPurgeAll, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_QUEUE_TX_DATA_PURGE_ALL
        
    def process_resp(self, resp):
        pass

# End Class



#--------------------------------------------
# AP Specific Commands
#--------------------------------------------
class NodeAPAssociate(wn_message.Cmd):
    """Command to add the association to the association table on the AP.
    
    Attributes:
        device        -- Device to add to the association table
        allow_timeout -- Allow the association to timeout if inactive
    
    NOTE:  This adds an association with the default tx/rx params.  If
        allow_timeout is not specified, the default on the node is to 
        not allow timeout of the association.
    """
    name = None

    def __init__(self, device, allow_timeout=None):
        super(NodeAPAssociate, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_ASSOCIATE

        flags = 0
        mask  = 0

        if allow_timeout is not None:
            mask += CMD_PARAM_ASSOCIATE_ALLOW_TIMEOUT
            if allow_timeout:
                flags += CMD_PARAM_ASSOCIATE_ALLOW_TIMEOUT
        
        self.add_args(flags)
        self.add_args(mask)

        self.name   = device.name
        mac_address = device.wlan_mac_address

        self.add_args(((mac_address >> 32) & 0xFFFF))
        self.add_args((mac_address & 0xFFFFFFFF))


    def process_resp(self, resp):
        args       = resp.get_args()

        if (resp.resp_is_valid(num_args=2, status_errors=[CMD_PARAM_ERROR], 
                               name='AP associate with {0}'.format(self.name))):
            return args[1]
        else:
            return CMD_PARAM_ERROR
    
# End Class


class NodeAPProcSSID(wn_message.Cmd):
    """Command to get / set the SSID of the AP."""
    ssid = None

    def __init__(self, ssid=None):
        super(NodeAPProcSSID, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_AP_SSID
        
        if ssid is None:
            self.add_args(CMD_PARAM_READ)
        else:
            self.ssid = ssid            
            self.add_args(CMD_PARAM_WRITE)

            _add_ssid_to_cmd(self, ssid)
            

    def process_resp(self, resp):
        args       = resp.get_args()
        arg_length = len(args)

        resp.resp_is_valid(num_args=arg_length, status_errors=[CMD_PARAM_ERROR], 
                                  name='Process SSID')

        # Actually check the number of arguments
        if(arg_length >= 2):
            length = args[1]
        else:
            raise Exception('ERROR: invalid response to process SSID - N_ARGS = {0}'.format(len(args)))

        # Get the SSID from the response
        if (length > 0):
            import struct
            ssid_buffer = struct.pack('!%dI' % (arg_length - 2), *args[2:] )
            ssid_tuple  = struct.unpack_from('!%ds' % length, ssid_buffer)

            # Python 3 vs 2 issue
            try:
                ssid    = str(ssid_tuple[0], encoding='UTF-8')
            except:
                ssid    = str(ssid_tuple[0])
        else:
            ssid        = ""
 
        if self.ssid is not None:
            if (self.ssid != ssid):
                msg  = "WARNING:  SSID requested:  {0} \n".format(self.ssid)
                msg += "    Does not equal SSID returned:  {0}".format(ssid)
                print(msg)
        
        return ssid

# End Class


class NodeAPSetAuthAddrFilter(wn_message.Cmd):
    """Command to set the authentication address filter on the node.
    
    Attributes:
        allow  -- List of (mask, address) tuples that will be used to filter addresses
                  on the node.

    NOTE:  For the mask, bits that are 1 are treated as "any" and bits that are 0 are 
    treated as "must equal".  For the address, locations of zeroed bits in the mask 
    must match the incoming addresses to pass the filter.
    """
    def __init__(self, allow):
        super(NodeAPSetAuthAddrFilter, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_AP_SET_AUTHENTICATION_ADDR_FILTER

        length = len(allow)

        if (length > 50):
            msg  = "Currently, the WLAN Exp framework does not support more than "
            msg += "50 address ranges in the association address filter."
            raise AttributeError(msg)

        self.add_args(CMD_PARAM_WRITE)
        self.add_args(length)
        
        for addr_range in allow:
            self.add_args(((addr_range[0] >> 32) & 0xFFFF))
            self.add_args((addr_range[0] & 0xFFFFFFFF))
            self.add_args(((addr_range[1] >> 32) & 0xFFFF))
            self.add_args((addr_range[1] & 0xFFFFFFFF))


    def process_resp(self, resp):
        resp.resp_is_valid(num_args=1, status_errors=[CMD_PARAM_ERROR], 
                           name='AP set association address filter')

# End Class



#--------------------------------------------
# STA Specific Commands
#--------------------------------------------
class NodeSTAConfigure(wn_message.Cmd):
    """Command to configure the STA.
    
    Attributes (default state on the node is in CAPS):
        beacon_ts_update    -- Enable timestamp updates from beacons (TRUE/False)
    """
    def __init__(self, beacon_ts_update=None):
        super(NodeSTAConfigure, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_STA_CONFIG

        flags = 0
        mask  = 0

        if beacon_ts_update is not None:
            mask += CMD_PARAM_NODE_STA_BEACON_TS_UPDATE
            if beacon_ts_update:
                flags += CMD_PARAM_NODE_STA_BEACON_TS_UPDATE
                
        self.add_args(flags)
        self.add_args(mask)
    
    def process_resp(self, resp):
        pass

# End Class


class NodeSTAAssociate(wn_message.Cmd):
    """Command to add the association to the association table on the STA.
    
    Attributes:
        device        -- Device to add to the association table
        aid           -- Association ID returned by the AP from the associate command
    
    NOTE:  This adds an association with the default tx/rx params
    """
    name = None
    def __init__(self, device, aid):
        super(NodeSTAAssociate, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_ASSOCIATE

        flags = 0
        mask  = 0

        # Currently, there are no association flags, but adding this into the 
        # message structure for future use.

        self.add_args(flags)
        self.add_args(mask)

        self.name   = device.name
        mac_address = device.wlan_mac_address

        self.add_args(((mac_address >> 32) & 0xFFFF))
        self.add_args((mac_address & 0xFFFFFFFF))

        self.add_args(aid)


    def process_resp(self, resp):
        if (resp.resp_is_valid(num_args=1, status_errors=[CMD_PARAM_ERROR], 
                               name='STA associate with {0}'.format(self.name))):
            return True
        else:
            return False

# End Class



#--------------------------------------------
# Memory Access Commands - For developer use only
#--------------------------------------------
class NodeMemAccess(wn_message.Cmd):
    """Command to read/write memory in CPU High / CPU Low
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
        high      -- True for CPU_High access, False for CPU_Low
        
        address   -- u32 memory address to read/write

        values    -- When cmd==CMD_PARAM_WRITE, scalar or list of u32 values to write
                     When cmd==CMD_PARAM_READ, None

        length    -- When cmd==CMD_PARAM_WRITE, None
                     When cmd==CMD_PARAM_READ, number of u32 values to read starting at address
    """
    _read_len = None
    
    def __init__(self, cmd, high, address, values=None, length=None):
        super(NodeMemAccess, self).__init__()
        if(high):
            self.command = _CMD_GRPID_NODE + CMDID_DEV_MEM_HIGH
        else:
            self.command = _CMD_GRPID_NODE + CMDID_DEV_MEM_LOW

        if(cmd == CMD_PARAM_READ):
            self.add_args(cmd)
            self.add_args(address)
            self.add_args(length)

            self._read_len = length

        elif(cmd == CMD_PARAM_WRITE):
            self.add_args(cmd)
            self.add_args(address)
            self.add_args(length)

            try:
                for v in values:
                    self.add_args(v)
            except TypeError:
                self.add_args(values)

        else:
            raise Exception('ERROR: NodeMemAccess constructor arguments invalid');
    
    def process_resp(self, resp):
        if (self._read_len is not None): # Was a read command
            if resp.resp_is_valid(num_args=(2 + self._read_len), status_errors=[CMD_PARAM_ERROR], 
                                  name='CPU Mem command'):
                args = resp.get_args()

                if(len(args) == 3):
                    return args[2]
                elif(len(args) > 3):
                    return args[2:]
                else:
                    raise Exception('ERROR: invalid response to read_mem - N_ARGS = {0}'.format(len(args)))
            else:
                return CMD_PARAM_ERROR
        else: # Was a write command
            pass

# End Class




#--------------------------------------------
# Misc Helper methods
#--------------------------------------------
def _add_ssid_to_cmd(cmd, ssid):
    """Internal method to add an ssid to the given command"""
    import struct
 
    ssid_len = len(ssid)
    
    if (ssid_len > CMD_PARAM_MAX_SSID_LEN):
        ssid_len = CMD_PARAM_MAX_SSID_LEN
        ssid     = ssid[:CMD_PARAM_MAX_SSID_LEN]        

        msg  = "WARNING:  Maximum SSID length is {0} ".format(CMD_PARAM_MAX_SSID_LEN)
        msg += "truncating to {0}.".format(ssid)
        print(msg)        
    
    cmd.add_args(ssid_len)

    # Null-teriminate the string for C
    ssid    += "\0"
    ssid_buf = bytearray(ssid, 'UTF-8')
    
    # Zero pad so that the ssid buffer is 32-bit aligned
    if ((len(ssid_buf) % 4) != 0):
        ssid_buf += bytearray(4 - (len(ssid_buf) % 4))
    
    idx = 0
    while (idx < len(ssid_buf)):
        arg = struct.unpack_from('!I', ssid_buf[idx:idx+4])
        cmd.add_args(arg[0])
        idx += 4

# End def





