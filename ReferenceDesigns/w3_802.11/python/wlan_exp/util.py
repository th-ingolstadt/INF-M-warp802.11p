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

import wlan_exp.defaults as defaults


__all__ = ['tx_rate_to_str', 'tx_rate_index_to_str',
           'init_nodes', 'broadcast_cmd_set_time', 'broadcast_cmd_write_time_to_logs',
           'filter_nodes']


#-----------------------------------------------------------------------------
# WLAN Exp Rate definitions
#-----------------------------------------------------------------------------
wlan_rates = [{'index' :  1, 'rate' :  6.0, 'desc' : 'BPSK 1/2',   'NDBPS': 24},
              {'index' :  2, 'rate' :  9.0, 'desc' : 'BPSK 3/4',   'NDBPS': 36},
              {'index' :  3, 'rate' : 12.0, 'desc' : 'QPSK 1/2',   'NDBPS': 48},
              {'index' :  4, 'rate' : 18.0, 'desc' : 'QPSK 3/4',   'NDBPS': 72},
              {'index' :  5, 'rate' : 24.0, 'desc' : '16-QAM 1/2', 'NDBPS': 96},
              {'index' :  6, 'rate' : 36.0, 'desc' : '16-QAM 3/4', 'NDBPS': 144},
              {'index' :  7, 'rate' : 48.0, 'desc' : '64-QAM 2/3', 'NDBPS': 192},
              {'index' :  8, 'rate' : 54.0, 'desc' : '64-QAM 3/4', 'NDBPS': 216}]

def find_tx_rate_by_index(index):
    """Return the wlan_rates entry for the given index."""
    return _find_param_by_index(index, wlan_rates, 'tx rate')

# End def


def tx_rate_to_str(rate):
    """Convert a wlan_rates entry to a string."""
    msg = ""
    if type(rate) is dict:
        msg += "{0:2.1f} Mbps ({1})".format(rate['rate'], rate['desc'])
    else:
        print("Invalid Tx rate type.  Needed dict, provided {0}.".format(type(rate)))
    return msg

# End def


def tx_rate_index_to_str(index):
    """Convert a wlan_rates entry index to a string."""
    rate = find_tx_rate_by_index(index)
    return tx_rate_to_str(rate)

# End def


#-----------------------------------------------------------------------------
# WLAN Exp Channel definitions
#-----------------------------------------------------------------------------
wlan_channel = [{'index' :   1, 'channel' :   1, 'freq': 2412, 'desc' : '2.4 GHz Band'},
                {'index' :   2, 'channel' :   2, 'freq': 2417, 'desc' : '2.4 GHz Band'},
                {'index' :   3, 'channel' :   3, 'freq': 2422, 'desc' : '2.4 GHz Band'},
                {'index' :   4, 'channel' :   4, 'freq': 2427, 'desc' : '2.4 GHz Band'},
                {'index' :   5, 'channel' :   5, 'freq': 2432, 'desc' : '2.4 GHz Band'},
                {'index' :   6, 'channel' :   6, 'freq': 2437, 'desc' : '2.4 GHz Band'},
                {'index' :   7, 'channel' :   7, 'freq': 2442, 'desc' : '2.4 GHz Band'},
                {'index' :   8, 'channel' :   8, 'freq': 2447, 'desc' : '2.4 GHz Band'},
                {'index' :   9, 'channel' :   9, 'freq': 2452, 'desc' : '2.4 GHz Band'},
                {'index' :  10, 'channel' :  10, 'freq': 2457, 'desc' : '2.4 GHz Band'},
                {'index' :  11, 'channel' :  11, 'freq': 2462, 'desc' : '2.4 GHz Band'},
                {'index' :  36, 'channel' :  36, 'freq': 5180, 'desc' : '5 GHz Band'},
                {'index' :  40, 'channel' :  40, 'freq': 5200, 'desc' : '5 GHz Band'},
                {'index' :  44, 'channel' :  44, 'freq': 5220, 'desc' : '5 GHz Band'},
                {'index' :  48, 'channel' :  48, 'freq': 5240, 'desc' : '5 GHz Band'}]

def find_channel_by_index(index):
    """Return the wlan_channel entry for the given index."""
    return _find_param_by_index(index, wlan_channel, 'channel')

# End def


def find_channel_by_channel_number(channel_number):
    """Return the wlan_channel entry for the given channel number."""
    return _find_param_by_name('channel', channel_number, wlan_channel, 'channel')

