import warpnet.wlan_exp_log.log_util as log_util
import warpnet.wlan_exp_log.log_util_hdf as hdf_util
import numpy as np
from matplotlib.pylab import *
import pandas as pd
import seaborn as sns

#AP_LOGFILE = 'example_logs/ap_log_stats_1dir_w_inf.hdf5'
#STA_LOGFILE = 'example_logs/sta_log_stats_1dir_w_inf.hdf5'

AP_LOGFILE = 'example_logs/ap_log_30s_per_rate.hdf5'
STA_LOGFILE = 'example_logs/sta_log_30s_per_rate.hdf5'

#Extract the log data and index from the log files
log_data_ap = hdf_util.hdf5_to_log_data(filename=AP_LOGFILE)
log_data_index_ap = hdf_util.hdf5_to_log_data_index(filename=AP_LOGFILE)

log_data_sta = hdf_util.hdf5_to_log_data(filename=STA_LOGFILE)
log_data_index_sta = hdf_util.hdf5_to_log_data_index(filename=STA_LOGFILE)

#Generate indexes with just Tx and Rx events
entries_filt = ['NODE_INFO', 'RX_OFDM', 'TX', 'TX_LOW']
log_index_txrx_ap = log_util.filter_log_index(log_data_index_ap, include_only=entries_filt)
log_index_txrx_sta = log_util.filter_log_index(log_data_index_sta, include_only=entries_filt)

#Generate numpy arrays
log_np_ap = log_util.log_data_to_np_arrays(log_data_ap, log_index_txrx_ap)
log_np_sta = log_util.log_data_to_np_arrays(log_data_sta, log_index_txrx_sta)

#Extract tne NODE_INFO's and determine each node's MAC address
addr_ap = log_np_ap['NODE_INFO']['wlan_mac_addr']
addr_sta = log_np_sta['NODE_INFO']['wlan_mac_addr']

#Extract Tx entry arrays
tx_ap = log_np_ap['TX']
tx_sta = log_np_sta['TX']

#Extract Rx entry arrays
rx_ap = log_np_ap['RX_OFDM']
rx_sta = log_np_sta['RX_OFDM']

print('AP Rx: {0}, AP Tx: {1}'.format(len(rx_ap), len(tx_ap)))
print('STA Rx: {0}, STA Tx: {1}'.format(len(rx_sta), len(tx_sta)))

tx_f = ['num_tx', 'time_to_done', 'time_to_accept', 'rate']

#Filter out non-LTG pkts
tx_ap = tx_ap[ tx_ap['pkt_type'] == 2]

tx_ap_df = pd.DataFrame(tx_ap[tx_f], index=tx_ap['timestamp'])

