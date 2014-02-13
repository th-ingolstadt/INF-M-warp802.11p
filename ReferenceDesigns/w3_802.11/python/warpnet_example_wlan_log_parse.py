import wlan_exp_log
from wlan_exp_log import log_util
from wlan_exp_log.log_entries import *
from wlan_exp_log.mac_hdr_util import *

import code
import sys


#Excellent util function for dropping into interactive Python shell
# From http://vjethava.blogspot.com/2010/11/matlabs-keyboard-command-in-python.html
def debug_here(banner=None):
    ''' Function that mimics the matlab keyboard command '''
    # use exception trick to pick up the current frame
    try:
        raise None
    except:
        frame = sys.exc_info()[2].tb_frame.f_back
    print "# Use quit() to exit :) Happy debugging!"
    # evaluate commands in current namespace
    namespace = frame.f_globals.copy()
    namespace.update(frame.f_locals)
    try:
        code.interact(banner=banner, local=namespace)
    except SystemExit:
        return

logfile = 'example_logs/log_stats.bin'

print("WLAN Exp Log Example\nReading log file %s\n" % logfile)

with open(logfile, 'rb') as fh:
    log_b = bytearray(fh.read())

# Generate the index of log entry locations sorted by log entry type
log_index = log_util.gen_log_index(log_b)

# Unpack the log into numpy structured arrays
# gen_log_ndarrays returns a dictionary with log entry type IDs as keys
#  Global 'wlan_exp_log_entry_types' lists all known log entry types
log_nd = log_util.gen_log_ndarrays(log_b, log_index)

# Describe the decoded log entries
print("Log Contents:\n  Num  Type")
for k in log_nd.keys():
	print("%5d  %s" % (len(log_nd[k]), wlan_exp_log_entry_types[k].name))

print("")

########################################################################
# Example 1: Count the number of receptions per PHY rate

# Extract all OFDM receptions
log_rx_ofdm = log_nd[log_entry_rx_ofdm.entry_type_ID]

# Extract an array of just the Rx rates
rx_rates = log_rx_ofdm['rate']

#Initialize an array to count number of Rx per PHY rate
# MAC uses rate_indexes 1:8 to encode OFDM rates
rx_rate_counts = 8*[0]

for r in rx_rates:
	if(r <= len(rx_rate_counts)):
		rx_rate_counts[r-1] += 1
	else:
		print("Invalid rate %d in rx entry %d\n" % (r, ii))

print("Example 1: Num Rx per Rate: %s" % rx_rate_counts)

###########################################################################
# Example 2: Calculate total bytes transmitted to each distinct MAC address

# Extract all OFDM transmissions
log_tx = log_nd[log_entry_tx.entry_type_ID]

# Extract an array of just the MAC headers
tx_hdrs = log_tx['mac_header']

debug_here()
