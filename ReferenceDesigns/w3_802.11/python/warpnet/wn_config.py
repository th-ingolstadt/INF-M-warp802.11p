# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Config
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

This module provides class definitions to manage the WARPNet configuration.

Functions (see below for more information):
    WnConfiguration() -- Allows interaction with wn_config.ini file
    WnNodesConfiguration() -- Allows interaction with a nodes configuration file

"""

import os
import inspect
import datetime
import re

try:                 # Python 3
  import configparser
except ImportError:  # Python 2
  import ConfigParser as configparser


from . import wn_defaults
from . import wn_util
from . import wn_exception


__all__ = ['WnConfiguration', 'WnNodesConfiguration']



class WnConfiguration(object):
    """Class for WARPNet configuration.
    
    This class can load and store WARPNet configurations in the 
    config directory in wn_config.ini
    
    Attributes:
        config_info
            date -- Date created
            wn_ver -- Version of WARPNet
        network
            host_address -- Host IP address
            host_id -- Host indentifier
            unicast_port -- Default unicast port
            bcast_port -- Default broadcast port
            transport_type -- Type of transport
            jumbo_frame_support -- Use jumbo Ethernet frames?
    """
    config              = None
    config_file         = None
    version_call        = None
    package_name        = None
    
    def __init__(self, filename=wn_defaults.WN_DEFAULT_INI_FILE):

        base_dir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
        self.config_file = os.path.join(base_dir, "config", filename)
        self.version_call = 'wn_util.wn_ver_str'
        self.package_name = wn_defaults.PACKAGE_NAME

        try:
            self.load_config()
        except wn_exception.WnConfigError:
            if (filename == wn_defaults.WN_DEFAULT_INI_FILE):
                # Default file does not exist; create & save default config
                self.set_default_config()
                self.save_config()
            else:
                print("WARNING:  Could not read {0}.".format(filename), 
                      "\n    Using default INI configuration.")
                self.set_default_config()


    def set_default_config(self,
                           host_address=wn_defaults.WN_DEFAULT_HOST_ADDR,
                           host_id=wn_defaults.WN_DEFAULT_HOST_ID,
                           unicast_port=wn_defaults.WN_DEFAULT_UNICAST_PORT,
                           bcast_port=wn_defaults.WN_DEFAULT_BCAST_PORT,
                           tx_buf_size=wn_defaults.WN_DEFAULT_TX_BUFFER_SIZE,
                           rx_buf_size=wn_defaults.WN_DEFAULT_RX_BUFFER_SIZE,
                           transport_type=wn_defaults.WN_DEFAULT_TRANSPORT_TYPE,
                           jumbo_frame_support=wn_defaults.WN_DEFAULT_JUMBO_FRAME_SUPPORT):
        self.config = configparser.ConfigParser()

        self.config.add_section('config_info')
        self.config.add_section('warpnet')
        self.config.add_section('network')
        
        self.update_config_info()

        self.config.set('warpnet', str(wn_defaults.WN_NODE_TYPE), wn_defaults.WN_NODE_TYPE_CLASS)

        self.config.set('network', 'host_address', host_address)
        self.config.set('network', 'host_id', host_id)
        self.config.set('network', 'unicast_port', unicast_port)
        self.config.set('network', 'bcast_port', bcast_port)
        self.config.set('network', 'tx_buffer_size', tx_buf_size)
        self.config.set('network', 'rx_buffer_size', rx_buf_size)
        self.config.set('network', 'transport_type', transport_type)
        self.config.set('network', 'jumbo_frame_support', jumbo_frame_support)


    def update_config_info(self):
        """Updates the config info fields in the config."""
        date = str(datetime.date.today())
        version = eval(str(self.version_call + "()"))

        self.config.set('config_info', 'date', date)
        self.config.set('config_info', 'package', self.package_name)
        self.config.set('config_info', 'version', version)


    def get_wn_types(self):
        """Returns the 'warpnet' section of the ini file"""
        return self.get_section('warpnet')
    

    def get_param(self, section, parameter):
        """Returns the value of the parameter within the section."""
        if (section in self.config.sections()):
            if (parameter in self.config.options(section)):
                return self.config.get(section, parameter)
            else:
                print("Parameter {} does not exist in section {}.".format(parameter, section))
        else:
            print("Section {} does not exist.".format(section))
        
        return None
 

    def get_section(self, section):
        """Returns the dictionary of the section within the config."""
        if (section in self.config.sections()):
            # Necessary for backward compatibility to ConfigParser.  New 
            # version of configparser does this automatically:
            #    return self.config['warpnet']
            ret_val = {}
            items = self.config.items(section);
            for item in items:
                ret_val[item[0]] = item[1]
            return ret_val
        else:
            print("Section {} not defined in config file.".format(section))
        
        return None


    def set_param(self, section, parameter, value):
        """Sets the parameter to the given value."""
        if (section in self.config.sections()):
            if (parameter in self.config.options(section)):
                self.config.set(section, parameter, value)
            else:
                print("Parameter {} does not exist in section {}.".format(parameter, section))
        else:
            print("Group {} does not exist.".format(section))
 

    def load_config(self):
        """Loads the WARPNet config from the config file."""
        self.config = configparser.ConfigParser()
        dataset = self.config.read(self.config_file)

        if len(dataset) != 1:
           raise wn_exception.WnConfigError(str("Error reading config file:\n" + 
                                                self.config_file))
        
        # TODO:  check version?  check warpnet section present?


    def save_config(self, output=False):
        """Saves the WARPNet config to the config file."""        
        self.update_config_info()
        
        if output:
            print("Saving config to: \n    {0}".format(self.config_file))
        
        try:
            with open(self.config_file, 'w') as configfile:
                self.config.write(configfile)
        except IOError as err:
            print("Error writing config file: {0}".format(err))
        

    def __str__(self):
        section_str = "Contains parameters: \n"
        for section in self.config.sections():
            section_str = str(section_str + 
                              "    Section '" + str(section) + "':\n" + 
                              "        " + str(self.config.options(section)) + "\n")
        
        if not self.config_file:
            return str("Default config: \n" + section_str)
        else:
            return str(self.config_file + ": \n" + section_str)
            

# End Class WnConfiguration



class WnNodesConfiguration(object):
    """Class for WARPNet Node Configuration.
    
    This class can load and store WARPNet Node configurations
    
    Depending on the configuration, it can either default the node_id
    to a monotonic counter or the last octet of the IP address.
    
    Attributes:
        Node serial number
            node_id -- Node ID
            ip_address -- IP address of the Node
            unicast_port -- Unicast port of the Node
            bcast_port -- Broadcast port of the Node
            node_name -- Node Name
            use_node -- Is this node part of the network
    """
    config              = None
    config_file         = None
    node_id_counter     = None
    node_id_map         = None
    default_params      = {'ip_address': wn_defaults.WN_NODE_DEFAULT_IP_ADDR, 
                           'node_id': wn_defaults.WN_NODE_DEFAULT_NODE_ID, 
                           'unicast_port': wn_defaults.WN_NODE_DEFAULT_UNICAST_PORT,
                           'bcast_port': wn_defaults.WN_NODE_DEFAULT_BCAST_PORT, 
                           'node_name': wn_defaults.WN_NODE_DEFAULT_NAME, 
                           'use_node': 'True'}

    def __init__(self, filename=wn_defaults.WN_DEFAULT_NODES_CONFIG_INI_FILE,
                       use_node_id_counter=True):
        if (use_node_id_counter):
            self.node_id_counter = 0
            self.node_id_map = {}
        
        try:
            self.load_config(filename)
        except wn_exception.WnConfigError:
            self.set_default_config()


    def set_default_config(self):
        """Set the default config."""
        self.config = configparser.ConfigParser()


    def add_node(self, serial_number, ip_address, 
                 node_id=None, unicast_port=None, bcast_port=None, 
                 node_name=None, use_node=True):
        """Add a node to the NodesConfig structure.
        
        Only serial_number and ip_address are required in the ini file.  Other
        fields will not be populated in the ini file unless they require a 
        non-default value.  
        """
        if type(serial_number) is int:
            sn = "W3-a-{0:05d}".format(serial_number)
        elif type(serial_number) is str:
            sn = serial_number
        else:
            raise TypeError("Invalid serial number: {0}".format(serial_number))
        
        if (sn in self.config.sections()):
            print("Node {0} exists.  Please use set_param to modify the node.".format(serial_number))
        else:
            self.config.add_section(sn)

            # Populate required parameters
            self.config.set(sn, 'ip_address', ip_address)
            
            # Populate optional parameters
            if not node_id      is None: self.config.set(sn, 'node_id', node_id)
            if not unicast_port is None: self.config.set(sn, 'unicast_port', unicast_port)
            if not bcast_port   is None: self.config.set(sn, 'bcast_port', bcast_port)
            if not node_name    is None: self.config.set(sn, 'node_name', node_name)
            if not use_node: self.config.set(sn, 'use_node', use_node)


    def remove_node(self, serial_number):
        """Remove a node from the NodesConfig structure."""
        if type(serial_number) is int:
            sn = "W3-a-{0:05d}".format(serial_number)
        elif type(serial_number) is str:
            sn = serial_number
        else:
            raise TypeError("Invalid serial number: {0}".format(serial_number))

        if (not self.config.remove_section(sn)):
            print("Node {0} not in nodes configuration.".format(sn))
        

    def get_param(self, section, parameter):
        """Returns the value of the parameter within the config for the node."""        
        if (type(section) is str):
            sn = section
        else:
            # Assume that we are dealing w/ a node
            sn = "W3-a-{0:05d}".format(section.serial_number)

        return self.get_param_helper(sn, parameter)


    def _get_node_id(self, section):
        """Internal method to get the node id.  
        
        If we are using the node_id_counter, then return the counter value
        associated with the node.  Otherwise, use the last octet of the ip
        address.
        """
        if (not self.node_id_counter is None):
            if (not section in self.node_id_map.keys()):
                self.node_id_map[section] = self.node_id_counter
                self.node_id_counter += 1
            return self.node_id_map[section]
        else:
            # Return the last octet of the IP address of the node
            expr = re.compile('\.')
            data = expr.split(self.get_param_helper(section, 'ip_address'))
            return data[3]


    def _get_default_param(self, section, parameter):
        """Internal method to get default parameters.
        
        NOTE:  This is where to implement any per node defaults.
        """
        if (parameter == 'node_id'):
            return self._get_node_id(section)
        else:
            return self.default_params[parameter]
        pass


    def get_param_helper(self, section, parameter):
        """Returns the value of the parameter within the config section."""
        if (section in self.config.sections()):
            if (parameter in self.config.options(section)):
                return self.config.get(section, parameter)
            else:
                if (parameter in self.default_params):
                    return self._get_default_param(section, parameter)
                else:
                    print("Parameter {} does not exist in node '{}'.".format(parameter, section))
        else:
            print("Node '{}' does not exist.".format(section))
        
        return ""


    def set_param(self, section, parameter, value):
        """Sets the parameter to the given value."""
        
        if (type(section) is str):
            sn = section
        else:
            # Assume that we are dealing w/ a node
            sn = "W3-a-{0:05d}".format(section.serial_number)
        
        if (sn in self.config.sections()):
            if (parameter in self.config.options(sn)):
                self.config.set(sn, parameter, value)
            else:
                if (parameter in self.default_params):
                    self.config.set(sn, parameter, value)
                else:
                    print("Parameter {} does not exist in node '{}'.".format(parameter, sn))
        else:
            print("Section '{}' does not exist.".format(sn))
 

    def get_nodes_dict(self):
        """Returns a list of dictionaries that contain the parameters of each
        WnNode specified in the config."""
        output = []

        if not self.config.sections():        
            raise wn_exception.WnConfigError("No Nodes in {0}".format(self.config_file))
        
        for node_config in self.config.sections():
            if (self.get_param_helper(node_config, 'use_node') == 'True'):
                add_node = True
                
                expr = re.compile('W3-a-(?P<sn>\d+)')
                try:
                    sn = int(expr.match(node_config).group('sn'))
                except AttributeError:
                    print(str("Incorrect serial number.  \n" +
                              "    Should be of the form : W3-a-XXXXX" +
                              "    Provided serial number: {}".format(node_config)))
                    add_node = False

                if add_node:
                    node_dict = {'serial_number': sn,
                                 'node_id': int(self.get_param_helper(node_config, 'node_id')),
                                 'node_name': self.get_param_helper(node_config, 'node_name'),
                                 'ip_address': self.get_param_helper(node_config, 'ip_address'),
                                 'unicast_port': int(self.get_param_helper(node_config, 'unicast_port')),
                                 'bcast_port': int(self.get_param_helper(node_config, 'bcast_port'))}
                    output.append(node_dict)
        
        return output


    def load_config(self, file):
        """Loads the WARPNet config from the provided file."""
        self.config_file = os.path.normpath(file)
        
        # TODO: allow relative paths

        self.config = configparser.ConfigParser()
        dataset = self.config.read(self.config_file)

        if len(dataset) != 1:
           raise wn_exception.WnConfigError(str("Error reading config file:\n" + 
                                                self.config_file))


    def save_config(self, file=None, output=False):
        """Saves the WARPNet config to the provided file."""
        if not file is None:
            self.config_file = os.path.normpath(file)
        else:
            self.config_file = wn_defaults.WN_DEFAULT_NODES_CONFIG_INI_FILE
        
        if output:
            print("Saving config to: \n    {0}".format(self.config_file))

        try:
            with open(self.config_file, 'w') as configfile:
                self.config.write(configfile)
        except IOError as err:
            print("Error writing config file: {0}".format(err))


    def print_nodes(self):
        return_val = {}
        print("Current Nodes:")
        if (len(self.config.sections()) == 0):
            print("    None")
        for idx, val in enumerate(self.config.sections()):
            ip_addr = self.get_param_helper(val, 'ip_address')
            use_node = self.get_param_helper(val, 'use_node')
            if (use_node):
                print("    [{0}] {1} at {2}   active".format(idx, val, ip_addr))
            else:
                print("    [{0}] {1} at {2} inactive".format(idx, val, ip_addr))                
            return_val[idx] = val
        return return_val


    def __str__(self):
        section_str = "Contains parameters: \n"
        for section in self.config.sections():
            section_str = str(section_str + 
                              "    Section '" + str(section) + "':\n" + 
                              "        " + str(self.config.options(section)) + "\n")
        
        if not self.config_file:
            return str("Default config: \n" + section_str)
        else:
            return str(self.config_file + ": \n" + section_str)


# End Class WnNodesConfiguration
