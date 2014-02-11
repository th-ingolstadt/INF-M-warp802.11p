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
                return self._get_param_hack(section, parameter)
            else:
                print("Parameter {} does not exist in section {}.".format(parameter, section))
        else:
            print("Section {} does not exist.".format(section))
        
        return None


    def _get_param_hack(self, section, parameter):
        """Internal method to work around differences in Python 2 vs 3"""
        if (section == 'network'):
            if ((parameter == 'host_address') or 
                (parameter == 'transport_type')):
                return self.config.get(section, parameter)
            else:
                return eval(self.config.get(section, parameter))
        else:
            return self.config.get(section, parameter)


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
    
    Attributes of a node:
        Node serial number
            node_id      -- Node ID
            node_name    -- Node Name
            ip_address   -- IP address of the Node
            unicast_port -- Unicast port of the Node
            bcast_port   -- Broadcast port of the Node
            use_node     -- Is this node part of the network
    
    Any parameter can be overridden by including it in the INI file.  
    
    When the configuration is read in, both a config and a shadow_config
    are created.  The config has values that will be written to the INI
    file, while the shadow_config has all values populated.  If values
    are not specified in the INI, they will get auto-populated defaults:
    
        node_id      - Monotonic counter starting at 0
        node_name    - "Node {0}".format(node_id)
        ip_address   - wn_config.ini get_param('network', 'host_address') for 
                       the first three octets and "node_id + 1" for the last 
                       octet
        unicast_port - wn_config.ini get_param('network', 'unicast_port')
        bcast_port   - wn_config.ini get_param('network', 'bcast_port')
        use_node     - "True"

    NOTE:  In order to be as consistent as possible, all nodes in the 
    configuration file get a node id regardless of whether they are used.
    Also, we do not sort the INI in our testing, it seems like the nodes are
    initialized in the order they appear in the config but are not guarenteed.

    """
    config              = None
    config_file         = None
    node_id_counter     = None
    used_node_ids       = None
    used_node_ips       = None
    shadow_config       = None

    def __init__(self, filename=wn_defaults.WN_DEFAULT_NODES_CONFIG_INI_FILE):

        self.node_id_counter = 0
        self.shadow_config   = {}
        self.used_node_ids   = []
        self.used_node_ips   = []

        temp_config = WnConfiguration()
        temp_unicast_port = temp_config.get_param('network', 'unicast_port')
        temp_bcast_port   = temp_config.get_param('network', 'bcast_port')
        temp_host_address = temp_config.get_param('network', 'host_address')

        # Convert host address to string:  "x.y.z.{0}"
        expr = re.compile('\.')
        data = expr.split(temp_host_address)
        temp_ip_address = str(data[0] + "." + data[1] + "." + data[2] + ".{0}")

        self.init_shadow_config(temp_ip_address, temp_unicast_port, temp_bcast_port)
        
        try:
            self.load_config(filename)
        except wn_exception.WnConfigError:
            self.set_default_config()


    #-------------------------------------------------------------------------
    # Methods for Config
    #-------------------------------------------------------------------------
    def set_default_config(self):
        """Set the default config."""
        self.config = configparser.ConfigParser()


    def add_node(self, serial_number, ip_address=None, 
                 node_id=None, unicast_port=None, bcast_port=None, 
                 node_name=None, use_node=None):
        """Add a node to the NodesConfig structure.
        
        Only serial_number and ip_address are required in the ini file.  Other
        fields will not be populated in the ini file unless they require a 
        non-default value.  
        """
        sn = self._get_serial_num(serial_number)
        
        if (sn in self.config.sections()):
            print("Node {0} exists.  Please use set_param to modify the node.".format(serial_number))
        else:
            self.config.add_section(sn)

            # Populate optional parameters
            if not ip_address   is None: self.config.set(sn, 'ip_address', ip_address)
            if not node_id      is None: self.config.set(sn, 'node_id', node_id)
            if not unicast_port is None: self.config.set(sn, 'unicast_port', unicast_port)
            if not bcast_port   is None: self.config.set(sn, 'bcast_port', bcast_port)
            if not node_name    is None: self.config.set(sn, 'node_name', node_name)
            if not use_node     is None: self.config.set(sn, 'use_node', use_node)

        # Add node to shadow_config
        self.add_shadow_node(serial_number)


    def remove_node(self, serial_number):
        """Remove a node from the NodesConfig structure."""
        sn = self._get_serial_num(serial_number)

        if (not self.config.remove_section(sn)):
            print("Node {0} not in nodes configuration.".format(sn))
        else:
            self.remove_shadow_node(sn)
        

    def get_param(self, section, parameter):
        """Returns the value of the parameter within the config for the node."""        
        sn = self._get_serial_num(section)

        return self.get_param_helper(sn, parameter)


    def get_param_helper(self, section, parameter):
        """Returns the value of the parameter within the config section."""
        if (section in self.config.sections()):
            if (parameter in self.config.options(section)):
                return self._get_param_hack(section, parameter)
            else:
                return self._get_shadow_param(section, parameter)
        else:
            print("Node '{}' does not exist.".format(section))
        
        return ""


    def set_param(self, section, parameter, value):
        """Sets the parameter to the given value."""
        sn = self._get_serial_num(section)
        
        if (sn in self.config.sections()):
            if (parameter in self.config.options(sn)):
                self._set_param_hack(sn, parameter, value)
                self.update_shadow_config(sn, parameter, value)
            else:
                if (parameter in self.shadow_config['default'].keys()):
                    self._set_param_hack(sn, parameter, value)
                    self.update_shadow_config(sn, parameter, value)
                else:
                    print("Parameter {} does not exist in node '{}'.".format(parameter, sn))
        else:
            print("Section '{}' does not exist.".format(sn))


    def remove_param(self, section, parameter):
        """Removes the parameter from the config."""
        sn = self._get_serial_num(section)
        
        if (sn in self.config.sections()):
            if (parameter in self.config.options(sn)):
                self.config.remove_option(sn, parameter)
                
                # Re-populate the shadow_config
                self.remove_shadow_node(sn)
                self.add_shadow_node(sn)
            else:
                # Fail silently so there are no issues when a user tries to 
                #    remove a shadow_config parameter
                pass
        else:
            print("Section '{}' does not exist.".format(sn))


    def get_nodes_dict(self):
        """Returns a list of dictionaries that contain the parameters of each
        WnNode specified in the config."""
        output = []

        if not self.config.sections():        
            raise wn_exception.WnConfigError("No Nodes in {0}".format(self.config_file))
        
        for node_config in self.config.sections():
            if (self.get_param_helper(node_config, 'use_node')):
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
                    node_dict = {
                        'serial_number': sn,
                        'node_id'      : self.get_param_helper(node_config, 'node_id'),
                        'node_name'    : self.get_param_helper(node_config, 'node_name'),
                        'ip_address'   : self.get_param_helper(node_config, 'ip_address'),
                        'unicast_port' : self.get_param_helper(node_config, 'unicast_port'),
                        'bcast_port'   : self.get_param_helper(node_config, 'bcast_port') }
                    output.append(node_dict)
        
        return output


    def load_config(self, file):
        """Loads the WARPNet config from the provided file."""
        self.config_file = os.path.normpath(file)
        
        # TODO: allow relative paths
        self.clear_shadow_config()
        self.config = configparser.ConfigParser()
        dataset = self.config.read(self.config_file)

        if len(dataset) != 1:
            msg = str("Error reading config file:\n" + self.config_file)
            raise wn_exception.WnConfigError(msg)
        else:
            self.init_used_node_lists()
            self.load_shadow_config()


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


    #-------------------------------------------------------------------------
    # Methods for Shadow Config
    #-------------------------------------------------------------------------
    def init_shadow_config(self, ip_addr_base, unicast_port, bcast_port):
        """Initialize the 'default' section of the shadow_config."""
        self.shadow_config['default'] = {'node_id'     : 'auto',
                                         'node_name'   : 'auto',
                                         'ip_address'  : ip_addr_base,
                                         'unicast_port': unicast_port,
                                         'bcast_port'  : bcast_port, 
                                         'use_node'    : True}

    def init_used_node_lists(self):
        """Initialize the lists used to keep track of fields that must 
        be unique.
        """
        self.used_node_ids = []
        self.used_node_ips = []
        
        for section in self.config.sections():
            if ('node_id' in self.config.options(section)):
                self.used_node_ids.append(self._get_param_hack(section, 'node_id'))

            if ('ip_address' in self.config.options(section)):
                self.used_node_ips.append(self._get_param_hack(section, 'ip_address'))        


    def clear_shadow_config(self):
        """Clear everything in the shadow config except 'default' section."""
        for section in self.shadow_config.keys():
            if (section != 'default'):
                del self.shadow_config[section]
        
        self.used_node_ids = []
        self.used_node_ips = []


    def load_shadow_config(self):
        """For each node in the config, populate the shadow_config."""        
        sections = self.config.sections()

        # Sort the config by serial number so there is consistent numbering
        # sections.sort()
        
        # Mirror any fields in the config and populate any missing fields 
        # with default values
        for section in sections:
            my_node_id      = self._get_node_id(section)
            my_node_name    = self._get_node_name(section, my_node_id)
            my_ip_address   = self._get_ip_address(section, my_node_id)
            my_unicast_port = self._get_unicast_port(section)
            my_bcast_port   = self._get_bcast_port(section)
            my_use_node     = self._get_use_node(section)
            
            # Set the node in the shadow_config
            self.set_shadow_node(section, my_ip_address, my_node_id, 
                                 my_unicast_port, my_bcast_port, 
                                 my_node_name, my_use_node)

        # TODO: Sanity check to make sure there a no duplicate Node IDs or IP Addresses


    def update_shadow_config(self, section, parameter, value):
        """Update the shadow_config with the given value."""
        self.shadow_config[section][parameter] = value


    def add_shadow_node(self, serial_number):
        """Add the given node to the shadow_config."""
        my_node_id      = self._get_node_id(serial_number)
        my_node_name    = self._get_node_name(serial_number, my_node_id)
        my_ip_address   = self._get_ip_address(serial_number, my_node_id)
        my_unicast_port = self._get_unicast_port(serial_number)
        my_bcast_port   = self._get_bcast_port(serial_number)
        my_use_node     = self._get_use_node(serial_number)

        # Set the node in the shadow_config
        self.set_shadow_node(serial_number, my_ip_address, my_node_id, 
                             my_unicast_port, my_bcast_port, 
                             my_node_name, my_use_node)

        

    def set_shadow_node(self, serial_number, ip_address, node_id, 
                        unicast_port, bcast_port, node_name, use_node):
        """Set the given node in the shadow_config."""
        self.shadow_config[serial_number] = {
            'node_id'      : node_id,
            'node_name'    : node_name,
            'ip_address'   : ip_address,
            'unicast_port' : unicast_port,
            'bcast_port'   : bcast_port,
            'use_node'     : use_node}

        self.used_node_ids.append(node_id)
        self.used_node_ips.append(ip_address)


    def remove_shadow_node(self, serial_number):
        """Remove the given node from the shadow_config."""
        self.used_node_ids.remove(self._get_shadow_param(serial_number, 'node_id'))
        self.used_node_ips.remove(self._get_shadow_param(serial_number, 'ip_address'))
        del self.shadow_config[serial_number]


    #-------------------------------------------------------------------------
    # Internal Methods
    #-------------------------------------------------------------------------
    def _get_next_node_id(self):
        next_node_id = self.node_id_counter
        self.node_id_counter += 1
        
        while (next_node_id in self.used_node_ids):
            next_node_id = self.node_id_counter
            self.node_id_counter += 1

        return next_node_id
    
    
    def _get_next_node_ip(self, node_id):
        ip_addr_base = self.shadow_config['default']['ip_address']
        node_id_base = node_id + 1

        next_ip_addr = ip_addr_base.format(node_id_base)
        node_id_base += 1
        
        while (next_ip_addr in self.used_node_ips):
            next_ip_addr = ip_addr_base.format(node_id_base)
            node_id_base += 1

        return next_ip_addr


    def _get_serial_num(self, value):
        if type(value) is int:
            sn = "W3-a-{0:05d}".format(value)
        elif type(value) is str:
            sn = value
        else:
            raise TypeError("Invalid serial number: {0}".format(value))

        return sn


    def _get_node_id(self, section):
        if ('node_id' in self.config.options(section)):
            return self._get_param_hack(section, 'node_id')
        else:
            return self._get_next_node_id()
            

    def _get_node_name(self, section, node_id):
        if ('node_name' in self.config.options(section)):
            return self._get_param_hack(section, 'node_name')
        else:
            return "Node {0}".format(node_id)


    def _get_ip_address(self, section, node_id):
        if ('ip_address' in self.config.options(section)):
            return self._get_param_hack(section, 'ip_address')
        else:
            return self._get_next_node_ip(node_id)


    def _get_unicast_port(self, section):
        if ('unicast_port' in self.config.options(section)):
            return self._get_param_hack(section, 'unicast_port')
        else:
            return self.shadow_config['default']['unicast_port']


    def _get_bcast_port(self, section):
        if ('bcast_port' in self.config.options(section)):
            return self._get_param_hack(section, 'bcast_port')
        else:
            return self.shadow_config['default']['bcast_port']


    def _get_use_node(self, section):
        if ('use_node' in self.config.options(section)):
            return self._get_param_hack(section, 'use_node')
        else:
            return True


    def _get_shadow_param(self, section, parameter):
        """Internal method to get shadow parameters.
        
        NOTE:  This is where to implement any per node defaults.
        """
        if (parameter in self.shadow_config[section].keys()):
            return self.shadow_config[section][parameter]
        else:
            print("Parameter {} does not exist in node '{}'.".format(parameter, section))
            return ""


    def _get_param_hack(self, section, parameter):
        """Internal method to work around differences in Python 2 vs 3"""
        if ((parameter == 'ip_address') or 
            (parameter == 'node_name')):
            return self.config.get(section, parameter)
        else:
            return eval(self.config.get(section, parameter))

 
    def _set_param_hack(self, section, parameter, value):
        """Internal method to work around differences in Python 2 vs 3"""
        my_value = str(value)
        self.config.set(section, parameter, my_value)


    #-------------------------------------------------------------------------
    # Printing / Debug Methods
    #-------------------------------------------------------------------------
    def print_shadow_config(self):
        for section in self.shadow_config.keys():
            print("{0}".format(section))
            for parameter in self.shadow_config[section].keys():
                print("    {0} = {1}".format(parameter, self.shadow_config[section][parameter]))
            print("")


    def print_config(self):
        for section in self.config.sections():
            print("{0}".format(section))
            for parameter in self.config.options(section):
                print("    {0} = {1}".format(parameter, self.config.get(section, parameter)))
            print("")


    def print_nodes(self):
        return_val = {}
        print("Current Nodes:")
        if (len(self.config.sections()) == 0):
            print("    None")
        sections = self.config.sections()
        # sections.sort()
        for idx, val in enumerate(sections):
            node_id = self.get_param_helper(val, 'node_id')
            ip_addr = self.get_param_helper(val, 'ip_address')
            use_node = self.get_param_helper(val, 'use_node')
            msg = "    [{0}] {1} - Node {2:3d} at {3:10s}".format(idx, val, node_id, ip_addr)
            if (use_node):
                print(str(msg + "   active"))
            else:
                print(str(msg + " inactive"))                
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
