# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment Utilities
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

This module provides WLAN Exp utility commands.

Functions (see below for more information):
    wlan_exp_ver() -- Returns WLAN Exp version
    wlan_exp_ver_str() -- Returns string of WLAN Exp version
    wlan_exp_init_nodes() -- Initialize nodes
    wlan_exp_init_time() -- Initialize the timebase on all nodes to be as 
                            similar as possible
    wlan_exp_setup() -- Set up wlan_exp_config.ini file

Integer constants:
    WLAN_EXP_MAJOR, WLAN_EXP_MINOR, WLAN_EXP_REVISION, WLAN_EXP_XTRA, 
        WLAN_EXP_RELEASE -- WLAN Exp verision constants

"""

import os
import sys
import time
import inspect

from . import wlan_exp_exception as ex
from . import wlan_exp_defaults as defaults


__all__ = ['wlan_exp_ver', 'wlan_exp_ver_str', 'wlan_exp_init_nodes']


# WARPNet Version defines
WLAN_EXP_MAJOR               = 0
WLAN_EXP_MINOR               = 7
WLAN_EXP_REVISION            = 0
WLAN_EXP_XTRA                = str('')
WLAN_EXP_RELEASE             = 1

# Fix to support Python 2.x and 3.x
if sys.version[0]=="3": raw_input=input


def wlan_exp_ver(major=WLAN_EXP_MAJOR, minor=WLAN_EXP_MINOR, 
                 revision=WLAN_EXP_REVISION, xtra=WLAN_EXP_XTRA, output=1):
    """Returns the version of WlanExp for this package.
    
    If arguments are passed in to this function it will print a warning
    message if the version specified is older than the current version and
    will raise an WarpNetVersionError if the version specified is newer
    than the current version.
    
    Attributes:
        major -- Major release number for WlanExp
        minor -- Minor release number for WlanExp
        revision -- Revision release number for WlanExp
        xtra -- Extra version string for WlanExp
    """

    # Print the release message if this is not an official release    
    if ( WLAN_EXP_RELEASE == 0 ): 
        print("***********************************************************")
        print("You are running a version of WlanExp that may not be ")
        print("compatible with released WlanExp bitstreams. Please use ")
        print("at your own risk.")
        print("***********************************************************")

    # Print the current version and location of the WLanExp Framework
    if (output == 1):
        print("WLAN Exp v" + wlan_exp_ver_str() + "\n")
        print("Framework Location:")
        print(os.path.dirname(
                  os.path.abspath(inspect.getfile(inspect.currentframe()))))

    # Check the provided version vs the current version
    output_str = str("Version Mismatch: Specified version " + 
                     wlan_exp_ver_str(major, minor, revision) + "is ")

    if (major == WLAN_EXP_MAJOR):
        if (minor == WLAN_EXP_MINOR):
            if (revision != WLAN_EXP_REVISION):
                # Since MAJOR & MINOR versions match, only print a warning
                if (revision < WLAN_EXP_REVISION):
                    print("WARNING: " + output_str + "older than WLAN Exp v" + wlan_exp_ver_str())
                else:
                    print("WARNING: " + output_str + "newer than WLAN Exp v" + wlan_exp_ver_str())
        else:
            if (minor < WLAN_EXP_MINOR):
                print("WARNING: " + output_str + "older than WLAN Exp v" + wlan_exp_ver_str())
            else:
                raise ex.WlanExpVersionError("\n" + output_str + 
                                             "newer than WLAN Exp v" + 
                                             wlan_exp_ver_str())
    else:
        if (major < WLAN_EXP_MAJOR):
            print("WARNING: " + output_str + "older than WLAN Exp v" + wlan_exp_ver_str())
        else:
            raise ex.WlanExpVersionError("\n" + output_str + 
                                         "newer than WLAN Exp v" + 
                                         wlan_exp_ver_str())
    
    return (WLAN_EXP_MAJOR, WLAN_EXP_MINOR, WLAN_EXP_REVISION)
    
    
# End of wn_ver()


def wlan_exp_ver_str(major=WLAN_EXP_MAJOR, minor=WLAN_EXP_MINOR, 
                     revision=WLAN_EXP_REVISION, xtra=WLAN_EXP_XTRA):
    """Return a string of the WLAN Exp version.
    
    NOTE:  This will raise a WlanExpVersionError if the arguments are not
    integers.    
    """
    output_str = ""
    exception  = False
    
    try:
        output_str = str("{0:d}.{1:d}.{2:d} {3:s}".format(major, minor, 
                                                          revision, xtra))
    except ValueError:
        # Set output string to default values so program can continue
        output_str = str("{0:d}.{1:d}.{2:d} {3:s}".format(WLAN_EXP_MAJOR, WLAN_EXP_MINOR, 
                                                          WLAN_EXP_REVISION, WLAN_EXP_XTRA))
        exception  = True

    if (exception):
        msg = str("\nUnknown Argument: All arguments should be integers: \n" +
                  "    major    = {} \n".format(major) +
                  "    minor    = {} \n".format(minor) +
                  "    revision = {} \n".format(revision))
        raise ex.WlanExpVersionError(msg)
    
    return output_str
    
# End of wn_ver_str()


def wlan_exp_init_nodes(nodes_config, node_factory=None, output=False):
    """Initalize WLAN Exp nodes.

    Attributes:
        nodes_config -- A WnNodesConfiguration describing the nodes
        node_factory -- A WlanExpNodeFactory or subclass to create nodes of a 
                        given WARPNet type
        output -- Print output about the WARPNet nodes
    """
    # If node_factory is not defined, create a default WlanExpNodeFactory
    if node_factory is None:
        from . import wlan_exp_node
        node_factory = wlan_exp_node.WlanExpNodeFactory()

    # Use the WARPNet utility, wn_init_nodes, to initialize the nodes
    import warpnet.wn_util as wn_util
    return wn_util.wn_init_nodes(nodes_config, node_factory, output)

# End of wlan_exp_init_nodes()


def wlan_exp_init_time(nodes, time_base=0, output=False, verbose=False, repeat=1):
    """Initialize the time on all of the WLAN Exp nodes.
    
    Attributes:
        nodes -- A list of nodes on which to initialize the time.
        time_base -- optional time base 
        output -- optional output to see jitter across nodes
    """
    return_times       = None
    node_start_times   = []

    print("Initializing {0} node(s) to time base: {1}".format(len(nodes), time_base))

    if output:
        return_times = {}
        for node in nodes:
            return_times[node.serial_number] = []
    
    for i in range(repeat):    
        start_time = wlan_exp_time()
    
        for node in nodes:
            node_time = time_base + (wlan_exp_time() - start_time)
            node.set_time(node_time)
            node_start_times.append(node_time)
        
        if output:
            for idx, node in enumerate(nodes):
                node_start_time = int(round(node_start_times[idx], 6) * (10**6))
                elapsed_time = int(round(wlan_exp_time() - start_time - node_start_times[idx], 6) * (10**6)) 
                node_time = node.get_time()
                diff_time = node_start_time + elapsed_time - node_time
                if (verbose):
                    print("Node {0}: \n".format(node.serial_number),
                          "    Start Time          = {0:8d}\n".format(node_start_time),
                          "    Elapsed System Time = {0:8d}\n".format(elapsed_time),
                          "    Time from Node      = {0:8d}\n".format(node_time), 
                          "    Time Difference     = {0:8d}\n".format(diff_time))

                return_times[node.serial_number].append(diff_time)

    if not return_times is None:
        return return_times
        
# End of wlan_exp_init_time()


def wlan_exp_time():
    """WLAN Exp time function to handle differences between Python 2.7 and 3.3"""
    try:
        return time.perf_counter()
    except AttributeError:
        return time.clock()

# End of wlan_exp_init_time()


def wlan_exp_setup():
    """Setup WLAN Exp framework."""

    print("-" * 70)
    print("WLAN Exp Setup:")

    #-------------------------------------------------------------------------
    # Configure Python Path

    print("-" * 70)
    print("Configuring Python Path")
    wlan_exp_dir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))

    if not wlan_exp_dir in sys.path:
        sys.path.append(wlan_exp_dir)
        print("    Adding: {0}".format(wlan_exp_dir))
    else:
        print("    In Path: {0}".format(wlan_exp_dir))    


    #-------------------------------------------------------------------------
    # Configure WARPNet

    import warpnet.wn_util as wn_util

    message = "Configure WARPNet installation [Y/n]: "
    temp = wn_util._get_confirmation_from_user(message)

    if (temp == 'y'):
        print("\n")
        wn_util.wn_setup()
    else:
        #-------------------------------------------------------------------------
        # Configure WARPNet Node Configuration
        #   - Called automatically if you do wn_setup()
        message = "\nPerform WARPNet Node Network Setup [Y/n]: "
        temp = wn_util._get_confirmation_from_user(message)
        if (temp == 'y'):
            print("\n")
            wn_util.wn_nodes_setup()
        else:
            print("-" * 70)
            print("Done.")
            print("-" * 70)

# End of wn_setup()


def wlan_exp_get_ap_nodes(nodes):
    """Returns all WLAN Exp AP nodes that are in the nodes list."""
    return wlan_exp_get_nodes(nodes, defaults.WLAN_EXP_AP_TYPE)

# End of wlan_exp_get_ap_nodes()


def wlan_exp_get_sta_nodes(nodes):
    """Returns all WLAN Exp STA nodes that are in the nodes list."""
    return wlan_exp_get_nodes(nodes, defaults.WLAN_EXP_STA_TYPE)

# End of wlan_exp_get_sta_nodes()


def wlan_exp_get_nodes(nodes, node_type):
    """Returns all nodes that are in the nodes list of type node_type."""
    return [n for n in nodes if (n.node_type == node_type)]

# End of wlan_exp_get_sta_nodes()