# End def


def find_channel_by_freq(freq):
    """Return the wlan_channel entry for the given frequency."""
    return _find_param_by_name('freq', freq, wlan_channel, 'channel')

# End def


def channel_to_str(channel):
    """Convert a wlan_channel entry to a string."""
    msg = ""
    
    if type(channel) is int:
        tmp_chan = find_channel_by_channel_number(channel)
    if type(channel) is dict:
        tmp_chan = channel
    else:
        print("Invalid channel type.  Needed int or dict, provided {0}.".format(type(channel)))
        return msg

    msg += "{0:4d} ({1} MHz)".format(tmp_chan['channel'], tmp_chan['freq'])
    
    return msg

# End def


def channel_freq_to_str(freq):
    """Convert a wlan_rates entry index to a string."""
    channel = find_channel_by_freq(freq)
    return channel_to_str(channel)

# End def


#-----------------------------------------------------------------------------
# WLAN Exp Antenna Mode definitions
#-----------------------------------------------------------------------------
wlan_rx_ant_mode = [{'index' :  0x01, 'desc' : 'Rx SISO Antenna A'},
                    {'index' :  0x02, 'desc' : 'Rx SISO Antenna B'},
                    {'index' :  0x03, 'desc' : 'Rx SISO Antenna C'},
                    {'index' :  0x04, 'desc' : 'Rx SISO Antenna D'},
                    {'index' :  0x05, 'desc' : 'Rx SISO Selection Diversity 2 Antennas'}]
                    
wlan_tx_ant_mode = [{'index' :  0x10, 'desc' : 'Tx SISO Antenna A'},
                    {'index' :  0x20, 'desc' : 'Tx SISO Antenna B'}]


def find_rx_ant_mode_by_index(index):
    """Return the wlan_rx_ant_mode entry for the given index."""
    return _find_param_by_index(index, wlan_rx_ant_mode, 'rx antenna mode')

# End def


def rx_ant_mode_to_str(ant_mode):
    """Convert a wlan_rx_ant_mode entry to a string."""
    msg = ""
    if type(ant_mode) is dict:
        msg += "{0}".format(ant_mode['desc'])
    else:
        print("Invalid Rx antenna mode type.  Needed dict, provided {0}.".format(type(ant_mode)))
    return msg

# End def


def rx_ant_mode_index_to_str(index):
    """Convert a wlan_rx_ant_mode entry index to a string."""
    ant_mode = find_rx_ant_mode_by_index(index)
    return rx_ant_mode_to_str(ant_mode)

# End def


def find_tx_ant_mode_by_index(index):
    """Return the wlan_tx_ant_mode entry for the given index."""
    return _find_param_by_index(index, wlan_tx_ant_mode, 'tx antenna mode')

# End def


def tx_ant_mode_to_str(ant_mode):
    """Convert a wlan_tx_ant_mode entry to a string."""
    msg = ""
    if type(ant_mode) is dict:
        msg += "{0}".format(ant_mode['desc'])
    else:
        print("Invalid Tx antenna mode type.  Needed dict, provided {0}.".format(type(ant_mode)))
    return msg

# End def


def tx_ant_mode_index_to_str(index):
    """Convert a wlan_tx_ant_mode entry index to a string."""
    ant_mode = find_tx_ant_mode_by_index(index)
    return tx_ant_mode_to_str(ant_mode)

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

mac_addr_desc_map = [(0xFFFFFFFFFFFF, 0xFFFFFFFFFFFF, 'Broadcast'),
                     (0xFFFFFF800000, 0x01005E000000, 'IP v4 Multicast'),
                     (0xFFFF00000000, 0x333300000000, 'IP v6 Multicast'),
                     (0xFFFFFF000000, 0xFEFFFF000000, 'Anonymized Device'),
                     (0xFFFFFFFF0000, 0xFFFFFFFF0000, 'Anonymized Device'),
                     (0xFFFFFFFFF000, 0x40D855042000, 'Mango WARP Hardware')]


#-----------------------------------------------------------------------------
# WLAN Exp Node Utilities
#-----------------------------------------------------------------------------

# WARPNet Type Dictionary
#   This variable is not valid until init_nodes has been run.
warpnet_type_dict = None



