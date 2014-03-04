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
    GetWarpNetNodeType()
    Identify()
    GetHwInfo() 
    SetupNetwork()
    ResetNetwork()
    Ping()    
    TestPayloadSize()
    AddNodeGrpId()
    ClearNodeGrpId()

Integer constants:
    GRPID_WARPNET, GRPID_NODE, GRPID_TRANS - WARPNet Command Groups
    GRP_WARPNET, GRP_NODE, GRP_TRANSPORT - WARPNet Command Group Names

Many other constants may be defined; please do not use these values when 
defining other sub-classes of WARPNet Cmd and BufferCmd.

"""


from . import wn_message


__all__ = ['NodeGetWarpNetType', 'NodeIdentify', 'NodeGetHwInfo', 
           'NodeSetupNetwork', 'NodeResetNetwork', 'NodeGetTemperature', 
           'Ping', 'TestPayloadSize', 'AddNodeGrpId', 'ClearNodeGrpId']


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

IDENTIFY_ALL_NODES      = 0xFFFF
IDENTIFY_WAIT_TIME      = 10

CMD_NODE_NETWORK_SETUP  = 0x000003
CMD_NODE_NETWORK_RESET  = 0x000004

NETWORK_RESET_ALL_NODES = 0xFFFF

CMD_NODE_TEMPERATURE    = 0x000005


# WARPNet Transport Command IDs
#   NOTE:  The C counterparts are found in *_transport.h
CMD_PING                = 0x000001
CMD_IP                  = 0x000002
CMD_PORT                = 0x000003
CMD_PAYLOAD_SIZE_TEST   = 0x000004
CMD_NODE_GRPID_ADD      = 0x000005
CMD_NODE_GRPID_CLEAR    = 0x000006


# Local Constants
_CMD_GRPID_WARPNET      = (GRPID_WARPNET << 24)
_CMD_GRPID_NODE         = (GRPID_NODE << 24)
_CMD_GRPID_TRANS        = (GRPID_TRANS << 24)


#-----------------------------------------------------------------------------
# Class Definitions for WARPNet Commands
#-----------------------------------------------------------------------------

class NodeGetWarpNetType(wn_message.Cmd):
    """Command to get the WARPNet Node Type of the node"""
    def __init__(self):
        super(NodeGetWarpNetType, self).__init__()
        self.command = _CMD_GRPID_WARPNET + CMD_WARPNET_TYPE
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response:")
            print(resp)
        return args[0]

# End Class


#-----------------------------------------------------------------------------
# Node Commands
#-----------------------------------------------------------------------------

class NodeIdentify(wn_message.Cmd):
    """Command to blink the WARPNet Node LEDs."""
    def __init__(self, serial_number):
        super(NodeIdentify, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_IDENTIFY
        
        if type(serial_number) is int:
            if (serial_number == IDENTIFY_ALL_NODES):
                self.id = "All nodes"
            else:
                self.id = "W3-a-{0:05d}".format(serial_number)
            self.add_args(serial_number)
        elif type(serial_number) is str:
            import re
            expr = re.compile('W3-a-(?P<sn>\d+)')
            try:
                sn = int(expr.match(serial_number).group('sn'))
                self.id = serial_number
                self.add_args(sn)
            except AttributeError:
                msg  = "Incorrect serial number.  \n"
                msg += "    Should be of the form : W3-a-XXXXX\n"
                raise TypeError(msg)
        else:
            raise TypeError("Serial number should be an int or str.")
    
    def process_resp(self, resp):
        import time
        print("Blinking LEDs of node: {0}".format(self.id))
        time.sleep(IDENTIFY_WAIT_TIME)

# End Class


class NodeGetHwInfo(wn_message.Cmd):
    """Command to get the hardware parameters from the node."""
    def __init__(self):
        super(NodeGetHwInfo, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_INFO
    
    def process_resp(self, resp):
        return resp.get_args()

# End Class


class NodeSetupNetwork(wn_message.Cmd):
    """Command to perform initial network setup of a node."""
    def __init__(self, node):
        super(NodeSetupNetwork, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_NODE_NETWORK_SETUP
        self.add_args(node.serial_number)
        self.add_args(node.node_id)
        self.add_args(node.transport.ip2int(node.transport.ip_address))
        self.add_args(node.transport.unicast_port)
        self.add_args(node.transport.bcast_port)
    
    def process_resp(self, resp):
        pass

# End Class


class NodeResetNetwork(wn_message.Cmd):
    """Command to reset the network configuration of a node."""
    def __init__(self, serial_number, output=False):
        super(NodeResetNetwork, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_NODE_NETWORK_RESET
        self.output = output

        if type(serial_number) is int:
            if (serial_number == NETWORK_RESET_ALL_NODES):
                self.id = "All nodes"
            else:
                self.id = "W3-a-{0:05d}".format(serial_number)
            self.add_args(serial_number)
        elif type(serial_number) is str:
            import re
            expr = re.compile('W3-a-(?P<sn>\d+)')
            try:
                sn = int(expr.match(serial_number).group('sn'))
                self.id = serial_number
                self.add_args(sn)
            except AttributeError:
                msg  = "Incorrect serial number.  \n"
                msg += "    Should be of the form : W3-a-XXXXX\n"
                raise TypeError(msg)
        else:
            raise TypeError("Serial number should be an int or str.")
    
    def process_resp(self, resp):
        if (self.output):
            print("Reset network config of node: {0}".format(self.id))

# End Class


class NodeGetTemperature(wn_message.Cmd):
    """Command to get the temperature of a node.
    
    NOTE:  The response must be converted to Celsius with the given formula:
        ((double(temp)/65536.0)/0.00198421639) - 273.15
        - http://www.xilinx.com/support/documentation/user_guides/ug370.pdf
        - 16 bit value where 10 MSBs are an ADC value
    """
    def __init__(self):
        super(NodeGetTemperature, self).__init__()
        self.command = _CMD_GRPID_NODE + CMD_NODE_TEMPERATURE
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 3:
            print("Invalid response.")
            print(resp)
            return (0, 0, 0)
        else:
            curr_temp = ((float(args[0])/65536.0)/0.00198421639) - 273.15
            min_temp  = ((float(args[1])/65536.0)/0.00198421639) - 273.15
            max_temp  = ((float(args[2])/65536.0)/0.00198421639) - 273.15
            return (curr_temp, min_temp, max_temp)

# End Class


#-----------------------------------------------------------------------------
# Transport Commands
#-----------------------------------------------------------------------------
class Ping(wn_message.Cmd):
    """Command to ping the node."""
    def __init__(self):
        super(Ping, self).__init__()
        self.command = _CMD_GRPID_TRANS + CMD_PING
    
    def process_resp(self, resp):
        pass

# End Class


class TestPayloadSize(wn_message.Cmd):
    """Command to perform a payload size test on a node."""
    def __init__(self, size):
        super(TestPayloadSize, self).__init__()
        self.command = _CMD_GRPID_TRANS + CMD_PAYLOAD_SIZE_TEST
        
        for i in range(size):
            self.add_args(i)
    
    def process_resp(self, resp):
        args = resp.get_args()
        if len(args) != 1:
            print("Invalid response.")
            print(resp)
        return args[0]
        
# End Class


class AddNodeGrpId(wn_message.Cmd):
    """Command to add a Node Group ID to the node so that it can process
    broadcast commands that are received from that node group."""
    def __init__(self, group):
        super(AddNodeGrpId, self).__init__()
        self.command = _CMD_GRPID_TRANS + CMD_NODE_GRPID_ADD
        self.add_args(group)
    
    def process_resp(self, resp):
        pass

# End Class


class ClearNodeGrpId(wn_message.Cmd):
    """Command to clear a Node Group ID to the node so that it can ignore
    broadcast commands that are received from that node group."""
    def __init__(self, group):
        super(ClearNodeGrpId, self).__init__()
        self.command = _CMD_GRPID_TRANS + CMD_NODE_GRPID_CLEAR
        self.add_args(group)
    
    def process_resp(self, resp):
        pass

# End Class


