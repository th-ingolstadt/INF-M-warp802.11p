"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework - Interactive Node
------------------------------------------------------------------------------
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
This will initialize the nodes in NODE_SERIAL_LIST to allow interactive use of 
wlan_exp nodes.

Hardware Setup:
 - Requires one or more WARP v3 nodes
 - PC NIC and ETH B on WARP v3 nodes connected to common Ethernet switch

Required Script Changes:
  - Set NETWORK to the IP address of your host PC NIC network (eg X.Y.Z.0 for IP X.Y.Z.W)
  - Set NODE_SERIAL_LIST to the serial numbers of your WARP nodes

Description:
  This will initialize all the nodes.  Then, the script will create an 
interactive prompt to allow for manipulation of the nodes.
------------------------------------------------------------------------------
"""
import wlan_exp.config as config
import wlan_exp.util as util


#-------------------------------------------------------------------------
#  Global experiment variables
#

# Change these values to match your experiment setup
NETWORK              = '10.0.0.0'
USE_JUMBO_ETH_FRAMES = False
NODE_SERIAL_LIST     = ['W3-a-00001']

#-------------------------------------------------------------------------
#  Initialization
#
# Create an object that describes the network configuration of the host PC
network_config = config.WlanExpNetworkConfiguration(network=NETWORK,
                                                    jumbo_frame_support=USE_JUMBO_ETH_FRAMES)

# Create an object that describes the WARP v3 nodes that will be used in this experiment
nodes_config   = config.WlanExpNodesConfiguration(network_config=network_config,
                                                  serial_numbers=NODE_SERIAL_LIST)

# Initialize the Nodes
#  This command will fail if any WARP v3 node does not respond
nodes = util.init_nodes(nodes_config, network_config)

print("\nInitialized nodes:")
# Put each node in a known, good state
for node in nodes:
    msg  = "    {0} ".format(repr(node))

    print(msg)

print("\n\n")
print("*********************************************************************")
print("Starting interactive console. The initialized wlan_exp node instances")
print(" are stored in the list variable 'nodes', indexed in order of node ID")
print("Example: blink LEDs at node ID 0: 'nodes[0].identify()'")
print("*********************************************************************")
print("\n\n")

# Create Debug prompt
util.debug_here()