def init_nodes(nodes_config, network_config=None, node_factory=None, output=False):
    """Initalize WLAN Exp nodes.

    Attributes:
        nodes_config   -- A NodesConfiguration describing the nodes
        network_config -- A NetworkConfiguration object describing the network configuration
        node_factory   -- A WlanExpNodeFactory or subclass to create nodes of a 
                          given WARPNet type
        output -- Print output about the WARPNet nodes
    """
    global warpnet_type_dict
    
    # Create a Host Configuration if there is none provided
    if network_config is None:
        import wlan_exp.warpnet.config as wn_config
        network_config = wn_config.NetworkConfiguration()

    # If node_factory is not defined, create a default WlanExpNodeFactory
    if node_factory is None:
        import wlan_exp.node as node
        node_factory = node.WlanExpNodeFactory(network_config)

    # Record the WARPNet type dictionary from the NodeFactory for later use
    warpnet_type_dict = node_factory.get_wn_type_dict()

    # Use the WARPNet utility, wn_init_nodes, to initialize the nodes
    import wlan_exp.warpnet.util as wn_util
    return wn_util.wn_init_nodes(nodes_config, network_config, node_factory, output)

# End def


def broadcast_cmd_set_time(time, network_config, time_id=None):
    """Initialize the timebase on all of the WLAN Exp nodes.
    
    This method will iterate through all host interfaces and issue a 
    broadcast packet on each interface that will set the time to the 
    timebase.  The method keeps track of how long it takes to send 
    each packet so that the time on all nodes is as close as possible
    even across interface.
    
    Attributes:
        network_config -- One or more NetworkConfiguration objects 
        time           -- Time to broadcast to the nodes either as a floating point number 
                          in seconds or an integer number of microseconds                 
        time_id        -- Optional value to identify broadcast time commands across nodes
    """
    import wlan_exp.cmds as cmds    
    _broadcast_time_to_nodes(time_cmd=cmds.CMD_PARAM_WRITE, network_config=network_config, time=time, time_id=time_id)

# End def


def broadcast_cmd_write_time_to_logs(network_config, time_id=None):
    """Add the current host time to the log on each node.

    This method will iterate through all host interfaces and issue a broadcast
    packet on each interface that will add the current time to the log
    
    Attributes:
        network_config -- One or more NetworkConfiguration objects 
        time_id        -- Optional value to identify broadcast time commands across nodes
    """
    import wlan_exp.cmds as cmds
    _broadcast_time_to_nodes(time_cmd=cmds.CMD_PARAM_TIME_ADD_TO_LOG, network_config=network_config, time_id=time_id)

# End def


def broadcast_cmd_write_exp_info_to_logs(network_config, info_type, message=None):
    """Add the EXP INFO log entry to the log on each node.

    This method will iterate through all host interfaces and issue a broadcast
    packet on each interface that will add the EXP INFO log entry to the log
    
    Attributes:
        network_config -- One or more NetworkConfiguration objects 
        info_type      -- Type of the experiment info.  This is an arbitrary 16 bit 
                          number chosen by the experimentor
        message        -- Information to be placed in the event log
    """
    import wlan_exp.cmds as cmds        

    if type(network_config) is list:
        configs = network_config
    else:
        configs = [network_config]
    
    for config in configs:
        _broadcast_cmd_to_nodes_helper(cmds.LogAddExpInfoEntry(info_type, message), config)

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
        import wlan_exp.warpnet.util as wn_util

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
        msg  = "\nNo nodes match filter: {0} = {1:x}".format(filter_type, filter_val)
        warnings.warn(msg)

    return ret_nodes

# End def


def int_to_ip(ip_address):
    """Convert an integer to IP address string (dotted notation)."""
    import wlan_exp.warpnet.transport_eth_udp as tport
    return tport.int_to_ip(ip_address)

# End def


def ip_to_int(ip_address):
    """Convert IP address string (dotted notation) to an integer."""
    import wlan_exp.warpnet.transport_eth_udp as tport
    return tport.ip_to_int(ip_address)

# End def


