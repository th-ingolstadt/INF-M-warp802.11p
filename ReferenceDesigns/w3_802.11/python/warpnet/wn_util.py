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
    wn_init_nodes()                  -- Initialize nodes
    wn_identify_all_nodes()          -- Send the 'identify' command to all nodes
    wn_reset_network_inf_all_nodes() -- Reset the network interface for all nodes
    wn_get_serial_number()           -- Standard way to check / process serial numbers

"""

import sys
import re

from . import wn_exception as wn_ex
from . import version


__all__ = ['wn_init_nodes', 'wn_identify_all_nodes', 'wn_reset_network_inf_all_nodes', 
           'wn_get_serial_number']


# Fix to support Python 2.x and 3.x
if sys.version[0]=="3": raw_input=input


#-----------------------------------------------------------------------------
# WARPNet Node Utilities
#-----------------------------------------------------------------------------

def wn_init_nodes(nodes_config, host_config=None, node_factory=None, 
                  output=False):
    """Initalize WARPNet nodes.

    Attributes:    
        nodes_config -- A WnNodesConfiguration object describing the nodes
        host_config  -- A WnConfiguration object describing the host configuration
        node_factory -- A WnNodeFactory or subclass to create nodes of a 
                        given WARPNet type
        output -- Print output about the WARPNet nodes
    """
    nodes = []

    # Create a Host Configuration if there is none provided
    if host_config is None:
        import warpnet.wn_config as wn_config
        host_config = wn_config.HostConfiguration()

    host_id = host_config.get_param('network', 'host_id')
    jumbo_frame_support = host_config.get_param('network', 'jumbo_frame_support')
    
    # Process the config to create nodes
    try:
        nodes_dict = nodes_config.get_nodes_dict()
    except wn_ex.ConfigError as err:
        print(err)
        return nodes

    # If node_factory is not defined, create a default WnNodeFactory
    if node_factory is None:
        import warpnet.wn_node as wn_node
        node_factory = wn_node.WnNodeFactory(host_config)

    # Send a broadcast network reset command to make sure all nodes are
    # in their default state.
    wn_reset_network_inf_all_nodes(host_config=host_config)

    # Create the nodes in the dictionary
    for node_dict in nodes_dict:
        if (host_id == node_dict['node_id']):
            msg = "Host id is set to {0} and must be unique.".format(host_id)
            raise wn_ex.ConfigError(msg)

        # Set up the node_factory from the Node dictionary
        node_factory.setup(node_dict)

        # Create the correct type of node; will return None and print a 
        #   warning if the node is not recognized
        node = node_factory.create_node(host_config)

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


def wn_identify_all_nodes(host_interfaces):
    """Issues a broadcast WARPNet command: Identify.

    All nodes should blink their Red LEDs for 10 seconds.    
    """
    import time
    import warpnet.wn_cmds as wn_cmds
    import warpnet.wn_config as wn_config
    import warpnet.wn_transport_eth_udp_py_bcast as tp_bcast

    if type(host_interfaces) is str:
        my_host_interfaces = [host_interfaces]
    elif type(host_interfaces) is list:
        my_host_interfaces = host_interfaces
    else:
        msg  = "Unknown host interface type: {0}\n".format(type(host_interfaces))
        msg += "    Should be either a list or a string.\n"
        raise TypeError(msg)
    
    host_config = wn_config.HostConfiguration(host_interfaces=my_host_interfaces)
    tx_buf_size = host_config.get_param('network', 'tx_buffer_size')
    rx_buf_size = host_config.get_param('network', 'rx_buffer_size')

    for ip_address in my_host_interfaces:

        host_ip_subnet = _get_ip_address_subnet(ip_address)

        msg  = "Identifying all nodes on subnet {0}.  ".format(host_ip_subnet)
        msg += "Please check the LEDs."
        print(msg)

        transport = tp_bcast.TransportEthUdpPyBcast(host_config=host_config,
                                                    host_ip=ip_address)

        transport.wn_open(tx_buf_size, rx_buf_size)

        cmd = wn_cmds.NodeIdentify(wn_cmds.IDENTIFY_ALL_NODES)
        payload = cmd.serialize()
        transport.send(payload)
        
        # Wait IDENTIFY_WAIT_TIME seconds for blink to complete since 
        #   broadcast commands cannot wait for a response.
        time.sleep(wn_cmds.IDENTIFY_WAIT_TIME)
        
        transport.wn_close()

# End of wn_identify_all_nodes()


def wn_reset_network_inf_all_nodes(host_config=None, host_interfaces=None):
    """Issues a broadcast WARPNet command: NodeResetNetwork.

    Attributes:
      host_config     - Host configuration (optional but takes precedence)
                        If host_config is present, host_interfaces is ignored.
      host_interfaces - Host interfaces (optional list of IP addresses)
                        Will use the default host_config.

    This will issue a broadcast network interface reset for all nodes on 
    each of the host_interfaces.
    """
    import warpnet.wn_cmds as wn_cmds
    import warpnet.wn_config as wn_config
    import warpnet.wn_transport_eth_udp_py_bcast as tp_bcast

    if not host_config is None:
        my_host_interfaces = host_config.get_param('network', 'host_interfaces')
    else:
        if not host_interfaces is None:
            if type(host_interfaces) is str:
                my_host_interfaces = [host_interfaces]
            elif type(host_interfaces) is list:
                my_host_interfaces = host_interfaces
            else:
                msg  = "Unknown host interface type: {0}\n".format(type(host_interfaces))
                msg += "    Should be either a list or a string.\n"
                raise TypeError(msg)
            
            host_config = wn_config.HostConfiguration(host_interfaces=my_host_interfaces)
        else:
            msg  = "Need to provide either host configuration or a list of "
            msg += "host interfaces.\n"
            raise AttributeError(msg)

    tx_buf_size  = host_config.get_param('network', 'tx_buffer_size')
    rx_buf_size  = host_config.get_param('network', 'rx_buffer_size')
    
    for ip_address in my_host_interfaces:

        host_ip_subnet = _get_ip_address_subnet(ip_address)

        msg  = "Reseting the network config for all nodes on subnet "
        msg += "{0}.".format(host_ip_subnet)
        print(msg)
    
        transport = tp_bcast.TransportEthUdpPyBcast(host_config=host_config,
                                                    host_ip=ip_address)

        transport.wn_open(tx_buf_size, rx_buf_size)

        cmd = wn_cmds.NodeResetNetwork(wn_cmds.NETWORK_RESET_ALL_NODES)
        payload = cmd.serialize()
        transport.send(payload)
        
        transport.wn_close()

# End of wn_identify_all_nodes()


#-----------------------------------------------------------------------------
# WARPNet Misc Utilities
#-----------------------------------------------------------------------------
def wn_get_serial_number(serial_number, output=True):
    """Function will check / convert the provided serial number to a WARPNet
    compatible format.
    
    Acceptable inputs:
        'W3-a-00001' or 'w3-a-00001' -- Succeeds quietly
        '00001' or '1' or 1          -- Succeeds with a warning

    Returns:
        Tuple:  (serial_number, serial_number_str)
    """
    ret_val       = None
    print_warning = False    
    
    if type(serial_number) is int or type(serial_number) is long:
        sn            = serial_number
        sn_str        = "W3-a-{0:05d}".format(sn)
        print_warning = True
        ret_val       = (sn, sn_str)
        
    elif type(serial_number) is str or type(serial_number) is unicode:
        expr = re.compile('((?P<prefix>[Ww]3-a-)|)(?P<sn>\d+)')
        m    = expr.match(serial_number)
        if m:
            sn      = int(m.group('sn'))
            sn_str  = "W3-a-{0:05d}".format(sn)            
            
            if not m.group('prefix'):
                print_warning = True            
            
            ret_val = (sn, sn_str)
        else:
            msg  = "Incorrect serial number.  \n"
            msg += "    Should be of the form : W3-a-XXXXX\n"
            raise TypeError(msg)
    else:
        raise TypeError("Serial number should be an int or str.")
    
    if print_warning and output:
        msg  = "Interpreting provided serial number as "
        msg += "{0}".format(sn_str)
        print(msg)
    
    return ret_val

# End of wn_get_serial_number()



#-----------------------------------------------------------------------------
# WARPNet Setup Utilities
#-----------------------------------------------------------------------------
def wn_setup():
    """Setup the WARPNet Host Configuration from user input.
    
    NOTE:      
    """
    import warpnet.wn_config as wn_config
    config = wn_config.HostConfiguration()

    print("-" * 70)
    print("WARPNet Host Configuration Setup:")
    print("-" * 70)

    #-------------------------------------------------------------------------
    # Configure Host Interfaces

    my_interfaces = config.get_param('network', 'host_interfaces')
    print("-" * 50)
    print("Please enter a list of WARPNet host interface addresses.")
    print("    Pressing Enter without typing an input will use the default: ")
    for idx, interface in enumerate(my_interfaces):
        print("        Interface {0} = {1} \n".format(idx, interface))

    temp_interfaces = []
    my_interfaces_valid = False

    while not my_interfaces_valid:
        temp = raw_input("WARPNet Ethernet Inteface Addresses (Enter to end): ")
        if not temp is '':
            if (_check_ip_address_format(temp)):                
                print("    Adding IP address: {0}".format(temp))
                temp_interfaces.append(temp)
            else:
                print("    Please enter a valid IP address.")
        else:
            my_interfaces_valid = True

    if (len(temp_interfaces) > 0):
        config.set_param('network', 'host_interfaces', temp_interfaces)
        my_interfaces = temp_interfaces
        
    print("    Setting host interfaces to: ")
    for idx, interface in enumerate(my_interfaces):
        print("        Interface {0} = {1} \n".format(idx, interface))

    for interface in my_interfaces:
        _check_host_interface(interface)

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
            temp = int(temp)
            if (_check_host_id(temp)):
                print("    Setting Host ID to {0}".format(temp))
                config.set_param('network', 'host_id', temp)
                my_id_valid = True
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

    print("-" * 70)
    print("WARPNet v {0} Configuration Complete.".format(version.wn_ver_str()))
    print("-" * 70)
    print("\n")
    
    return config

# End of wn_setup()


def wn_nodes_setup(ini_file=None):
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
    nodes_config = wn_config.NodesConfiguration(ini_file=ini_file)

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
    nodes_config = wn_config.NodesConfiguration(ini_file=ini_file)
    nodes_config.print_nodes()

    print("-" * 70)
    print("Nodes Configuration Complete.")
    print("-" * 70)
    print("\n")

# End of wn_nodes_setup()



#-----------------------------------------------------------------------------
# Internal Methods
#-----------------------------------------------------------------------------

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
            if _check_ip_address_format(temp):
                ip_address = temp
                ip_address_done = True
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


def _check_host_interface(host_interface, bcast_port):
    """Check that this is a valid IP address."""
    import socket

    # Get the first three octets of the host_interface address
    host_ip_subnet = _get_ip_address_subnet(host_interface)
    test_ip_addr   = host_ip_subnet + ".255"

    # Create a temporary UDP socket to get the hostname
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect((test_ip_addr, bcast_port))
    socket_name = s.getsockname()[0]

    # Get the subnet of the socket
    sock_ip_subnet = _get_ip_address_subnet(socket_name)
    
    # Check that the socket_name is equal to the host_interface    
    if (host_interface == socket_name):
        return host_interface
    else:
        import warnings
        msg  = "\n    For {0} subnet, ".format(host_ip_subnet)
        msg += "the OS provided host interface is {0}\n".format(socket_name)
        msg += "        not {0}.\n".format(host_interface)
        msg += "    Please check your IP configuration.  "
        
        if (host_ip_subnet == sock_ip_subnet):
            msg += "Using host interface {0}.\n".format(socket_name)
            warnings.warn(msg)
            return socket_name
        else:
            warnings.warn(msg)
            return None

# End of _check_host_interface()


def _get_ip_address_subnet(ip_address):
    """Get the subnet X.Y.Z of ip_address X.Y.Z.W"""
    expr = re.compile('\.')
    tmp = [int(n) for n in expr.split(ip_address)]
    return "{0:d}.{1:d}.{2:d}".format(tmp[0], tmp[1], tmp[2])
    
# End of _get_ip_address_subnet()
    

def _check_ip_address_format(ip_address):
    """Check that the string has a valid IP address format."""
    expr = re.compile('\d+\.\d+\.\d+\.\d+')

    if not expr.match(ip_address) is None:
        return True
    else:
        import warnings
        msg  = "\n'{0}' is not a valid IP address format.\n" .format(ip_address) 
        msg += "    Please use A.B.C.D where A, B, C and D are in [0, 254]"
        warnings.warn(msg)
        return False

# End of _check_ip_address_format()


def _check_host_id(host_id):
    """Check that the host_id is valid."""
    if (host_id >= 200) and (host_id <= 254):
        return True
    else:
        import warnings
        msg  = "\n'{0}' is not a valid Host ID.\n" .format(host_id) 
        msg += "    Valid Host IDs are integers in the range of [200,254]"
        warnings.warn(msg)
        return False

# End of _check_host_id()


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

# End of _get_os_socket_buffer_size()
    



