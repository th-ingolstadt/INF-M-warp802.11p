# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment Commands
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2015, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
MODIFICATION HISTORY:

Ver   Who  Date     Changes
----- ---- -------- -----------------------------------------------------
1.00a ejw  1/23/14  Initial release

------------------------------------------------------------------------------

This module provides class definitions for all WLAN Exp commands.

"""

import wlan_exp.transport.cmds as cmds
import wlan_exp.transport.message as message
import wlan_exp.transport.transport_eth_ip_udp as transport



__all__ = [# Log command classes
           'LogGetEvents', 'LogConfigure', 'LogGetStatus', 'LogGetCapacity', 
           'LogAddExpInfoEntry', 'LogAddCountsTxRx',
           # Counts command classes
           'CountsConfigure', 'CountsGetTxRx',
           # LTG classes
           'LTGConfigure', 'LTGStart', 'LTGStop', 'LTGRemove', 'LTGStatus',
           # Node command classes
           'NodeResetState', 'NodeConfigure', 'NodeProcWLANMACAddr', 
           'NodeProcTime', 'NodeSetLowToHighFilter', 'NodeProcRandomSeed', 
           'NodeLowParam', 'NodeProcTxPower', 'NodeProcTxRate', 
           'NodeProcTxAntMode', 'NodeProcRxAntMode',
           # Scan command classes
           'NodeProcScanParam', 'NodeProcScan', 
           # Association command classes
           'NodeConfigBSS', 'NodeDisassociate', 'NodeGetStationInfo', 
           'NodeGetBSSInfo',
           # Queue command classes
           'QueueTxDataPurgeAll',
           # AP command classes
           'NodeAPConfigure', 'NodeAPAddAssociation', 'NodeAPSetAuthAddrFilter',
           # STA command classes
           'NodeSTASetAID', 'NodeSTAJoin', 'NodeSTAJoinStatus',
           # IBSS command classes
           # None
           # User command classes
           'UserSendCmd'
          ]


# WLAN Exp Command IDs
#   NOTE:  The C counterparts are found in wlan_exp_node.h
#   NOTE:  All Command IDs (CMDID_*) must be unique 24-bit numbers

# Node commands and defined values
CMDID_NODE_RESET_STATE                           = 0x001000
CMDID_NODE_CONFIGURE                             = 0x001001
CMDID_NODE_CONFIG_BSS                            = 0x001002
CMDID_NODE_TIME                                  = 0x001010
CMDID_NODE_TX_POWER                              = 0x001012
CMDID_NODE_TX_RATE                               = 0x001013
CMDID_NODE_TX_ANT_MODE                           = 0x001014
CMDID_NODE_RX_ANT_MODE                           = 0x001015
CMDID_NODE_LOW_TO_HIGH_FILTER                    = 0x001016
CMDID_NODE_RANDOM_SEED                           = 0x001017
CMDID_NODE_WLAN_MAC_ADDR                         = 0x001018
CMDID_NODE_LOW_PARAM                             = 0x001020

CMD_PARAM_WRITE                                  = 0x00000000
CMD_PARAM_READ                                   = 0x00000001
CMD_PARAM_WRITE_DEFAULT                          = 0x00000002
CMD_PARAM_READ_DEFAULT                           = 0x00000004
CMD_PARAM_RSVD                                   = 0xFFFFFFFF

CMD_PARAM_SUCCESS                                = 0x00000000
CMD_PARAM_ERROR                                  = 0xFF000000

CMD_PARAM_UNICAST                                = 0x00000000
CMD_PARAM_MULTICAST_DATA                         = 0x00000001
CMD_PARAM_MULTICAST_MGMT                         = 0x00000002

CMD_PARAM_NODE_CONFIG_ALL                        = 0xFFFFFFFF

CMD_PARAM_NODE_RESET_FLAG_LOG                    = 0x00000001
CMD_PARAM_NODE_RESET_FLAG_TXRX_COUNTS            = 0x00000002
CMD_PARAM_NODE_RESET_FLAG_LTG                    = 0x00000004
CMD_PARAM_NODE_RESET_FLAG_TX_DATA_QUEUE          = 0x00000008
CMD_PARAM_NODE_RESET_FLAG_BSS                    = 0x00000010
CMD_PARAM_NODE_RESET_FLAG_NETWORK_LIST           = 0x00000020

CMD_PARAM_NODE_CONFIG_FLAG_DSSS_ENABLE           = 0x00000001
CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TIME_UPDATE    = 0x00000002
CMD_PARAM_NODE_CONFIG_SET_WLAN_EXP_PRINT_LEVEL   = 0x80000000

ERROR_CONFIG_BSS_BSSID_INVALID                   = 0x000001
ERROR_CONFIG_BSS_BSSID_INSUFFICIENT_ARGUMENTS    = 0x000002
ERROR_CONFIG_BSS_CHANNEL_INVALID                 = 0x000004
ERROR_CONFIG_BSS_BEACON_INTERVAL_INVALID         = 0x000008
ERROR_CONFIG_BSS_HT_CAPABLE_INVALID              = 0x000010

CMD_PARAM_TIME_ADD_TO_LOG                        = 0x00000002
CMD_PARAM_RSVD_TIME                              = 0xFFFFFFFF

TIME_TYPE_FLOAT                                  = 0x00000000
TIME_TYPE_INT                                    = 0x00000001

CMD_PARAM_RSVD_MAC_ADDR                          = 0x00000000

CMD_PARAM_NODE_TX_POWER_LOW                      = 0x00000010
CMD_PARAM_NODE_TX_POWER_ALL                      = 0x00000020
CMD_PARAM_NODE_TX_POWER_MAX_DBM                  = 21
CMD_PARAM_NODE_TX_POWER_MIN_DBM                  = -9

CMD_PARAM_NODE_TX_ANT_ALL                        = 0x00000010

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
CMD_PARAM_LOW_PARAM_BB_GAIN                      = 0x00000001
CMD_PARAM_LOW_PARAM_LINEARITY_PA                 = 0x00000002
CMD_PARAM_LOW_PARAM_LINEARITY_VGA                = 0x00000003
CMD_PARAM_LOW_PARAM_LINEARITY_UPCONV             = 0x00000004
CMD_PARAM_LOW_PARAM_AD_SCALING                   = 0x00000005
CMD_PARAM_LOW_PARAM_PKT_DET_MIN_POWER            = 0x00000006

CMD_PARAM_LOW_PARAM_DCF_RTS_THRESH               = 0x10000001
CMD_PARAM_LOW_PARAM_DCF_DOT11SHORTRETRY          = 0x10000002
CMD_PARAM_LOW_PARAM_DCF_DOT11LONGRETRY           = 0x10000003
CMD_PARAM_LOW_PARAM_DCF_PHYSICAL_CS_THRESH       = 0x10000004
CMD_PARAM_LOW_PARAM_DCF_CW_EXP_MIN               = 0x10000005
CMD_PARAM_LOW_PARAM_DCF_CW_EXP_MAX               = 0x10000006

CMD_PARAM_NODE_MIN_MIN_PKT_DET_POWER_DBM         = -90
CMD_PARAM_NODE_MAX_MIN_PKT_DET_POWER_DBM         = -30

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
CMDID_LOG_ADD_COUNTS_TXRX                        = 0x003005
CMDID_LOG_ENABLE_ENTRY                           = 0x003006
CMDID_LOG_STREAM_ENTRIES                         = 0x003007


CMD_PARAM_LOG_GET_ALL_ENTRIES                    = 0xFFFFFFFF

CMD_PARAM_LOG_CONFIG_FLAG_LOGGING                = 0x00000001
CMD_PARAM_LOG_CONFIG_FLAG_WRAP                   = 0x00000002
CMD_PARAM_LOG_CONFIG_FLAG_LOG_PAYLOADS           = 0x00000004
CMD_PARAM_LOG_CONFIG_FLAG_LOG_CMDS               = 0x00000008
CMD_PARAM_LOG_CONFIG_FLAG_TXRX_MPDU              = 0x00000010
CMD_PARAM_LOG_CONFIG_FLAG_TXRX_CTRL              = 0x00000020


# Counts commands and defined values
CMDID_COUNTS_CONFIG_TXRX                         = 0x004000
CMDID_COUNTS_GET_TXRX                            = 0x004001

CMD_PARAM_COUNTS_CONFIG_FLAG_PROMISC             = 0x00000001

CMD_PARAM_COUNTS_RETURN_ZEROED_IF_NONE           = 0x80000000


# Queue commands and defined values
CMDID_QUEUE_TX_DATA_PURGE_ALL                    = 0x005000


# Scan commands and defined values
CMDID_NODE_SCAN_PARAM                            = 0x006000
CMDID_NODE_SCAN                                  = 0x006001

CMD_PARAM_NODE_SCAN_ENABLE                       = 0x00000001
CMD_PARAM_NODE_SCAN_DISABLE                      = 0x00000000


# Association commands and defined values
CMDID_GET_STATION_INFO                           = 0x007001
CMDID_GET_BSS_INFO                               = 0x007002

CMDID_NODE_DISASSOCIATE                          = 0x007010
CMDID_NODE_ADD_ASSOCIATION                       = 0x007011

CMD_PARAM_ADD_ASSOCIATION_ALLOW_TIMEOUT          = 0x00000001

CMD_PARAM_GET_ALL_ASSOCIATED                     = 0x0000000000000000

CMD_PARAM_MAX_SSID_LEN                           = 32
CMD_PARAM_RSVD_CHANNEL                           = 0


# AP commands and defined values
CMDID_NODE_AP_CONFIG                             = 0x100000
CMDID_NODE_AP_SET_AUTHENTICATION_ADDR_FILTER     = 0x100001

CMD_PARAM_AP_CONFIG_FLAG_DTIM_MULTICAST_BUFFER   = 0x00000001


# STA commands and defined values
CMDID_NODE_STA_JOIN                              = 0x100000
CMDID_NODE_STA_JOIN_STATUS                       = 0x100001
CMDID_NODE_STA_SET_AID                           = 0x100002


# IBSS commands and defined values


# Developer commands and defined values
CMDID_DEV_MEM_HIGH                               = 0xFFF000
CMDID_DEV_MEM_LOW                                = 0xFFF001
CMDID_DEV_EEPROM                                 = 0xFFF002


# Local Constants
_CMD_GROUP_NODE                                  = (cmds.GROUP_NODE << 24)
_CMD_GROUP_USER                                  = (cmds.GROUP_USER << 24)


#-----------------------------------------------------------------------------
# Class Definitions for WLAN Exp Commands
#-----------------------------------------------------------------------------

#--------------------------------------------
# Log Commands
#--------------------------------------------
class LogGetEvents(message.BufferCmd):
    """Command to get the WLAN Exp log events of the node"""
    def __init__(self, size, start_byte=0):
        command = _CMD_GROUP_NODE + CMDID_LOG_GET_ENTRIES

        if (size == CMD_PARAM_LOG_GET_ALL_ENTRIES):
            size = message.CMD_BUFFER_GET_SIZE_FROM_DATA

        super(LogGetEvents, self).__init__(
                command=command, buffer_id=0, flags=0, start_byte=start_byte, size=size)

    def process_resp(self, resp):
        return resp

# End Class


class LogConfigure(message.Cmd):
    """Command to configure the Event log.

    Attributes (default state on the node is in CAPS):
        log_enable           -- Enable the event log (TRUE/False)
        log_wrap_enable      -- Enable event log wrapping (True/FALSE)
        log_full_payloads    -- Record full Tx/Rx payloads in event log (True/FALSE)
        log_commands         -- Record commands in event log (True/FALSE)
        log_txrx_mpdu        -- Enable Tx/Rx log entries for MPDU frames (TRUE/False)
        log_txrx_ctrl        -- Enable Tx/Rx log entries for CTRL frames (TRUE/False)
    """
    def __init__(self, log_enable=None, log_wrap_enable=None,
                       log_full_payloads=None, log_commands=None,
                       log_txrx_mpdu=None, log_txrx_ctrl=None):
        super(LogConfigure, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_LOG_CONFIG

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

        if log_commands is not None:
            mask += CMD_PARAM_LOG_CONFIG_FLAG_LOG_CMDS
            if log_commands:
                flags += CMD_PARAM_LOG_CONFIG_FLAG_LOG_CMDS

        if log_txrx_mpdu is not None:
            mask += CMD_PARAM_LOG_CONFIG_FLAG_TXRX_MPDU
            if log_txrx_mpdu:
                flags += CMD_PARAM_LOG_CONFIG_FLAG_TXRX_MPDU

        if log_txrx_ctrl is not None:
            mask += CMD_PARAM_LOG_CONFIG_FLAG_TXRX_CTRL
            if log_txrx_ctrl:
                flags += CMD_PARAM_LOG_CONFIG_FLAG_TXRX_CTRL

        self.add_args(flags)
        self.add_args(mask)

    def process_resp(self, resp):
        pass

# End Class


class LogGetStatus(message.Cmd):
    """Command to get the state information about the log."""
    def __init__(self):
        super(LogGetStatus, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_LOG_GET_STATUS

    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=4):
            args = resp.get_args()
            return (args[0], args[1], args[2], args[3])
        else:
            return (0,0,0,0)

# End Class


class LogGetCapacity(message.Cmd):
    """Command to get the log capacity and current use."""
    def __init__(self):
        super(LogGetCapacity, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_LOG_GET_CAPACITY

    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=2):
            args = resp.get_args()
            return (args[0], args[1])
        else:
            return (0,0)

# End Class


class LogStreamEntries(message.Cmd):
    """Command to configure the node log streaming."""
    def __init__(self, enable, host_id, ip_address, port):
        super(LogStreamEntries, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_LOG_STREAM_ENTRIES

        if (type(ip_address) is str):
            addr = transport.ip_to_int(ip_address)
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


class LogAddExpInfoEntry(message.Cmd):
    """Command to write a EXP_INFO Log Entry to the node."""
    def __init__(self, info_type, message):
        super(LogAddExpInfoEntry, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_LOG_ADD_EXP_INFO_ENTRY

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


class LogAddCountsTxRx(message.Cmd):
    """Command to add the current counts to the Event log"""
    def __init__(self):
        super(LogAddCountsTxRx, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_LOG_ADD_COUNTS_TXRX

    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=1):
            args = resp.get_args()
            return args[0]
        else:
            return 0

# End Class



#--------------------------------------------
# Counts Commands
#--------------------------------------------
class CountsConfigure(message.Cmd):
    """Command to configure the Counts collection.

    Attributes (default state on the node is in CAPS):
        promisc_counts       -- Enable promiscuous counts collection (TRUE/False)
    """
    def __init__(self, promisc_counts=None):
        super(CountsConfigure, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_COUNTS_CONFIG_TXRX

        flags = 0
        mask  = 0

        if promisc_counts is not None:
            mask += CMD_PARAM_COUNTS_CONFIG_FLAG_PROMISC
            if promisc_counts:
                flags += CMD_PARAM_COUNTS_CONFIG_FLAG_PROMISC

        self.add_args(flags)
        self.add_args(mask)

    def process_resp(self, resp):
        pass

# End Class


class CountsGetTxRx(message.BufferCmd):
    """Command to get the counts from the node for a given node."""
    def __init__(self, node=None, return_zeroed_counts_if_none=False):
        flags       = 0

        # Compute the argument for the BufferCmd flags
        if return_zeroed_counts_if_none:
            flags += CMD_PARAM_COUNTS_RETURN_ZEROED_IF_NONE

        # Call parent initialziation
        super(CountsGetTxRx, self).__init__(flags=flags)
        self.command = _CMD_GROUP_NODE + CMDID_COUNTS_GET_TXRX
        mac_address  = None

        if node is not None:
            mac_address = node.wlan_mac_address

        _add_mac_address_to_cmd(self, mac_address)


    def process_resp(self, resp):
        # Contains a Buffer of all Tx / Rx counts.  Need to convert to a list
        #   of TxRxCounts()
        import wlan_exp.info as info

        index   = 0
        data    = resp.get_bytes()
        ret_val = info.deserialize_info_buffer(data[index:], "TxRxCounts()")

        return ret_val

# End Class



#--------------------------------------------
# Local Traffic Generation (LTG) Commands
#--------------------------------------------
class LTGCommon(message.Cmd):
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
        error_code    = CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR
        error_msg     = "Could not {0} the LTG with that LTG ID.".format(self.name)
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=1,
                              status_errors=status_errors,
                              name='for the LTG {0} command'.format(self.name)):
            args = resp.get_args()
            return args[0]
        else:
            return CMD_PARAM_ERROR

# End Class


class LTGConfigure(message.Cmd):
    """Command to configure an LTG with the given traffic flow to the
    specified node.
    """
    name = 'configure'

    def __init__(self, traffic_flow, auto_start=False):
        super(LTGConfigure, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_LTG_CONFIG

        flags = 0

        if auto_start:
            flags += CMD_PARAM_LTG_CONFIG_FLAG_AUTOSTART

        self.add_args(flags)

        for arg in traffic_flow.serialize():
            self.add_args(arg)

    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR
        error_msg     = "\n\nERROR MESSAGE: Could not create the LTG.  Check that the node \n"
        error_msg    += "has enough heap available to perform this operation. Commonly, \n"
        error_msg    += "this can occur if LTGs are not removed (ie 'ltg_remove(ltg_id)') \n"
        error_msg    += "when they are finished being used.\n"
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=2,
                              status_errors=status_errors,
                              name='for the LTG {0} command'.format(self.name)):
            args = resp.get_args()
            return args[1]
        else:
            return CMD_PARAM_ERROR

# End Class


class LTGStart(LTGCommon):
    """Command to start a configured LTG to the given node.

    NOTE:  By providing no node argument, this command will start all
    configured LTGs on the node.
    """
    name = 'start'

    def __init__(self, ltg_id=None):
        super(LTGStart, self).__init__(ltg_id)
        self.command = _CMD_GROUP_NODE + CMDID_LTG_START

# End Class


class LTGStop(LTGCommon):
    """Command to stop a configured LTG to the given node.

    NOTE:  By providing no node argument, this command will stop all
    configured LTGs on the node.
    """
    name = 'stop'

    def __init__(self, ltg_id=None):
        super(LTGStop, self).__init__(ltg_id)
        self.command = _CMD_GROUP_NODE + CMDID_LTG_STOP

# End Class


class LTGRemove(LTGCommon):
    """Command to remove a configured LTG to the given node.

    NOTE:  By providing no node argument, this command will remove all
    configured LTGs on the node.
    """
    name = 'remove'

    def __init__(self, ltg_id=None):
        super(LTGRemove, self).__init__(ltg_id)
        self.command = _CMD_GROUP_NODE + CMDID_LTG_REMOVE

# End Class


class LTGStatus(message.Cmd):
    """Command to get the status of the LTG."""
    time_factor = 6
    name        = 'status'

    def __init__(self, ltg_id):
        super(LTGStatus, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_LTG_STATUS

        if type(ltg_id) is not int:
            raise TypeError("LTG ID must be an integer.")
        self.add_args(ltg_id)

    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR
        error_msg     = "\n\nERROR MESSAGE:  Could not find status for given LTG ID.\n"
        error_msg    += "Please check that it has not been removed."
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=6,
                              status_errors=status_errors,
                              name='for the LTG {0} command'.format(self.name)):
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
class NodeResetState(message.Cmd):
    """Command to reset the state of a portion of the node defined by the flags.

    Attributes:
        flags -- [0] NODE_RESET_LOG
                 [1] NODE_RESET_TXRX_STATS
                 [2] NODE_RESET_LTG
                 [3] NODE_RESET_TX_DATA_QUEUE
                 [4] NODE_RESET_FLAG_BSS
                 [5] NODE_RESET_FLAG_NETWORK_LIST
    """
    def __init__(self, flags):
        super(NodeResetState, self).__init__()
        self.command = _CMD_GROUP_NODE +  CMDID_NODE_RESET_STATE
        self.add_args(flags)

    def process_resp(self, resp):
        pass

# End Class


class NodeConfigure(message.Cmd):
    """Command to configure flag parameters on the node

    Attributes:
        dsss_enable (bool) -- Whether DSSS packets are received.
        beacon_mac_time_update (bool) -- Whether MAC time is updated from beacons
        print_level (int) -- Controls WLAN Exp print level
    """
    def __init__(self, dsss_enable=None, beacon_mac_time_update=None, print_level=None):
        super(NodeConfigure, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_CONFIGURE

        flags = 0
        mask  = 0
        level = 0

        if dsss_enable is not None:
            mask += CMD_PARAM_NODE_CONFIG_FLAG_DSSS_ENABLE
            if dsss_enable:
                flags += CMD_PARAM_NODE_CONFIG_FLAG_DSSS_ENABLE

        if beacon_mac_time_update is not None:
            mask += CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TIME_UPDATE
            if beacon_mac_time_update:
                flags += CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TIME_UPDATE

        if print_level is not None:
            level = CMD_PARAM_NODE_CONFIG_SET_WLAN_EXP_PRINT_LEVEL + print_level

        self.add_args(flags)
        self.add_args(mask)
        self.add_args(level)

    def process_resp(self, resp):
        pass

# End Class


class NodeProcWLANMACAddr(message.Cmd):
    """Command to get / set the WLAN MAC Address on the node.

    Attributes:
        cmd           -- Sub-command to send over the command.  Valid values are:
                           CMD_PARAM_READ
                           CMD_PARAM_WRITE
        wlan_mac_addr -- 48-bit MAC address to write (optional)
    """
    def __init__(self, cmd, wlan_mac_address=None):
        super(NodeProcWLANMACAddr, self).__init__()
        self.command  = _CMD_GROUP_NODE + CMDID_NODE_WLAN_MAC_ADDR

        if (cmd == CMD_PARAM_WRITE):
            self.add_args(cmd)
            _add_mac_address_to_cmd(self, wlan_mac_address)

        elif (cmd == CMD_PARAM_READ):
            self.add_args(cmd)

        else:
            msg = "Unsupported command: {0}".format(cmd)
            raise ValueError(msg)


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not get / set the WLAN MAC address on the node"
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=3, status_errors=status_errors, name='from the WLAN Mac Address command'):
            args = resp.get_args()
            addr = (2**32 * args[1]) + args[2]
        else:
            addr = None

        return addr

# End Class


class NodeProcTime(message.Cmd):
    """Command to get / set the time on the node.

    NOTE:  Python time functions operate on floating point numbers in
        seconds, while the WARP Node operates on microseconds.  In order
        to be more flexible, this class can be initialized with either
        type of input.  It will return the time based on the type of the
        input (either interger microseconds or float seconds).

    Attributes:
        cmd       -- Sub-command to send over the command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
                       TIME_ADD_TO_LOG
        node_time -- Time as either an integer number of microseconds or
                       a floating point number in seconds.
        time_id   -- ID to use identify the time command in the log.
    """
    time_factor = 6
    time_type   = None

    def __init__(self, cmd, node_time, time_id=None):
        super(NodeProcTime, self).__init__()
        self.command  = _CMD_GROUP_NODE + CMDID_NODE_TIME

        # Read the time as a float
        if (cmd == CMD_PARAM_READ):
            self.time_type = TIME_TYPE_FLOAT
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
            else:
                self.add_args(CMD_PARAM_TIME_ADD_TO_LOG)
                node_time = None

            # Get the current time on the host
            now = int(round(time.time(), self.time_factor) * (10**self.time_factor))

            self.add_args(int(time_id))
            self.time_type = _add_time_to_cmd64(self, node_time, self.time_factor)
            self.add_args((now & 0xFFFFFFFF))
            self.add_args(((now >> 32) & 0xFFFFFFFF))


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not get / set the time on the node"
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=5, status_errors=status_errors, name='from the Time command'):
            args = resp.get_args()
            mac_time = (2**32 * args[2]) + args[1]
            sys_time = (2**32 * args[4]) + args[3]
        else:
            mac_time = 0
            sys_time = 0

        if (self.time_type == TIME_TYPE_FLOAT):
            mac_time = float(mac_time / (10**self.time_factor))
            sys_time = float(sys_time / (10**self.time_factor))

        # Use existing mac_time, sys_time if time_type is TIME_TYPE_INT

        return (mac_time, sys_time)

# End Class


class NodeSetLowToHighFilter(message.Cmd):
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
        self.command  = _CMD_GROUP_NODE + CMDID_NODE_LOW_TO_HIGH_FILTER

        self.add_args(cmd)

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
                rx_filter += CMD_PARAM_RX_FILTER_FCS_ALL
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


class NodeProcRandomSeed(message.Cmd):
    """Command to set the random seed of the node.

    Attributes:
        cmd       -- Sub-command to send over the command.  Valid values are:
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
        self.command = _CMD_GROUP_NODE + CMDID_NODE_RANDOM_SEED

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


