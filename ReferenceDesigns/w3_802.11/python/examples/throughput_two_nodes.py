"""
This script uses the 802.11 ref design and WARPnet to measure throughput between
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
  This script initializes two WARP v3 nodes, one AP and one STA. It assumes the STA is
  already associated with the AP. The script then initiates a traffic flow from the AP to STA,
  sets the AP Tx rate and measures throughput by counting the number of bytes received successfully
  at the STA. This process repeats for multiple Tx rates.
"""
import sys
import time
import wlan_exp.config        as config
import wlan_exp.util          as wlan_exp_util
import wlan_exp.ltg           as ltg

# NOTE: change these values to match your experiment setup
HOST_INTERFACES   = ['10.0.0.250']
NODE_SERIAL_LIST  = ['W3-a-00006', 'W3-a-00183']


# Set the per-trial duration (in seconds)
TRIAL_TIME = 10

print("\nInitializing experiment\n")

# Create an object that describes the configuration of the host PC
host_config  = config.WlanExpHostConfiguration(host_interfaces=HOST_INTERFACES)

# Create an object that describes the WARP v3 nodes that will be used in this experiment
nodes_config = config.WlanExpNodesConfiguration(host_config=host_config,
                                                serial_numbers=NODE_SERIAL_LIST)

# Initialize the Nodes
#  This command will fail if either WARP v3 node does not respond
nodes = wlan_exp_util.init_nodes(nodes_config, host_config)

# Set the time of all the nodes to zero
wlan_exp_util.broadcast_cmd_set_time(0.0, host_config)

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

# Initialize each node and gather some print some experiment information
print("\nExperimental Setup:")

# Set the rate of both nodes to 18 Mbps
rate = wlan_exp_util.wlan_rates[3]

# Put each node in a known, good state
for node in nodes:
    node.set_tx_rate_unicast(rate, curr_assoc=True, new_assoc=True)
    node.reset_all()

    # Get some additional information about the experiment
    channel  = node.get_channel()
    tx_power = node.get_tx_power()

    print("\n{0}:".format(node.name))
    print("    Channel  = {0}".format(channel))
    print("    Tx Power = {0} dBm".format(tx_power[0]))  # Unicast Data power
    print("    Rate     = {0}".format(wlan_exp_util.tx_rate_to_str(rate)))

print("")

# Add the current time to all the nodes
wlan_exp_util.broadcast_cmd_write_time_to_logs(host_config)

# Check that the nodes are associated.  Otherwise, the LTGs below will fail.
if not n_ap.is_associated(n_sta):
    print("\nERROR: Nodes are not associated.")
    print("    Ensure that the AP and the STA are associated.")
    sys.exit(0)


print("\nRun Experiment:")


#-------------------------------------------------------------------------
#  Experiment 1:  AP -> STA throughput
#

print("\nTesting AP -> STA throughput for rate {0} ...".format(wlan_exp_util.tx_rate_to_str(rate)))

# Start a flow from the AP's local traffic generator (LTG) to the STA
#  Set the flow to 1400 byte payloads, fully backlogged (0 usec between new pkts), run forever
#  Start the flow immediately
ap_ltg_id  = n_ap.ltg_configure(ltg.FlowConfigCBR(n_sta.wlan_mac_address, 1400, 0, 0), auto_start=True)

# Record the station's initial Tx/Rx stats for the AP
ap_rx_stats_start = n_sta.stats_get_txrx(n_ap)

# Wait for the TRIAL_TIME
time.sleep(TRIAL_TIME)

# Record the station's ending Tx/Rx stats for the AP
ap_rx_stats_end = n_sta.stats_get_txrx(n_ap)

# Stop the LTG flow and purge any remaining transmissions in the queue 
#  so that nodes are in a known, good state
n_ap.ltg_stop(ap_ltg_id)
n_ap.queue_tx_data_purge_all()

# Timestamps are in microseconds; bits/usec == Mbits/sec
# NOTE: In Python 3.x, the division operator is always floating point.
#    In order to be compatible with all versions of python, cast
#    operands to floats to ensure floating point division
ap_num_bytes = float((ap_rx_stats_end['data_num_rx_bytes'] - ap_rx_stats_start['data_num_rx_bytes']) * 8)
ap_time_span = float(ap_rx_stats_end['timestamp'] - ap_rx_stats_start['timestamp'])
ap_xput      = ap_num_bytes / ap_time_span
print("    Rate = {0:>4.1f} Mbps   Throughput = {1:>5.2f} Mbps".format(rate['rate'], ap_xput))


