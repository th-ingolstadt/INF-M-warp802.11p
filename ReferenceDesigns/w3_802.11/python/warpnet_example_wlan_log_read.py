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
  This script initializes two WARP v3 nodes, one AP and one STA. It assumes 
  the STA is already associated with the AP. The script then initiates a 
  traffic flow from the AP to STA, sets the AP Tx rate and measures throughput 
  by counting the number of bytes received successfully at the STA. This 
  process repeats for multiple Tx rates.
"""
import sys
import time
import warpnet.wn_config as wn_config
import warpnet.wlan_exp.util as wlan_exp_util
import warpnet.wlan_exp.ltg  as ltg

# NOTE: change these values to match your experiment setup
HOST_INTERFACES   = ['10.0.0.250']
NODE_SERIAL_LIST  = ['W3-a-00001', 'W3-a-00002']

AP_LOG_FILENAME   = 'example_logs/ap_log_stats.bin'
STA_LOG_FILENAME  = 'example_logs/sta_log_stats.bin'

# Set the per-trial duration (in seconds)
TRIAL_TIME = 30

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
    node.ltg_remove_all();
    node.log_reset();
    node.stats_reset_txrx();

print("\nRun Experiment:\n")


# Look at the initial log sizes for reference
ap_log_size  = n_ap.log_get_size()
sta_log_size = n_sta.log_get_size()

print("Log Sizes:  AP  = {0:10d} bytes".format(ap_log_size))
print("            STA = {0:10d} bytes".format(sta_log_size))


# Check that the nodes are associated.  Otherwise, the LTGs below will fail.
if not n_ap.node_is_associated(n_sta):
    print("\nERROR: Nodes are not associated.")
    print("    Ensure that the AP and the STA are associated.")
    sys.exit(0)


print("\nStart LTG - AP -> STA:")
# Start a flow from the AP's local traffic generator (LTG) to the STA
#  Set the flow to 1400 byte payloads, fully backlogged (0 usec between new pkts)
n_ap.ltg_to_node_configure(n_sta, ltg.FlowConfigCBR(1400, 0))
n_ap.ltg_to_node_start(n_sta)

# Wait for a while
time.sleep(TRIAL_TIME)
print("Done.\n")
	
# Stop the LTG flow and purge the transmit queue so that nodes are in a known, good state
n_ap.ltg_to_node_stop(n_sta)
n_ap.queue_tx_data_purge_all()


# Look at the log sizes after the first LTG
ap_log_size  = n_ap.log_get_size()
sta_log_size = n_sta.log_get_size()

print("Log Sizes:  AP  = {0:10d} bytes".format(ap_log_size))
print("            STA = {0:10d} bytes".format(sta_log_size))


print("\nStart LTG - STA -> AP:")
# Start a flow from the STA's local traffic generator (LTG) to the AP
#  Set the flow to 1400 byte payloads, fully backlogged (0 usec between new pkts)
n_sta.ltg_to_node_configure(n_ap, ltg.FlowConfigCBR(1400, 0))
n_sta.ltg_to_node_start(n_ap)

# Wait for a while
time.sleep(TRIAL_TIME)
print("Done.\n")
	
# Stop the LTG flow and purge the transmit queue so that nodes are in a known, good state
n_sta.ltg_to_node_stop(n_ap)
n_sta.queue_tx_data_purge_all()


# Look at the log sizes after the second LTG
ap_log_size  = n_ap.log_get_size()
sta_log_size = n_sta.log_get_size()

print("Log Sizes:  AP  = {0:10d} bytes".format(ap_log_size))
print("            STA = {0:10d} bytes".format(sta_log_size))


# Write Statistics to log
n_ap.stats_write_txrx_to_log()
n_sta.stats_write_txrx_to_log()

# Write Log Files for processing by other scripts
print("\nWriting Log Files...")
n_ap.log_get_all(AP_LOG_FILENAME)
n_sta.log_get_all(STA_LOG_FILENAME)
print("Done.")
