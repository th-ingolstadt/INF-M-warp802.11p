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
LOGFILE = 'example_logs/ap_log_stats.bin'
# LOGFILE = 'example_logs/sta_log_stats.bin'


print("WLAN Exp Log Example")
print("Reading log file:  {0}".format(LOGFILE))

with open(LOGFILE, 'rb') as fh:
    log_b = fh.read()

# Generate the index of log entry locations sorted by log entry type
log_index_raw = log_util.gen_log_index_raw(log_b)

# Describe the raw log
# log_util.log_index_print_summary(log_index_raw, "Raw Log Index:")


# Example Log Filters:
#
# log_index = log_util.filter_log_index(log_index_raw)
# log_index = log_util.filter_log_index(log_index_raw, include_only=['RX_OFDM'])
# log_index = log_util.filter_log_index(log_index_raw, include_only=['RX_OFDM', 'TX'])
# log_index = log_util.filter_log_index(log_index_raw, include_only=['TXRX_STATS', 'NODE_INFO'])
# log_index = log_util.filter_log_index(log_index_raw, merge={'RX_ALL':['RX_OFDM', 'RX_DSSS']})

# Make sure we have 'RX_OFDM', 'TX' and 'RX_ALL' entries in the output index
#   for examles 1, 2 and 3 respectively
log_index = log_util.filter_log_index(log_index_raw, 
                                      include_only=['RX_ALL', 'RX_OFDM', 'TX', 'WN_CMD_INFO'], 
                                      merge={'RX_ALL':['RX_OFDM', 'RX_DSSS']})

log_util.log_index_print_summary(log_index, "Filtered Log Index:")


# Unpack the log into numpy structured arrays
#   gen_log_ndarrays returns a dictionary with log entry type IDs as keys
#   Global 'wlan_exp_log_entry_types' lists all known log entry types
log_nd = log_util.gen_log_ndarrays(log_b, log_index)

# Describe the NumPy arrays
# log_util.log_index_print_summary(log_nd, "NumPy Array Summary:")


###############################################################################
# Example 1: Count the number of receptions per PHY rate
#   NOTE:  Since there are only loops, this example can deal with RX_OFDM being an 
#          empty list and does not need a try / except.
#

# Extract all OFDM receptions
log_rx_ofdm = log_nd['RX_OFDM']

# Extract an array of just the Rx rates
rx_rates = log_rx_ofdm['rate']

# Initialize an array to count number of Rx per PHY rate
#   MAC uses rate_indexes 1:8 to encode OFDM rates
rx_rate_counts = 8*[0]

for ii, r in enumerate(rx_rates):
	if(r <= len(rx_rate_counts)):
		rx_rate_counts[r-1] += 1
	else:
		print("Invalid rate {0} in rx entry {1}\n".format(r, ii))

print("Example 1: Num Rx per Rate: %s\n" % rx_rate_counts)

###############################################################################
# Example 2: Calculate total number of packets and bytes transmitted to each 
#            distinct MAC address for each of the MAC addresses in the header
#   NOTE:  Since there are direct accesses to array memory, we need a try/except 
#          in order to catch index errors when there are no 'TX' entries in the log.
#

try:
    # Extract all OFDM transmissions
    log_tx = log_nd['TX']
    
    # Extract an array of just the MAC headers
    tx_hdrs = log_tx['mac_header']
    
    # Create a dictionary of all the address fields
    #     Address1 - the wireless receiver address for AP transmissions
    #     Address2 - 
    #     Address3 - 
    tx_addr_dict = {'Address 1': tx_hdrs[:,4:10], 
                    'Address 2': tx_hdrs[:,10:16], 
                    'Address 3': tx_hdrs[:,16:22]}
    
    # Reduce each 6-byte MAC addresses to a unit64 for easier processing
    for address in sorted(tx_addr_dict.keys()):
        tx_addr = tx_addr_dict[address]
    
        tx_addr = np.sum(tx_addr * [2**40, 2**32, 2**24, 2**16, 2**8, 2**0], 1)
    
        # Build a dictionary using unique MAC addresses as keys
        tx_counts = dict()
        for addr in np.unique(tx_addr):
        	addr_idx = np.squeeze(tx_addr == addr)
        
        	tx_pkts_to_addr  = np.sum(addr_idx)
        	tx_bytes_to_addr = np.sum(log_tx['length'][addr_idx])
        
        	tx_counts[addr] = (tx_pkts_to_addr, tx_bytes_to_addr)
    
        # Print the results
        msg  = "\nExample 2: Tx Counts {0}:\n".format(address)
        msg += "Addr            \t# Pkts\t# Bytes\n"
        for k in tx_counts.keys():
        	# Use the string version of the MAC address as the key for readability
        	msg += "{0}\t{1}\t{2}\n".format(wlan_exp_util.mac2str(k), tx_counts[k][0], tx_counts[k][1])
        print(msg)

