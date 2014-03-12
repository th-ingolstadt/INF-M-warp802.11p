import warpnet.wlan_exp_log.log_util as log_util
from warpnet.wlan_exp_log.log_entries import wlan_exp_log_entry_types as log_entry_types
import numpy as np
import matplotlib.mlab as mlab

with open('example_logs/ap_log_stats.bin', 'rb') as fh:
    print("Reading log file...")
    log_b = fh.read()

print("Generating log index...")
log_index_raw = log_util.gen_log_index_raw(log_b)

#Extract just OFDM Rx events
log_idx_tx = log_util.filter_log_index(log_index_raw, include_only=['TX'])

#Generate numpy array
print("Generating numpy arrays...")
tx = log_util.gen_log_ndarrays(log_b, log_idx_tx).values()[0]

