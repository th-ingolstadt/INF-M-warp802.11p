# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Transport - Ethernet UDP base class
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

This module provides the base class for WARPNet Ethernet UDP transports.

Functions:
    WnTransportEthUdp() -- Base class for Ethernet UDP transports
    int2ip() -- Convert 32 bit integer to 'w.x.y.z' IP address string
    ip2int() -- Convert 'w.x.y.z' IP address string to 32 bit integer
    mac2str() -- Convert 6 byte MAC address to 'uu:vv:ww:xx:yy:zz' string

"""

import re
import time
import errno
import socket
from socket import error as socket_error

from . import wn_cmds
from . import wn_message
from . import wn_exception as ex
from . import wn_transport as tp


__all__ = ['WnTransportEthUdp']


class WnTransportEthUdp(tp.WnTransport):
    """Base Class for WARPNet Ethernet UDP Transport class.
       
    Attributes:
        timeout-- Maximum time spent waiting before retransmission
        transport_type -- Unique type of the WARPNet transport
        sock -- UDP socket
        status -- Status of the UDP socket
        max_payload -- Maximum payload size (eg. MTU - ETH/IP/UDP headers)
        
        mac_address -- Node's MAC address
        ip_address -- IP address of destination
        unicast_port -- Unicast port of destination
        bcast_port -- Broadcast port of destination
        group_id -- Group ID of the node attached to the transport
        rx_buffer_size -- OS's receive buffer size (in bytes)        
        tx_buffer_size -- OS's transmit buffer size (in bytes)        
    """
    timeout         = None
    transport_type  = None
    sock            = None
    status          = None
    max_payload     = None
    mac_address     = None
    ip_address      = None
    unicast_port    = None
    bcast_port      = None
    group_id        = None
    rx_buffer_size  = None
    tx_buffer_size  = None
    
    def __init__(self):
        self.hdr = wn_message.WnTransportHeader()
        self.status = 0
        self.timeout = 1
        self.max_payload = 1000   # Sane default.  Overwritten by payload test.
        
        self.check_setup()

    def __del__(self):
        """Closes any open sockets"""
        self.wn_close()

    def __repr__(self):
        """Return transport IP address and Ethernet MAC address"""
        return str("Eth UDP Transport: " + self.ip_address + "(" + self.eth_mac_addr + ")")

    def check_setup(self):
        """Check the setup of the transport."""
        pass

    def set_max_payload(self, value):  self.max_payload = value
    def set_ip_address(self, value):   self.ip_address = value
    def set_unicast_port(self, value): self.unicast_port = value
    def set_bcast_port(self, value):   self.bcast_port = value
    def set_src_id(self, value):       self.hdr.set_src_id(value)
    def set_dest_id(self, value):      self.hdr.set_dest_id(value)

    def get_max_payload(self):         return self.max_payload
    def get_ip_address(self):          return self.ip_address
    def get_unicast_port(self):        return self.unicast_port
    def get_bcast_port(self):          return self.bcast_port
    def get_src_id(self):              return self.hdr.get_src_id()
    def get_dest_id(self):             return self.hdr.get_dest_id()


    def wn_open(self, tx_buf_size=None, rx_buf_size=None):
        """Opens an Ethernet UDP socket.""" 
        self.sock = socket.socket(socket.AF_INET,       # Internet
                                  socket.SOCK_DGRAM);   # UDP
        
        self.sock.settimeout(self.timeout)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        
        if not tx_buf_size is None:
            try:
                self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, tx_buf_size)
            except socket_error as serr:
                # On some HW we cannot set the buffer size
                if serr.errno != errno.ENOBUFS:
                    raise serr

        if not rx_buf_size is None:
            try:
                self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, rx_buf_size)
            except socket_error as serr:
                # On some HW we cannot set the buffer size
                if serr.errno != errno.ENOBUFS:
                    raise serr

        self.tx_buffer_size = self.sock.getsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF)
        self.rx_buffer_size = self.sock.getsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF)
        self.status = 1
        
        if (self.tx_buffer_size != tx_buf_size):
            print("OS reduced send buffer size from {0} to {1}".format(self.tx_buffer_size, tx_buf_size))
            
        if (self.rx_buffer_size != rx_buf_size):
            print("OS reduced receive buffer size from {0} to {1}".format(self.rx_buffer_size, rx_buf_size))


    def wn_close(self):
        """Closes an Ethernet UDP socket."""
        if self.sock:
            try:
                self.sock.close()
            except socket.error as err:
                print("Error closing socket:  {0}".format(err))

        self.status = 0


    #-------------------------------------------------------------------------
    # Commands that must be implemented by child classes
    #-------------------------------------------------------------------------
    def send(self,):
        """Send a message over the transport."""
        raise NotImplementedError

    def receive(self,):
        """Return a response from the transport."""
        raise NotImplementedError


    #-------------------------------------------------------------------------
    # WARPNet Commands for the Transport
    #-------------------------------------------------------------------------
    def ping(self, node, output=False):
        """Issues a ping command to the given node.
        
        Will optionally print the result of the ping.
        """
        start_time = time.clock()
        node.send_cmd(wn_cmds.WnCmdPing())
        end_time = time.clock()
        
        if output:
            ms_time = (end_time - start_time) * 1000
            print("Reply from {0}:  time = {1:.3f} ms".format(self.ip_address, 
                                                              ms_time))
    
    def test_payload_size(self, node, jumbo_frame_support=False):
        """Determines the object's max_payload parameter."""

        if (jumbo_frame_support == True):
            payload_test_sizes = [1000, 1470, 5000, 8966]
        else:
            payload_test_sizes = [1000, 1470]
        
        for size in payload_test_sizes:
            my_arg_size = (size - (self.hdr.sizeof() + wn_message.WnCmd().sizeof() + 4)) // 4
 
            try:
                resp = node.send_cmd(wn_cmds.WnCmdTestPayloadSize(my_arg_size))
                self.set_max_payload(resp)
                
                if (self.get_max_payload() < (4*my_arg_size)):
                    # If the node received a smaller payload, then break
                    break
            except:
                # If there was a transmission error, then break
                break
    
    def add_node_group_id(self, node, group):
        node.send_cmd(wn_cmds.WnCmdAddNodeGrpId(group))
    
    def clear_node_group_id(self, node, group):
        node.send_cmd(wn_cmds.WnCmdClearNodeGrpId(group))


    #-------------------------------------------------------------------------
    # WARPNet Parameter Framework
    #   Allows for processing of hardware parameters
    #-------------------------------------------------------------------------
    def process_parameter(self, identifier, length, values):
        """Extract values from the parameters"""
        if   (identifier == tp.TRANSPORT_TYPE):
            if (length == 1):
                self.transport_type = values[0]
            else:
                raise ex.WnParameterError("TRANSPORT_TYPE", "Incorrect length")
                
        elif (identifier == tp.TRANSPORT_HW_ADDR):
            if (length == 2):
                self.mac_address = ((2**32) * (values[0] & 0xFFFF) + values[1])
            else:
                raise ex.WnParameterError("TRANSPORT_HW_ADDR", "Incorrect length")
                
        elif (identifier == tp.TRANSPORT_IP_ADDR):
            if (length == 1):
                self.ip_address = self.int2ip(values[0])
            else:
                raise ex.WnParameterError("TRANSPORT_IP_ADDR", "Incorrect length")
                
        elif (identifier == tp.TRANSPORT_UNICAST_PORT):
            if (length == 1):
                self.unicast_port = values[0]
            else:
                raise ex.WnParameterError("TRANSPORT_UNICAST_PORT", "Incorrect length")
                
        elif (identifier == tp.TRANSPORT_BCAST_PORT):
            if (length == 1):
                self.bcast_port = values[0]
            else:
                raise ex.WnParameterError("TRANSPORT_BCAST_PORT", "Incorrect length")
                
        elif (identifier == tp.TRANSPORT_GRP_ID):
            if (length == 1):
                self.group_id = values[0]
            else:
                raise ex.WnParameterError("TRANSPORT_GRP_ID", "Incorrect length")
                
        else:
            raise ex.WnParameterError(identifier, "Unknown transport parameter")


    #-------------------------------------------------------------------------
    # Misc methods for the Transport
    #-------------------------------------------------------------------------
    def int2ip(self, ipaddr):  return int2ip(ipaddr)

    def ip2int(self, ipaddr):  return ip2int(ipaddr)

    def mac2str(self, mac_address):  return mac2str(mac_address)

    def __str__(self):
        """Pretty print the Transport parameters"""
        print("Transport {}:".format(self.transport_type))
        print("    IP address    :  {}".format(self.ip_address))
        print("    MAC address   :  {}".format(self.mac2str(self.mac_address)))
        print("    Max payload   :  {}".format(self.max_payload))
        print("    Unicast port  :  {}".format(self.unicast_port))
        print("    Broadcast port:  {}".format(self.bcast_port))
        print("    Timeout       :  {}".format(self.timeout))
        print("    Rx Buffer Size:  {}".format(self.rx_buffer_size))
        print("    Tx Buffer Size:  {}".format(self.tx_buffer_size))
        print("    Group ID      :  {}".format(self.group_id))
        
# End Class WnTransportEthUdp



#-------------------------------------------------------------------------
# Global methods for the Transport
#-------------------------------------------------------------------------

def int2ip(ipaddr):
    return str("{0:d}.{1:d}.{2:d}.{3:d}".format(((ipaddr >> 24) & 0xFF),
                                                ((ipaddr >> 16) & 0xFF),
                                                ((ipaddr >>  8) & 0xFF),
                                                (ipaddr & 0xFF)))


def ip2int(ipaddr):
    expr = re.compile('\.')
    dataTuple = []
    for data in expr.split(ipaddr):
        dataTuple.append(int(data))
    return (dataTuple[3]) + (dataTuple[2] * 2**8) + (dataTuple[1] * 2**16) + (dataTuple[0] * 2**24)


def mac2str(mac_address):
    return str("{0:02x}".format((mac_address >> 40) & 0xFF) + ":" +
               "{0:02x}".format((mac_address >> 32) & 0xFF) + ":" +
               "{0:02x}".format((mac_address >> 24) & 0xFF) + ":" +
               "{0:02x}".format((mac_address >> 16) & 0xFF) + ":" +
               "{0:02x}".format((mac_address >>  8) & 0xFF) + ":" +
               "{0:02x}".format(mac_address & 0xFF))









