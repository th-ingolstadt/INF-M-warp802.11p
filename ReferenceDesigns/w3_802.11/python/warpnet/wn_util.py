# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Utilities
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

This module provides WARPNet utility commands.

Functions (see below for more information):
    wn_ver() -- Returns WARPNet version
    wn_ver_str() -- Returns string of WARPNet version
    wn_init_nodes() -- Initialize nodes
    wn_setup() -- Set up wn_config.ini file
    wn_nodes_setup() -- Set up inital nodes_config.ini file

Integer constants:
    WN_MAJOR, WN_MINOR, WN_REVISION, WN_XTRA, WN_RELEASE
        -- WARPNet verision constants

"""

import os
import sys
import re
import inspect

from . import wn_exception as ex


__all__ = ['wn_ver', 'wn_ver_str', 'wn_init_nodes', 'wn_setup', 
           'wn_nodes_setup']


# WARPNet Version defines
WN_MAJOR                = 2
WN_MINOR                = 0
WN_REVISION             = 0
WN_XTRA                 = str('')
WN_RELEASE              = 1

# Fix to support Python 2.x and 3.x
if sys.version[0]=="3": raw_input=input



def wn_ver(major=WN_MAJOR, minor=WN_MINOR, revision=WN_REVISION, xtra=WN_XTRA, 
           output=1):
    """Returns the version of WARPNet for this package.
    
    If arguments are passed in to this function it will print a warning
    message if the version specified is older than the current version and
    will raise an WnVersionError if the version specified is newer
    than the current version.
    
    Attributes:
        major -- Major release number for WARPNet
        minor -- Minor release number for WARPnet
        revision -- Revision release number for WARPNet
        xtra -- Extra version string for WARPNet
        output -- Print output about the WARPNet version
    """

    # Print the release message if this is not an official release    
    if ( WN_RELEASE == 0 ): 
        print("***********************************************************")
        print("You are running a version of WARPNet that may not be ")
        print("compatible with released WARPNet bitstreams. Please use ")
        print("at your own risk.")
        print("***********************************************************")

    # Print the current version and location of the WARPNet Framework
    if (output == 1):
        print("WARPNet v" + wn_ver_str() + "\n\n")
        print("Framework Location:")
        print(os.path.dirname(
                  os.path.abspath(inspect.getfile(inspect.currentframe()))))

    # Check the provided version vs the current version
    output_str = str("Version Mismatch: Specified version " + 
                     wn_ver_str(major, minor, revision) + "is ")

    if (major == WN_MAJOR):
        if (minor == WN_MINOR):
            if (revision != WN_REVISION):
                # Since MAJOR & MINOR versions match, only print a warning
                if (revision < WN_REVISION):
                    print("WARNING: " + output_str + "older than WARPnet v" + wn_ver_str())
                else:
                    print("WARNING: " + output_str + "newer than WARPnet v" + wn_ver_str())
        else:
            if (minor < WN_MINOR):
                print("WARNING: " + output_str + "older than WARPnet v" + wn_ver_str())
            else:
                raise ex.WnVersionError("\n" + output_str + 
                                        "newer than WARPnet v" + 
                                        wn_ver_str())
    else:
        if (major < WN_MAJOR):
            print("WARNING: " + output_str + "older than WARPnet v" + wn_ver_str())
        else:
            raise ex.WnVersionError("\n" + output_str + 
                                    "newer than WARPnet v" + 
                                    wn_ver_str())
    
    return (WN_MAJOR, WN_MINOR, WN_REVISION)
    
    
# End of wn_ver()


def wn_ver_str(major=WN_MAJOR, minor=WN_MINOR, 
               revision=WN_REVISION, xtra=WN_XTRA):
    """Return a string of the WARPNet version.
    
    NOTE:  This will raise a WnVersionError if the arguments are not
    integers.    
    """
    output_str = ""
    exception  = False
    
    try:
        output_str = str("{0:d}.{1:d}.{2:d} {3:s}".format(major, minor, 
                                                          revision, xtra))
    except ValueError:
        # Set output string to default values so program can continue
        output_str = str("{0:d}.{1:d}.{2:d} {3:s}".format(WN_MAJOR, WN_MINOR, 
                                                          WN_REVISION, WN_XTRA))
        exception  = True

    if (exception):
        msg = str("\nUnknown Argument: All arguments should be integers: \n" +
                  "    major    = {} \n".format(major) +
                  "    minor    = {} \n".format(minor) +
                  "    revision = {} \n".format(revision))
        raise ex.WnVersionError(msg)
    
    return output_str
    
# End of wn_ver_str()


def wn_init_nodes(nodes_config, node_factory=None, output=False):
    """Initalize WARPNet nodes.

    Attributes:    
        nodes_config -- A WnNodesConfiguration object describing the nodes
        node_factory -- A WnNodeFactory or subclass to create nodes of a 
                        given WARPNet type
        output -- Print output about the WARPNet nodes
    """
    nodes = []

    # Parse the WARPNet INI file 
    import warpnet.wn_config as wn_config
    config = wn_config.WnConfiguration()

    host_id = int(config.get_param('network', 'host_id'))
    jumbo_frame_support = config.get_param('network', 'jumbo_frame_support')
    
    # Process the config to create nodes
    nodes_dict = nodes_config.get_nodes_dict()
    
    # If node_factory is not defined, create a default WnNodeFactory
    if node_factory is None:
        import warpnet.wn_node as wn_node
        node_factory = wn_node.WnNodeFactory()

    for node_dict in nodes_dict:
        if (host_id == node_dict['node_id']):
            raise ex.WnConfigError("Host id is set to {} and must be unique.".format(host_id))

        # Set up the node_factory from the Node dictionary
        node_factory.setup(node_dict)

        # Create the correct type of node; will return None and print a 
        #   warning if the node is not recognized
        node = node_factory.create_node()

        if not node is None:
            node.configure_node(jumbo_frame_support)
            nodes.append(node)

    if output:
        print("-" * 50)
        print("Initialized Nodes:")
        print("-" * 50)
        for node in nodes:
            print(node)
            print("-" * 30)
        print("-" * 50)

    return nodes

# End of wn_init_nodes()


def wn_setup():
    """Create WARPNet ini file from user input."""
    import warpnet.wn_config as wn_config
    config = wn_config.WnConfiguration()

    print("-" * 50)
    print("WARPNet Setup:")

    #-------------------------------------------------------------------------
    # Configure Python Path
    warpnet_dir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))

    print("-" * 50)
    print("Configuring Python Path:")

    if not warpnet_dir in sys.path:
        sys.path.append(warpnet_dir)
        print("    Adding: {0}".format(warpnet_dir))
    else:
        print("    In Path: {0}".format(warpnet_dir))    


    #-------------------------------------------------------------------------
    # Configure Host Address

    my_address = config.get_param('network', 'host_address')
    print("-" * 50)
    print("Please enter a WARPNet Ethernet interface address.")
    print("    Pressing Enter without typing an input will use a default ")
    print("    IP address of {0} \n".format(my_address))

    my_address_valid = False

    while not my_address_valid:
        temp = raw_input("WARPNet Ethernet Inteface Address: ")
        if not temp is '':
            expr = re.compile('\d+\.\d+\.\d+\.\d+')
            if not expr.match(temp) is None:
                print("    Setting IP address to {0}".format(temp))
                config.set_param('network', 'host_address', temp)
                my_address_valid = True
            else:
                print("    '{0}' is not a valid IP address.".format(temp))
                print("    Please enter a valid IP address.")
        else:
            print("    Setting IP address to {0}".format(my_address))
            my_address_valid = True

    # Check that this is a valid IP address
    output = ""
    if (os.name != 'posix'):
        temp = os.popen("ipconfig /all", "r")
    else:
        temp = os.popen("ifconfig -a", "r")
        
    while 1:
        line = temp.readline()
        output = output + line
        if not line: break

    my_address = config.get_param('network', 'host_address')
    if output.find(my_address) == -1:
        print("WARNING: No interface found.  Please make sure your network")
        print("    interface is connected and configured with static IP {0}".format(my_address))

    #-------------------------------------------------------------------------
    # Configure Host ID

    my_id = config.get_param('network', 'host_id')
    print("-" * 50)
    print("Please enter a WARPNet Host ID.")
    print("    Valid Host IDs are integers in the range of [200,254].")
    print("    Pressing Enter without typing an input will use a default")
    print("      Host ID of {0}\n".format(my_id))

    my_id_valid = False

    while not my_id_valid:
        temp = raw_input("WARPNet Host ID: ")
        if not temp is '':
            if (int(temp) >= 200) and (int(temp) <= 254):
                print("    Setting Host ID to {0}".format(temp))
                config.set_param('network', 'host_id', temp)
                my_id_valid = True
            else:
                print("    '{0}' is not a valid Host ID.".format(temp))
                print("    Valid Host IDs are integers in the range of [200,254].")
        else:
            print("    Setting Host ID to {0}".format(my_id))
            my_id_valid = True

    #-------------------------------------------------------------------------
    # Configure Unicast Port

    my_port = config.get_param('network', 'unicast_port')
    print("-" * 50)
    print("Please enter a unicast port.")
    print("    Pressing Enter without typing an input will use a default")
    print("      unicast port of {0}\n".format(my_port))

    temp = raw_input("WARPNet unicast port: ")
    if not temp is '':
        print("    Setting unicast port to {0}".format(temp))
        config.set_param('network', 'unicast_port', temp)
    else:
        print("    Setting unicast port to {0}".format(my_port))

    #-------------------------------------------------------------------------
    # Configure Broadcast Port

    my_port = config.get_param('network', 'bcast_port')
    print("-" * 50)
    print("Please enter a broadcast port.")
    print("    Pressing Enter without typing an input will use a default")
    print("      broadcast port of {0}\n".format(my_port))

    temp = raw_input("WARPNet broadcast port: ")
    if not temp is '':
        print("    Setting broadcast port to {0}".format(temp))
        config.set_param('network', 'bcast_port', temp)
    else:
        print("    Setting broadcast port to {0}".format(my_port))

    #-------------------------------------------------------------------------
    # Configure Transport 

    my_transport_type = config.get_param('network', 'transport_type')
    print("-" * 50)
    print("Currently the only transport supported by the Python WARPNet")
    print("    framework is '{0}'\n".format(my_transport_type))

    #-------------------------------------------------------------------------
    # Configure Transport Jumbo Frame Support

    my_jumbo_frame_support = config.get_param('network', 'jumbo_frame_support')
    print("-" * 50)
    print("Enable jumbo frame support? (experimental)")
    if my_jumbo_frame_support:
        print("    [1] false ")
        print("    [2] true  (default)\n")
    else:
        print("    [1] false (default)")
        print("    [2] true \n")
    
    my_jumbo_frame_support_valid = False

    while not my_jumbo_frame_support_valid:
        temp = raw_input("Selection: ")
        if not temp is '':
            if (int(temp) != 1) or (int(temp) != 2):
                if (int(temp) == 1):
                    print("    Setting jumbo frame support to false")
                    config.set_param('network', 'jumbo_frame_support', False)
                else:
                    print("    Setting jumbo frame support to true")
                    config.set_param('network', 'jumbo_frame_support', True)
                my_jumbo_frame_support_valid = True
            else:
                print("    '{0}' is not a valid selection.".format(temp))
                print("    Please select [1] or [2].")
        else:
            if my_jumbo_frame_support:
                print("    Setting jumbo frame support to true.")
            else:
                print("    Setting jumbo frame support to false.")
            my_jumbo_frame_support_valid = True

    #-------------------------------------------------------------------------
    # Finish WARPNet INI file

    print("-" * 50)
    config.save_config(output=True)
    print("WARPNet v {0} Configuration Complete.".format(wn_ver_str()))
    print("-" * 50, "\n\n\n")


    #-------------------------------------------------------------------------
    # Configure WARPNet Node Network

    network_setup_valid = False

    while not network_setup_valid:
        temp = raw_input("Perform Initial Network Setup [Y/n]: ").lower()
        if not temp is '':
            if (temp != 'y') or (temp != 'n'):
                if (temp == 'y'):
                    print("\n\n")
                    wn_nodes_setup()
                else:
                    print("-" * 50)
                    print("Done.")
                    print("-" * 50)                    
                network_setup_valid = True
            else:
                print("    '{0}' is not a valid selection.".format(temp))
                print("    Please select [y] or [n].")
        else:
            wn_nodes_setup()
            network_setup_valid = True

# End of wn_setup()



def wn_nodes_setup():
    """Create WARPNet Nodes ini file from user input."""    
    nodes_config = None

    print("-" * 50)
    print("WARPNet Nodes Setup:")

    base_dir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
    config_file = os.path.normpath(os.path.join(base_dir, "../", "nodes_config.ini"))

    print(config_file)

    print("!!! TBD !!!")
    print("-" * 50)

'''
    Select file
    Open Config 
    if exists
        print current contents
    Menu:
        [1] Add node
        [2] Remove node

       






    print("-" * 50)
    print("Please enter the filename for your node configuration file.")
    print("    Pressing Enter without typing an input will use a default ")
    print("    filename of {0}".format(host_address), end="\n\n")

    host_address_valid = False

    while not host_address_valid:
        temp = input("WARPNet Ethernet Inteface Address: ")
        if not temp is '':
            expr = re.compile('\d+\.\d+\.\d+\.\d+')
            if not expr.match(temp) is None:
                print("    Setting IP address to {0}".format(temp))
                config.set_param('network', 'host_address', temp)
                host_address_valid = True
            else:
                print("    '{0}' is not a valid IP address.".format(temp))
                print("    Please enter a valid IP address.")
        else:
            print("    Setting IP address to {0}".format(host_address))
            host_address_valid = True
'''


'''
    from . import wn_config
    config = wn_config.WnConfiguration()
'''

'''
    from Tkinter import Tk
    from tkFileDialog import askopenfilename
    Tk().withdraw() # we don't want a full GUI, so keep the root window from appearing
    filename = askopenfilename() # show an "Open" dialog box and return the path to the selected file
    print(filename)
'''


























