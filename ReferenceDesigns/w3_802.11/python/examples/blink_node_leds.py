"""
This script blinks the red LEDs of all nodes to ensure broadcast communication

Hardware Setup:
 - Requires one or more WARP v3 nodes configured with the 802.11 Reference
   Design v0.81 or later.
 - PC NIC and ETH B on WARP v3 nodes connected to common gigbit Ethernet switch

Required Script Changes:
  - Set HOST_INTERFACES to the IP address of your host PC NIC

Description:
  This script will cause all nodes on each of the host interfaces to 
blink their LEDs.
"""
import time
import wlan_exp.warpnet.util as wn_util

# NOTE: change these values to match your experiment setup
HOST_INTERFACES   = ['10.0.0.250']

# Issue identify all command
#   Wait 1 second before issuing the command to make sure all nodes are ready
time.sleep(1)
wn_util.wn_identify_all_nodes(HOST_INTERFACES)
