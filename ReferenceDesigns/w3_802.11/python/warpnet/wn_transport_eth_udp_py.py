# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Transport - Unicast Ethernet UDP Python Socket Implementation
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

This module provides the WARPNet unicast Ethernet UDP transport based on 
the python socket class.

Functions:
    WnTransportEthUdpPy() -- Unicast Ethernet UDP transport based on python
        sockets

Integer constants:
    REQUESTED_BUF_SIZE -- Size of TX/RX buffer that will requested from the 
        operating system.

"""

import re
import time
import errno
import socket
from socket import error as socket_error

from . import wn_transport_eth_udp
from . import wn_exception as ex


__all__ = ['WnTransportEthUdpPy']


REQUESTED_BUF_SIZE = 2**22


class WnTransportEthUdpPy(wn_transport_eth_udp.WnTransportEthUdp):
    """Class for WARPNet Ethernet UDP Transport class using Python libraries.
       
    Attributes:
        See WnTransportEthUdp for all attributes
    """
    def wn_open(self, ip_addr=None, unicast_port=None):
        """Opens an Ethernet UDP socket.""" 
        if ip_addr:
            if isinstance(ip_addr, str):
                self.ip_address = ip_addr
            else:
                self.ip_address = self.int2ip(ip_addr)
        
        if unicast_port:
            self.unicast_port = unicast_port
        
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


    def send(self, payload, robust=True):
        """Send a message over the transport.
        
        Attributes:
            data -- Data to be sent over the socket
            robust -- Do we want a response to the sent data
            max_attempts -- Maximum attempts to transmit the data
        """
        
        if robust:
            self.hdr.response_required()
        else:
            self.hdr.response_not_required()
        
        self.hdr.set_length(len(payload))
        self.hdr.increment()

        # Pad the data with two extra bytes for 32 bit alignment after the 
        #   ethernet header
        data = bytes(b'\x00\x00' + self.hdr.serialize() + payload)        
 
        size = self.sock.sendto(data, (self.ip_address, self.unicast_port))
        
        if size != len(data):
            print("Only {} of {} bytes of data sent".format(size, len(data)))


    def receive(self):
        """Return a response from the transport.
        
        NOTE:  This function will block until a response is received or a
        timeout occurs.  If a timeout occurs, it will raise a WnTransportError
        exception.
        """
        reply = b''

        max_pkt_len = self.get_max_payload() + 100;
        received_resp = 0
        start_time = time.time()
        
        while received_resp == 0:
            recv_data = []
            
            try:
                (recv_data, recv_addr) = self.sock.recvfrom(max_pkt_len)
            except socket.error as err:
                expr = re.compile("timed out")
                if not expr.match(str(err)):
                    print("Failed to receive UDP packet.\nError message:\n{}".format(err))
            
            if len(recv_data) > 0:
                hdr_len = 2 + self.hdr.sizeof()
                # print("Received pkt from", recv_addr)
                if (self.hdr.is_reply(recv_data[2:hdr_len])):
                    reply = recv_data[hdr_len:]
                    received_resp = 1
            
            if ((time.time() - start_time) > self.timeout ) and (received_resp == 0):
                raise ex.WnTransportError(self, "Transport receive timed out.")

        return reply


    def receive_nb(self):
        """Return a response from the transport.
        
        NOTE:  This function will not block and should be called in a polling
        loop until a response is received.
        """
        reply = b''
        recv_data = []

        max_pkt_len = self.get_max_payload() + 100;
        
        try:
            (recv_data, recv_addr) = self.sock.recvfrom(max_pkt_len)
        except socket.error as err:
            expr = re.compile("timed out")
            if not expr.match(str(err)):
                print("Failed to receive UDP packet.\nError message:\n{}".format(err))
        
        if len(recv_data) > 0:
            hdr_len = 2 + self.hdr.sizeof()
            if (self.hdr.is_reply(recv_data[2:hdr_len])):
                reply = recv_data[hdr_len:]
            
        return reply


# End Class WnTransportEthUdpPy
































