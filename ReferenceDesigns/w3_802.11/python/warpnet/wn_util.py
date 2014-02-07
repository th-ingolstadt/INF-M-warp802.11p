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

    host_id = config.get_param('network', 'host_id')
    jumbo_frame_support = config.get_param('network', 'jumbo_frame_support')
    
    # Process the config to create nodes
    try:
        nodes_dict = nodes_config.get_nodes_dict()
    except ex.WnConfigError as err:
        print(err)
        return nodes
    
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
    """Setup the WARPNet framework and configure the WARPNet INI file 
    from user input."""
    import warpnet.wn_config as wn_config
    config = wn_config.WnConfiguration()

    print("-" * 70)
    print("WARPNet Setup:")
    print("-" * 70)

    #-------------------------------------------------------------------------
    # Configure Python Path
    warpnet_dir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))

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

    temp = os.popen("ifconfig -a", "r")

    while 1:
        line = temp.readline()
        output = output + line
        if not line: break

    if (output == ''):
        temp = os.popen("ipconfig /all", "r")
        
        while 1:
            line = temp.readline()
            output = output + line
            if not line: break

    my_address = config.get_param('network', 'host_address')
    if (output.find(my_address) == -1):
        print("")
        print("!" * 50)
        print("WARNING: No interface found.  Please make sure your network")
        print("    interface is connected and configured with static IP {0}".format(my_address))
        print("!" * 50)
        print("")

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
    # Configure Buffer Sizes

    my_tx_buf_size = config.get_param('network', 'tx_buffer_size')
    my_rx_buf_size = config.get_param('network', 'rx_buffer_size')

    # Check value in the config to make sure that is a valid size
    (tx_buf_size, rx_buf_size) = _get_os_socket_buffer_size(my_tx_buf_size, my_rx_buf_size)

    print("-" * 50)
    print("Set Default Transport buffer sizes:")
    print("    Send    = {0} bytes".format(tx_buf_size))
    print("    Receive = {0} bytes".format(rx_buf_size))
    print("Pressing enter will use defaults.\n")
    
    temp = raw_input("WARPNet send buffer size: ")
    if temp is '':
        temp = tx_buf_size
        
    print("    Setting send buffer size to {0}".format(temp))
    config.set_param('network', 'tx_buffer_size', temp)
    
    temp = raw_input("WARPNet receive buffer size: ")
    if temp is '':
        temp = rx_buf_size
        
    print("    Setting receive buffer size to {0}".format(temp))
    config.set_param('network', 'rx_buffer_size', temp)

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
    print("-" * 70)
    print("WARPNet v {0} Configuration Complete.".format(wn_ver_str()))
    print("-" * 70)
    print("\n")

    #-------------------------------------------------------------------------
    # Configure WARPNet Node Configuration

    message = "Perform WARPNet Node Network Setup [Y/n]: "
    temp = _get_confirmation_from_user(message)

    if (temp == 'y'):
        print("\n")
        wn_nodes_setup()
    else:
        print("-" * 70)
        print("Done.")
        print("-" * 70)                    

# End of wn_setup()


def wn_nodes_setup(file_name=None):
    """Create / Modify WARPNet Nodes ini file from user input."""    
    nodes_config = None

    print("-" * 70)
    print("WARPNet Nodes Setup:")
    print("   NOTE:  Many values are populated automatically to ease setup.  Excessive")
    print("          editing using this menu can result in the 'Current Nodes' displayed") 
    print("          being out of sync with the final nodes configuration.  The final")
    print("          nodes configuration will be printed on exit.  Please check that")
    print("          and re-run WARPNet Nodes Setup if necessary or manually edit the")
    print("          nodes_config ini file.")
    print("-" * 70)

    # base_dir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
    # config_file = os.path.normpath(os.path.join(base_dir, "../", "nodes_config.ini"))

    import warpnet.wn_config as wn_config
    if not file_name is None:
        nodes_config = wn_config.WarpNetNodesConfig(file_name)
    else:
        nodes_config = wn_config.WnNodesConfiguration()

    # Actions Menu    
    actions = {0 : "Add node",
               1 : "Remove node",
               2 : "Change Node ID",
               3 : "Disable / Enable node",
               4 : "Change IP address",
               5 : "Quit (default)"}

    nodes_done = False

    # Process actions until either a Ctrl-C or a quit action
    try:
        while not nodes_done:
            nodes = nodes_config.print_nodes()
            
            print("Actions:")
            for idx in range(len(actions.keys())):
                print("    [{0}] {1}".format(idx, actions[idx]))
            
            action = _select_from_table("Selection : ", actions)
            print("-" * 50)
            
            if (action is None) or (action == 5):
                nodes_done = True
            elif (action == 0):
                serial_num = _add_node_from_user(nodes_config)
                if not serial_num is None:
                    nodes_config.add_node(serial_num)
                    print("    Adding node: {0}".format(serial_num))
            elif (action == 1):
                node = _select_from_table("Select node : ", nodes)
                if not node is None:
                    nodes_config.remove_node(nodes[node])
                    print("    Removing node: {0}".format(nodes[node]))
            elif (action == 2):
                node = _select_from_table("Select node : ", nodes)
                if not node is None:
                    tmp_node_id = _get_node_id_from_user(nodes[node])
                    if not tmp_node_id is None:
                        nodes_config.set_param(nodes[node], 'node_id', tmp_node_id)
                        print("    Setting node: {0} to ID {1}".format(nodes[node], tmp_node_id))
                    else:
                        nodes_config.remove_param(nodes[node], 'node_id')
                        print("    Setting node: {0} to IP address {1}".format(nodes[node], 
                                nodes_config.get_param(nodes[node], 'node_id')))
            elif (action == 3):
                node = _select_from_table("Select node : ", nodes)
                if not node is None:
                    if (nodes_config.get_param(nodes[node], 'use_node')):
                        nodes_config.set_param(nodes[node], 'use_node', False)
                        print("    Disabling node: {0}".format(nodes[node]))
                    else:
                        nodes_config.set_param(nodes[node], 'use_node', True)
                        print("    Enabling node: {0}".format(nodes[node]))
            elif (action == 4):
                node = _select_from_table("Select node : ", nodes)
                if not node is None:
                    ip_addr = _get_ip_address_from_user(nodes[node])
                    if not ip_addr is None:
                        nodes_config.set_param(nodes[node], 'ip_address', ip_addr)
                        print("    Setting node: {0} to IP address {1}".format(nodes[node], ip_addr))
                    else:
                        nodes_config.remove_param(nodes[node], 'ip_address')
                        print("    Setting node: {0} to IP address {1}".format(nodes[node], 
                                nodes_config.get_param(nodes[node], 'ip_address')))

            print("-" * 50)
            
    except KeyboardInterrupt:
        pass

    nodes_config.save_config(output=True)
    print("-" * 70)
    print("Final Nodes Configuration:")
    print("-" * 70)

    # Print final configuration
    if not file_name is None:
        nodes_config = wn_config.WarpNetNodesConfig(file_name)
    else:
        nodes_config = wn_config.WnNodesConfiguration()

    nodes_config.print_nodes()

    print("-" * 70)
    print("Nodes Configuration Complete.")
    print("-" * 70)
    print("\n")

