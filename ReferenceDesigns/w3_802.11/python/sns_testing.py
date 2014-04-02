import warpnet.wlan_exp_log.log_util as log_util
import warpnet.wlan_exp_log.log_util_hdf as hdf_util
import numpy as np
from matplotlib.pylab import *
import pandas as pd
import seaborn as sns

#AP_LOGFILE = 'example_logs/ap_log_stats_1dir_w_inf.hdf5'
#STA_LOGFILE = 'example_logs/sta_log_stats_1dir_w_inf.hdf5'

AP_LOGFILE = 'example_logs/ap_log_5min_h2h.hdf5'
STA_LOGFILE = 'example_logs/sta_log_5min_h2h.hdf5'

#Extract the log data and index from the log files
log_data_ap = hdf_util.hdf5_to_log_data(filename=AP_LOGFILE)
log_data_index_ap = hdf_util.hdf5_to_log_data_index(filename=AP_LOGFILE)

log_data_sta = hdf_util.hdf5_to_log_data(filename=STA_LOGFILE)
log_data_index_sta = hdf_util.hdf5_to_log_data_index(filename=STA_LOGFILE)

#Generate indexes with just Tx and Rx events
entries_filt = ['NODE_INFO', 'RX_OFDM', 'TX', 'TX_LOW', 'TIME_INFO']
log_index_txrx_ap = log_util.filter_log_index(log_data_index_ap, include_only=entries_filt)
log_index_txrx_sta = log_util.filter_log_index(log_data_index_sta, include_only=entries_filt)

#Generate numpy arrays
log_np_ap = log_util.log_data_to_np_arrays(log_data_ap, log_index_txrx_ap)
log_np_sta = log_util.log_data_to_np_arrays(log_data_sta, log_index_txrx_sta)

#Extract the NODE_INFO's and determine each node's MAC address
addr_ap = log_np_ap['NODE_INFO']['wlan_mac_addr']
addr_sta = log_np_sta['NODE_INFO']['wlan_mac_addr']

#Extract Tx and Rx entry arrays
tx_ap = log_np_ap['TX']
tx_sta = log_np_sta['TX']

tx_low_ap = log_np_ap['TX_LOW']
tx_low_sta = log_np_sta['TX_LOW']

rx_ap = log_np_ap['RX_OFDM']
rx_sta = log_np_sta['RX_OFDM']

#Select only LTG data packets to the partner node
tx_idx_ap       = (tx_ap['pkt_type'] == 2) & (tx_ap['addr1'] == addr_sta)
tx_low_idx_ap   = (tx_low_ap['pkt_type'] == 2) & (tx_low_ap['addr1'] == addr_sta)

tx_idx_sta      = (tx_sta['pkt_type'] == 2) & (tx_sta['addr1'] == addr_ap)
tx_low_idx_sta  = (tx_low_sta['pkt_type'] == 2) & (tx_low_sta['addr1'] == addr_ap)

tx_ap       = tx_ap[tx_idx_ap]
tx_low_ap   = tx_low_ap[tx_low_idx_ap]

tx_sta      = tx_sta[tx_idx_sta]
tx_low_sta  = tx_low_sta[tx_low_idx_sta]

rx_ap = rx_ap[ rx_ap['pkt_type'] == 2 ]
rx_sta = rx_sta[ rx_sta['pkt_type'] == 2 ]

#%%
#Convert to pandas DataFrames
tx_df_f = ['mac_seq', 'num_tx', 'time_to_done', 'time_to_accept', 'rate', 'result', 'uniq_seq']
tx_low_df_f = ['mac_seq', 'rate', 'tx_count', 'length', 'uniq_seq', 'num_slots', 'cw']

rx_df_f = ['mac_seq', 'rate', 'fcs_result', 'flags', 'length']

tx_ap_df        = pd.DataFrame(tx_ap[tx_df_f], index=(tx_ap['timestamp'] + tx_ap['time_to_accept']))
tx_low_ap_df    = pd.DataFrame(tx_low_ap[tx_low_df_f], index=tx_low_ap['timestamp'])


tx_sta_df        = pd.DataFrame(tx_sta[tx_df_f], index=tx_sta['timestamp'])
tx_low_sta_df    = pd.DataFrame(tx_low_sta[tx_low_df_f], index=tx_low_sta['timestamp'])

rx_ap_df = pd.DataFrame(rx_ap[rx_df_f], index=rx_ap['timestamp'])
rx_sta_df = pd.DataFrame(rx_sta[rx_df_f], index=rx_sta['timestamp'])

#%%

idx_low = tx_low_ap_df.index
idx_high = tx_ap_df.index
idx_u = idx_high.union(idx_low)

tx_ap_df_u = tx_ap_df.reindex(idx_u)
tx_low_ap_df_u = tx_low_ap_df.reindex(idx_u)


#%%
clf()
sns.violinplot(tx_ap_df.time_to_done, tx_ap_df.rate)

#%%
tx_ap_df['tx_lows'] = None
for idx in tx_ap_df.index:
    print idx
    t1 = idx + tx_ap_df.loc[idx].time_to_accept
    t2 = t1 + tx_ap_df.loc[idx].time_to_done
    tx_ap_df[idx, 'tx_lows'] = tx_low_ap_df[ (tx_low_ap_df.index > t1) & (tx_low_ap_df.index < t2) ].index
