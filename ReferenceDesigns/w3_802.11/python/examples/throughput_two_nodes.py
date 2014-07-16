"""
------------------------------------------------------------------------------
WARPNet Example
------------------------------------------------------------------------------
License:   Copyright 2014, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
This script uses the 802.11 ref design and WARPnet to measure throughput between
an AP and an associated STA using the AP's local traffic generator (LTG).

Hardware Setup:
  - Requires two WARP v3 nodes
    - One node configured as AP using 802.11 Reference Design v0.8 or later
    - One node configured as STA using 802.11 Reference Design v0.8 or later
  - PC NIC and ETH B on WARP v3 nodes connected to common Ethernet switch

Required Script Changes:
  - Set NETWORK to the IP address of your host PC NIC network (eg X.Y.Z.0 for IP X.Y.Z.W)
  - Set NODE_SERIAL_LIST to the serial numbers of your WARP nodes

Description:
  This script initializes two WARP v3 nodes, one AP and one STA. It assumes the STA is 
already associated with the AP. The script then initiates a traffic flow from the AP to 
the STA, sets the AP Tx rate and measures throughput by counting the number of bytes 
received successfully at the STA. This process repeats for STA -> AP and head-to-head
traffic flows.
------------------------------------------------------------------------------
"""
import sys
import time
import wlan_exp.config as wlan_exp_config
import wlan_exp.util as wlan_exp_util
import wlan_exp.ltg as wlan_exp_ltg


#-------------------------------------------------------------------------
#  Global experiment variables
#

# NOTE: change these values to match your experiment setup
NETWORK           = '10.0.0.0'
NODE_SERIAL_LIST  = ['W3-a-00001', 'W3-a-00002']

# Set the per-trial duration (in seconds)
TRIAL_TIME        = 10


#-------------------------------------------------------------------------
#  Initialization
#
print("\nInitializing experiment\n")

# Create an object that describes the network configuration of the host PC
network_config = wlan_exp_config.WlanExpNetworkConfiguration(network=NETWORK)

# Create an object that describes the WARP v3 nodes that will be used in this experiment
nodes_config   = wlan_exp_config.WlanExpNodesConfiguration(network_config=network_config,
                                                           serial_numbers=NODE_SERIAL_LIST)

# Initialize the Nodes
#   This command will fail if either WARP v3 node does not respond
nodes = wlan_exp_util.init_nodes(nodes_config, network_config)

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
    print(" Ensure two nodes are ready, one using the AP design, one using the STA design\n")
    sys.exit(0)



#-------------------------------------------------------------------------
#  Setup
#
print("\nExperimental Setup:")

# Set the rate of both nodes to 18 Mbps
rate = wlan_exp_util.wlan_rates[3]

# Put each node in a known, good state
for node in nodes:
    node.set_tx_rate_unicast(rate, curr_assoc=True, new_assoc=True)
    node.reset_all()

    # Get some additional information about the experiment
    channel  = node.get_channel()

    print("\n{0}:".format(node.name))
    print("    Channel  = {0}".format(wlan_exp_util.channel_to_str(channel)))
    print("    Rate     = {0}".format(wlan_exp_util.tx_rate_to_str(rate)))

print("")

# Check that the nodes are associated.  Otherwise, the LTGs below will fail.
if not n_ap.is_associated(n_sta):
    print("\nERROR: Nodes are not associated.")
    print("    Ensure that the AP and the STA are associated.")
    sys.exit(0)



#-------------------------------------------------------------------------
#  Run Experiments
#
print("\nRun Experiment:")

# Experiments:
#   1) AP -> STA throughput
#   2) STA -> AP throughput
#   3) Head-to-head throughput
#
#   Since this experiment is basically the same for each iteration, we have pulled out 
# the main control variables so that we do not have repeated code for easier readability.
#
experiment_params = [{'ap_ltg_en' : True,  'sta_ltg_en' : False, 'desc' : 'AP -> STA'},
                     {'ap_ltg_en' : False, 'sta_ltg_en' : True,  'desc' : 'STA -> AP'},
                     {'ap_ltg_en' : True,  'sta_ltg_en' : True,  'desc' : 'Head-to-Head'}]


#-------------------------------------------------------------------------
#  Experiment:  Compute throughput from node statistics
#
for experiment in experiment_params:
    
    print("\nTesting {0} throughput for rate {1} ...".format(experiment['desc'], wlan_exp_util.tx_rate_to_str(rate)))

    # Start a flow from the AP's local traffic generator (LTG) to the STA
    #    Set the flow to 1400 byte payloads, fully backlogged (0 usec between new pkts), run forever
    #    Start the flow immediately
    if (experiment['ap_ltg_en']):
        ap_ltg_id  = n_ap.ltg_configure(wlan_exp_ltg.FlowConfigCBR(n_sta.wlan_mac_address, 1400, 0, 0), auto_start=True)

    # Start a flow from the STA's local traffic generator (LTG) to the AP
    #    Set the flow to 1400 byte payloads, fully backlogged (0 usec between new pkts), run forever
    #    Start the flow immediately
    if (experiment['sta_ltg_en']):
        sta_ltg_id  = n_sta.ltg_configure(wlan_exp_ltg.FlowConfigCBR(n_ap.wlan_mac_address, 1400, 0, 0), auto_start=True)

    # Record the initial Tx/Rx stats
    sta_rx_stats_start = n_ap.stats_get_txrx(n_sta)
    ap_rx_stats_start  = n_sta.stats_get_txrx(n_ap)
    
    # Wait for the TRIAL_TIME
    time.sleep(TRIAL_TIME)
    
    # Record the ending Tx/Rx stats
    sta_rx_stats_end = n_ap.stats_get_txrx(n_sta)
    ap_rx_stats_end  = n_sta.stats_get_txrx(n_ap)

    # Stop the AP LTG flow and purge any remaining transmissions in the queue so that nodes are in a known, good state
    if (experiment['ap_ltg_en']):
        n_ap.ltg_stop(ap_ltg_id)
        n_ap.ltg_remove(ap_ltg_id)
        n_ap.queue_tx_data_purge_all()

    # Stop the STA LTG flow and purge any remaining transmissions in the queue so that nodes are in a known, good state
    if (experiment['sta_ltg_en']):
        n_sta.ltg_stop(sta_ltg_id)
        n_sta.ltg_remove(sta_ltg_id)
        n_sta.queue_tx_data_purge_all()

    # Compute the throughput
    # NOTE:  Timestamps are in microseconds; bits/usec == Mbits/sec
    # NOTE:  In Python 3.x, the division operator is always floating point.  In order to be compatible with all versions 
    #    of python, cast operands to floats to ensure floating point division
    #
    sta_num_bits  = float((sta_rx_stats_end['data_num_rx_bytes'] - sta_rx_stats_start['data_num_rx_bytes']) * 8)
    sta_time_span = float(sta_rx_stats_end['timestamp'] - sta_rx_stats_start['timestamp'])
    sta_xput      = sta_num_bits / sta_time_span
    print("    STA Rate = {0:>4.1f} Mbps   Throughput = {1:>5.2f} Mbps".format(rate['rate'], sta_xput))
    
    ap_num_bits  = float((ap_rx_stats_end['data_num_rx_bytes'] - ap_rx_stats_start['data_num_rx_bytes']) * 8)
    ap_time_span = float(ap_rx_stats_end['timestamp'] - ap_rx_stats_start['timestamp'])
    ap_xput      = ap_num_bits / ap_time_span
    print("    AP  Rate = {0:>4.1f} Mbps   Throughput = {1:>5.2f} Mbps".format(rate['rate'], ap_xput))