class NodeLowParam(message.Cmd):
    """Command to set parameter in CPU Low

    Attributes:
        cmd          -- Sub-command to send over the command.  Valid values are:
                          CMD_PARAM_READ   (Not supported)
                          CMD_PARAM_WRITE
        param_id     -- ID of parameter to modify
        param_values -- Scalar or list of u32 values to write

    """
    def __init__(self, cmd, param_id, param_values=None):
        super(NodeLowParam, self).__init__()

        self.command    = _CMD_GROUP_NODE + CMDID_NODE_LOW_PARAM

        if (cmd == CMD_PARAM_READ):
            raise AttributeError("Read not supported for NodeLowParam.")

        # Caluculate the size of the entire message to CPU Low [PARAM_ID, ARGS[]]
        size = 1
        if param_values is not None:
            try:
                size += len(param_values)
            except TypeError:
                pass

        self.add_args(cmd)
        self.add_args(size)
        self.add_args(param_id)

        if param_values is not None:
            try:
                for v in param_values:
                    self.add_args(v)
            except TypeError:
                self.add_args(param_values)

    def process_resp(self, resp):
        """ Message format:
                respArgs32[0]   Status
        """
        error_code_base         = CMD_PARAM_ERROR
        error_code_cs_thresh    = CMD_PARAM_ERROR + CMD_PARAM_LOW_PARAM_DCF_PHYSICAL_CS_THRESH
        error_code_cw_min       = CMD_PARAM_ERROR + CMD_PARAM_LOW_PARAM_DCF_CW_EXP_MIN
        error_code_cw_max       = CMD_PARAM_ERROR + CMD_PARAM_LOW_PARAM_DCF_CW_EXP_MAX

        status_errors = {error_code_base      : "Error setting low parameter",
                         error_code_cs_thresh : "Could not set carrier sense threshold",
                         error_code_cw_min    : "Could not set minimum contention window",
                         error_code_cw_max    : "Could not set maximum contention window"}

        resp.resp_is_valid(num_args=1, status_errors=status_errors, name='from Low Param command')

        return None