except IndexError:
    print("\nExample 2: No Transmit packets in log.")


#################################################################################################
# Example 3: Calculate total number of packets and bytes received from each distinct MAC address

# For this experiment, only look at Good = 0  or Bad = 1 receptions
FCS_BAD = 0

try:
    # Extract all receptions
    log_rx = log_nd['RX_ALL']
    
    # Extract an array of the MAC headers and FCS results   
    rx_hdrs    = log_rx['mac_header']
    rx_fcs_bad = log_rx['fcs_result']

    # Create a dictionary of all the address fields
    #     Address1 - the wireless receiver address for AP transmissions
    #     Address2 - 
    #     Address3 - 
    rx_addr_dict = {'Address 1': rx_hdrs[(rx_fcs_bad == FCS_BAD),4:10], 
                    'Address 2': rx_hdrs[(rx_fcs_bad == FCS_BAD),10:16], 
                    'Address 3': rx_hdrs[(rx_fcs_bad == FCS_BAD),16:22]}
    
    # Reduce each 6-byte MAC addresses to a unit64 for easier processing
    for address in sorted(rx_addr_dict.keys()):
        rx_addr = rx_addr_dict[address]
    
        rx_addr = np.sum(rx_addr * [2**40, 2**32, 2**24, 2**16, 2**8, 2**0], 1)
    
        # Build a dictionary using unique MAC addresses as keys
        rx_counts = dict()
        for addr in np.unique(rx_addr):
        	addr_idx = np.squeeze(rx_addr == addr)
        
        	rx_pkts_to_addr  = np.sum(addr_idx)
        	rx_bytes_to_addr = np.sum(log_rx['length'][addr_idx])
        
        	rx_counts[addr] = (rx_pkts_to_addr, rx_bytes_to_addr)
    
        # Print the results
        msg  = "\nExample 3: Rx Counts {0}:\n".format(address)
        msg += "Addr            \t# Pkts\t# Bytes\n"
        for k in tx_counts.keys():
        	# Use the string version of the MAC address as the key for readability
        	msg += "{0}\t{1}\t{2}\n".format(wlan_exp_util.mac2str(k), tx_counts[k][0], tx_counts[k][1])
        print(msg)

except IndexError:
    print("\nExample 3: No Receive packets in log.")


#################################################################################################
# Example 4: Count the different WARPNet commands for a given source ID

try:
    # Extract all WARPNet commands
    log_wn_cmd = log_nd['WN_CMD_INFO']
        
    # Extract an array of the source IDs and Commands
    wn_src_id  = log_wn_cmd['src_id']
    wn_cmd     = log_wn_cmd['command']

    # Build a dictionary using src id as a key to a dictionary of command counts
    wn_cmd_counts = dict()
    
    for src_id in np.unique(wn_src_id):
        wn_cmd_counts[src_id] = dict()
        
        wn_src_id_idx = np.squeeze(wn_src_id == src_id)
        
        wn_cmds_for_src_id = wn_cmd[wn_src_id_idx]

        for cmd in np.unique(wn_cmds_for_src_id):
            wn_cmd_idx = np.squeeze(wn_cmds_for_src_id == cmd)

            wn_cmd_counts[src_id][cmd] = len(wn_cmds_for_src_id[wn_cmd_idx])

    # Print the results
    msg  = "\nExample 4: WARPNet Command Counts:\n"
    for src_id in sorted(wn_cmd_counts.keys()):
        msg += "    Commands for source ID {0}:\n".format(src_id)
        for cmd in sorted(wn_cmd_counts[src_id].keys()):
            cmd_group = (cmd & 0xFF000000) >> 24
            cmd_id    = (cmd & 0x00FFFFFF)
            msg += "        Group ID = 0x{0:02X}  ".format(cmd_group)
            if (cmd_id < 1000):
                msg += "ID =   {0:6d}  ".format(cmd_id)
            else:
                msg += "ID = 0x{0:6X}  ".format(cmd_id)
            msg += "Count = {0:8d}\n".format(wn_cmd_counts[src_id][cmd])
    print(msg)

except IndexError:
    print("\nExample 4: No WARPNet command packets in log.")



print("\nDone.")




