import sys
import os
import numpy as np
import mmap

import warpnet.wlan_exp_log.log_entries as log
import warpnet.wlan_exp_log.log_util as log_util

import warpnet.wlan_exp.util as wlan_exp_util

#LOGFILE_DIR = './big_logs'
#HDF5FILE = 'big_logs.hdf5'

LOGFILE_DIR = './example_logs'
HDF5FILE = 'small_logs_new.hdf5'

LOGFILE_EXT = '.bin'

logfiles = []
for f in os.listdir(LOGFILE_DIR):
    if(os.path.isfile(os.path.join(LOGFILE_DIR, f)) & (f[-4:] == LOGFILE_EXT)):
        logfiles.append(f)

print("Found %d log files" % len(logfiles))

all_logs = dict()

for ii,logfile in enumerate(logfiles):
    filename_full = os.path.join(LOGFILE_DIR, logfile)
    filename_short = logfile.split(LOGFILE_EXT)[0]

    print("\n********")
    print("Reading log file {0}: {1}".format(ii, logfile))

    with open(filename_full, 'rb') as fh:
        log_b = fh.read()

    log_index = log_util.gen_log_index(log_b)
    log_util.log_dict_convert_to_named_keys(log_index)

    log_nd = log_util.gen_log_ndarrays(log_b, log_index)

    #Skip outer dictionary if there is only one log file
    if(len(logfiles) > 1):
        all_logs[filename_short] = log_nd
    else:
        all_logs = log_nd

    # Describe the decoded log entries
    print("Log Contents:\n    Num  Type")
    for k in sorted(log_nd.keys()):
        print("{0:7d}  {1}".format(len(log_nd[k]), k))


print("\n")
log_util.debug_here()
#Generate an HDF5 file containing all log data, group by log file
print("Genereating HDF5 file %s" % HDF5FILE)
log_util.gen_hdf5_file(HDF5FILE, all_logs, compression=None)

print("Done.")
