# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Default Constants
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

This module provides WARPNet default constants.

"""

# Default Variables
PACKAGE_NAME                      = 'warpnet'

# WARPNet INI Files
WN_DEFAULT_INI_FILE               = 'wn_config.ini'
WN_DEFAULT_NODES_CONFIG_INI_FILE  = 'nodes_config.ini'


# WARPNet Node Types
WN_NODE_TYPE                      = 0
WN_NODE_TYPE_CLASS                = 'WnNode'


# WARPNet Default values
#   NOTE:  All defaults are strings; Numerical values will be evaluated and
#          converted to integers before being used
WN_DEFAULT_HOST_ADDR              = '10.0.0.250'
WN_DEFAULT_HOST_ID                = '250'
WN_DEFAULT_UNICAST_PORT           = '9500'
WN_DEFAULT_BCAST_PORT             = '9750'
WN_DEFAULT_TRANSPORT_TYPE         = 'python'
WN_DEFAULT_JUMBO_FRAME_SUPPORT    = 'False'
WN_DEFAULT_TX_BUFFER_SIZE         = '2**22'
WN_DEFAULT_RX_BUFFER_SIZE         = '2**22'



