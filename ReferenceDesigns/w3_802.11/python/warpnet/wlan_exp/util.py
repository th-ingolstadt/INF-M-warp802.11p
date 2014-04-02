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
    tx_rate_index_to_str -- Converts tx_rate_index to string

    init_nodes()         -- Initialize nodes
    init_timestamp()     -- Initialize the timestamps on all nodes to be as 
                              similar as possible

    filter_nodes()       -- Filter a list of nodes

Integer constants:
    WLAN_EXP_MAJOR, WLAN_EXP_MINOR, WLAN_EXP_REVISION, WLAN_EXP_XTRA, 
        WLAN_EXP_RELEASE -- WLAN Exp verision constants

"""

import sys
import time

from . import defaults


__all__ = ['tx_rate_to_str', 'tx_rate_index_to_str',
           'init_nodes', 'broadcast_node_set_time', 'filter_nodes']


#-----------------------------------------------------------------------------
# WLAN Exp Rate definitions
#-----------------------------------------------------------------------------

wlan_rates = [{'index' :  1, 'rate' :  6.0, 'rate_str' : 'BPSK 1/2'},
              {'index' :  2, 'rate' :  9.0, 'rate_str' : 'BPSK 3/4'},
              {'index' :  3, 'rate' : 12.0, 'rate_str' : 'QPSK 1/2'},
              {'index' :  4, 'rate' : 18.0, 'rate_str' : 'QPSK 3/4'},
              {'index' :  5, 'rate' : 24.0, 'rate_str' : '16-QAM 1/2'},
              {'index' :  6, 'rate' : 36.0, 'rate_str' : '16-QAM 3/4'},
              {'index' :  7, 'rate' : 48.0, 'rate_str' : '64-QAM 2/3'},
              {'index' :  8, 'rate' : 54.0, 'rate_str' : '64-QAM 3/4'}]


def tx_rate_to_str(tx_rate):
    """Convert a TX rate from the 'rates' list to a string."""
    msg = ""
    if type(tx_rate) is dict:
        msg += "{0:2.1f} Mbps ({1})".format(tx_rate['rate'], tx_rate['rate_str'])
    else:
        print("Invalid TX rate type.  Needed dict, provided {0}.".format(type(tx_rate)))
    return msg

# End def


def tx_rate_index_to_str(tx_rate_index):
    """Convert a TX rate index to a string."""
    tx_rate = None
    msg     = ""

    if type(tx_rate_index) is int:
        for rate in wlan_rates:
            if (rate['index'] == tx_rate_index):
                tx_rate = rate
                break

    if not tx_rate is None:
        msg += "{0:2.1f} Mbps ({1})".format(tx_rate['rate'], tx_rate['rate_str'])
    else:
        print("Unknown tx rate index: {0}".format(tx_rate_index))

    return msg

# End def


#-----------------------------------------------------------------------------
# WLAN Exp MAC Address definitions
#-----------------------------------------------------------------------------

# MAC Description Map
#   List of tuples:  (mask, MAC value, description) describe various MAC addresses
#
# IP -> MAC multicast references: 
#     http://technet.microsoft.com/en-us/library/cc957928.aspx
#     http://en.wikipedia.org/wiki/Multicast_address#Ethernet
#     http://www.cavebear.com/archive/cavebear/Ethernet/multicast.html

mac_desc_map = [(0xFFFFFFFFFFFF, 0xFFFFFFFFFFFF, 'Broadcast'),
                (0xFFFFFF800000, 0x01005E000000, 'IP v4 Multicast'),
                (0xFFFF00000000, 0x333300000000, 'IP v6 Multicast'),
                (0xFFFFFFFF0000, 0xFFFFFFFF0000, 'Anonymized Device'),
                (0xFFFFFFFFF000, 0x40D855042000, 'Mango WARP Hardware')]


#-----------------------------------------------------------------------------
# WLAN Exp Node Utilities
#-----------------------------------------------------------------------------

# WARPNet Type Dictionary
#   This variable is not valid until init_nodes has been run.
warpnet_type_dict = None



def init_nodes(nodes_config, host_config=None, node_factory=None, output=False):
    """Initalize WLAN Exp nodes.

    Attributes:
        nodes_config -- A WnNodesConfiguration describing the nodes
        host_config  -- A WnConfiguration object describing the host configuration
        node_factory -- A WlanExpNodeFactory or subclass to create nodes of a 
                        given WARPNet type
        output -- Print output about the WARPNet nodes
    """
    global warpnet_type_dict
    
    # Create a Host Configuration if there is none provided
    if host_config is None:
        import warpnet.wn_config as wn_config
        host_config = wn_config.HostConfiguration()

    # If node_factory is not defined, create a default WlanExpNodeFactory
    if node_factory is None:
        from . import node
        node_factory = node.WlanExpNodeFactory(host_config)

    # Record the WARPNet type dictionary from the NodeFactory for later use
    warpnet_type_dict = node_factory.get_wn_type_dict()

    # Use the WARPNet utility, wn_init_nodes, to initialize the nodes
    import warpnet.wn_util as wn_util
    return wn_util.wn_init_nodes(nodes_config, host_config, node_factory, output)

# End def


def broadcast_node_set_time(time, host_config=None, time_id=None):
    """Initialize the timebase on all of the WLAN Exp nodes.
    
    This method will iterate through all host interfaces and issue a 
    broadcast packet on each interface that will set the time to the 
    timebase.  The method keeps track of how long it takes to send 
    each packet so that the time on all nodes is as close as possible
    even across interface.
    
    Attributes:
        host_config  -- A WnConfiguration object describing the host configuration
        time         -- Optional time to broadcast to the nodes (defaults to 0.0) 
                        either as an integer number of microseconds or a floating 
                        point number in seconds
        time_id      -- Optional value to identify broadcast time commands across nodes
    """
    import warpnet.wlan_exp.cmds as cmds
    _broadcast_node_time(time_cmd=cmds.TIME_WRITE, host_config=host_config, time=time, time_id=time_id)

# End def


def broadcast_node_add_current_time_to_log(host_config=None, time_id=None):
    """Add the current host time to the log on each node.

    This method will iterate through all host interfaces and issue a broadcast
    packet on each interface that will add the current time to the log
    
    Attributes:
        host_config  -- A WnConfiguration object describing the host configuration
        time_id      -- Optional value to identify broadcast time commands across nodes
    """
    import warpnet.wlan_exp.cmds as cmds
    _broadcast_node_time(time_cmd=cmds.TIME_ADD_TO_LOG, host_config=host_config, time_id=time_id)

# End def


#-----------------------------------------------------------------------------
# WLAN Exp Misc Utilities
#-----------------------------------------------------------------------------
def filter_nodes(nodes, filter_type, filter_val):
    """Return a list of nodes that match the filter_val for the given 
    filter_type.

    Supported Filter Types:
        'node_type' -- Filters nodes by WARPNet Types.  filter_val must 
            be either a integer corresponding to a WARPNet type (see 
            wlan_exp_defaults for example WARPNet types) or the following
            strings:  'AP'  (equivalent to WLAN_EXP_AP_TYPE)
                      'STA' (equivalent to WLAN_EXP_STA_TYPE)

        'serial_number' -- Filters nodes by WARPv3 serial number.  filter_val
            must be of the form:  'W3-a-XXXXX' where XXXXX is the five 
            digit serial number of the board.
    
    NOTE:  If the list is empty, then this method will issue a warning
    """
    ret_nodes = []

    if   (filter_type == 'node_type'):
        error = False

        if   (type(filter_val) is str):
            if   (filter_val.lower() == 'ap'):
                filter_val = defaults.WLAN_EXP_AP_TYPE
            elif (filter_val.lower() == 'sta'):
                filter_val = defaults.WLAN_EXP_STA_TYPE
            else:
                error = True

        elif (type(filter_val) is int):
            pass

        else:
            error = True;

        if not error:
            ret_nodes = _get_nodes_by_type(nodes, filter_val)
        else:            
            msg  = "Unknown filter value: {0}\n".format(filter_val)
            msg += "    Must be either 'AP' or 'STA'"
            print(msg)


    elif (filter_type == 'serial_number'):
        import warpnet.wn_util as wn_util

        try:
            (sn, sn_str) = wn_util.wn_get_serial_number(filter_val)
            ret_nodes    = _get_nodes_by_sn(nodes, sn)
        except TypeError as err:
            print(err)
    
    else:
        msg  = "Unknown filter type: {0}\n".format(filter_type)
        msg += "    Supported types: 'node_type', 'serial_number'"
        print(msg)

    if (len(ret_nodes) == 0):
        import warnings
        msg  = "\nNo nodes match filter: {0} = {1}".format(filter_type, filter_val)
        warnings.warn(msg)

    return ret_nodes

# End def


def int_to_ip(ip_address):
    """Convert an integer to IP address string (dotted notation)."""
    import warpnet.wn_transport_eth_udp as tport
    return tport.int_to_ip(ip_address)

# End def


def ip_to_int(ip_address):
    """Convert IP address string (dotted notation) to an integer."""
    import warpnet.wn_transport_eth_udp as tport
    return tport.ip_to_int(ip_address)

# End def


def mac_addr_to_str(mac_address):
    """Convert an integer to a colon separated MAC address string."""
    import warpnet.wn_transport_eth_udp as tport
    return tport.mac_addr_to_str(mac_address)

# End def


def sn_to_str(hw_generation, serial_number):
    """Convert serial number to a string for a given hardware generation."""
    if(hw_generation == 3):
        return ('W3-a-{0:05d}'.format(int(serial_number)))
    else:
        print("ERROR:  Not valid Hardware Generation: {0}".format(hw_generation))

# End def


def ver_code_to_str(ver_code):
    """Convert a WLAN Exp version code to a string."""
    import warpnet.wlan_exp.version as version
    return version.wlan_exp_ver_code_to_str(ver_code)

# End def


def node_type_to_str(node_type, node_factory=None):
    """Convert the Node WARPNet Type to a string description.

    Attributes:
        node_type    -- WARPNet node type code (u32)
        node_factory -- A WlanExpNodeFactory or subclass to create nodes of a 
                        given WARPNet type
    
    By default, the dictionary of WARPNet types is built dynamically 
    during init_nodes().  If init_nodes() has not been run, then the method 
    will try to create a WARPNet type dictionary.  If a node_factory is not 
    provided then a default WlanExpNodeFactory will be used to determine the 
    WARPNet type.  If a default WlanExpNodeFactory is used, then only 
    framework WARPNet types will be known and custom WARPNet types will
    return:  "Unknown WARPNet type: <value>"
    """
    global warpnet_type_dict

    # Build a warpnet_type_dict if it is not present
    if warpnet_type_dict is None:
        
        # Build a default node_factory if it is not present
        if node_factory is None:
            from . import node
            import warpnet.wn_config as wn_config
    
            host_config  = wn_config.HostConfiguration()
            node_factory = node.WlanExpNodeFactory(host_config)

        # Record the WARPNet type dictionary from the NodeFactory for later use
        warpnet_type_dict = node_factory.get_wn_type_dict()

    if (node_type in warpnet_type_dict.keys()):
        return warpnet_type_dict[node_type]['description']
    else:
        return "Unknown WARPNet type: 0x{0:8x}".format(node_type)
    
# End def


def mac_addr_desc(mac_addr, desc_map=None):
    """Returns a string description of a MAC address.

    This is useful when printing a table of addresses.  Custom MAC address 
    descriptions can be provided via the desc_map argument.  In addition 
    to the provided desc_map, the global mac_desc_map that describes mappings 
    of different MAC addresses will also be used.

    Attributes:
        desc_map -- list or tuple of 3-tuples (addr_mask, addr_value, descritpion)

    The mac_addr argument will be bitwise AND'd with each addr_mask, then compared to
    addr_value. If the result is non-zero the corresponding descprition will be returned.
    
    Example:
    desc_map = [ (0xFFFFFFFFFFFF, 0x000102030405, 'My Custom MAC Addr'), 
                 (0xFFFFFFFFFFFF, 0x000203040506, 'My Other MAC Addr') ]
    """
    global mac_desc_map

    #Cast to python int in case input is still numpy uint64
    mac_addr = int(mac_addr)

    desc_out = ''

    if(desc_map is None):
        desc_map = mac_desc_map
    else:
        desc_map = list(desc_map) + mac_desc_map

    for ii,(mask, req, desc) in enumerate(desc_map):
        if( (mac_addr & mask) == req):
            desc_out += desc
            break

    return desc_out

# End def


# Excellent util function for dropping into interactive Python shell
#   From http://vjethava.blogspot.com/2010/11/matlabs-keyboard-command-in-python.html
def debug_here(banner=None):
    """Function that mimics the matlab keyboard command for interactive debbug."""
    import code
    # Use exception trick to pick up the current frame
    try:
        raise None
    except:
        frame = sys.exc_info()[2].tb_frame.f_back

    print("# Use quit() or Ctrl-D to exit")

    # evaluate commands in current namespace
    namespace = frame.f_globals.copy()
    namespace.update(frame.f_locals)

    try:
        code.interact(banner=banner, local=namespace)
    except SystemExit:
        return

# End def


#-----------------------------------------------------------------------------
# Internal Methods
#-----------------------------------------------------------------------------
def _get_nodes_by_type(nodes, node_type):
    """Returns all nodes in the list that have the given node_type."""
    return [n for n in nodes if (n.node_type == node_type)]

# End def


def _get_nodes_by_sn(nodes, serial_number):
    """Returns all nodes in the list that have the given serial number."""
    return [n for n in nodes if (n.serial_number == serial_number)]

# End def


def _time():
    """WLAN Exp time function to handle differences between Python 2.7 and 3.3"""
    try:
        return time.perf_counter()
    except AttributeError:
        return time.clock()

# End def


def _broadcast_node_time(time_cmd, host_config=None, time=0.0, time_id=None):
    """Internal method to issue broadcast time commands
    
    This method will iterate through all host interfaces and issue a 
    broadcast packet on each interface that will set the time to the 
    timebase.  The method keeps track of how long it takes to send 
    each packet so that the time on all nodes is as close as possible
    even across interface.
    
    Attributes:
        time_cmd     -- NodeProcTime command to issue
        host_config  -- A WnConfiguration object describing the host configuration
        time         -- Optional time to broadcast to the nodes (defaults to 0.0) 
                        either as an integer number of microseconds or a floating 
                        point number in seconds
        time_id      -- Optional value to identify broadcast time commands across nodes
    """
    import warpnet.wn_transport_eth_udp_py_bcast as bcast
    import warpnet.wlan_exp.cmds                 as cmds
    import warpnet.wn_util                       as util

    time_factor = 6               # Time can be in # of microseconds (ie 10^(-6) seconds)
    
    # Create a Host Configuration if there is none provided
    if host_config is None:
        import warpnet.wn_config as wn_config
        host_config = wn_config.HostConfiguration()

    interfaces   = host_config.get_param('network', 'host_interfaces')
    tx_buf_size  = host_config.get_param('network', 'tx_buffer_size')
    rx_buf_size  = host_config.get_param('network', 'rx_buffer_size')

    # Need to convert time input to float so that it can be added to _time()
    if   (type(time) is float):
        node_time   = time
    elif (type(time) is int):
        node_time   = float(time / (10**time_factor))   # Convert to float
    else:
        raise TypeError("Time must be either a float or int")       

    if time_id is None:
        import random
        time_id = 2**32 * random.random()

    for idx, interface in enumerate(interfaces):
        # Create broadcast transport
        transport_bcast = bcast.TransportEthUdpPyBcast(host_config, interface);

        transport_bcast.wn_open(tx_buf_size, rx_buf_size)

        if (idx == 0):
            node_time   = node_time
            start_time  = _time()
        else:
            node_time   = node_time + (_time() - start_time)
            
        cmd             = cmds.NodeProcTime(time_cmd, node_time, time_id);

        transport_bcast.send(cmd.serialize(), 'message')

        msg = ""
        if (time_cmd == cmds.TIME_WRITE):
            msg += "Initializing the time of all nodes on "
            msg += "{0} to: {1}".format(util._get_ip_address_subnet(interface), node_time)
        elif (time_cmd == cmds.TIME_ADD_TO_LOG):
            msg += "Adding current time to log for nodes on {0}".format(util._get_ip_address_subnet(interface))            
        print(msg)

# End def


def _broadcast_node_cmd_helper(cmd, host_config=None):
    """Internal method to issue broadcast commands 

    Attributes:
        host_config  -- A WnConfiguration object describing the host configuration
        cmd          -- A wn_message.Cmd object describing the command
    """
    # Create a Host Configuration if there is none provided
    if host_config is None:
        import warpnet.wn_config as wn_config
        host_config = wn_config.HostConfiguration()

    interfaces   = host_config.get_param('network', 'host_interfaces')
    tx_buf_size  = host_config.get_param('network', 'tx_buffer_size')
    rx_buf_size  = host_config.get_param('network', 'rx_buffer_size')

    for interface in interfaces:
        import warpnet.wn_transport_eth_udp_py_bcast as bcast

        # Create broadcast transport
        transport_bcast = bcast.TransportEthUdpPyBcast(host_config, interface);

        transport_bcast.wn_open(tx_buf_size, rx_buf_size)

        transport_bcast.send(cmd.serialize(), 'message')

# End def





