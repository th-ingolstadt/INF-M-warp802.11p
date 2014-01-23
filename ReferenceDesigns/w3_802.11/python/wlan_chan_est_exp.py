#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
WLAN Exp Demo 01 Script

@author: mango communications
"""

import time
import signal
import sys

# Import WARPNet Framework
import warpnet
from warpnet import wn_config

# Import WLAN Exp Framework
import wlan_exp
from wlan_exp import wlan_exp_util
from wlan_exp import wlan_exp_node_ap
from wlan_exp import wlan_exp_node_sta


# TOP Level script variables
CHANNEL           = 1
IP_ADDRESS        = '10.0.0.220'
PORT              = 9050
HOST_ID           = 0xBEEF

nodes = []

def run_experiment():
    global nodes
    nodes_cfg_ini       = r"nodes_config.ini"

    # Read the Node Configuration file and generate a WnNodesConfiguration
    nodes_config = wn_config.WnNodesConfiguration(nodes_cfg_ini)

    # Initialize the Nodes
    nodes = wlan_exp_util.wlan_exp_init_nodes(nodes_config)
    times = wlan_exp_util.wlan_exp_init_time(nodes);

    # For each node:
    #   Reset the Log
    #   Stream log entries to port 9999
    #   
    for node in nodes:
        node.reset_log()
        node.set_channel(CHANNEL)
        node.stream_log_entries(port=PORT, ip_address=IP_ADDRESS, host_id=HOST_ID)

    while(True):
        time.sleep(100)

def end_experiment(signal, frame):
    global nodes
    print("\nEnding experiment\n")
    for node in nodes:
        node.disable_log_entries_stream()
    sys.exit(0)

if __name__ == '__main__':
    signal.signal(signal.SIGINT, end_experiment)
    run_experiment()
























