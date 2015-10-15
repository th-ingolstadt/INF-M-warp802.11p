# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Transport
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

This module provides the base class for transports.

Integer constants:
    TRANSPORT_TYPE, TRANSPORT_HW_ADDR, TRANSPORT_IP_ADDR, 
    TRANSPORT_GROUP_ID, TRANSPORT_UNICAST_PORT, TRANSPORT_BROADCAST_PORT
      -- Transport hardware parameter constants 

    TRANSPORT_NO_RESP, TRANSPORT_RESP, TRANSPORT_BUFFER
      -- Transport response types

If additional hardware parameters are needed for sub-classes of Node, please
make sure that the values of these hardware parameters are not reused.
"""


__all__ = ['Transport']


# Transport Parameter Identifiers
#   NOTE:  The C counterparts are found in *_transport.h
TRANSPORT_TYPE                                   = 0
TRANSPORT_HW_ADDR                                = 1
TRANSPORT_IP_ADDR                                = 2
TRANSPORT_GROUP_ID                               = 3
TRANSPORT_UNICAST_PORT                           = 4
TRANSPORT_BROADCAST_PORT                         = 5

# Transport response types
TRANSPORT_NO_RESP                                = 0
TRANSPORT_RESP                                   = 1
TRANSPORT_BUFFER                                 = 2

# Transport Types
TRANSPORT_TYPE_IP_UDP                            = 0


class Transport(object):
    """Base class for transports.

    Attributes:
        hdr -- Transport header object
    """
    hdr = None

# End Class




