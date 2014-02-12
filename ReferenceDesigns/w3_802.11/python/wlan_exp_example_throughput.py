#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Example: Simple throughput test with two nodes
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
1.00a ejw  2/11/14  Initial release

------------------------------------------------------------------------------
This script provides a simple WARPNet example that measures throughput between
an AP and an associated STA using the AP's local traffic generator (LTG).

Hardware Setup:
 - Requires two WARP v3 nodes
  - One node configured as AP using 802.11 Reference Design v0.8 or later
  - One node configured as STA using 802.11 Reference Design v0.8 or later
 - PC NIC and ETH B on WARP v3 nodes connected to common Ethernet switch

Required Script Changes:
  - Set HOST_IP_ADDRESS to the IP address of your host PC NIC
  - Set NODE_SERIAL_LIST to the serial numbers of your WARP nodes

Description:
  This script will initialize two WARP v3 nodes, one AP and one STA. The script assumes
the STA is associated with the AP. The script initiates a traffic flow from the AP to STA.
The script then iterates over a few wireless Tx rates, measuring the AP -> STA throughput
for each Tx rate.

"""
import time
import warpnet.wn_config as wn_config
import wlan_exp.wlan_exp_util as wlan_exp_util

# NOTE: change these values to match your experiment setup
HOST_INTERFACES   = ['10.0.0.250']
NODE_SERIAL_LIST  = ['W3-a-00001', 'W3-a-00002']

rates = wlan_exp_util.rates[0:4]
TRIAL_TIME = 10

print("\nInitializing experiment\n")

# Create an object that describes the configuration of the host PC
host_config = wn_config.WnHostConfiguration(interfaces=HOST_INTERFACES)

# Create an object that describes the WARP v3 nodes that will be used in this experiment
nodes_config = wn_config.WnNodesConfiguration(serial_numbers=NODE_SERIAL_LIST)

# Initialize the Nodes
#  This command will fail if either WARP v3 node does not respond
nodes = wlan_exp_util.init_nodes(nodes_config, host_config)

# Extract the AP and STA nodes from the list of initialized nodes
n_ap  = wlan_exp_util.filer_nodes(nodes, 'node_type', 'AP')
n_sta = wlan_exp_util.filer_nodes(nodes, 'node_type', 'STA')

# Start a flow from the AP's local traffic generator (LTG) to the STA
#  Set the flow to fully backlogged with 1400 byte payloads
n_ap.config_ltg(n_sta, LTG_CBR(1400, 0))
n_ap.start_ltg(n_sta)

# Arrays to hold results
rx_bytes = []
rx_time_spans = []

for ii,rate in enumerate(rates):
  #Configure the AP's Tx rate for the selected station
  n_ap.set_tx_rate(n_sta, rate)
  
  #Record the station's initial Tx/Rx stats 
  rx_stats_start = n_sta.get_txrx_stats(n_ap)

  #Wait for a while
  time.sleep(TRIAL_TIME)

  #Record the station's ending Tx/Rx stats
  rx_stats_end = n_sta.get_txrx_stats(n_ap)

  #Compute the number of new bytes received and the time span
  rx_bytes[ii] = rx_stats_end['rx_bytes'] - rx_stats_start['rx_bytes']
  rx_time_spans[ii] = rx_stats_end['timestamp'] - rx_stats_start['timestamp']

#Calculate and display the throughput results
for ii in len(num_rx[ii]):
  #Timestamps are in microseconds; bits/usec == Mbits/sec
  xput = (rx_bytes[ii] * 8) / rx_time_spans[ii]
  print("Rate = %2.1f Mbps   Throughput = %2.1 Mbps", (rates[ii]['rate'], xput))
