import numpy as np
import pandas as pd

from matplotlib.pyplot import *

import wlan_exp.log.util as log_util
import wlan_exp.log.util_hdf as hdf_util
import wlan_exp.log.util_sample_data as sample_data_util

#-----------------------------------------------------------------------------
# Process command line arguments
#-----------------------------------------------------------------------------

LOGFILE       = 'raw_log_dual_flow_ap.hdf5'
logfile_error = False

# Use log file given as command line argument, if present
if(len(sys.argv) != 1):
    LOGFILE = str(sys.argv[1])

# See if the command line argument was for a sample data file
try:
    LOGFILE = sample_data_util.get_sample_data_file(LOGFILE)
except:
    logfile_error = True

# Ensure the log file actually exists - quit immediately if not
if ((not os.path.isfile(LOGFILE)) and logfile_error):
    print("ERROR: Logfile {0} not found".format(LOGFILE))
    sys.exit()
else:
    print("Reading log file '{0}' ({1:5.1f} MB)\n".format(os.path.split(LOGFILE)[1], (os.path.getsize(LOGFILE)/1E6)))


#-----------------------------------------------------------------------------
# Main script 
#-----------------------------------------------------------------------------

# Get the log_data from the file
log_data      = hdf_util.hdf5_to_log_data(filename=LOGFILE)

# Get the raw_log_index from the file
raw_log_index = hdf_util.hdf5_to_log_index(filename=LOGFILE)

# Extract just OFDM Rx events
rx_ofdm_log_index = log_util.filter_log_index(raw_log_index, include_only=['RX_OFDM'])

# Generate numpy array
print("Generating numpy arrays...")
log_nd = log_util.log_data_to_np_arrays(log_data, rx_ofdm_log_index)
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

