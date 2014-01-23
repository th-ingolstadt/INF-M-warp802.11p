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
CHANNEL           = 4
IP_ADDRESS        = '10.0.0.250'
PORT              = 9999
HOST_ID           = 0xBEEF
EXPERIMENT_TIME   = 10


nodes = []

def run_experiment():
    """WLAN Experiment.""" 
    global nodes
    nodes_cfg_ini       = r"nodes_config.ini"

    # Print initial message
    print("\nRunning experiment\n")

    # Read the Node Configuration file and generate a WnNodesConfiguration
    nodes_config = wn_config.WnNodesConfiguration(nodes_cfg_ini)

    # Initialize the Nodes
    nodes = wlan_exp_util.wlan_exp_init_nodes(nodes_config)
    times = wlan_exp_util.wlan_exp_init_time(nodes);

    # For each node:
    #   Reset the Log
    #   Set the channel
    #   Stream log entries to IP_ADDRESS:PORT
    #   
    for node in nodes:
        node.reset_log()
        node.set_channel(CHANNEL)
        node.stream_log_entries(port=PORT, ip_address=IP_ADDRESS, host_id=HOST_ID)

    while(True):
        # Commands to run during the experiment
        #     Currently, this experiment will end when the user kills the 
        #     experiment with Ctrl-C
        time.sleep(100)


def end_experiment():
    """Experiment cleanup / post processing."""
    global nodes
    print("\nEnding experiment\n")
    for node in nodes:
        node.disable_log_entries_stream()


if __name__ == '__main__':
    # Run WLAN Setup
    # if (wlan_exp_util.wlan_exp_check_setup()):
    #     wlan_exp_util.wlan_exp_setup()

    try:
        # Run the experiment
        run_experiment()
    except KeyboardInterrupt:
        end_experiment()
        print("\nExperiment Finished.")



#------------------------------------------------------------------------------
# Other useful WLAN Exp Node commands:
#
# node.get_channel()
#
# node.write_statistics_to_log()
#
# buffer = node.get_log('temp01.bin')
#
# times = wlan_exp_util.wlan_exp_init_time(nodes, output=True)
# times = wlan_exp_util.wlan_exp_init_time(nodes, output=True, repeat=1000)


























