# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment Exceptions
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

This module provides class definitions for all WLAN Exp exceptions.

Functions (see below for more information):
    WlanExpVersionError()

"""

__all__ = ['WlanExpVersionError']

class WlanExpError(Exception):
    """Base class for WlanExp exceptions."""
    pass

# End Class WlanExpError

class WlanExpVersionError(WlanExpError):
    """Exception for configuration file errors.
    
    Attributes:
        message -- explanation message of the error
    """
    
    def __init__(self, message):
        self.message = message

    def __str__(self):
        return str(self.message)
        
# End Class WlanExpVersionError

