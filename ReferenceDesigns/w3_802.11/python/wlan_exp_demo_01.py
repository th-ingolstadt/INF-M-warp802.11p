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
import warpnet.wn_config as wn_config

# Import WLAN Exp Framework
import wlan_exp.wlan_exp_util as wlan_exp_util
import wlan_exp.wlan_exp_defaults as defaults
import wlan_exp.wlan_exp_ltg as ltg


# TOP Level script variables
HOST_INTERFACES   = ['10.0.0.250']

CHANNEL           = 4
TX_RATE           = wlan_exp_util.rates[0]
TX_GAIN           = 45
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

    # Create a WnConfiguration
    #   This describes the Host configuration.
    host_config = wn_config.HostConfiguration(host_interfaces=HOST_INTERFACES)
    
    # Create a WnNodesConfiguration
    #   This describes each node to be initialized
    nodes_config = wn_config.NodesConfiguration(host_config=host_config,
                                                ini_file=nodes_cfg_ini)

    # Initialize the Nodes
    #   This will initialize all of the networking and gather the necessary
    #   information to control and communicate with the nodes
    nodes = wlan_exp_util.init_nodes(nodes_config, host_config)

    # Initialize the time on all nodes
    #   This will set all the nodes to a common time base.
    wlan_exp_util.init_timestamp(nodes);


    # Get all AP nodes from the list of initialize nodes    
    nodes_ap  = wlan_exp_util.filter_nodes(nodes, 'node_type', 'AP')
    nodes_sta = wlan_exp_util.filter_nodes(nodes, 'node_type', 'STA')

    # For each node:
    #   Reset the Log
    #   Set the channel
    #   Stream log entries to IP_ADDRESS:PORT
    #   
    for node in nodes:
        node.reset_log()
        channel = node.set_channel(CHANNEL)
        tx_rate = node.set_tx_rate(TX_RATE)
        tx_gain = node.set_tx_gain(TX_GAIN)
        
        print("{0}:".format(node.name))
        print("    Channel = {0}".format(channel))
        print("    Tx Rate = {0}".format(wlan_exp_util.tx_rate_index_to_str(tx_rate)))
        print("    Tx Gain = {0}".format(tx_gain))
        
        # node.stream_log_entries(port=PORT, ip_address=IP_ADDRESS, host_id=HOST_ID)


    # Start and LTG from the AP to all stations
    nodes_ap[0].configure_ltg(nodes_sta, ltg.FlowConfigCBR(1500, 100))
    nodes_ap[0].start_ltg(nodes_sta)

    time.sleep(10)

    nodes_ap[0].stop_ltg(nodes_sta)
    nodes_ap[0].remove_ltg(nodes_sta)

    while(True):
        # Commands to run during the experiment
        #     Currently, this experiment will end when the user kills the 
        #     experiment with Ctrl-C
        time.sleep(10)
        for node in nodes:
            curr_temp = node.get_node_temp()
            print("Node {0} current temp is {1}".format(node.node_id, curr_temp))


def end_experiment():
    """Experiment cleanup / post processing."""
    global nodes
    print("\nEnding experiment\n")
    for node in nodes:
        try:
            node.disable_log_entries_stream()
        except:
            pass


if __name__ == '__main__':
    # Run WLAN Setup - TBD
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


























