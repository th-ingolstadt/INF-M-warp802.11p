# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment Version
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

This module provides WLAN Exp version information and commands.

Functions (see below for more information):
    wlan_exp_ver()       -- Returns WLAN Exp version
    wlan_exp_ver_str()   -- Returns string of WLAN Exp version

Integer constants:
    WLAN_EXP_MAJOR, WLAN_EXP_MINOR, WLAN_EXP_REVISION, WLAN_EXP_XTRA, 
        WLAN_EXP_RELEASE -- WLAN Exp verision constants

"""

import os
import inspect

import warpnet.wn_exception as wn_ex


__all__ = ['wlan_exp_ver', 'wlan_exp_ver_str']


# WARPNet Version defines
WLAN_EXP_MAJOR               = 0
WLAN_EXP_MINOR               = 8
WLAN_EXP_REVISION            = 0
WLAN_EXP_XTRA                = str('')
WLAN_EXP_RELEASE             = True


#-----------------------------------------------------------------------------
# WLAN Exp Version Utilities
#-----------------------------------------------------------------------------

def wlan_exp_ver(major=WLAN_EXP_MAJOR, minor=WLAN_EXP_MINOR, 
                 revision=WLAN_EXP_REVISION, xtra=WLAN_EXP_XTRA, output=True):
    """Returns the version of WlanExp for this package.
    
    If arguments are passed in to this function it will print a warning
    message if the version specified is older than the current version and
    will raise an WarpNetVersionError if the version specified is newer
    than the current version.
    
    Attributes:
        major    -- Major release number for WLAN Exp
        minor    -- Minor release number for WLAN Exp
        revision -- Revision release number for WLAN Exp
        xtra     -- Extra version string for WLAN Exp
        output   -- Print output about the WLAN Exp version
    """
    
    # Print the release message if this is not an official release    
    if not WLAN_EXP_RELEASE: 
        print("-" * 60)
        print("You are running a version of WlanExp that may not be ")
        print("compatible with released WlanExp bitstreams. Please use ")
        print("at your own risk.")
        print("-" * 60)
    
    # Print the current version and location of the WLanExp Framework
    if output:
        print("WLAN Exp v" + wlan_exp_ver_str() + "\n")
        print("Framework Location:")
        print(os.path.dirname(
                  os.path.abspath(inspect.getfile(inspect.currentframe()))))
    
    # Check the provided version vs the current version
    msg  = "WLAN Exp Version Mismatch: \n:"
    msg += "    Specified version {0}\n".format(wlan_exp_ver_str(major, minor, revision))
    msg += "    Current   version {0}".format(wlan_exp_ver_str())
    
    if (major == WLAN_EXP_MAJOR):
        if (minor == WLAN_EXP_MINOR):
            if (revision != WLAN_EXP_REVISION):
                # Since MAJOR & MINOR versions match, only print a warning
                if (revision < WLAN_EXP_REVISION):
                    print("WARNING: " + msg + " (older)")
                else:
                    print("WARNING: " + msg + " (newer)")
        else:
            if (minor < WLAN_EXP_MINOR):
                print("WARNING: " + msg + " (older)")
            else:
                raise wn_ex.VersionError("ERROR: " + msg + " (newer)")
    else:
        if (major < WLAN_EXP_MAJOR):
            print("WARNING: " + msg + " (older)")
        else:
            raise wn_ex.VersionError("ERROR: " + msg + " (newer)")
    
    return (WLAN_EXP_MAJOR, WLAN_EXP_MINOR, WLAN_EXP_REVISION)
    
# End of wlan_exp_ver()


def wlan_exp_ver_str(major=WLAN_EXP_MAJOR, minor=WLAN_EXP_MINOR, 
                     revision=WLAN_EXP_REVISION, xtra=WLAN_EXP_XTRA):
    """Return a string of the WLAN Exp version.
    
    NOTE:  This will raise a WlanExpVersionError if the arguments are not
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
        
        msg  = "{0:d}.".format(WLAN_EXP_MAJOR)
        msg += "{0:d}.".format(WLAN_EXP_MINOR)
        msg += "{0:d} ".format(WLAN_EXP_REVISION)
        msg += "{0:s}".format(WLAN_EXP_XTRA)
        
        raise wn_ex.VersionError(error)
        
    return msg
    
# End of wlan_exp_ver_str()



