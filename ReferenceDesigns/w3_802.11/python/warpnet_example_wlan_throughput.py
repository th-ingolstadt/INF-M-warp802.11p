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
import warpnet.wn_config as wn_config
import warpnet.wlan_exp.util  as wlan_exp_util
import warpnet.wlan_exp.ltg   as ltg

# NOTE: change these values to match your experiment setup
HOST_INTERFACES   = ['10.0.0.250']
NODE_SERIAL_LIST  = ['W3-a-00001', 'W3-a-00002']

# Select some PHY rates to test
#  wlan_exp_util.rates is an array of dictionaries with keys:
#      'rate_index', 'rate', 'rate_str'
#  NOTE: rates must be a list for the below loops to work.  To select a
#    single rate please use the syntax:  wlan_exp_util.rates[0:1] to select
#    just entry [0] of the list.
rates = wlan_exp_util.rates[0:4]

# Set the per-trial duration (in seconds)
TRIAL_TIME = 10

print("\nInitializing experiment\n")

# Create an object that describes the configuration of the host PC
host_config = wn_config.HostConfiguration(host_interfaces=HOST_INTERFACES)

# Create an object that describes the WARP v3 nodes that will be used in this experiment
nodes_config = wn_config.NodesConfiguration(host_config=host_config,
                                            serial_numbers=NODE_SERIAL_LIST)

# Initialize the Nodes
#  This command will fail if either WARP v3 node does not respond
nodes = wlan_exp_util.init_nodes(nodes_config, host_config)

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
for node in nodes:
    # Put each node in a known, good state
    node.ltg_remove_all()
    node.log_reset()
    node.stats_reset_txrx()

    # Get some additional information about the experiment
    channel = node.node_get_channel()
    tx_gain = node.node_get_tx_gain()

    print("\n{0}:".format(node.name))
    print("    Channel = {0}".format(channel))
    print("    Tx Gain = {0}".format(tx_gain))


# Check that the nodes are associated.  Otherwise, the LTGs below will fail.
if not n_ap.node_is_associated(n_sta):
    print("\nERROR: Nodes are not associated.")
    print("    Ensure that the AP and the STA are associated.")
    sys.exit(0)


# Start a flow from the AP's local traffic generator (LTG) to the STA
#  Set the flow to 1400 byte payloads, fully backlogged (0 usec between new pkts)
n_ap.ltg_to_node_configure(n_sta, ltg.FlowConfigCBR(1400, 0))
n_ap.ltg_to_node_start(n_sta)

# Arrays to hold results
rx_bytes = []
rx_time_spans = []

print("\nRun Experiment:")

# Iterate over each selected Tx rate, running a new trial for each rate
for ii,rate in enumerate(rates):
    print("\nStarting {0} sec trial for rate {1} ...".format(TRIAL_TIME, wlan_exp_util.tx_rate_to_str(rate)))

    #Configure the AP's Tx rate for the selected station
    n_ap.node_set_tx_rate(n_sta, rate)

    #Record the station's initial Tx/Rx stats
    rx_stats_start = n_sta.stats_get_txrx(n_ap)

    #Wait for a while
    time.sleep(TRIAL_TIME)

    #Record the station's ending Tx/Rx stats
    rx_stats_end = n_sta.stats_get_txrx(n_ap)

    #Compute the number of new bytes received and the time span
    rx_bytes.insert(ii, rx_stats_end['data_num_rx_bytes'] - rx_stats_start['data_num_rx_bytes'])
    rx_time_spans.insert(ii, rx_stats_end['timestamp'] - rx_stats_start['timestamp'])
    print("Done.")

print("\n")

# Stop the LTG flow so that nodes are in a known, good state
n_ap.ltg_to_node_stop(n_sta)

#Calculate and display the throughput results
print("Results:")

for ii in range(len(rates)):
    #Timestamps are in microseconds; bits/usec == Mbits/sec
    #  NOTE: In Python 3.x, the division operator is always floating point.
    #    In order to be compatible with all versions of python, cast
    #    operands to floats to ensure floating point division
    num_bytes = float(rx_bytes[ii] * 8)
    time_span = float(rx_time_spans[ii])
    xput = num_bytes / time_span
    print("    Rate = {0:>4.1f} Mbps   Throughput = {1:>5.2f} Mbps".format(rates[ii]['rate'], xput))
