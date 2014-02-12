#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Example
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

This module provides a simple WARPNet example.

Setup:
  - Set HOST_IP_ADDRESS to the IP address of your host PC NIC
  - Set NODE_SERIAL_LIST to the serial numbers of your WARP nodes
  - Download the 802.11 reference AP to the nodes

Run Command:
  python wlan_exp_example_01.py

Description:
  This script will initialize the given nodes; extract any APs from the 
initialized nodes; then for each AP, it will get the associations and 
statistics and display them.

"""
# Import Python modules
import time

# Import WARPNet Framework
import warpnet.wn_config as wn_config

# Import WLAN Exp Framework
import wlan_exp.wlan_exp_util as wlan_exp_util



# TOP Level script variables
HOST_INTERFACES   = ['10.0.0.250']
NODE_SERIAL_LIST  = ['W3-a-00006']


nodes = []

def initialize_experiment():
    """Initialize the WLAN Exp experiment."""
    global nodes

    # Print initial message
    print("\nInitializing experiment\n")

    # Create a WnConfiguration
    #   This describes the Host configuration.
    host_config = wn_config.HostConfiguration(host_interfaces=HOST_INTERFACES)
    
    # Create a WnNodesConfiguration
    #   This describes each node to be initialized
    nodes_config = wn_config.NodesConfiguration(host_config=host_config,
                                                serial_numbers=NODE_SERIAL_LIST)

    # Initialize the Nodes
    #   This will initialize all of the networking and gather the necessary
    #   information to control and communicate with the nodes
    nodes = wlan_exp_util.init_nodes(nodes_config, host_config)

    # Initialize the time on all nodes
    #   This will set all the nodes to a common time base.
    wlan_exp_util.init_timestamp(nodes);



def run_experiment():
    """WLAN Experiment.""" 
    global nodes

    # Print initial message
    print("\nRunning experiment\n")

    # Get all AP nodes from the list of initialize nodes    
    nodes_ap  = wlan_exp_util.filter_nodes(nodes, 'node_type', 'AP')

    # If the node is not configured correctly, exit
    if (len(nodes_ap) == 0):
        print("AP not initialized.  Exiting.")
        return


    while(True):

        # For each of the APs, get the statistics
        for ap in nodes_ap:
            associations = ap.get_association_table()
            stats = ap.get_txrx_stats()
            print_stats(associations, stats)
        
        # Wait for 100 seconds
        time.sleep(100)
        
        # Invoke interactive keyboard
        wlan_exp_util.keyboard()



def print_stats(associations, stats):
    """Helper method to print the statistics."""
    print("--------------- ---------- ---------- ")
    print("ID              Num Tx     Num Rx     ")
    for stat in stats:
        stat_id = stat['hw_addr']
        for association in associations:
            if (stat['hw_addr'] == association['hw_addr']):
                stat_id = association['hostname']
        print("{0:15s} {1:10d} {2:10d}".format(stat_id, 
                                               stat['num_tx_total'],
                                               stat['num_rx_success']))



def end_experiment():
    """Experiment cleanup / post processing."""
    global nodes
    print("\nEnding experiment\n")



if __name__ == '__main__':
    initialize_experiment();

    try:
        # Run the experiment
        run_experiment()
    except KeyboardInterrupt:
        pass
        
    end_experiment()
    print("\nExperiment Finished.")



