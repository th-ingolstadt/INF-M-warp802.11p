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

import re
import sys
import time

from . import defaults


__all__ = ['tx_rate_index_to_str',
           'init_nodes', 'init_timestamp',
           'filter_nodes']


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



#-----------------------------------------------------------------------------
# WLAN Exp Node Utilities
#-----------------------------------------------------------------------------
def init_nodes(nodes_config, host_config= None, node_factory=None, output=False):
    """Initalize WLAN Exp nodes.

    Attributes:
        nodes_config -- A WnNodesConfiguration describing the nodes
        host_config  -- A WnConfiguration object describing the host configuration
        node_factory -- A WlanExpNodeFactory or subclass to create nodes of a 
                        given WARPNet type
        output -- Print output about the WARPNet nodes
    """
    # Create a Host Configuration if there is none provided
    if host_config is None:
        import warpnet.wn_config as wn_config
        host_config = wn_config.HostConfiguration()

    # If node_factory is not defined, create a default WlanExpNodeFactory
    if node_factory is None:
        from . import node
        node_factory = node.WlanExpNodeFactory(host_config)

    # Use the WARPNet utility, wn_init_nodes, to initialize the nodes
    import warpnet.wn_util as wn_util
    return wn_util.wn_init_nodes(nodes_config, host_config, node_factory, output)

# End of init_nodes()


def init_timestamp(nodes, time_base=0, output=False, verbose=False, repeat=1):
    """Initialize the time on all of the WLAN Exp nodes.
    
    Attributes:
        nodes -- A list of nodes on which to initialize the time.
        time_base -- optional time base 
        output -- optional output to see jitter across nodes
    """
    return_times       = None
    node_start_times   = []

    print("Initializing {0} node(s) timestamps to: {1}".format(len(nodes), time_base))

    if output:
        return_times = {}
        for node in nodes:
            return_times[node.serial_number] = []
    
    for i in range(repeat):    
        start_time = _time()
    
        for node in nodes:
            node_time = time_base + (_time() - start_time)
            node.node_set_time(node_time)
            node_start_times.append(node_time)
        
        if output:
            for idx, node in enumerate(nodes):
                node_start_time = int(round(node_start_times[idx], 6) * (10**6))
                elapsed_time = int(round(_time() - start_time - node_start_times[idx], 6) * (10**6)) 
                node_time = node.node_get_time()
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
        
# End of init_timestamp()


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


def int2ip(ipaddr):
    """Convert an integer to IP address string (dotted notation)."""
    msg = ""
    if not ipaddr is None:
        msg += "{0:d}.".format((ipaddr >> 24) & 0xFF)
        msg += "{0:d}.".format((ipaddr >> 16) & 0xFF)
        msg += "{0:d}.".format((ipaddr >>  8) & 0xFF)
        msg += "{0:d}".format(ipaddr & 0xFF)
    return msg


def ip2int(ipaddr):
    """Convert IP address string (dotted notation) to an integer."""
    ret_val = 0
    if not ipaddr is None:
        expr = re.compile('\.')
        tmp = [int(n) for n in expr.split(ipaddr)]        
        ret_val = (tmp[3]) + (tmp[2] * 2**8) + (tmp[1] * 2**16) + (tmp[0] * 2**24)
    return ret_val


def mac2str(mac_address):
    """Convert an integer to a colon separated MAC address string."""
    msg = ""
    if not mac_address is None:

        #Force the input address to a Python int
        # This handles the case of a numpy uint64 input, which kills the >> operator
        if(type(mac_address) is not int):
            mac_address = int(mac_address)

        msg += "{0:02x}:".format((mac_address >> 40) & 0xFF)
        msg += "{0:02x}:".format((mac_address >> 32) & 0xFF)
        msg += "{0:02x}:".format((mac_address >> 24) & 0xFF)
        msg += "{0:02x}:".format((mac_address >> 16) & 0xFF)
        msg += "{0:02x}:".format((mac_address >>  8) & 0xFF)
        msg += "{0:02x}".format(mac_address & 0xFF)

    return msg

def mac_addr_desc(mac_addr, desc_map=None):
    """Returns a string description of a MAC address, 
    useful when printing a table of addresses

    Custom MAC address descriptions can be provided via the desc_map argument.
    desc_map must be a list or tuple of 3-tuples (addr_mask, addr_value, descritpion)
    The mac_addr argument will be bitwise AND'd with each addr_mask, then compared to
    addr_value. If the result is non-zero the corresponding descprition will be returned.

    If desc_map is not provided a few default 
    
    Example:
    desc_map = [ (0xFFFFFFFFFFFF, 0x000102030405, 'My Custom MAC Addr'), 
                 (0xFFFFFFFFFFFF, 0x000203040506, 'My Other MAC Addr') ]
    """

    #Cast to python int in case input is still numpy uint64
    mac_addr = int(mac_addr)

    desc_out = ''

    # IP->MAC multicast def: http://technet.microsoft.com/en-us/library/cc957928.aspx

    default_desc_map = [
            (0xFFFFFFFFFFFF, 0xFFFFFFFFFFFF, 'Broadcast'),
            (0xFFFFFF800000, 0x01005E000000, 'IP Multicast'),
            (0xFFFFFFFF0000, 0xFFFFFFFF0000, 'Anonymized Device'),
            (0xFFFFFFFFF000, 0x40D855042000, 'Mango WARP Hardware')]

    if(desc_map is None):
        desc_map = default_desc_map
    else:
        desc_map = list(desc_map) + default_desc_map

    for ii,(mask, req, desc) in enumerate(desc_map):
        if( (mac_addr & mask) == req):
            desc_out += desc
            break

    return desc_out

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


#-----------------------------------------------------------------------------
# Internal Methods
#-----------------------------------------------------------------------------
def _get_nodes_by_type(nodes, node_type):
    """Returns all nodes in the list that have the given node_type."""
    return [n for n in nodes if (n.node_type == node_type)]

# End of _get_nodes_by_type()


def _get_nodes_by_sn(nodes, serial_number):
    """Returns all nodes in the list that have the given serial number."""
    return [n for n in nodes if (n.serial_number == serial_number)]

# End of _get_nodes_by_type()


def _time():
    """WLAN Exp time function to handle differences between Python 2.7 and 3.3"""
    try:
        return time.perf_counter()
    except AttributeError:
        return time.clock()

# End of wlan_exp_init_time()