#-------------------------------------------------------------------------
#  Experiment 2:  STA -> AP throughput
#

print("\nTesting STA -> AP throughput for rate {0} ...".format(wlan_exp_util.tx_rate_to_str(rate)))

# Start a flow from the AP's local traffic generator (LTG) to the STA
#  Set the flow to 1400 byte payloads, fully backlogged (0 usec between new pkts), run forever
#  Start the flow immediately
sta_ltg_id  = n_sta.ltg_configure(ltg.FlowConfigCBR(n_ap.wlan_mac_address, 1400, 0, 0), auto_start=True)

# Record the AP's initial Tx/Rx stats for the station
sta_rx_stats_start = n_ap.stats_get_txrx(n_sta)

# Wait for the TRIAL_TIME
time.sleep(TRIAL_TIME)

# Record the AP's ending Tx/Rx stats for the station
sta_rx_stats_end = n_ap.stats_get_txrx(n_sta)

# Stop the LTG flow and purge any remaining transmissions in the queue 
#  so that nodes are in a known, good state
n_sta.ltg_stop(sta_ltg_id)
n_sta.queue_tx_data_purge_all()

# Timestamps are in microseconds; bits/usec == Mbits/sec
# NOTE: In Python 3.x, the division operator is always floating point.
#    In order to be compatible with all versions of python, cast
#    operands to floats to ensure floating point division
sta_num_bytes = float((sta_rx_stats_end['data_num_rx_bytes'] - sta_rx_stats_start['data_num_rx_bytes']) * 8)
sta_time_span = float(sta_rx_stats_end['timestamp'] - sta_rx_stats_start['timestamp'])
sta_xput      = sta_num_bytes / sta_time_span
print("    Rate = {0:>4.1f} Mbps   Throughput = {1:>5.2f} Mbps".format(rate['rate'], sta_xput))


#-------------------------------------------------------------------------
#  Experiment 3:  Head-to-head throughput
#

print("\nTesting Head-to-Head throughput for rate {0} ...".format(wlan_exp_util.tx_rate_to_str(rate)))


# Start both local traffic generator (LTG) flows
#  Set the flow to 1400 byte payloads, fully backlogged (0 usec between new pkts), run forever
#  Start the flow immediately
sta_ltg_id = n_sta.ltg_configure(ltg.FlowConfigCBR(n_ap.wlan_mac_address, 1400, 0, 0), auto_start=True)
ap_ltg_id  = n_ap.ltg_configure(ltg.FlowConfigCBR(n_sta.wlan_mac_address, 1400, 0, 0), auto_start=True)

# Record the initial Tx/Rx stats
sta_rx_stats_start = n_ap.stats_get_txrx(n_sta)
ap_rx_stats_start  = n_sta.stats_get_txrx(n_ap)

# Wait for the TRIAL_TIME
time.sleep(TRIAL_TIME)

# Record the ending Tx/Rx stats
sta_rx_stats_end = n_ap.stats_get_txrx(n_sta)
ap_rx_stats_end  = n_sta.stats_get_txrx(n_ap)

# Stop the LTG flows and purge any remaining transmissions in the queue 
#  so that nodes are in a known, good state
n_sta.ltg_stop(sta_ltg_id)
n_sta.queue_tx_data_purge_all()
n_ap.ltg_stop(ap_ltg_id)
n_ap.queue_tx_data_purge_all()


# Timestamps are in microseconds; bits/usec == Mbits/sec
# NOTE: In Python 3.x, the division operator is always floating point.
#    In order to be compatible with all versions of python, cast
#    operands to floats to ensure floating point division
sta_num_bytes = float((sta_rx_stats_end['data_num_rx_bytes'] - sta_rx_stats_start['data_num_rx_bytes']) * 8)
sta_time_span = float(sta_rx_stats_end['timestamp'] - sta_rx_stats_start['timestamp'])
sta_xput      = sta_num_bytes / sta_time_span
print("    STA Rate = {0:>4.1f} Mbps   Throughput = {1:>5.2f} Mbps".format(rate['rate'], sta_xput))
ap_num_bytes = float((ap_rx_stats_end['data_num_rx_bytes'] - ap_rx_stats_start['data_num_rx_bytes']) * 8)
ap_time_span = float(ap_rx_stats_end['timestamp'] - ap_rx_stats_start['timestamp'])
ap_xput      = ap_num_bytes / ap_time_span
print("    AP  Rate = {0:>4.1f} Mbps   Throughput = {1:>5.2f} Mbps".format(rate['rate'], ap_xput))


