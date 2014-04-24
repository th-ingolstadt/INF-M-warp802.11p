import os
import sys

import numpy as np
import matplotlib.mlab as mlab

import wlan_exp.util as wlan_exp_util

import wlan_exp.log.util as log_util
import wlan_exp.log.util_hdf as hdf_util


#Use log file given as command line argument, if present
if(len(sys.argv) == 1):
    #No filename on command line
    LOGFILE = 'sample_data/ap_log_stats.hdf5'
else:
    LOGFILE = str(sys.argv[1])

# Ensure the log file actually exists - quit immediately if not
if(not os.path.isfile(LOGFILE)):
    print("ERROR: Logfile {0} not found".format(LOGFILE))
    sys.exit()
else:
    print("Reading log file '{0}' ({1:5.1f} MB)\n".format(LOGFILE, (os.path.getsize(LOGFILE)/1E6)))


# Get the log_data from the file
log_data      = hdf_util.hdf5_to_log_data(filename=LOGFILE)

# Get the raw_log_index from the file
raw_log_index = hdf_util.hdf5_to_log_index(filename=LOGFILE)

# Extract just OFDM Tx events
tx_log_index  = log_util.filter_log_index(raw_log_index, include_only=['TX'])

# Generate numpy array
tx_recs       = log_np = log_util.log_data_to_np_arrays(log_data, tx_log_index)

# Define the fields to group by
group_fields = ('addr1',)

# Define the aggregation functions
stat_calc = (
    ('retry_count',  np.mean, 'avg_num_tx'),
    ('length',       len,     'num_pkts'),
    ('length',       np.mean, 'avg_len'),
    ('length',       sum,     'tot_len'),
    ('time_to_done', np.mean, 'avg_time'))

# Calculate the aggregate statistics
tx_stats = mlab.rec_groupby(tx_recs, group_fields, stat_calc)

# Display the results
print('\nTx Statistics for {0}:\n'.format(LOGFILE))

print('{0:^18} | {1:^8} | {2:^10} | {3:^14} | {4:^11} | {5:^5}'.format(
    'Dest Addr',
    'Num MPDUs',
    'Avg Length',
    'Total Tx Bytes',
    'Avg Tx Time',
    'Avg Num Tx'))

for ii in range(len(tx_stats)):
    print('{0:<18} | {1:8d} | {2:10.1f} | {3:14} | {4:11.3f} | {5:5.1f}'.format(
        wlan_exp_util.mac_addr_to_str(tx_stats['addr1'][ii]),
        tx_stats['num_pkts'][ii],
        tx_stats['avg_len'][ii],
        tx_stats['tot_len'][ii],
        tx_stats['avg_time'][ii],
        tx_stats['avg_num_tx'][ii]))

print('')
log_util.debug_here()

