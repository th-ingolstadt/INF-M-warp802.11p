"""
This script uses the WLAN Exp Log utilities to prase raw log data and print
a few interesting summary statistics about the log entries.

Hardware Setup:
    - None.  Parsing log data can be done completely off-line

Required Script Changes:
    - Set LOGFILE to the file name of your WLAN Exp log HDF5 file

Description:
    This script parses the log file and generates numpy arrays of each type
of log entry.  It then uses these arrays to count the number of receptions
per PHY rate and the total number of packets transmitted to each distinct
MAC address.  Finally, it optionally opens an interactive python shell to allow
you to play with the data.

License:   Copyright 2014, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
"""
import os
import sys
import numpy as np

import warpnet.wlan_exp_log.log_util as log_util
import warpnet.wlan_exp_log.log_util_hdf as hdf_util
from warpnet.wlan_exp_log.log_entries import wlan_exp_log_entry_types as entry_types

import warpnet.wlan_exp.util as wlan_exp_util
from  warpnet.wlan_exp.util import wlan_rates

#Use log file given as command line argument, if present
if(len(sys.argv) == 1):
    #No filename on command line
    LOGFILE = 'example_logs/sta_log_stats_headers_only.hdf5'
else:
    LOGFILE = str(sys.argv[1])

# Ensure the log file actually exists - quit immediately if not
if(not os.path.isfile(LOGFILE)):
    print("ERROR: Logfile {0} not found".format(LOGFILE))
    sys.exit()
else:
    print("Reading log file '{0}' ({1:5.1f} MB)\n".format(LOGFILE, (os.path.getsize(LOGFILE)/1E6)))

# Get the log_data from the file
log_data = hdf_util.hdf5_to_log_data(filename=LOGFILE)

# Get the log_data_index from the file
log_data_index = hdf_util.hdf5_to_log_data_index(filename=LOGFILE)

# Describe the log_data_index
log_util.print_log_index_summary(log_data_index, "Log Index Contents:")

# Filter log index to include all Rx entries, merged into RX_ALL, and all Tx entries
log_index = log_util.filter_log_index(log_data_index,
                                      include_only=['NODE_INFO', 'RX_OFDM', 'TX'])

log_util.print_log_index_summary(log_index, "Filtered Log Index:")

# Unpack the log into numpy structured arrays
#   log_data_to_np_arrays returns a dictionary with one key-value pair per
#    entry type included in the log_index argument. The log_index keys are reused
#    as the output dictionary keys. Each output dictionary value is a numpy record array
#    Refer to wlan_exp_log.log_entries.py for the definition of each record array datatype
log_np = log_util.log_data_to_np_arrays(log_data, log_index)

###############################################################################
# Example 0: Print node info
#TODO: use new util methods to print node_info info


###############################################################################
# Example 1: Count the number of receptions per PHY rate
#   NOTE:  Since there are only loops, this example can deal with RX_OFDM being an
#          empty list and does not need a try / except.
#

#TODO: Add column of #Tx/rate

# Extract all OFDM receptions
log_rx_ofdm = log_np['RX_OFDM']

# Extract an array of just the Rx rates
rx_rates = log_rx_ofdm['rate']

# Initialize an array to count number of Rx per PHY rate
#   MAC uses rate_indexes 1:8 to encode OFDM rates
rx_rate_counts = np.bincount(rx_rates, minlength=9)
rx_rate_counts = rx_rate_counts[1:9] #only rate values 1:8 are valid

print("Example 1: Num Rx Pkts per Rate:")
for (i,c) in enumerate(rx_rate_counts):
    print(" {0:2d} Mbps: {1:7}".format(int(wlan_rates[i]['rate']), c))


###############################################################################
# Example 2: Calculate total number of packets and bytes transmitted to each
#            distinct MAC address for each of the MAC addresses in the header

# Skip this example if the log doesn't contain TX events
if('TX' in log_np.keys()):
    # Extract all OFDM transmissions
    log_tx = log_np['TX']

    # Count number of packets transmitted to each unique address in the 'addr1' field
    tx_addrs_1 = log_tx['addr1']
    tx_counts = dict()

    for addr in np.unique(tx_addrs_1):
        # Find indexes of all instances where addresses match
        #   np.squeeze here flattens the result to a 1-D array
        addr_idx = np.squeeze(tx_addrs_1 == addr)

        # Count the number of packets (True values in index array) to this address
        tx_pkts_to_addr  = np.sum(addr_idx)

        # Count the number of bytes to this address
        tx_bytes_to_addr = np.sum(log_tx['length'][addr_idx])

        # Record the results in the output dictionary
        tx_counts[addr] = (tx_pkts_to_addr, tx_bytes_to_addr)

    # Print the results
    print("\nExample 2: Tx MPDU Counts:");
    print("{0:18}\t{1:>7}\t{2:>10}\t{3}".format(
        "Dest Addr",
        "# Pkts",
        "# Bytes",
        "MAC Addr Type"))

    for k in sorted(tx_counts.keys()):
        # Use the string version of the MAC address as the key for readability
        print("{0:18}\t{1:>7}\t{2:>10}\t{3}".format(
            wlan_exp_util.mac2str(k), 
            tx_counts[k][0], 
            tx_counts[k][1], 
            wlan_exp_util.mac_addr_desc(k)))

#################################################################################################
# Example 3: Calculate total number of packets and bytes received from each distinct MAC address

# Skip this example if the log doesn't contain RX events
if('RX_OFDM' in log_np.keys()):

    # Extract all receptions
    log_rx = log_np['RX_OFDM']

    # For this experiment, only look at Good = 0  or Bad = 1 receptions
    # Extract only Rx entries with good checksum (FCS = good)
    rx_good_fcs = log_rx[log_rx['fcs_result'] == entry_types['RX_OFDM'].consts['FCS_GOOD']]

    # Extract addr2 field from all good packets
    rx_addrs_2 = rx_good_fcs['addr2']

    # Build a dictionary using unique MAC addresses as keys
    rx_counts = dict()
    for addr in np.unique(rx_addrs_2):
        # Find indexes of all instances where addresses match
        #   np.squeeze here flattens the result to a 1-D array
        addr_idx = np.squeeze(rx_addrs_2 == addr)

        # Count the number of packets (True values in index array) from this address
        rx_pkts_from_addr  = np.sum(addr_idx)

        # Count the number of bytes from this address
        rx_bytes_from_addr = np.sum(rx_good_fcs['length'][addr_idx])

        # Add the information about the address to the dictionary
        rx_counts[addr] = (rx_pkts_from_addr, rx_bytes_from_addr)

    # Print the results
    print("\nExample 4: Rx Counts (including duplicates):");
    print("{0:18}\t{1:>7}\t{2:>10}\t{3}".format(
        "Src Addr",
        "# Pkts",
        "# Bytes",
        "MAC Addr Type"))

    for k in sorted(rx_counts.keys()):
        # Use the string version of the MAC address as the key for readability
        print("{0:18}\t{1:>7}\t{2:>10}\t{3}".format(
            wlan_exp_util.mac2str(k), 
            rx_counts[k][0], 
            rx_counts[k][1], 
            wlan_exp_util.mac_addr_desc(k)))

print('')

# Uncomment this line to open an interactive console after the script runs
#   This console will have access to all variables defined above
# wlan_exp_util.debug_here()