# End Class


class NodeProcTxPower(message.Cmd):
    """Command to get / set the transmit power of the node.

    Attributes:
        cmd       -- Sub-command to send over the command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
                       CMD_PARAM_WRITE_DEFAULT
                       CMD_PARAM_READ_DEFAULT
        tx_type   -- Which Tx parameters to set.  Valid values are:
                       CMD_PARAM_UNICAST
                       CMD_PARAM_MULTICAST_DATA
                       CMD_PARAM_MULTICAST_MGMT
                       CMD_PARAM_NODE_TX_POWER_LOW
                       CMD_PARAM_NODE_TX_POWER_ALL
        power     -- Transmit power for the WARP node (in dBm).
        device    -- 802.11 device for which the rate is being set.
    """
    rate     = None
    dev_name = None

    def __init__(self, cmd, tx_type, power=None, device=None):
        super(NodeProcTxPower, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_TX_POWER
        mac_address  = None

        self.add_args(cmd)

        self.add_args(self.check_type(tx_type))

        if (cmd == CMD_PARAM_WRITE):
            self.add_args(self.check_power(power))
        else:
            self.add_args(0)

        if device is not None:
            mac_address = device.wlan_mac_address

        _add_mac_address_to_cmd(self, mac_address)


    def check_type(self, tx_type):
        """Check the tx type value is valid."""
        return_type = None
        valid_types = [('CMD_PARAM_UNICAST',           CMD_PARAM_UNICAST),
                       ('CMD_PARAM_MULTICAST_DATA',    CMD_PARAM_MULTICAST_DATA),
                       ('CMD_PARAM_MULTICAST_MGMT',    CMD_PARAM_MULTICAST_MGMT),
                       ('CMD_PARAM_NODE_TX_POWER_LOW', CMD_PARAM_NODE_TX_POWER_LOW),
                       ('CMD_PARAM_NODE_TX_POWER_ALL', CMD_PARAM_NODE_TX_POWER_ALL)]

        for tmp_type in valid_types:
            if (tx_type == tmp_type[1]):
                return_type = tx_type
                break

        if return_type is not None:
            return return_type
        else:
            msg  = "The type must be one of: ["
            for tmp_type in valid_types:
                msg += "{0} ".format(tmp_type[0])
            msg += "]"
            raise ValueError(msg)


    def check_power(self, power):
        """Check the power value is valid."""
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
        return (power - CMD_PARAM_NODE_TX_POWER_MIN_DBM)


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not get / set the transmit power on the node"
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=2, status_errors=status_errors, name='from Tx power command'):
            args = resp.get_args()

            # Shift the value since only positive integers are transmitted over the wire
            return (args[1] + CMD_PARAM_NODE_TX_POWER_MIN_DBM)
        else:
            return None

