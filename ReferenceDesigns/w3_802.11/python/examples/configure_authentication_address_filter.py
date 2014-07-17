"""
------------------------------------------------------------------------------
WARPNet Example
------------------------------------------------------------------------------
License:   Copyright 2014, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
This script uses the 802.11 ref design and WARPnet to create a closed network
between the AP and the specified devices.  Than allows the user to explore this
network.  

Hardware Setup:
  - Requires one WARP v3 node configured as AP using 802.11 Reference Design v0.95 or later
  - PC NIC and ETH B on WARP v3 nodes connected to common Ethernet switch

Required Script Changes:
  - Set NETWORK to the IP address of your host PC NIC network (eg X.Y.Z.0 for IP X.Y.Z.W)
  - Set NODE_SERIAL_LIST to the serial numbers of your WARP nodes
  - Set CHANNEL to the channel you want the network to be on
  - Set NETWORK_NAME to the string for the SSID of the network
  - Set WLAN_DEVICE_LIST to the list of devices you want on this network

Description:
  This script initializes one WARP v3 AP node.  It will then configure the network.  
First, it will set the address filter on the AP such that only the device in 
WLAN_DEVICE_LIST can join wirelessly.  Then it will remove any associations from the 
AP and set it to the specified channel.  It will then configure the AP with a new SSID.  
Finally, it will wait until all the devices have associated.
------------------------------------------------------------------------------
"""
import sys

import wlan_exp.config as wlan_exp_config
import wlan_exp.util as wlan_exp_util

import wlan_exp.device as wlan_device


#-----------------------------------------------------------------------------
# Top Level Script Variables
#-----------------------------------------------------------------------------
# NOTE: change these values to match your experiment setup
NETWORK           = '10.0.0.0'
NODE_SERIAL_LIST  = ['W3-a-00001']
CHANNEL           = 1
NETWORK_NAME      = "WARP Device Example"

# WLAN devices
#   Contains a list of tuples: (MAC Address, 'String description of device')
#
WLAN_DEVICE_LIST  = [(0x000000000000, 'My Device')]


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

# Setup WLAN devices
devices = []

for device in WLAN_DEVICE_LIST:
    devices.append(wlan_device.WlanDevice(mac_address=device[0], name=device[1]))

# Check that we have a valid AP and at least one valid WLAN device
if ((len(n_ap_l) == 1) and devices):
    # Extract the two nodes from the lists for easier referencing below
    n_ap = n_ap_l[0]
else:
    print("ERROR: Node configurations did not match requirements of script.\n")
    print("    Ensure the AP is ready and there is one valid WLAN device\n")
    sys.exit(0)


#-----------------------------------------------------------------------------
# Configure the Network
#-----------------------------------------------------------------------------
print("\nConfiguring Network\n")

# Set the AP address filter to not allow anyone to join while we configure the network
n_ap.set_authentication_address_filter((0x000000000000, 0x000000000000))

# Configure the network of nodes
for node in nodes:
    # Set all nodes to be on the same channel
    node.set_channel(CHANNEL)
    
    # Remove any current association information
    node.disassociate_all()

# Set the network SSID
ssid = n_ap.set_ssid(NETWORK_NAME)
print("Network Name: '{0}'\n".format(ssid))

# Set the AP address filter for each device
for device in WLAN_DEVICE_LIST:
    n_ap.set_authentication_address_filter((device[0], device[0]))

# Wait for devices to associate with the AP
print("Waiting for devices to join:")

total_devices = len(devices)
tmp_devices   = list(devices)
num_joined    = 0

while (total_devices != num_joined):
    
    for device in tmp_devices:
        if n_ap.is_associated(device):
            print("    {0} joined".format(device.name))
            num_joined += 1
            tmp_devices.remove(device)

print("")


#-----------------------------------------------------------------------------
# Explore with the network
#-----------------------------------------------------------------------------

# Create Debug prompt
wlan_exp_util.debug_here()




