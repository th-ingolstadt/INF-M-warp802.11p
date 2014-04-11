"""

"""
import os
import sys
import time
import threading

import wlan_exp.util as wlan_exp_util
import wlan_exp.config as wlan_exp_config

import wlan_exp.log.util_hdf as hdf_util


try:
    from Queue import Queue, Empty
except ImportError:
    from queue import Queue, Empty     # Python 3.x

# Fix to support Python 2.x and 3.x
if sys.version[0]=="3": raw_input=input


#-----------------------------------------------------------------------------
# Experiment Variables
#-----------------------------------------------------------------------------
HOST_INTERFACES    = ['10.0.0.250']
NODE_SERIAL_LIST   = ['W3-a-00006']

AP_HDF5_FILENAME   = 'log_files/ap_traffic_capture.hdf5'

# Interval for printing
PRINT_TIME         = 1


#-----------------------------------------------------------------------------
# Global Variables
#-----------------------------------------------------------------------------
host_config        = None
nodes              = []
n_ap               = None
n_sta              = None
exp_done           = False
input_done         = False
timeout            = 0.1

#-----------------------------------------------------------------------------
# Local Helper Utilities
#-----------------------------------------------------------------------------
def write_hdf5_file(file_name, data):
    """Writes log data to a HDF5 file."""
    try:
        # Write the byte Log_data to the file 
        hdf_util.log_data_to_hdf5(data, file_name)
    except AttributeError as err:
        print("Error writing log file: {0}".format(err))


def get_log_size_str(nodes):
    """Gets the log size str for each node."""

    msg  = "Log Size:"

    for node in nodes:    
        log_size  = node.log_get_size()
        msg += "    {0:10s}  = {1:10d} bytes".format(node.name, log_size)

    return msg


def get_exp_duration_str(start_time):
    """Gets the duration str of the experiment since start_time."""
    return "Duration:  {0:8.0f} sec".format(time.time() - start_time)


def print_node_state(start_time):
    """Print the current state of the node."""
    
    msg  = "\r"
    msg += get_exp_duration_str(start_time)
    msg += " " * 5
    msg += get_log_size_str(nodes)
    msg += " " * 5

    sys.stdout.write(msg)
    sys.stdout.flush()



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
        return 

    # Add the current time to all the nodes
    wlan_exp_util.broadcast_cmd_write_time_to_logs(host_config)



def run_experiment():
    """WLAN Experiment.""" 
    global nodes
    global n_ap
    
    print("\nRun Experiment:\n")
    print("Use 'q' or Ctrl-C to end the experiment.\n")
    print("  NOTE:  In IPython, press return to see status update.\n")

    for node in nodes:
        # Reset the log and statistics now that we are ready to start 
        node.reset(log=True, txrx_stats=True)

    # Write Statistics to log
    n_ap.stats_write_txrx_to_log()

    # Get the start time
    start_time = time.time()
    last_print = time.time()
    
    # Print the current state of the node
    print_node_state(start_time)

    while not exp_done:
        loop_time = time.time()
        
        if ((loop_time - last_print) > PRINT_TIME):
            # Print the current state of the node    
            print_node_state(start_time)

            # Write Statistics to log
            n_ap.stats_write_txrx_to_log()
            
            # Set the last_print time
            last_print = time.time()



def end_experiment():
    """Experiment cleanup / post processing."""
    global nodes
    print("\nEnding experiment\n")

    wn_buffer = n_ap.log_get_all_new(log_tail_pad=0, max_req_size=2**23)
    data      = wn_buffer.get_bytes()

    # Write Log Files for processing by other scripts
    print("Writing {0} bytes of data to log file...".format(len(data)))
    
    print("    {0}".format(LOGFILE))
    write_hdf5_file(LOGFILE, data)
    print("Done.")
    return



if __name__ == '__main__':

    # Use log file given as command line argument, if present
    if(len(sys.argv) == 1):
        #No filename on command line
        LOGFILE = AP_HDF5_FILENAME
    else:
        LOGFILE = str(sys.argv[1])
    
    # Ensure the log file actually exists - quit immediately if not
    try:
        if not os.access(LOGFILE, os.W_OK):
            f = open(LOGFILE, 'w')
            f.close()
            os.remove(LOGFILE)
    except IOError as err:
        print("ERROR: Logfile {0} not able to be created".format(LOGFILE))
        sys.exit()


    # Create thread for experiment
    exp_thread = threading.Thread(target=run_experiment)
    exp_thread.daemon = True
    
    try:
        # Initialize the experiment
        init_experiment()

        # Setup the experiment
        setup_experiment()

        # Start the experiment loop thread
        exp_thread.start()

        # See if there is any input from the user
        while not input_done:
            sys.stdout.flush()
            temp = raw_input("")

            if temp is not '':
                user_input = temp.strip()
                user_input = user_input.upper()
                
                if ((user_input == 'Q') or (user_input == 'QUIT') or (user_input == 'EXIT')):
                    input_done = True
                    exp_done   = True

        # Wait for all threads
        exp_thread.join()
        sys.stdout.flush()
        
        # End the experiment
        end_experiment()

    except KeyboardInterrupt:
        exp_done   = True
        input_done = True

        # If there is a keyboard interrupt, then end the experiment
        end_experiment()

    print("\nExperiment Finished.")