# End Class


class NodeProcTxRate(message.Cmd):
    """Command to get / set the transmit rate of the node.

    Attributes:
        cmd       -- Sub-command to send over the command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
                       CMD_PARAM_WRITE_DEFAULT
                       CMD_PARAM_READ_DEFAULT
        tx_type   -- Which Tx parameters to set.  Valid values are:
                       CMD_PARAM_UNICAST
                       CMD_PARAM_MULTICAST_DATA
                       CMD_PARAM_MULTICAST_MGMT
        rate      -- Tuple of values that make up Tx rate
                       [0] - mcs       -- Modulation and coding scheme (MCS) index
                       [1] - phy_mode  -- PHY mode (from util.phy_modes)
        device    -- 802.11 device for which the rate is being set.
    """
    def __init__(self, cmd, tx_type, rate=None, device=None):
        super(NodeProcTxRate, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_TX_RATE
        mac_address  = None
        mcs          = None
        phy_mode     = None

        self.add_args(cmd)

        self.add_args(self.check_type(tx_type))

        if (rate is not None):
            mcs      = rate[0]
            phy_mode = rate[1]

        if (mcs is not None):
            self.add_args(mcs)
        else:
            self.add_args(0)

        if (phy_mode is not None):
            if (type(phy_mode) is str):
                import wlan_exp.util as util
                self.add_args(util.phy_modes[phy_mode])
            else:
                self.add_args(phy_mode)
        else:
            self.add_args(0)

        if device is not None:
            mac_address   = device.wlan_mac_address

        _add_mac_address_to_cmd(self, mac_address)


    def check_type(self, tx_type):
        """Check the tx type value is valid."""
        return_type = None
        valid_types = [('CMD_PARAM_UNICAST',           CMD_PARAM_UNICAST),
                       ('CMD_PARAM_MULTICAST_DATA',    CMD_PARAM_MULTICAST_DATA),
                       ('CMD_PARAM_MULTICAST_MGMT',    CMD_PARAM_MULTICAST_MGMT)]

        for tmp_type in valid_types:
            if (tx_type == tmp_type[1]):
                return_type = tx_type
                break

        if return_type is not None:
            return return_type
        else:
            msg  = "The type must be one of: ["
            for tmp_type in valid_types:
                msg += "{0} ".format(tmp_type[0])
            msg += "]"
            raise ValueError(msg)

    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not get / set transmit rate on the node"
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=3, status_errors=status_errors, name='from Tx rate command'):
            args = resp.get_args()
            return (args[1], args[2])
        else:
            return None

