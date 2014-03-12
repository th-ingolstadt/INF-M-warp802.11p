import warpnet.wlan_exp_log.log_util as log_util
from warpnet.wlan_exp_log.log_entries import wlan_exp_log_entry_types as log_entry_types
import numpy as np
import pandas as pd
from matplotlib.pyplot import *

with open('big_logs/sta_log_stats_2014_03_06.bin', 'rb') as fh:
    print("Reading log file...")
    log_b = fh.read()

print("Generating log index...")
log_index_raw = log_util.gen_log_index_raw(log_b)

#Extract just OFDM Rx events
log_idx_rx_ofdm = log_util.filter_log_index(log_index_raw, include_only=['RX_OFDM'])

#Generate numpy array
print("Generating numpy arrays...")
log_nd = log_util.gen_log_ndarrays(log_b, log_idx_rx_ofdm)
rx = log_nd['RX_OFDM']

#Extract length and timestamp fields
l = rx['length']
t = rx['timestamp']

#Convert to pandas dataset, indexed by microsecond timestamp
print("Calculating throughput...")
t_pd = pd.to_datetime(t, unit='us')
len_pd = pd.Series(l, index=t_pd)

rs_interval = 100 #msec
rolling_winow = 600 #samples

#Resample length vector, summing in each interval; fill empty intervals with 0
# Interval argument must be 'NL' for Nmsec ('100L' for 100msec)
len_rs = len_pd.resample(('%dL' % rs_interval), how='sum').fillna(value=0)

#Calculate rolling mean of throghput (units of bytes per rs_interval)
xput_roll = pd.rolling_mean(len_rs, rolling_winow)

#Scale to Mb/sec
xput_roll = xput_roll  * (1.0e-6 * 8.0 * (1.0/(rs_interval * 1e-3)))

#----------------------
# Plot results

#X axis in units of minutes
t_p = np.linspace(0, (1.0/60) * 1e-6 * (max(t) - min(t)), len(xput_roll))

#enter interactive mode from script, so figures/plots update live
ion()

figure()
clf()
plot(t_p, xput_roll)
grid()
xlabel('Time (minutes)')
ylabel('Throughput (Mb/sec)')
title('Throughput vs. Time (%d sec rolling mean)' % (rolling_winow * rs_interval * 1.0e-3))

