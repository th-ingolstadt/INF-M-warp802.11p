import warpnet.wlan_exp_log.log_util as log_util
from warpnet.wlan_exp_log.log_entries import wlan_exp_log_entry_types as log_entry_types
import numpy as np
import matplotlib.mlab as mlab

#LOGFILE = 'example_logs/ap_log_stats.bin'
LOGFILE = 'big_logs/ap_log_stats_2014_03_06.bin'

with open(LOGFILE, 'rb') as fh:
    log_b = fh.read()

log_index_raw = log_util.gen_log_index_raw(log_b)

#Extract just OFDM Tx events
log_idx_tx = log_util.filter_log_index(log_index_raw, include_only=['TX'])

#Generate numpy array
tx_recs = log_util.gen_log_ndarrays(log_b, log_idx_tx).values()[0]

#Define the fields to group by
group_fields = ('addr1',)

#Define the aggregation functions
stat_calc = ( 
    ('length',       len,     'num_pkts'),
    ('length',       np.mean, 'avg_len'),
    ('length',       sum,     'tot_len'),
    ('time_to_done', np.mean, 'avg_time'))

#Calculate the aggregate statistics
tx_stats = mlab.rec_groupby(tx_recs, group_fields, stat_calc)

#Display the results
print('\nTx Statistics for {0}:\n'.format(LOGFILE))

print('{0:^18} | {1:^8} | {2:^10} | {3:^14} | {4:^11}'.format(
    'Dest Addr',
    'Num Tx',
    'Avg Length',
    'Total Tx Bytes',
    'Avg Tx Time'))

for ii in range(len(tx_stats)):
    print('{0:<18} | {1:8d} | {2:10.1f} | {3:14} | {4:11.3f}'.format(
        log_util.mac_addr_int_to_str(tx_stats['addr1'][ii]),
        tx_stats['num_pkts'][ii],
        tx_stats['avg_len'][ii],
        tx_stats['tot_len'][ii],
        tx_stats['avg_time'][ii]))

print('')

