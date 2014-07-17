"""
------------------------------------------------------------------------------
WARPNet Example
------------------------------------------------------------------------------
License:   Copyright 2014, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
This script uses the 802.11 ref design and WARPnet to create a closed network
of two nodes that no one is able to join.  Than allows the user to explore this
network.

Hardware Setup:
  - Requires two WARP v3 nodes
    - One node configured as AP using 802.11 Reference Design v0.95 or later
    - One node configured as STA using 802.11 Reference Design v0.95 or later
  - PC NIC and ETH B on WARP v3 nodes connected to common Ethernet switch

Required Script Changes:
  - Set NETWORK to the IP address of your host PC NIC network (eg X.Y.Z.0 for IP X.Y.Z.W)
  - Set NODE_SERIAL_LIST to the serial numbers of your WARP nodes
  - Set CHANNEL to the channel you want the network to be on
  - Set NETWORK_NAME to the string for the SSID of the network
  - Set ADDR_FILTER_LIST to the addresses of nodes that are allowed to associate wirelessly

Description:
  This script initializes two WARP v3 nodes, one AP and one STA.  It will then
configure the network.  First, it will set the address filter on the AP such that
no node can join wirelessly.  Then it will remove any associations from the AP and
STA and set all nodes on the same channel.  It will then configure the AP with a 
new SSID.  Finally, it will add the STA as an association to the AP so they can 
communicate and open a debug prompt so that the user can explore this network.
------------------------------------------------------------------------------
"""
import sys

import wlan_exp.config as wlan_exp_config
import wlan_exp.util as wlan_exp_util

#-----------------------------------------------------------------------------
# Top Level Script Variables
#-----------------------------------------------------------------------------
# NOTE: change these values to match your experiment setup
NETWORK           = '10.0.0.0'
NODE_SERIAL_LIST  = ['W3-a-00001', 'W3-a-00002']
CHANNEL           = 1
NETWORK_NAME      = "WARP Assoc Example"

# Device filter
#   Contains a list of tuples:  (Mask, MAC Address)
#
ADDR_FILTER_LIST  = [(0x000000000000, 0x000000000000)]               # Do not allow anyone


#-----------------------------------------------------------------------------
# Initialize the experiment
#-----------------------------------------------------------------------------
print("\nInitializing experiment\n")

# Create an object that describes the configuration of the host PC
network_config = wlan_exp_config.WlanExpNetworkConfiguration(network=NETWORK)

# Create an object that describes the WARP v3 nodes that will be used in this experiment
nodes_config   = wlan_exp_config.WlanExpNodesConfiguration(network_config=network_config,
                                                           serial_numbers=NODE_SERIAL_LIST)

# Initialize the Nodes
#  This command will fail if either WARP v3 node does not respond
nodes = wlan_exp_util.init_nodes(nodes_config, network_config)

# Set the time of all the nodes to zero
wlan_exp_util.broadcast_cmd_set_time(0.0, network_config)

# Extract the AP and STA nodes from the list of initialized nodes
n_ap_l  = wlan_exp_util.filter_nodes(nodes, 'node_type', 'AP')
n_sta_l = wlan_exp_util.filter_nodes(nodes, 'node_type', 'STA')

# Check that we have a valid AP and STA
if (((len(n_ap_l) == 1) and (len(n_sta_l) == 1))):
    # Extract the two nodes from the lists for easier referencing below
    n_ap = n_ap_l[0]
    n_sta = n_sta_l[0]
else:
    print("ERROR: Node configurations did not match requirements of script.\n")
    print("    Ensure two nodes are ready, one using the AP design, one using the STA design\n")
    sys.exit(0)


#-----------------------------------------------------------------------------
# Configure the Network
#-----------------------------------------------------------------------------
print("\nConfiguring Network\n")

# Set the AP address filter 
n_ap.set_association_address_filter(ADDR_FILTER_LIST)

# Configure the network of nodes
for node in nodes:
    # Set all nodes to be on the same channel
    node.set_channel(CHANNEL)
    
    # Remove any current association information
    node.disassociate_all()

# Set the network SSID
ssid = n_ap.set_ssid(NETWORK_NAME)
print("Network Name: '{0}'\n".format(ssid))

# Add the association(s) we want
n_ap.add_association(n_sta)

# Check that the nodes are associated.  Otherwise, the LTGs below will fail.
if not n_ap.is_associated(n_sta):
    print("\nERROR: Nodes are not associated.")
    print("    Ensure that the AP and the STA are associated.")
    sys.exit(0)


#-----------------------------------------------------------------------------
# Explore with the network
#-----------------------------------------------------------------------------

# Create Debug prompt
wlan_exp_util.debug_here()




