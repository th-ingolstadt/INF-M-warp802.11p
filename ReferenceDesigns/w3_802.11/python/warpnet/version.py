# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Version
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
1.00a ejw  03/18/14 Initial release

------------------------------------------------------------------------------

This module provides WARPNet version information and commands.

Functions (see below for more information):
    wn_ver()     -- Returns WARPNet version
    wn_ver_str() -- Returns string of WARPNet version

Integer constants:
    WN_MAJOR, WN_MINOR, WN_REVISION, WN_XTRA, WN_RELEASE
        -- WARPNet verision constants

"""

import os
import inspect

from . import wn_exception as wn_ex


__all__ = ['wn_ver', 'wn_ver_str']


# WARPNet Version defines
WN_MAJOR                = 2
WN_MINOR                = 0
WN_REVISION             = 0
WN_XTRA                 = str('')
WN_RELEASE              = True


#-----------------------------------------------------------------------------
# WARPNet Version Utilities
#-----------------------------------------------------------------------------

def wn_ver(major=WN_MAJOR, minor=WN_MINOR, revision=WN_REVISION, xtra=WN_XTRA, 
           output=True):
    """Returns the version of WARPNet for this package.
    
    If arguments are passed in to this function it will print a warning
    message if the version specified is older than the current version and
    will raise an WnVersionError if the version specified is newer
    than the current version.
    
    Attributes:
        major    -- Major release number for WARPNet
        minor    -- Minor release number for WARPnet
        revision -- Revision release number for WARPNet
        xtra     -- Extra version string for WARPNet
        output   -- Print output about the WARPNet version
    """
    
    # Print the release message if this is not an official release    
    if not WN_RELEASE: 
        print("-" * 60)
        print("You are running a version of WARPNet that may not be ")
        print("compatible with released WARPNet bitstreams. Please use ")
        print("at your own risk.")
        print("-" * 60)
        
    # Print the current version and location of the WARPNet Framework
    if output:
        print("WARPNet v" + wn_ver_str() + "\n\n")
        print("Framework Location:")
        print(os.path.dirname(
                  os.path.abspath(inspect.getfile(inspect.currentframe()))))
        
    # Check the provided version vs the current version
    msg  = "WARPNet Version Mismatch: \n:"
    msg += "    Specified version {0}\n".format(wn_ver_str(major, minor, revision))
    msg += "    Current   version {0}".format(wn_ver_str())
    
    if (major == WN_MAJOR):
        if (minor == WN_MINOR):
            if (revision != WN_REVISION):
                # Since MAJOR & MINOR versions match, only print a warning
                if (revision < WN_REVISION):
                    print("WARNING: " + msg + " (older)")
                else:
                    print("WARNING: " + msg + " (newer)")
        else:
            if (minor < WN_MINOR):
                print("WARNING: " + msg + " (older)")
            else:
                raise wn_ex.VersionError("ERROR: " + msg + " (newer)")
    else:
        if (major < WN_MAJOR):
            print("WARNING: " + msg + " (older)")
        else:
            raise wn_ex.VersionError("ERROR: " + msg + " (newer)")
    
    return (WN_MAJOR, WN_MINOR, WN_REVISION)
    
# End of wn_ver()


def wn_ver_str(major=WN_MAJOR, minor=WN_MINOR, 
               revision=WN_REVISION, xtra=WN_XTRA):
    """Return a string of the WARPNet version.
    
    NOTE:  This will raise a VersionError if the arguments are not
    integers.    
    """
    try:
        msg  = "{0:d}.".format(major)
        msg += "{0:d}.".format(minor)
        msg += "{0:d} ".format(revision)
        msg += "{0:s}".format(xtra)
    except ValueError as err:
        # Set output string to default values so program can continue
        error = "Unknown Argument: All arguments should be integers"
        print(err)
        
        msg  = "{0:d}.".format(WN_MAJOR)
        msg += "{0:d}.".format(WN_MINOR)
        msg += "{0:d} ".format(WN_REVISION)
        msg += "{0:s}".format(WN_XTRA)
        
        raise wn_ex.VersionError(error)
    
    return msg
    
# End of wn_ver_str()

