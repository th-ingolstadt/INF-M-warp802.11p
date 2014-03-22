import warpnet.wlan_exp_log.log_util as log_util
import warpnet.wlan_exp_log.log_entries as log_entries
from warpnet.wlan_exp_log.log_entries import wlan_exp_log_entry_types as entry_types
import numpy as np
import datetime

# NOTE: change these values to match your experiment setup
LOGFILE = 'netflix_log.bin'
HDF5FILE = 'netflix_log_rx_only.hdf5'

print("WLAN Exp Log Example: OFDM Rx Entry Exporter")


with open(LOGFILE, 'rb') as fh:
    print("Reading log file:  {0}".format(LOGFILE))
    log_b = fh.read()

# Generate the index of log entry locations sorted by log entry type
log_index_raw = log_util.gen_log_index_raw(log_b)

# Filter the index to include only OFDM Rx entries
log_index_rx = log_util.filter_log_index(log_index_raw, include_only=['RX_OFDM'])

# Generate numpy array of all OFDM Rx entries
log_np = log_util.gen_log_np_arrays(log_b, log_index_rx)
log_rx_ofdm = log_np['RX_OFDM']

##Find the source for which we have the most receptions

#Extract unique values for address 2 (transmitting address in received MAC headers)
uniq_addrs = np.unique(log_rx_ofdm['addr2'])

#Count the number of receptions in the log per source address
num_rx = list()
for ii,ua in enumerate(uniq_addrs):
    num_rx.append(np.sum(log_rx_ofdm['addr2'] == ua))

#Find the source address responsible for the most receptions
most_rx = max(num_rx)
most_common_addr = uniq_addrs[num_rx.index(most_rx)]

print("Found {0} receptions from {1}".format(most_rx, log_util.mac_addr_int_to_str(most_common_addr)))

#Create new numpy array of all receptions from most-common source
log_rx_one_src = np.empty(most_rx, dtype=log_rx_ofdm.dtype)
log_rx_one_src[:] = log_rx_ofdm[(log_rx_ofdm['addr2'] == most_common_addr)]


#Create some strings to use as attributes in the HDF5 file
# attributes are only for convenience - they aren't required for writing or reading HDF5 files
root_desc = 'Source Log file: {0}\n'.format(LOGFILE)
root_desc += 'HDF5 file written at {0}\n'.format(datetime.datetime.now())

ds_desc = 'All Rx OFDM entries from addr {0}\n'.format(log_util.mac_addr_int_to_str(most_common_addr))

#Construct dictionaries to feed the HDF5 exporter
# The dict key names are used as HDF5 dataset names
log_dict = {'RX_OFDM': log_rx_one_src}
attr_dict = {'/': root_desc, 'RX_OFDM': ds_desc}

print('Genereating HDF5 file {0}'.format(HDF5FILE))
log_util.gen_hdf5_file(HDF5FILE, np_log_dict=log_dict, attr_dict=attr_dict, compression=None)
