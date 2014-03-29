import warpnet.wlan_exp_log.log_util as log_util
import warpnet.wlan_exp_log.log_util_hdf as hdf_util
import numpy as np
from matplotlib.pylab import *
import pandas as pd

AP_LOGFILE = 'example_logs/ap_log_stats_1dir_w_inf.hdf5'
STA_LOGFILE = 'example_logs/sta_log_stats_1dir_w_inf.hdf5'

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

#Resample docs: http://stackoverflow.com/questions/17001389/pandas-resample-documentation
rs_interval = 1 #msec
rolling_winow = 10000 #samples

#############
# AP Rx
#############

#Select non-duplicate packets from partner node
rx_ap_idx = (rx_ap['addr2'] == addr_sta) & ((rx_ap['flags'] & 0x1) == 0) #needs valid flags field
rx_ap_from_sta = rx_ap[rx_ap_idx]

rx_ap_t = rx_ap_from_sta['timestamp']
rx_ap_len = rx_ap_from_sta['length']

#Select successful packets to partner node
tx_ap_idx = (tx_ap['addr1'] == addr_sta) & (tx_ap['result'] == 0)
tx_ap_to_sta = tx_ap[tx_ap_idx]

tx_ap_t = tx_ap_to_sta['timestamp'] + tx_ap_to_sta['time_to_accept']
tx_ap_len = tx_ap_to_sta['length']


#Convert to Pandas series
rx_ap_t_pd = pd.to_datetime(rx_ap_t, unit='us')
rx_ap_len_pd = pd.Series(rx_ap_len, index=rx_ap_t_pd)

tx_ap_t_pd = pd.to_datetime(tx_ap_t, unit='us')
tx_ap_len_pd = pd.Series(tx_ap_len, index=tx_ap_t_pd)

rx_ap_len_rs = rx_ap_len_pd.resample(('%dL' % rs_interval), how='sum').fillna(value=0)
tx_ap_len_rs = tx_ap_len_pd.resample(('%dL' % rs_interval), how='sum').fillna(value=0)

#Merge the indexes
t_idx = rx_ap_len_rs.index.union(tx_ap_len_rs.index)

#Reindex both Series to the common index, filling 0 in empty slots
rx_ap_len_rs = rx_ap_len_rs.reindex(t_idx, fill_value=0)
tx_ap_len_rs = tx_ap_len_rs.reindex(t_idx, fill_value=0)

#Compute rolling means
rx_xput_ap_r = pd.rolling_mean(rx_ap_len_rs, window=rolling_winow, min_periods=1)
tx_xput_ap_r = pd.rolling_mean(tx_ap_len_rs, window=rolling_winow, min_periods=1)

#Set NaN values to 0 (no packets == zero throughput)
rx_xput_ap_r = rx_xput_ap_r.fillna(value=0)
tx_xput_ap_r = tx_xput_ap_r.fillna(value=0)

#figure(1)
#clf()
#plot(tx_fin)
#plot(rx_fin)
#plot(tx_fin + rx_fin)


#plot(tx_fin, rx_fin, '.')
#axis([0,700,0,700])
