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
import wlan_exp.config       as config

# Import WLAN Exp Framework
import wlan_exp.util         as wlan_exp_util



# TOP Level script variables
HOST_INTERFACES   = ['10.0.0.250']
NODE_SERIAL_LIST  = ['W3-a-00001']


nodes = []

def initialize_experiment():
    """Initialize the WLAN Exp experiment."""
    global nodes

    # Print initial message
    print("\nInitializing experiment\n")

    # Create a WnConfiguration
    #   This describes the Host configuration.
    host_config  = config.WlanExpHostConfiguration(host_interfaces=HOST_INTERFACES)
    
    # Create a WnNodesConfiguration
    #   This describes each node to be initialized
    nodes_config = config.WlanExpNodesConfiguration(host_config=host_config,
                                                    serial_numbers=NODE_SERIAL_LIST)

    # Initialize the Nodes
    #   This will initialize all of the networking and gather the necessary
    #   information to control and communicate with the nodes
    nodes = wlan_exp_util.init_nodes(nodes_config, host_config)

    # Initialize the time on all nodes to zero
    wlan_exp_util.broadcast_cmd_set_time(0.0, host_config)



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
            station_info = ap.get_station_info()
            stats        = ap.stats_get_txrx()
            print_stats(stats, station_info)
       
        # Wait for 5 seconds
        time.sleep(5)


def print_stats(stats, station_info=None):
    """Helper method to print the statistics."""
    print("-------------------- ----------------------------------- ----------------------------------- ")
    print("                               Number of Packets                   Number of Bytes           ")
    print("ID                   Tx Data  Tx Mgmt  Rx Data  Rx Mgmt  Tx Data  Tx Mgmt  Rx Data  Rx Mgmt  ")
    print("-------------------- -------- -------- -------- -------- -------- -------- -------- -------- ")

    msg = ""
    for stat in stats:
        stat_id = stat['mac_addr']

        hostname = False
        if not station_info is None:
            for station in station_info:
                if (stat['mac_addr'] == station['mac_addr']):
                    stat_id  = station['host_name']
                    stat_id  = stat_id.strip('\x00')
                    if (stat_id == ''):
                        stat_id  = stat['mac_addr']
                        hostname = False
                    else:
                        hostname = True
        
        if not hostname:
            stat_id = ''.join('{0:02X}:'.format(ord(x)) for x in stat_id)[:-1]

        msg += "{0:<20} ".format(stat_id)
        msg += "{0:8d} ".format(stat['data_num_tx_packets_success'])
        msg += "{0:8d} ".format(stat['mgmt_num_tx_packets_success'])        
        msg += "{0:8d} ".format(stat['data_num_rx_packets'])
        msg += "{0:8d} ".format(stat['mgmt_num_rx_packets'])
        msg += "{0:8d} ".format(stat['data_num_tx_bytes_success'])
        msg += "{0:8d} ".format(stat['mgmt_num_tx_bytes_success'])
        msg += "{0:8d} ".format(stat['data_num_rx_bytes'])
        msg += "{0:8d} ".format(stat['mgmt_num_rx_bytes'])
        msg += "\n"
    print(msg)


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