# End of wn_nodes_setup()


def _add_node_from_user(nodes_config):
    """Internal method to add a node based on info from the user."""
    serial_num = None
    serial_num_done = False

    while not serial_num_done:
        temp = raw_input("WARP Node Serial Number (last 5 digits or enter to end): ")
        if not temp is '':
            serial_num = "W3-a-{0:05d}".format(int(temp))
            message = "    Is {0} Correct? [Y/n]: ".format(serial_num)
            confirmation = _get_confirmation_from_user(message)
            if (confirmation == 'y'):
                serial_num_done = True
            else:
                serial_num = None
        else:
            break

    return serial_num

# End of _add_node_from_user()


def _get_node_id_from_user(msg):
    """Internal method to change a node's ID based on info from the user."""
    node_id = None
    node_id_done = False

    while not node_id_done:
        print("-" * 30)
        temp = raw_input("Enter ID for node {0}: ".format(msg))
        if not temp is '':
            if (int(temp) < 254) and (int(temp) >= 0):
                print("    Setting Node ID to {0}".format(temp))
                node_id = int(temp)
                node_id_done = True
            else:
                print("    '{0}' is not a valid Node ID.".format(temp))
                print("    Valid Node IDs are integers in the range of [0,254).")
        else:
            break
    
    return node_id

# End of _get_ip_address_from_user()


def _get_ip_address_from_user(msg):
    """Internal method to change a node's IP address based on info from 
    the user.
    """
    ip_address = None
    ip_address_done = False

    while not ip_address_done:
        print("-" * 30)
        temp = raw_input("Enter IP address for {0}: ".format(msg))
        if not temp is '':
            expr = re.compile('\d+\.\d+\.\d+\.\d+')
            if not expr.match(temp) is None:
                ip_address = temp
                ip_address_done = True
            else:
                print("    '{0}' is not a valid IP address.".format(temp))
                print("    Please enter a valid IP address.")
        else:
            break
    
    return ip_address

# End of _get_ip_address_from_user()


def _select_from_table(select_msg, table):
    return_val = None
    selection_done = False

    while not selection_done:
        temp = raw_input(select_msg)
        if not temp is '':
            if (not int(temp) in table.keys()):
                print("{0} is an invalid Selection.  Please choose again.".format(temp))
            else:
                return_val = int(temp)
                selection_done = True            
        else:
            selection_done = True

    return return_val

# End of _select_from_table()


def _get_confirmation_from_user(message):
    """Get confirmation from the user.  Default return value is 'y'."""
    confirmation = False
    
    while not confirmation:            
        selection = raw_input(message).lower()
        if not selection is '':
            if (selection == 'y') or (selection == 'n'):
                confirmation = True
            else:
                print("    '{0}' is not a valid selection.".format(selection))
                print("    Please select [y] or [n].")
        else:
            selection = 'y'
            confirmation = True
    
    return selection

# End of _get_confirmation_from_user()


def _get_os_socket_buffer_size(req_tx_buf_size, req_rx_buf_size):
    """Get the size of the send and receive buffers from the OS given the 
    requested TX/RX buffer sizes.
    """
    import errno
    import socket
    from socket import error as socket_error
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM);

    try:
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, req_tx_buf_size)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, req_rx_buf_size)
    except socket_error as serr:
        # On some HW we cannot set the buffer size
        if serr.errno != errno.ENOBUFS:
            raise serr

    sock_tx_buf_size = sock.getsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF)
    sock_rx_buf_size = sock.getsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF)
    
    return (sock_tx_buf_size, sock_rx_buf_size)

# End of _get_confirmation_from_user()
    



