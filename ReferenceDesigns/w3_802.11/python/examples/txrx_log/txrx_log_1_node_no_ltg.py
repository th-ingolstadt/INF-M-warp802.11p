"""

"""
import sys
import time

import wlan_exp.util as wlan_exp_util
import wlan_exp.config as wlan_exp_config

import wlan_exp.log.util_hdf as hdf_util


#-----------------------------------------------------------------------------
# Experiment Variables
#-----------------------------------------------------------------------------
HOST_INTERFACES    = ['10.0.0.250']
NODE_SERIAL_LIST   = ['W3-a-00006']

AP_HDF5_FILENAME   = 'log_files/ap_traffic_capture.hdf5'

# Interval for printing
TRIAL_TIME         = 10


#-----------------------------------------------------------------------------
# Global Variables
#-----------------------------------------------------------------------------
host_config        = None
nodes              = []
n_ap               = None
n_sta              = None

#-----------------------------------------------------------------------------
# Local Helper Utilities
#-----------------------------------------------------------------------------
def write_hdf5_file(file_name, data):
    """Writes log data to a HDF5 file."""
    try:
        # Write the byte Log_data to the file 
        hdf_util.log_data_to_hdf5(data, file_name, overwrite=True)
    except AttributeError as err:
        print("Error writing log file: {0}".format(err))


def print_log_size(nodes):
    """Prints the log size for each node."""

    msg  = "Log Size:\n"

    for node in nodes:    
        log_size  = node.log_get_size()
        msg += "    {0:20s}  = {1:10d} bytes\n".format(node.name, log_size)

    print(msg)


def print_exp_duration(start_time):
    """Prints the duration of the experiment since start_time."""
    print("Duration:  {0:.0f} sec".format(time.time() - start_time))



#-----------------------------------------------------------------------------
# Experiment Script
#-----------------------------------------------------------------------------
def init_experiment():
    """Initialize WLAN Experiment."""
    global host_config
    global nodes
    
    print("\nInitializing experiment\n")
    
    # Create an object that describes the configuration of the host PC
    host_config  = wlan_exp_config.WlanExpHostConfiguration(host_interfaces=HOST_INTERFACES)
    
    # Create an object that describes the WARP v3 nodes that will be used in this experiment
    nodes_config = wlan_exp_config.WlanExpNodesConfiguration(host_config=host_config,
                                                             serial_numbers=NODE_SERIAL_LIST)
    
    # Initialize the Nodes
    #  This command will fail if either WARP v3 node does not respond
    nodes = wlan_exp_util.init_nodes(nodes_config, host_config)
    
    # Set each not into the default state
    for node in nodes:        
        # Issue a reset command to stop current operation / initialize components
        node.reset_all()
        
        # Configure the log
        node.log_configure(log_full_payloads=False)

    # Set the time of all the nodes to zero
    wlan_exp_util.broadcast_cmd_set_time(0.0, host_config)



def setup_experiment():
    """Setup WLAN Experiment."""
    global n_ap
    
    #---------------------------------------------------------------------
    # Setup experinmental associations
    
    # Extract the AP and STA nodes from the list of initialized nodes
    n_ap_l  = wlan_exp_util.filter_nodes(nodes, 'node_type', 'AP')
    
    # Check that we have a valid AP and STA
    if (len(n_ap_l) == 1):
        # Extract the two nodes from the lists for easier referencing below
        n_ap = n_ap_l[0]
    else:
        print("ERROR: Node configurations did not match requirements of script.\n")
        sys.exit(0)

    # Add the current time to all the nodes
    wlan_exp_util.broadcast_cmd_write_time_to_logs(host_config)



def run_experiment():
    """WLAN Experiment.""" 
    global nodes
    global n_ap
    
    print("\nRun Experiment:\n")
    print("Use Ctrl-C to end the experiment.\n")

    for node in nodes:
        # Reset the log and statistics now that we are ready to start 
        node.reset(log=True, txrx_stats=True)

    # Write Statistics to log
    n_ap.stats_write_txrx_to_log()
    
    # Look at the initial log sizes for reference
    print_log_size(nodes)

    # Get the start time
    start_time = time.time()
    
    # Commands to run during the experiment
    #     This experiment will end when the user kills the experiment with Ctrl-C
    while(True):

        # Wait a while
        time.sleep(TRIAL_TIME)

        # Print the duration of the experiment
        print_exp_duration(start_time)

        # Write Statistics to log
        n_ap.stats_write_txrx_to_log()
        
        # Look at the initial log sizes for reference
        print_log_size(nodes)



def end_experiment():
    """Experiment cleanup / post processing."""
    global nodes
    print("\nEnding experiment\n")

    wn_buffer = n_ap.log_get_all_new(log_tail_pad=0, max_req_size=2**23)
    data      = wn_buffer.get_bytes()

    # Write Log Files for processing by other scripts
    print("Writing {0} bytes of data to log file...".format(len(data)))
    
    print("    {0}".format(AP_HDF5_FILENAME))
    write_hdf5_file(AP_HDF5_FILENAME, data)
    print("Done.")



if __name__ == '__main__':
    try:
        # Initialize the experiment
        init_experiment()

        # Setup the experiment
        setup_experiment()

        # Run the experiment
        run_experiment()

        # End the experiment
        end_experiment()

    except KeyboardInterrupt:
        # If there is a keyboard interrupt, then end the experiment
        end_experiment()

    print("\nExperiment Finished.")
