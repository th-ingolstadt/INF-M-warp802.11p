"""
This script uses the 802.11 ref design and WARPnet to create a log
file that contains all data assocated with an experiment of head-to-head 
backlogged data transfer using the local traffic generators.

Hardware Setup:
 - Requires two WARP v3 nodes
  - One node configured as AP using 802.11 Reference Design v0.82 or later
  - One node configured as STA using 802.11 Reference Design v0.82 or later
 - PC NIC and ETH B on WARP v3 nodes connected to common Ethernet switch

Required Script Changes:
  - Set HOST_IP_ADDRESS to the IP address of your host PC NIC
  - Set NODE_SERIAL_LIST to the serial numbers of your WARP nodes

Description:
  This script initializes two WARP v3 nodes, one AP and one STA. It assumes 
  the STA is already associated with the AP.  The script then initializes
  all of the experiment parameters and starts a traffic flow from the AP
  to the STA and the STA to the AP.  Finally, it resets the log, allows
  the experiment to run and then captures all the log data after the
  TRIAL_TIME.
"""
import sys
import time
import warpnet.wn_config                 as wn_config
import warpnet.wlan_exp.util             as wlan_exp_util
import warpnet.wlan_exp.ltg              as ltg
import warpnet.wlan_exp_log.log_util_hdf as hdf_util


#-----------------------------------------------------------------------------
# Top Level Script Variables
#-----------------------------------------------------------------------------
# NOTE: change these values to match your experiment setup
HOST_INTERFACES   = ['10.0.0.250']
NODE_SERIAL_LIST  = ['W3-a-00006', 'W3-a-00183']

AP_HDF5_FILENAME  = "example_logs/ap_log_stats.hdf5"
STA_HDF5_FILENAME = "example_logs/sta_log_stats.hdf5"

# Set the per-trial duration (in seconds)
TRIAL_TIME        = 10



#-----------------------------------------------------------------------------
# Local Helper Utilities
#-----------------------------------------------------------------------------
def write_log_file(file_name, data_buffer):
    """Writes log data to a HDF5 file."""
    try:
        # Get the byte log_data out of the WARPNet buffer
        data = data_buffer.get_bytes()
        
        # Write the byte Log_data to the file 
        hdf_util.log_data_to_hdf5(data, file_name)
    except AttributeError as err:
        print("Error writing log file: {0}".format(err))


def print_log_size():
    """Prints the size of the AP / STA Logs."""
    global n_ap, n_sta

    ap_log_size  = n_ap.log_get_size()
    sta_log_size = n_sta.log_get_size()
    
    print("Log Sizes:  AP  = {0:10d} bytes".format(ap_log_size))
    print("            STA = {0:10d} bytes".format(sta_log_size))



#-----------------------------------------------------------------------------
# Experiment Script
#-----------------------------------------------------------------------------
print("\nInitializing experiment\n")

# Create an object that describes the configuration of the host PC
host_config = wn_config.HostConfiguration(host_interfaces=HOST_INTERFACES)

# Create an object that describes the WARP v3 nodes that will be used in this experiment
nodes_config = wn_config.NodesConfiguration(host_config=host_config,
                                            serial_numbers=NODE_SERIAL_LIST)

# Initialize the Nodes
#  This command will fail if either WARP v3 node does not respond
nodes = wlan_exp_util.init_nodes(nodes_config, host_config)

# Set the time of the nodes
wlan_exp_util.broadcast_node_set_time(host_config)

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

# Check that the nodes are associated.  Otherwise, the LTGs below will fail.
if not n_ap.node_is_associated(n_sta):
    print("\nERROR: Nodes are not associated.")
    print("    Ensure that the AP and the STA are associated.")
    sys.exit(0)



print("\nExperimental Setup:")

# Get the rates that we will move through during the experiment
wlan_rates = wlan_exp_util.wlan_rates

# Put each node in a known, good state
for node in nodes:
    node.node_set_tx_rate_unicast(wlan_rates[0])
    node.node_reset_all()



print("\nRun Experiment:\n")

# Look at the initial log sizes for reference
print_log_size()

print("\nStart LTG - AP -> STA:")
# Start a flow from the AP's local traffic generator (LTG) to the STA
#  Set the flow to 1400 byte payloads, fully backlogged (0 usec between new pkts)
n_ap.ltg_to_node_configure(n_sta, ltg.FlowConfigCBR(1400, 0))
n_ap.ltg_to_node_start(n_sta)

print("\nStart LTG - STA -> AP:")
# Start a flow from the STA's local traffic generator (LTG) to the AP
#  Set the flow to 1400 byte payloads, fully backlogged (0 usec between new pkts)
n_sta.ltg_to_node_configure(n_ap, ltg.FlowConfigCBR(1400, 0))
n_sta.ltg_to_node_start(n_ap)

print("\nStarting trials ...")

# Create some interesting traffic for the log file
for idx, rate in enumerate(wlan_rates):
    print("  Rate {0} ... ".format(wlan_exp_util.tx_rate_to_str(rate)))

    # Configure the AP's Tx rate for the selected STA
    n_ap.node_set_tx_rate_unicast(rate, n_sta)

    # Configure the STA's Tx rate for the selected AP
    n_sta.node_set_tx_rate_unicast(rate, n_ap)

    # Let the LTG flows run at the new rate
    time.sleep(TRIAL_TIME)


# Stop the LTG flow and purge the transmit queue so that nodes are in a known, good state
n_ap.ltg_to_node_stop(n_sta)
n_sta.ltg_to_node_stop(n_ap)

n_ap.queue_tx_data_purge_all()
n_sta.queue_tx_data_purge_all()

# Look at the final log sizes for reference
print_log_size()

# Write Log Files for processing by other scripts
print("\nWriting Log Files...")

write_log_file(AP_HDF5_FILENAME, n_ap.log_get_all_new())
write_log_file(STA_HDF5_FILENAME, n_sta.log_get_all_new())

print("Done.")







