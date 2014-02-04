# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Commands
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

This module provides class definitions for all WARPNet commands.  

Functions (see below for more information):
    WnCmdGetWarpNetNodeType()
    WnCmdIdentify()
    WnCmdGetHwInfo() 
    WnCmdNetworkSetup()
    WnCmdNetworkReset()
    WnCmdPing()    
    WnCmdTestPayloadSize()
    WnCmdAddNodeGrpId()
    WnCmdClearNodeGrpId()

Integer constants:
    GRPID_WARPNET, GRPID_NODE, GRPID_TRANS - WARPNet Command Groups
    GRP_WARPNET, GRP_NODE, GRP_TRANSPORT - WARPNet Command Group Names

Many other constants may be defined; please do not use these values when 
defining other sub-classes of WnCmd and WnBufferCmd.

"""


from . import wn_message


__all__ = ['WnCmdGetWarpNetNodeType', 'WnCmdIdentify', 'WnCmdGetHwInfo', 
           'WnCmdNetworkSetup', 'WnCmdNetworkReset', 'WnCmdPing', 
           'WnCmdTestPayloadSize', 'WnCmdAddNodeGrpId', 'WnCmdClearNodeGrpId']


# WARPNet Command Groups
GRPID_WARPNET           = 0xFF
GRPID_NODE              = 0x00
GRPID_TRANS             = 0x10


# WARPNet Command Group names
GRP_WARPNET             = 'warpnet'
GRP_NODE                = 'node'
GRP_TRANSPORT           = 'transport'


# WARPNet Command IDs
#   NOTE:  The C counterparts are found in *_node.h
CMD_WARPNET_TYPE        = 0xFFFFFF


# WARPNet Node Command IDs
#   NOTE:  The C counterparts are found in *_node.h
CMD_INFO                = 0x000001
CMD_IDENTIFY            = 0x000002
CMD_NODE_NETWORK_SETUP  = 0x000003
CMD_NODE_NETWORK_RESET  = 0x000004
CMD_NODE_TEMPERATURE    = 0x000005


# WARPNet Transport Command IDs
#   NOTE:  The C counterparts are found in *_transport.h
CMD_PING                = 0x000001
CMD_IP                  = 0x000002
CMD_PORT                = 0x000003
CMD_PAYLOAD_SIZE_TEST   = 0x000004
CMD_NODE_GRPID_ADD      = 0x000005
CMD_NODE_GRPID_CLEAR    = 0x000006


#-----------------------------------------------------------------------------
# Class Definitions for WARPNet Commands
#-----------------------------------------------------------------------------

class WnCmdGetWarpNetNodeType(wn_message.WnCmd):
    """Command to get the WARPNet Node Type of the node"""
    def __init__(self):
        super(WnCmdGetWarpNetNodeType, self).__init__()
        self.command = (GRPID_WARPNET << 24) | CMD_WARPNET_TYPE
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response:")
            print(resp)
        return args[0]

# End Class


class WnCmdIdentify(wn_message.WnCmd):
    """Command to blink the WARPNet Node LEDs."""
    def __init__(self, node_id):
        super(WnCmdIdentify, self).__init__()
        self.command = (GRPID_NODE << 24) | CMD_IDENTIFY
        self.node_id = node_id
    
    def process_resp(self, resp):
        print("Blinking LEDs of node: {0}".format(self.node_id))

# End Class


class WnCmdGetHwInfo(wn_message.WnCmd):
    """Command to get the hardware parameters from the node."""
    def __init__(self):
        super(WnCmdGetHwInfo, self).__init__()
        self.command = (GRPID_NODE << 24) | CMD_INFO
    
    def process_resp(self, resp):
        return resp.get_args()

# End Class


class WnCmdNetworkSetup(wn_message.WnCmd):
    """Command to perform initial network setup of a node."""
    def __init__(self, node):
        super(WnCmdNetworkSetup, self).__init__()
        self.command = (GRPID_NODE << 24) | CMD_NODE_NETWORK_SETUP
        self.add_args(node.serial_number)
        self.add_args(node.node_id)
        self.add_args(node.transport.ip2int(node.transport.ip_address))
        self.add_args(node.transport.unicast_port)
        self.add_args(node.transport.bcast_port)
    
    def process_resp(self, resp):
        pass

# End Class


class WnCmdNetworkReset(wn_message.WnCmd):
    """Command to reset the network configuration of a node."""
    def __init__(self, node):
        super(WnCmdNetworkReset, self).__init__()
        self.command = (GRPID_NODE << 24) | CMD_NODE_NETWORK_RESET
        self.add_args(node.serial_number)
    
    def process_resp(self, resp):
        pass

# End Class


class WnCmdGetTemperature(wn_message.WnCmd):
    """Command to get the temperature of a node."""
    def __init__(self, node):
        super(WnCmdNetworkReset, self).__init__()
        self.command = (GRPID_NODE << 24) | CMD_NODE_TEMPERATURE
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 3:
            print("Invalid response.")
            print(resp)
            return (0, 0, 0)
        else:
            return (args[0], args[1], args[2])

# End Class


class WnCmdPing(wn_message.WnCmd):
    """Command to ping the node."""
    def __init__(self):
        super(WnCmdPing, self).__init__()
        self.command = (GRPID_TRANS << 24) | CMD_PING
    
    def process_resp(self, resp):
        pass

# End Class


class WnCmdTestPayloadSize(wn_message.WnCmd):
    """Command to perform a payload size test on a node."""
    def __init__(self, size):
        super(WnCmdTestPayloadSize, self).__init__()
        self.command = (GRPID_TRANS << 24) | CMD_PAYLOAD_SIZE_TEST
        
        for i in range(size):
            self.add_args(i)
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]
        
# End Class


class WnCmdAddNodeGrpId(wn_message.WnCmd):
    """Command to add a Node Group ID to the node so that it can process
    broadcast commands that are received from that node group."""
    def __init__(self, group):
        super(WnCmdAddNodeGrpId, self).__init__()
        self.command = (GRPID_NODE << 24) | CMD_NODE_GRPID_ADD
        self.add_args(group)
    
    def process_resp(self, resp):
        pass

# End Class


class WnCmdClearNodeGrpId(wn_message.WnCmd):
    """Command to clear a Node Group ID to the node so that it can ignore
    broadcast commands that are received from that node group."""
    def __init__(self, group):
        super(WnCmdClearNodeGrpId, self).__init__()
        self.command = (GRPID_NODE << 24) | CMD_NODE_GRPID_CLEAR
        self.add_args(group)
    
    def process_resp(self, resp):
        pass

# End Class


