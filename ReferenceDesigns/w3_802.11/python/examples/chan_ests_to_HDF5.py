import wlan_exp.util                   as wlan_exp_util
import wlan_exp.log.util               as log_util
import wlan_exp.log.util_hdf           as hdf_util
import numpy                           as np
import datetime

# NOTE: change these values to match your experiment setup
LOGFILE_IN    = 'sample_data/raw_log_sta_10s_per_rate.hdf5'
HDF5_FILE_OUT = 'sample_data/np_rx_ofdm_entries.hdf5'

print("WLAN Exp Log Example: OFDM Rx Entry Exporter")

log_data      = hdf_util.hdf5_to_log_data(filename=LOGFILE_IN)
raw_log_index = hdf_util.hdf5_to_log_data_index(filename=LOGFILE_IN)

#Generate indexes with just Rx_OFDM events
entries_filt = ['RX_OFDM']
log_index_rx = log_util.filter_log_index(raw_log_index, include_only=entries_filt)

# Generate numpy array of all OFDM Rx entries
log_np = log_util.log_data_to_np_arrays(log_data, log_index_rx)
log_rx_ofdm = log_np['RX_OFDM']

#####
#Find the source for which we have the most receptions

#Extract unique values for address 2 (transmitting address in received MAC headers)
uniq_addrs = np.unique(log_rx_ofdm['addr2'])

#Count the number of receptions in the log per source address
num_rx = list()
for ii,ua in enumerate(uniq_addrs):
    num_rx.append(np.sum(log_rx_ofdm['addr2'] == ua))

#Find the source address responsible for the most receptions
most_rx = max(num_rx)
most_common_addr = uniq_addrs[num_rx.index(most_rx)]

print("Found {0} receptions from {1}".format(most_rx, wlan_exp_util.mac_addr_to_str(most_common_addr)))

#Create new numpy array of all receptions from most-common source
arr_rx_one_src = np.empty(most_rx, dtype=log_rx_ofdm.dtype)
arr_rx_one_src[:] = log_rx_ofdm[(log_rx_ofdm['addr2'] == most_common_addr)]

#Create some strings to use as attributes in the HDF5 file
# attributes are only for convenience - they aren't required for writing or reading HDF5 files
root_desc = 'Source Log file: {0}\n'.format(LOGFILE_IN)
root_desc += 'HDF5 file written at {0}\n'.format(datetime.datetime.now())

ds_desc = 'All Rx OFDM entries from addr {0}\n'.format(wlan_exp_util.mac_addr_to_str(most_common_addr))

#Construct dictionaries to feed the HDF5 exporter
# The dict key names are used as HDF5 dataset names
log_dict = {'RX_OFDM': arr_rx_one_src}
attr_dict = {'/': root_desc, 'RX_OFDM': ds_desc}

print('Genereating HDF5 file {0}'.format(HDF5_FILE_OUT))
hdf_util.np_arrays_to_hdf5(HDF5_FILE_OUT, np_log_dict=log_dict, attr_dict=attr_dict, compression=None)
