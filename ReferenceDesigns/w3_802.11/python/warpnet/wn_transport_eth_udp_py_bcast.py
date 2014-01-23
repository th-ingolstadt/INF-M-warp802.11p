# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Transport - Broadcast Ethernet UDP Python Socket Implementation
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

This module provides the WARPNet broadcast Ethernet UDP transport based on 
the python socket class.

Functions:
    WnTransportEthUdpPyBcast() -- Broadcast Ethernet UDP transport based on 
        python sockets

Integer constants:
    REQUESTED_BUF_SIZE -- Size of TX/RX buffer that will requested from the 
        operating system.

"""

import re
import errno
import socket
from socket import error as socket_error

from . import wn_config
from . import wn_transport_eth_udp as tp


__all__ = ['WnTransportEthUdpPyBcast']


REQUESTED_BUF_SIZE = 2**22


class WnTransportEthUdpPyBcast(tp.WnTransportEthUdp):
    """Class for WARPNet Ethernet UDP Broadcast Transport class using Python libraries.
       
    Attributes:
        See WnTransportEthUdp for all attributes
    """
    def __init__(self):
        super(WnTransportEthUdpPyBcast, self).__init__()        
        self.set_default_config()

        
    def set_default_config(self):
        """Set the default configuration of a Broadcast transport."""
        config = wn_config.WnConfiguration()
        
        # Set default values of the Transport
        self.set_ip_address(config.get_param('network', 'host_address'))
        self.unicast_port = int(config.get_param('network', 'unicast_port'))
        self.bcast_port = int(config.get_param('network', 'bcast_port'))
        self.hdr.set_src_id(int(config.get_param('network', 'host_id')))
        self.hdr.set_dest_id(0xFFFF)
        self.timeout = 2000


    def set_ip_address(self, ip_addr):
        """Sets the IP address of the Broadcast transport.
        
        This method will take an IP address of the for W.X.Y.Z and set the
        transport IP address to W.X.Y.255 so that it is a proper broadcast
        address.
        """
        expr = re.compile('\.')
        tmp_data = []
        for data in expr.split(ip_addr):
            tmp_data.append(int(data))
        
        self.ip_address = str("{0:d}.{1:d}.{2:d}.{3:d}".format(tmp_data[0],
                                                               tmp_data[1],
                                                               tmp_data[2],
                                                               0xFF))

    
    def wn_open(self, ip_addr=None, bcast_port=None):
        """Opens an Ethernet UDP Broadcast socket.""" 
        if ip_addr:
            if isinstance(ip_addr, str):
                self.set_ip_address(ip_addr)
            else:
                self.set_ip_address(self.int2ip(ip_addr))
        
        if bcast_port:
            self.bcast_port = bcast_port
        
        self.sock = socket.socket(socket.AF_INET,       # Internet
                                  socket.SOCK_DGRAM);   # UDP
        
        self.sock.settimeout(self.timeout)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

        try:
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, REQUESTED_BUF_SIZE)
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, REQUESTED_BUF_SIZE)
        except socket_error as serr:
            # On some HW we cannot set the buffer size
            if serr.errno != errno.ENOBUFS:
                raise serr

        self.tx_buffer_size = self.sock.getsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF)
        self.rx_buffer_size = self.sock.getsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF)
        self.status = 1
                                  

    def wn_close(self):
        """Closes an Ethernet UDP socket."""
        if self.sock:
            try:
                self.sock.close()
            except socket.error as err:
                print("Error closing socket:  {0}".format(err))

        self.status = 0


    def send(self, payload, pkt_type="message"):
        """Send a broadcast message over the transport.
        
        Attributes:
            data -- Data to be sent over the socket
        """
        self.hdr.set_type(pkt_type)
        self.hdr.response_not_required()
        self.hdr.set_length(len(payload))
        self.hdr.increment()

        data = bytes(b'\x00\x00' + self.hdr.serialize() + payload)
        
        size = self.sock.sendto(data, (self.ip_address, self.bcast_port))
        
        if size != len(data):
            print("Only {} of {} bytes of data sent".format(size, len(data)))


    def receive(self):
        """Not used on a broadcast transport"""
        raise NotImplementedError


# End Class WnTransportEthUdpPyBcast
