# End Class


class NodeProcTxAntMode(message.Cmd):
    """Command to get / set the transmit antenna mode of the node.

    Attributes:
        cmd       -- Sub-command to send over the command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
                       CMD_PARAM_WRITE_DEFAULT
                       CMD_PARAM_READ_DEFAULT
        tx_type   -- Which Tx parameters to set.  Valid values are:
                       CMD_PARAM_UNICAST
                       CMD_PARAM_MULTICAST_DATA
                       CMD_PARAM_MULTICAST_MGMT
                       CMD_PARAM_NODE_TX_ANT_ALL
        ant_mode  -- Transmit antenna mode for the node.  Checking is
                     done both in the command and on the node.  The current
                     antenna mode will be returned by the node.
        device    -- 802.11 device for which the rate is being set.
    """
    def __init__(self, cmd, tx_type, ant_mode=None, device=None):
        super(NodeProcTxAntMode, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_TX_ANT_MODE
        mac_address  = None

        self.add_args(cmd)

        self.add_args(self.check_type(tx_type))

        if ant_mode is not None:
            self.add_args(self.check_ant_mode(ant_mode))
        else:
            self.add_args(0)

        if device is not None:
            mac_address = device.wlan_mac_address

        _add_mac_address_to_cmd(self, mac_address)


    def check_type(self, tx_type):
        """Check the tx type value is valid."""
        return_type = None
        valid_types = [('CMD_PARAM_UNICAST',           CMD_PARAM_UNICAST),
                       ('CMD_PARAM_MULTICAST_DATA',    CMD_PARAM_MULTICAST_DATA),
                       ('CMD_PARAM_MULTICAST_MGMT',    CMD_PARAM_MULTICAST_MGMT),
                       ('CMD_PARAM_NODE_TX_ANT_ALL',   CMD_PARAM_NODE_TX_ANT_ALL)]

        for tmp_type in valid_types:
            if (tx_type == tmp_type[1]):
                return_type = tx_type
                break

        if return_type is not None:
            return return_type
        else:
            msg  = "The type must be one of: ["
            for tmp_type in valid_types:
                msg += "{0} ".format(tmp_type[0])
            msg += "]"
            raise ValueError(msg)


    def check_ant_mode(self, ant_mode):
        """Check the antenna mode to see if it is valid."""
        try:
            return ant_mode['index']
        except KeyError:
            msg  = "The antenna mode must be an entry from the wlan_tx_ant_mode list in wlan_exp.util\n"
            raise ValueError(msg)


    def process_resp(self, resp):
        import wlan_exp.util as util

        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not get / set transmit antenna mode of the node"
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=2, status_errors=status_errors, name='from Tx antenna mode command'):
            args = resp.get_args()
            return util.find_tx_ant_mode_by_index(args[1])
        else:
            return CMD_PARAM_ERROR

# End Class


class NodeProcRxAntMode(message.Cmd):
    """Command to get / set the receive antenna mode of the node.

    Attributes:
        cmd       -- Sub-command to send over the command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
        ant_mode  -- Receive antenna mode for the node.  Checking is
                     done both in the command and on the node.  The current
                     antenna mode will be returned by the node.
    """
    def __init__(self, cmd, ant_mode=None):
        super(NodeProcRxAntMode, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_RX_ANT_MODE

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

        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could get / set receive antenna mode of the node"
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=2, status_errors=status_errors, name='from Rx antenna mode command'):
            args = resp.get_args()
            return util.find_rx_ant_mode_by_index(args[1])
        else:
            return CMD_PARAM_ERROR

# End Class