def mac_addr_to_str(mac_address):
    """Convert an integer to a colon separated MAC address string."""
    import wlan_exp.warpnet.transport_eth_udp as tport
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
    import wlan_exp.version as version
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
            import wlan_exp.node as node
            import wlan_exp.warpnet.config as wn_config
    
            network_config = wn_config.NetworkConfiguration()
            node_factory   = node.WlanExpNodeFactory(network_config)

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
    to the provided desc_map, the global mac_addr_desc_map that describes mappings 
    of different MAC addresses will also be used.

    Attributes:
        desc_map -- list or tuple of 3-tuples (addr_mask, addr_value, descritpion)

    The mac_addr argument will be bitwise AND'd with each addr_mask, then compared to
    addr_value. If the result is non-zero the corresponding descprition will be returned.
    
    Example:
    desc_map = [ (0xFFFFFFFFFFFF, 0x000102030405, 'My Custom MAC Addr'), 
                 (0xFFFFFFFFFFFF, 0x000203040506, 'My Other MAC Addr') ]
    """
    global mac_addr_desc_map

    #Cast to python int in case input is still numpy uint64
    mac_addr = int(mac_addr)

    desc_out = ''

    if(desc_map is None):
        desc_map = mac_addr_desc_map
    else:
        desc_map = list(desc_map) + mac_addr_desc_map

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


def _broadcast_time_to_nodes(time_cmd, network_config, time=0.0, time_id=None):
    """Internal method to issue broadcast time commands
    
    This method will iterate through all host interfaces and issue a 
    broadcast packet on each interface that will set the time to the 
    timebase.  The method keeps track of how long it takes to send 
    each packet so that the time on all nodes is as close as possible
    even across interface.
    
    Attributes:
        time_cmd       -- NodeProcTime command to issue
        network_config -- A NetworkConfiguration object
        time           -- Optional time to broadcast to the nodes (defaults to 0.0) 
                          either as an integer number of microseconds or a floating 
                          point number in seconds
        time_id        -- Optional value to identify broadcast time commands across nodes
    """
    import wlan_exp.cmds as cmds

    time_factor = 6               # Time can be in # of microseconds (ie 10^(-6) seconds)

    # Need to convert time input to float so that it can be added to _time()
    if   (type(time) is float):
        node_time   = time
    elif (type(time) is int):
        node_time   = float(time / (10**time_factor))   # Convert to float
    else:
        raise TypeError("Time must be either a float or int")       

    # Determine if we are sending to multiple networks
    if type(network_config) is list:
        configs = network_config
    else:
        configs = [network_config]

    # Send command to each network    
    for idx, config in enumerate(configs):
        network_addr    = config.get_param('network')

        if (idx == 0):
            node_time   = node_time
            start_time  = _time()
        else:
            node_time   = node_time + (_time() - start_time)
            
        cmd             = cmds.NodeProcTime(time_cmd, node_time, time_id);

        _broadcast_cmd_to_nodes_helper(cmd, network_config)
    
        msg = ""
        if (time_cmd == cmds.CMD_PARAM_WRITE):
            msg += "Initializing the time of all nodes on network "
            msg += "{0} to: {1}".format(network_addr, node_time)
        elif (time_cmd == cmds.CMD_PARAM_TIME_ADD_TO_LOG):
            msg += "Adding current time to log for nodes on network {0}".format(network_addr)
        print(msg)

# End def


def _broadcast_cmd_to_nodes_helper(cmd, network_config):
    """Internal method to issue broadcast commands 

    Attributes:
        network_config -- A NetworkConfiguration object
        cmd            -- A wn_message.Cmd object describing the command
    """
    import wlan_exp.warpnet.transport_eth_udp_py_bcast as bcast

    # Get information out of the NetworkConfiguration    
    tx_buf_size  = network_config.get_param('tx_buffer_size')
    rx_buf_size  = network_config.get_param('rx_buffer_size')

    # Create broadcast transport and send message
    transport_bcast = bcast.TransportEthUdpPyBcast(network_config);

    transport_bcast.wn_open(tx_buf_size, rx_buf_size)

    transport_bcast.send(cmd.serialize(), 'message')

    transport_bcast.wn_close()

# End def



def _find_param_by_index(index, param_list, param_name):
    """Return the entry for the given index."""
    ret_val = None

    if type(index) is int:
        for param in param_list:
            if (param['index'] == index):
                ret_val = param
                break

    if ret_val is None:
        print("Unknown {0} index: {1}".format(param_name, index))
    
    return ret_val

# End def


def _find_param_by_name(name, value, param_list, param_name):
    """Return the entry for the given name."""
    ret_val = None

    for param in param_list:
        if (param[name] == value):
            ret_val = param
            break

    if ret_val is None:
        print("Unknown {0} '{1}': {2}".format(param_name, name, value))
    
    return ret_val

# End def





