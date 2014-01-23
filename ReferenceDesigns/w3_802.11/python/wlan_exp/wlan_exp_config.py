# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment Configuration
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

This module provides class definitions to manage the WLAN Exp configuration.

Functions (see below for more information):
    WlanExpConfiguration() -- Allows interaction with wlan_exp_config.ini file

"""

import os
import inspect
import datetime

try:                 # Python 3
  import configparser
except ImportError:  # Python 2
  import ConfigParser as configparser


import warpnet.wn_config as wn_config
import warpnet.wn_exception as wn_exception

import wlan_exp.wlan_exp_defaults as wlan_exp_defaults
import wlan_exp.wlan_exp_util as wlan_exp_util


__all__ = ['WlanExpConfiguration']


class WlanExpConfiguration(wn_config.WnConfiguration):
    """Class for WLAN Exp configuration.
    
    This class can load and store WLAN Exp configurations in the 
    config directory in wlan_exp_config.ini
    
    Attributes:
        config_info
            date -- Date created
            package -- Name of the package
            version -- Version of WLAN Exp
    """
    def __init__(self, filename=wlan_exp_defaults.WLAN_EXP_DEFAULT_INI_FILE):

        base_dir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
        self.config_file = os.path.join(base_dir, "config", filename)
        self.version_call = 'wlan_exp_util.wlan_exp_ver_str'
        self.package_name = wlan_exp_defaults.PACKAGE_NAME

        try:
            self.load_config()
        except wn_exception.WnConfigError:
            if (filename == wlan_exp_defaults.WLAN_EXP_DEFAULT_INI_FILE):
                # Default file does not exist; create & save default config
                self.set_default_config()
                self.save_config()
            else:
                print("WARNING:  Could not read {0}.".format(filename), 
                      "\n    Using default INI configuration.")
                self.set_default_config()


    def set_default_config(self):
        self.config = configparser.ConfigParser()

        self.config.add_section('config_info')
        self.config.add_section('warpnet')
        
        self.update_config_info()

        self.config.set('warpnet', str(wlan_exp_defaults.WLAN_EXP_AP_TYPE), wlan_exp_defaults.WLAN_EXP_AP_CLASS)
        self.config.set('warpnet', str(wlan_exp_defaults.WLAN_EXP_STA_TYPE), wlan_exp_defaults.WLAN_EXP_STA_CLASS)


    def update_config_info(self):
        """Updates the config info fields in the config."""
        date = str(datetime.date.today())
        version = eval(str(self.version_call + "()"))

        self.config.set('config_info', 'date', date)
        self.config.set('config_info', 'package', self.package_name)
        self.config.set('config_info', 'version', version)

# End Class WlanExpConfiguration
