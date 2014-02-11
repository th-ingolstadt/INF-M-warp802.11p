# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Exceptions
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

This module provides class definitions for all WARPNet exceptions.

Functions (see below for more information):
    WnVersionError()
    WnConfigError()
    WnParameterError()
    WnNodeError()
    WnTransportError()

"""

__all__ = ['WnVersionError', 'WnConfigError', 'WnParameterError',
           'WnNodeError', 'WnTransportError']

class WnError(Exception):
    """Base class for Wn exceptions."""
    pass

# End Class WnError

class WnVersionError(WnError):
    """Exception for configuration file errors.
    
    Attributes:
        message -- explanation message of the error
    """
    
    def __init__(self, message):
        self.message = message

    def __str__(self):
        msg  = "Version Error:"
        msg += "    {0} \n".format(self.message)
        return msg
        
# End Class WnVersionError

class WnConfigError(WnError):
    """Exception for configuration file errors.
    
    Attributes:
        message -- explanation message of the error
    """
    
    def __init__(self, message):
        self.message = message

    def __str__(self):
        msg  = "Config Error:"
        msg += "    {0} \n".format(self.message)
        return msg
        
# End Class WnConfigError

class WnParameterError(WnError):
    """Exception for parameter errors.
    
    Attributes:
        name -- name of the parameter
        message -- explanation message of the error
    """
    
    def __init__(self, name, message):
        self.name = name
        self.message = message

    def __str__(self):
        msg  = "Parameter {0} \n".format(self.name)
        msg += "    Error: {0} \n".format(self.message)
        return msg

# End Class WnParameterError

class WnNodeError(WnError):
    """Exception for WARPNet nodes.
    
    Attributes:
        node -- WARPNet node in which the error occurred.
        message -- explanation message of the error
    """
    
    def __init__(self, node, message):
        self.node = node
        self.message = message

    def __str__(self):
        msg  = "{0} \n".format(self.node.name)
        msg += "    Error: {0} \n".format(self.message)
        return msg

# End Class WnNodeError

class WnTransportError(WnError):
    """Exception for WARPNet transports.
    
    Attributes:
        transport -- WARPNet transport in which the error occurred.
        message -- explanation message of the error
    """
    
    def __init__(self, transport, message):
        self.transport = transport
        self.message = message

    def __str__(self):
        msg  = "{0} \n".format(self.transport)
        msg += "    Error: {0} \n".format(self.message)
        return msg

# End Class WnTransportError

