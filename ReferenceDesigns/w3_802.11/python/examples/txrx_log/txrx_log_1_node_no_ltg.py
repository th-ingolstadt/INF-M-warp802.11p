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

import wlan_exp.config as wlan_exp_config
import wlan_exp.util as wlan_exp_util
import wlan_exp.ltg as wlan_exp_ltg

import wlan_exp.log.util_hdf as hdf_util


#-----------------------------------------------------------------------------
# Top Level Script Variables
#-----------------------------------------------------------------------------
# NOTE: change these values to match your experiment setup
HOST_INTERFACES   = ['10.0.0.250']
NODE_SERIAL_LIST  = ['W3-a-00001']

AP_HDF5_FILENAME  = "log_files/ap_log_stats.hdf5"

# Set the per-trial duration (in seconds)
TRIAL_TIME        = 120



#-----------------------------------------------------------------------------
# Local Helper Utilities
#-----------------------------------------------------------------------------
def write_log_file(file_name, data_buffer):
    """Writes log data to a HDF5 file."""
    try:
        print("    {0}".format(file_name))
        
        # Get the byte log_data out of the WARPNet buffer
        data = data_buffer.get_bytes()
        
        # Write the byte Log_data to the file 
        hdf_util.log_data_to_hdf5(data, file_name)
    except AttributeError as err:
        print("Error writing log file: {0}".format(err))


def print_log_size():
    """Prints the size of the AP / STA Logs."""
    global n_ap

    ap_log_size  = n_ap.log_get_size()
    
    print("\nLog Sizes:  AP  = {0:10,d} bytes".format(ap_log_size))



#-----------------------------------------------------------------------------
# Experiment Script
#-----------------------------------------------------------------------------
print("\nInitializing experiment\n")

# Create an object that describes the configuration of the host PC
host_config  = wlan_exp_config.WlanExpHostConfiguration(host_interfaces=HOST_INTERFACES)

# Create an object that describes the WARP v3 nodes that will be used in this experiment
nodes_config = wlan_exp_config.WlanExpNodesConfiguration(host_config=host_config,
                                                         serial_numbers=NODE_SERIAL_LIST)

# Initialize the Nodes
#  This command will fail if either WARP v3 node does not respond
nodes = wlan_exp_util.init_nodes(nodes_config, host_config)

# Set the time of all the nodes to zero
wlan_exp_util.broadcast_cmd_set_time(0.0, host_config)

# Extract the AP and STA nodes from the list of initialized nodes
n_ap_l  = wlan_exp_util.filter_nodes(nodes, 'node_type', 'AP')


# Check that we have a valid AP and STA
if ((len(n_ap_l) == 1)):
    # Extract the two nodes from the lists for easier referencing below
    n_ap = n_ap_l[0]

else:
    print("ERROR: Node configurations did not match requirements of script.\n")
    print(" Ensure two nodes are ready, one using the AP design, one using the STA design\n")
    sys.exit(0)

print("\nExperimental Setup:")

# Get the rates that we will move through during the experiment
rate = wlan_exp_util.wlan_rates[3]

# Put each node in a known, good state
for node in nodes:
    node.set_tx_rate_unicast(rate, curr_assoc=True, new_assoc=True)
    node.log_configure(log_full_payloads=False)
    node.reset_all()

# Add the current time to all the nodes
wlan_exp_util.broadcast_cmd_write_time_to_logs(host_config)



print("\nRun Experiment:")

# Look at the initial log sizes for reference
print_log_size()

time.sleep(TRIAL_TIME)

# Look at the final log sizes for reference
print_log_size()

# Write Log Files for processing by other scripts
print("\nWriting Log Files...")

write_log_file(AP_HDF5_FILENAME, n_ap.log_get_all_new(log_tail_pad=0))

print("Done.")