#--------------------------------------------
# Scan Commands
#--------------------------------------------
class NodeProcScanParam(message.Cmd):
    """Command to configure the scan parameters

    Attributes:
        cmd                -- Command for Process Scan Param:
                                CMD_PARAM_WRITE
        time_per_channel   -- Time (in float sec) to spend on each channel (optional)
        probe_tx_interval  -- Time (in float sec) to wait between each probe transmission (optional)
        channel_list       -- Channels to scan (optional)
                                Defaults to all channels in util.py wlan_channels array
                                Can provide either a channel number or list of
                                  channel numbers
        ssid               -- SSID (optional)
    """
    time_factor = 6
    time_type   = None

    def __init__(self, cmd, time_per_channel=None, probe_tx_interval=None, channel_list=None, ssid=None):
        super(NodeProcScanParam, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_SCAN_PARAM

        if (cmd == CMD_PARAM_WRITE):
            self.add_args(cmd)

            # Add the time_per_channel to the command
            _add_time_to_cmd32(self, time_per_channel, self.time_factor)

            # Add the probe_tx_interval to the command
            _add_time_to_cmd32(self, probe_tx_interval, self.time_factor)

            # Format the channel list
            if channel_list is not None:

                if type(channel_list) is list:
                    self.add_args(len(channel_list))

                    for channel in channel_list:
                        self.add_channel(channel)
                else:
                    self.add_args(1)
                    self.add_channel(channel_list)
            else:
                self.add_args(CMD_PARAM_RSVD)

            # Add SSID
            #     - SSID should be added last to the command so that the corresponding
            #       C code does not have to compute the index of the next argument 
            #       after the SSID
            if ssid is not None:
                _add_ssid_to_cmd(self, ssid)
            else:
                self.add_args(CMD_PARAM_RSVD)                
                
        else:
            msg = "Unsupported command: {0}".format(cmd)
            raise ValueError(msg)


    def add_channel(self, channel):
        """Internal method to add a channel"""
        import wlan_exp.util as util
        
        if channel not in util.wlan_channels:
            msg  = "Unknown channel:  {0}".format(channel)
            raise ValueError(msg)

        self.add_args(channel)
        

    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not set scan parameters."
        status_errors = { error_code : error_msg }

        if (resp.resp_is_valid(num_args=1, status_errors=status_errors, name='from Scan parameter command')):
            return True
        else:
            return False

# End Class


class NodeProcScan(message.Cmd):
    """Command to enable / disable active scan

    Attributes:
        enable -- Whether we are enabling (True) or disabling (False) active scan
    """
    def __init__(self, enable=None):
        super(NodeProcScan, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_SCAN

        if enable is None:        
            self.add_args(CMD_PARAM_RSVD)
        else:
            if enable:
                self.add_args(CMD_PARAM_NODE_SCAN_ENABLE)
            else:
                self.add_args(CMD_PARAM_NODE_SCAN_DISABLE)

    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Scan command error."
        status_errors = { error_code : error_msg }

        if (resp.resp_is_valid(num_args=2, status_errors=status_errors, name='from Scan command')):
            args = resp.get_args()
            if (args[1] == 1):
                return True
            else:
                return False
        else:
            return False

# End Class



#--------------------------------------------
# Association Commands
#--------------------------------------------
class NodeConfigBSS(message.Cmd):
    """Command to configure the BSS on the node.

    Command assumes all parameters are appropriately validated and processed.

    Attributes:
        bssid (int):  48-bit ID of the BSS either as a integer; A value of 
            None will remove current BSS on the node (similar to node.reset(bss=True));
            A value of False will not update the current bssid.
        ssid (str):  SSID string (Must be 32 characters or less); A value of 
            None will not update the current SSID.
        channel (int): Channel on which the BSS operates; A value of None will
            not update the current channel.
        beacon_interval (int): Integer number of beacon Time Units in [1, 65535]
            (http://en.wikipedia.org/wiki/TU_(Time_Unit); a TU is 1024 microseconds);
            A value of None will disable beacons;  A value of False will not 
            update the current beacon interval.
        ht_capable (bool):  TBD.  A value of None will not update the current
            value of HT capable.

    ..note::  This uses the BSSConfig() class defined in info.py to transfer 
        the parameters to the node.
    """
    bssid           = None
    ssid            = None
    channel         = None
    beacon_interval = None
    ht_capable      = None
    
    
    def __init__(self,  bssid=False, ssid=None, channel=None, beacon_interval=False, ht_capable=None):
        super(NodeConfigBSS, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_CONFIG_BSS
        
        import struct
        import wlan_exp.info as info

        # Create BSS Config         
        bss_config = info.BSSConfig(bssid=bssid, ssid=ssid, channel=channel, 
                                    beacon_interval=beacon_interval, 
                                    ht_capable=ht_capable)

        # Set local variables for error messages
        self.bssid           = bssid
        self.ssid            = ssid
        self.channel         = channel
        self.beacon_interval = beacon_interval
        self.ht_capable      = ht_capable

        # Convert BSSConfig() to bytes for transfer
        data_to_send = bss_config.serialize()
        data_len     = len(data_to_send)

        self.add_args(data_len)

        data_buf = bytearray(data_to_send)

        # Zero pad so that the data buffer is 32-bit aligned
        if ((len(data_buf) % 4) != 0):
            data_buf += bytearray(4 - (len(data_buf) % 4))

        idx = 0
        while (idx < len(data_buf)):
            arg = struct.unpack_from('!I', data_buf[idx:idx+4])
            self.add_args(arg[0])
            idx += 4
            

    def process_resp(self, resp):
        # Check number of arguments
        if (resp.resp_is_valid(num_args=1, name='from config BSS command')):
            args    = resp.get_args()
            status  = args[0]
            msg     = "ERROR:  Invalid response from node:\n"
            ret_val = True
            
            # Check status
            if (status & ERROR_CONFIG_BSS_BSSID_INVALID):
                msg    += "    BSSID {0} was invalid.\n".format(self.bssid)
                ret_val = False
            
            if (status & ERROR_CONFIG_BSS_BSSID_INSUFFICIENT_ARGUMENTS):
                msg    += "    Insufficient arguments to create BSS.\n"
                msg    += "        BSSID           = {0}\n".format(self.bssid)
                msg    += "        SSID            = {0}\n".format(self.ssid)
                msg    += "        CHANNEL         = {0}\n".format(self.channel)
                msg    += "        BEACON_INTERVAL = {0}\n".format(self.beacon_interval)
                msg    += "        HT_CAPABLE      = {0}\n".format(self.ht_capable)
                ret_val = False
            
            if (status & ERROR_CONFIG_BSS_CHANNEL_INVALID):
                msg    += "    Channel {0} was invalid.\n".format(self.channel)
                ret_val = False
            
            if (status & ERROR_CONFIG_BSS_BEACON_INTERVAL_INVALID):
                msg    += "    Beacon interval {0} was invalid.\n".format(self.beacon_interval)
                ret_val = False
            
            if (status & ERROR_CONFIG_BSS_HT_CAPABLE_INVALID):
                msg    += "    HT capable {0} was invalid.\n".format(self.ht_capable)
                ret_val = False
            
            if not ret_val:
                print(msg)
            
            return ret_val
        else:
            return False

# End Class


class NodeDisassociate(message.Cmd):
    """Command to remove associations from the association table."""
    description = None

    def __init__(self, device=None):
        super(NodeDisassociate, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_DISASSOCIATE

        if device is not None:
            self.description = device.description
            mac_address      = device.wlan_mac_address
        else:
            self.description = "All nodes"
            mac_address      = CMD_PARAM_GET_ALL_ASSOCIATED

        _add_mac_address_to_cmd(self, mac_address)


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not disassociate from {0}.".format(self.description)
        status_errors = { error_code : error_msg }

        resp.resp_is_valid(num_args=1, status_errors=status_errors,
                           name='from Disassociate command')

# End Class


class NodeGetStationInfo(message.BufferCmd):
    """Command to get the station info for a given node."""
    def __init__(self, node=None):
        super(NodeGetStationInfo, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_GET_STATION_INFO

        if node is not None:
            mac_address = node.wlan_mac_address
        else:
            mac_address = CMD_PARAM_GET_ALL_ASSOCIATED

        _add_mac_address_to_cmd(self, mac_address)


    def process_resp(self, resp):
        # Contains a Buffer of all station infos.  Need to convert to a list
        #   of StationInfo()
        import wlan_exp.info as info

        index   = 0
        data    = resp.get_bytes()
        ret_val = info.deserialize_info_buffer(data[index:], "StationInfo()")

        return ret_val

# End Class


class NodeGetBSSInfo(message.BufferCmd):
    """Command to get the BSS info for a given BSS ID."""
    def __init__(self, bssid=None):
        super(NodeGetBSSInfo, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_GET_BSS_INFO

        if bssid is None:
            self.add_args(CMD_PARAM_RSVD)
            self.add_args(CMD_PARAM_RSVD)
        else:
            if type(bssid) is str:
                bssid = CMD_PARAM_GET_ALL_ASSOCIATED      # If BSSID is a string, then get all bss info
    
            _add_mac_address_to_cmd(self, bssid)


    def process_resp(self, resp):
        # Contains a Buffer of all BSS infos.  Need to convert to a list of BSSInfo()
        import wlan_exp.info as info

        index   = 0
        data    = resp.get_bytes()
        ret_val = info.deserialize_info_buffer(data[index:], "BSSInfo()")

        return ret_val

# End Class



#--------------------------------------------
# Queue Commands
#--------------------------------------------
class QueueTxDataPurgeAll(message.Cmd):
    """Command to purge all data transmit queues on the node."""
    def __init__(self):
        super(QueueTxDataPurgeAll, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_QUEUE_TX_DATA_PURGE_ALL

    def process_resp(self, resp):
        pass

# End Class



#--------------------------------------------
# AP Specific Commands
#--------------------------------------------
class NodeAPConfigure(message.Cmd):
    """Command to configure the AP.

    Attributes (default state on the node is in CAPS):
        dtim_multicast_buffering   -- Enable DTIM multicast buffering (TRUE/False)
    """
    def __init__(self, dtim_multicast_buffering=None):
        super(NodeAPConfigure, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_AP_CONFIG

        flags = 0
        mask  = 0

        if dtim_multicast_buffering is not None:
            mask += CMD_PARAM_AP_CONFIG_FLAG_DTIM_MULTICAST_BUFFER
            if dtim_multicast_buffering:
                flags += CMD_PARAM_AP_CONFIG_FLAG_DTIM_MULTICAST_BUFFER

        self.add_args(flags)
        self.add_args(mask)

    def process_resp(self, resp):
        pass

# End Class


class NodeAPAddAssociation(message.Cmd):
    """Command to add the association to the association table on the AP.

    Attributes:
        device        -- Device to add to the association table
        allow_timeout -- Allow the association to timeout if inactive

    NOTE:  This adds an association with the default tx/rx params.  If
        allow_timeout is not specified, the default on the node is to
        not allow timeout of the association.
    """
    description = None

    def __init__(self, device, allow_timeout=None):
        super(NodeAPAddAssociation, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_ADD_ASSOCIATION

        flags = 0
        mask  = 0

        if allow_timeout is not None:
            mask += CMD_PARAM_ADD_ASSOCIATION_ALLOW_TIMEOUT
            if allow_timeout:
                flags += CMD_PARAM_ADD_ASSOCIATION_ALLOW_TIMEOUT

        self.add_args(flags)
        self.add_args(mask)

        self.description = device.description
        mac_address      = device.wlan_mac_address

        _add_mac_address_to_cmd(self, mac_address)


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not add AP association to {0}.".format(self.description)
        status_errors = { error_code : error_msg }

        if (resp.resp_is_valid(num_args=2, status_errors=status_errors,
                               name='from AP associate command')):
            args = resp.get_args()
            return args[1]
        else:
            return CMD_PARAM_ERROR

# End Class


class NodeAPSetAuthAddrFilter(message.Cmd):
    """Command to set the authentication address filter on the node.

    Attributes:
        allow  -- List of (address, mask) tuples that will be used to filter addresses
                  on the node.

    NOTE:  For the mask, bits that are 0 are treated as "any" and bits that are 1 are
    treated as "must equal".  For the address, locations of one bits in the mask
    must match the incoming addresses to pass the filter.
    """
    def __init__(self, allow):
        super(NodeAPSetAuthAddrFilter, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_AP_SET_AUTHENTICATION_ADDR_FILTER

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
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not set authentication address filter on the node."
        status_errors = { error_code : error_msg }

        resp.resp_is_valid(num_args=1, status_errors=status_errors,
                           name='from AP set association address filter command')

# End Class



#--------------------------------------------
# STA Specific Commands
#--------------------------------------------
class NodeSTASetAID(message.Cmd):
    """Command to get the AID of the node"""
    def __init__(self, aid):
        super(NodeSTASetAID, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_STA_SET_AID
        
        if ((aid < 1) or (aid > 255)):
            raise AttributeError("AID value must be in [1, 255].")
        
        self.add_args(aid)


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not set the AID of node."
        status_errors = { error_code : error_msg }

        if (resp.resp_is_valid(num_args=1, status_errors=status_errors, name='from set AID command')):
            return True
        else:
            return False

# End Class


class NodeSTAJoin(message.Cmd):
    """Command to join a given BSS

    Attributes:
        ssid     -- SSID of BSS to join.  If value is None, then this will 
                    stop the join process
        bssid    -- (optional) BSSID of the BSS to join
        channel  -- (optional) Channel of BSS to join
        
    ..note::  If neither bssid or channel are provided node will start the 
        scanning state machine until it finds a BSS matching the ssid.  If 
        only one of bssid or channel is provided, raise error.
    """
    def __init__(self, ssid, bssid=None, channel=None):
        super(NodeSTAJoin, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_STA_JOIN

        # Add BSSID
        #     - Command assumes that this is a valid bssid
        if bssid is None:
            bssid = None
        else:
            # Check that a channel is specified
            if channel is None:
                raise AttributeError("Join must specify both BSSID and channel if either BSSID or channel is specified.")
            
        _add_mac_address_to_cmd(self, bssid)

        # Add Channel
        #     - Command assumes that this is a valid channel
        if channel is None:
            channel = 0
        else:
            # Check that a bssid is specified
            if bssid is None:
                raise AttributeError("Join must specify both BSSID and channel if either BSSID or channel is specified.")

        self.add_args(channel)

        # Add SSID
        #     - SSID should be added last to the command so that the corresponding
        #       C code does not have to compute the index of the next argument 
        #       after the SSID
        if ssid is None:
            self.add_args(CMD_PARAM_RSVD)
        else:
            _add_ssid_to_cmd(self, ssid)


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not join the network."
        status_errors = { error_code : error_msg }

        if (resp.resp_is_valid(num_args=1, status_errors=status_errors, name='from Join command')):
            return True
        else:
            return False

# End Class


class NodeSTAJoinStatus(message.Cmd):
    """Command to get the join status of the node"""
    def __init__(self):
        super(NodeSTAJoinStatus, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_STA_JOIN_STATUS


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not get join status of node."
        status_errors = { error_code : error_msg }

        if (resp.resp_is_valid(num_args=2, status_errors=status_errors, name='from Join Status command')):
            args = resp.get_args()
            if (args[1] == 1):
                return True
            else:
                return False
        else:
            return False

# End Class



#--------------------------------------------
# IBSS Specific Commands
#--------------------------------------------



#--------------------------------------------
# Memory Access Commands - For developer use only
#--------------------------------------------
class NodeMemAccess(message.Cmd):
    """Command to read/write memory in CPU High / CPU Low

    Attributes:
        cmd       -- Sub-command to send over the command.  Valid values are:
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
        if (high):
            self.command = _CMD_GROUP_NODE + CMDID_DEV_MEM_HIGH
        else:
            self.command = _CMD_GROUP_NODE + CMDID_DEV_MEM_LOW

        self.add_args(cmd)
        self.add_args(address)
        self.add_args(length)

        if (cmd == CMD_PARAM_READ):
            self._read_len = length

        elif (cmd == CMD_PARAM_WRITE):
            try:
                for v in values:
                    self.add_args(v)
            except TypeError:
                self.add_args(values)

        else:
            raise Exception('ERROR: NodeMemAccess constructor arguments invalid');


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Memory access failed."
        status_errors = { error_code : error_msg }
        
        if (self._read_len is not None): 
            # Read command
            if (self.command == _CMD_GROUP_NODE + CMDID_DEV_MEM_HIGH):
                msg = "from CPU High Mem read command"
            else:
                msg = "from CPU Low Mem read command"
            
            if resp.resp_is_valid(num_args=(2 + self._read_len), status_errors=status_errors, name=msg):
                args = resp.get_args()

                if (len(args) == 3):
                    return args[2]
                elif (len(args) > 3):
                    return args[2:]
                else:
                    raise Exception('ERROR: Invalid memory read response.  Num Args = {0}'.format(len(args)))
            else:
                return CMD_PARAM_ERROR
        else: 
            # Write command
            if (self.command == _CMD_GROUP_NODE + CMDID_DEV_MEM_HIGH):
                msg = "from CPU High Mem write command"
            else:
                msg = "from CPU Low Mem write command"
            
            if resp.resp_is_valid(num_args=1, status_errors=status_errors, name=msg):
                pass
            else:
                return CMD_PARAM_ERROR

# End ClassNodeAPProcBeaconInterval



#--------------------------------------------
# EEPROM Access Commands - For developer use only
#--------------------------------------------
class NodeEEPROMAccess(message.Cmd):
    """Command to read/write EEPROM

    Attributes:
        cmd       -- Sub-command to send over the command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
        address   -- u16 memory address to read/write

        values    -- When cmd==CMD_PARAM_WRITE, scalar or list of u8 values to write
                     When cmd==CMD_PARAM_READ, None

        length    -- When cmd==CMD_PARAM_WRITE, number of u8 values to write
                     When cmd==CMD_PARAM_READ, number of u8 values to read
                     
        on_board  -- True for "On Board" EEPROM access, False for "FMC" EEPROM access        
    """
    _read_len = None

    def __init__(self, cmd, address, values=None, length=None, on_board=True):
        super(NodeEEPROMAccess, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_DEV_EEPROM
        
        self.add_args(cmd)
        self.add_args(on_board)
        self.add_args(address)
        self.add_args(length)

        if (cmd == CMD_PARAM_READ):
            self._read_len = length

        elif (cmd == CMD_PARAM_WRITE):
            try:
                for v in values:
                    self.add_args(v)
            except TypeError:
                self.add_args(values)

        else:
            raise Exception('ERROR: NodeEEPROMAccess constructor arguments invalid');


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Memory access failed."
        status_errors = { error_code : error_msg }
            
        if (self._read_len is not None):
            # Read command
            msg = "from EEPROM write command"
        
            if resp.resp_is_valid(num_args=(2 + self._read_len), status_errors=status_errors, name=msg):
                args = resp.get_args()

                if (len(args) == 3):
                    return args[2]
                elif (len(args) > 3):
                    return args[2:]
                else:
                    raise Exception('ERROR: Invalid EEPROM read response:  Num Args = {0}'.format(len(args)))
            else:
                return CMD_PARAM_ERROR
        else: 
            # Write command
            msg = "from EEPROM write command"
        
            if resp.resp_is_valid(num_args=1, status_errors=status_errors, name=msg):
                pass
            else:
                return CMD_PARAM_ERROR

# End Class



#--------------------------------------------
# User Commands
#--------------------------------------------
class UserSendCmd(message.Cmd):
    """Command to send User Command to the node

    Attributes:
        cmdid     -- User-defined Command ID
        args      -- Scalar or list of u32 arguments to write
    """
    def __init__(self, cmd_id, args=None):
        super(UserSendCmd, self).__init__()

        self.command    = _CMD_GROUP_USER + cmd_id

        if args is not None:
            try:
                for a in args:
                    self.add_args(a)
            except TypeError:
                self.add_args(args)


    def process_resp(self, resp):
        """ Message format:
                respArgs32[0]   Status
        """
        args = resp.get_args()

        try:
            return args[0:]
        except:
            return None

# End Class



#--------------------------------------------
# Misc Helper methods
#--------------------------------------------
def _add_mac_address_to_cmd(cmd, mac_address):
    if mac_address is not None:
        if type(mac_address) is str:
            import wlan_exp.util as util
            mac_address = util.str_to_mac_addr(mac_address)
            
        cmd.add_args(((mac_address >> 32) & 0xFFFF))
        cmd.add_args((mac_address & 0xFFFFFFFF))
    else:
        cmd.add_args(CMD_PARAM_RSVD_MAC_ADDR)
        cmd.add_args(CMD_PARAM_RSVD_MAC_ADDR)

# End def


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


def _add_time_to_cmd64(cmd, time, time_factor):
    """Internal method to add a 64-bit time value to the given command.

    Returns:
        Type of the time argument
    """
    ret_val = TIME_TYPE_FLOAT

    if time is not None:
        # Format the time appropriately
        if   (type(time) is float):
            time_to_send   = int(round(time, time_factor) * (10**time_factor))
            ret_val        = TIME_TYPE_FLOAT
        elif (type(time) is int):
            time_to_send   = time
            ret_val        = TIME_TYPE_INT
        else:
            raise TypeError("Time must be either a float or int")

        cmd.add_args((time_to_send & 0xFFFFFFFF))
        cmd.add_args(((time_to_send >> 32) & 0xFFFFFFFF))
    else:
        cmd.add_args(CMD_PARAM_RSVD_TIME)
        cmd.add_args(CMD_PARAM_RSVD_TIME)

    return ret_val

# End def


def _add_time_to_cmd32(cmd, time, time_factor):
    """Internal method to add a 32-bit time value to the given command.

    Returns:
        Type of the time argument
    """
    ret_val = TIME_TYPE_FLOAT

    if time is not None:
        # Format the time appropriately
        if   (type(time) is float):
            time_to_send   = int(round(time, time_factor) * (10**time_factor))
            ret_val        = TIME_TYPE_FLOAT
        elif (type(time) is int):
            time_to_send   = time
            ret_val        = TIME_TYPE_INT
        else:
            raise TypeError("Time must be either a float or int")

        if (time_to_send > 0xFFFFFFFF):
            time_to_send = 0xFFFFFFFF
            print("WARNING:  Time value (in microseconds) exceeds 32-bits.  Truncating.\n")

        cmd.add_args((time_to_send & 0xFFFFFFFF))
    else:
        cmd.add_args(CMD_PARAM_RSVD_TIME)

    return ret_val

# End def


def _get_ssid_from_resp(resp, status_errors):
    """Internal method to process an SSID from a response."""
    args       = resp.get_args()
    arg_length = len(args)

    resp.resp_is_valid(num_args=arg_length, status_errors=status_errors,
                              name='while processing SSID')

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
        except TypeError:
            ssid    = str(ssid_tuple[0])
    else:
        ssid        = ""

    return ssid

# End def

