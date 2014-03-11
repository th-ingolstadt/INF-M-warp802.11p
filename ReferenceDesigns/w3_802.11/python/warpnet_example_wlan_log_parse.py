"""
This script uses the WLAN Exp Log parsing framework to create useful 
data structures for processing data from log files retrieved from the
802.11 ref design and WARPnet.

Hardware Setup:
    - None.  Parsing log data can be done completely off-line

Required Script Changes:
    - Set LOGFILE to the file name of your WLAN Exp log

Description:
    This script parses the log file and generates numpy arrays of each type 
of log entry.  It then uses these arrays to count the number of receptions
per PHY rate and the total number of packets transmitted to each distinct 
MAC address.  Finally, it opens up an interactive python shell to allow 
you to play with the data.
"""
import numpy as np

import warpnet.wlan_exp_log.log_entries as log
import warpnet.wlan_exp_log.log_util as log_util

import warpnet.wlan_exp.util as wlan_exp_util


# NOTE: change these values to match your experiment setup
# LOGFILE = 'example_logs/temp_log.bin'
LOGFILE = 'example_logs/ap_log_stats_2014_03_06.bin'
# LOGFILE = 'example_logs/ap_log_stats.bin'
# LOGFILE = 'example_logs/sta_log_stats.bin'


print("WLAN Exp Log Example")
print("Reading log file:  {0}".format(LOGFILE))

with open(LOGFILE, 'rb') as fh:
    log_b = fh.read()

# Generate the index of log entry locations sorted by log entry type
log_index_raw = log_util.gen_log_index_raw(log_b)

log_util.log_index_print_summary(log_index_raw, "Raw Log Index:")

# log_index = log_util.filter_log_index(log_index_raw, include_only=['RX_OFDM'])
# log_index = log_util.filter_log_index(log_index_raw, include_only=['RX_OFDM', 'TX'])
# log_index = log_util.filter_log_index(log_index_raw, exclude=['TXRX_STATS', 'NODE_INFO'])
# log_index = log_util.filter_log_index(log_index_raw, include_only=['RX_ALL'], merge={log.Rx():['RX_OFDM', 'RX_DSSS']})

log_index = log_util.filter_log_index(log_index_raw, 
                                      exclude=['TXRX_STATS', 'NODE_INFO', 'RX_DSSS', 'TX_LOW', 'WN_CMD_INFO'], 
                                      merge={log.Rx():['RX_OFDM', 'RX_DSSS']})

# log_index = log_util.gen_log_index(log_index_raw, merge={'RX_ALL':'TEMP'})


log_util.log_index_print_summary(log_index, "Filtered Log Index:")


# Unpack the log into numpy structured arrays
#   gen_log_ndarrays returns a dictionary with log entry type IDs as keys
#   Global 'wlan_exp_log_entry_types' lists all known log entry types
log_nd = log_util.gen_log_ndarrays(log_b, log_index)

# Describe the decoded log entries
log_util.log_index_print_summary(log_nd, "NumPy Array Summary:")


########################################################################
# Example 1: Count the number of receptions per PHY rate

try:
    # Extract all OFDM receptions
    log_rx_ofdm = log_nd[log.RxOFDM().name]
    
    # Extract an array of just the Rx rates
    rx_rates = log_rx_ofdm['rate']
    
    #Initialize an array to count number of Rx per PHY rate
    # MAC uses rate_indexes 1:8 to encode OFDM rates
    rx_rate_counts = 8*[0]
    
    for ii, r in enumerate(rx_rates):
    	if(r <= len(rx_rate_counts)):
    		rx_rate_counts[r-1] += 1
    	else:
    		print("Invalid rate {0} in rx entry {1}\n".format(r, ii))
    
    print("Example 1: Num Rx per Rate: %s\n" % rx_rate_counts)
except KeyError:
    print("Example 1: No received OFDM packets in log.")

#################################################################################################
# Example 2: Calculate total number of packets and bytes transmitted to each distinct MAC address

try:
    # Extract all OFDM transmissions
    log_tx = log_nd[log.Tx().name]
    
    # Extract an array of just the MAC headers
    tx_hdrs = log_tx['mac_header']
    
    # Extract the address1 field (the wireless receiver address for AP transmissions)
    tx_addr1 = tx_hdrs[:,4:10]
    
    # Reduce each 6-byte MAC addresses to a unit64 for easier processing
    tx_addr1 = np.sum(tx_addr1 * [2**40, 2**32, 2**24, 2**16, 2**8, 2**0], 1)
    
    # Build a dictionary using unique MAC addresses as keys
    tx_counts = dict()
    for addr in np.unique(tx_addr1):
    	addr_idx = np.squeeze(tx_addr1 == addr)
    
    	tx_pkts_to_addr = np.sum(addr_idx)
    	tx_bytes_to_addr = np.sum(log_tx['length'][addr_idx])
    
    	tx_counts[addr] = (tx_pkts_to_addr, tx_bytes_to_addr)
    
    # Print the results
    print("Example 2: Tx Counts\nAddr            \t# Pkts\t# Bytes")
    for k in tx_counts.keys():
    	# Use the string version of the MAC address as the key for readability
    	print("%s\t%d\t%d" % (wlan_exp_util.mac2str(k), tx_counts[k][0], tx_counts[k][1]))
except KeyError:
    print("Example 2: No Transmit packets in log.")

print("\nDone.")
