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
    TransportEthUdpPyBcast() -- Broadcast Ethernet UDP transport based on 
        python sockets

Integer constants:
    REQUESTED_BUF_SIZE -- Size of TX/RX buffer that will requested from the 
        operating system.

"""

import re

from . import wn_config
from . import wn_transport_eth_udp as tp


__all__ = ['TransportEthUdpPyBcast']


class TransportEthUdpPyBcast(tp.TransportEthUdp):
    """Class for WARPNet Ethernet UDP Broadcast Transport class using Python libraries.
       
    Attributes:
        See TransportEthUdp for attributes
        host_config -- A HostConfiguration that describes the default 
           transport configuration.
        host_ip -- IP address of the host interface
        
    If host_ip is not specified, then the transport will chose the IP address
    of the first host interface, ie host interface zero.
    """
    host_config = None
    host_ip     = None
    
    def __init__(self, host_config=None, host_ip=None):
        super(TransportEthUdpPyBcast, self).__init__()

        if not host_config is None:
            self.host_config = host_config
        else:
            self.host_config = wn_config.HostConfiguration()
        
        if host_ip is None:
            host_infs = self.host_config.get_param('network', 'host_interfaces')
            self.host_ip = host_infs[0]
        
        self.set_default_config()

        
    def set_default_config(self):
        """Set the default configuration of a Broadcast transport."""
        unicast_port = self.host_config.get_param('network', 'unicast_port')
        bcast_port   = self.host_config.get_param('network', 'bcast_port')
        host_id      = self.host_config.get_param('network', 'host_id')
        
        # Set default values of the Transport
        self.set_ip_address(self.host_ip)
        self.set_unicast_port(unicast_port)
        self.set_bcast_port(bcast_port)
        self.set_src_id(host_id)
        self.set_dest_id(0xFFFF)
        self.timeout = 1


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


# End Class
































