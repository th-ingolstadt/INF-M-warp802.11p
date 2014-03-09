# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment Log Utilities
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
MODIFICATION HISTORY:

Ver   Who  Date     Changes
----- ---- -------- -----------------------------------------------------
1.00a pom  1/27/14  Initial release

------------------------------------------------------------------------------

This module provides utility functions to parse a WLAN Exp log file.

Functions (see below for more information):
    gen_log_index() -- Generate a byte index given a WLAN Exp log file
    gen_log_ndarrays() -- Generate a numpy structured array (ndarray) of log entries
"""

def gen_log_index(log_bytes, strict=False):
    """Parses a binary WARPnet log file by recording the byte index of each
    entry. The byte indexes are returned in a dictionary with the entry
    type IDs as keys. This method does not unpack or interpret each log
    entry and does not change any values in the log file itself (the
    log_bytes array argument can be read-only).

    Format of log entry header:

        typedef struct{
            u32 delimiter;
            u16 entry_type;
            u16 entry_length;
        } entry_header;

    fmt_log_hdr = 'I H H' #if we were using struct.unpack
    """

    offset = 0
    hdr_size = 8
    num_entries = 0
    log_len = len(log_bytes)

    log_index = dict()

    while True:
        # Stop here if the next log entry header is incomplete
        if( (offset + hdr_size) > log_len):
            break

        # Check if entry starts with valid header.  struct.unpack is the
        # natural way to interpret the entry header, but it's slower
        # than accessing the bytes directly.

        # hdr = unpack(fmt_log_hdr, log_bytes[offset:offset+hdr_size])
        # ltk = hdr[1]
        # if( (hdr[0] & wn_entries.WN_LOG_DELIM) != wn_entries.WN_LOG_DELIM):
        #     raise Exception("ERROR: Log file didn't start with valid entry header!")

        # Use raw byte slicing for better performance
        # Values below are hard coded to match current WLAN Exp log entry formats
        hdr_b = log_bytes[offset:offset+hdr_size]

        if( (hdr_b[2:4] != '\xed\xac') ):
            raise Exception("ERROR: Log file didn't start with valid entry header (offset %d)!" % (offset))

        entry_type_id = (ord(hdr_b[4]) + (ord(hdr_b[5]) * 256))
        entry_size = (ord(hdr_b[6]) + (ord(hdr_b[7]) * 256))

        offset += hdr_size

        # Stop here if the last log entry is incomplete
        if( (offset + entry_size) > log_len):
            break

        #Try/except slightly faster than "if(entry_type_id in log_index.keys()):"
        # ~3 seconds faster (13s -> 10s) for ~1GB log file
        try:
            log_index[entry_type_id].append(offset)
        except KeyError:
            log_index[entry_type_id] = [offset]

        # Increment the byte offset for the next iteration
        offset += entry_size

    return log_index

# End gen_log_index()

def log_dict_print_contents_summary(log_index):
    total_len = 0
    for k in log_index.keys():
        print('%9d of Type %s' % (len(log_index[k]), k))
        total_len += len(log_index[k])

    print('--------------------------\n')
    print('%9d total entries' % total_len)


def log_dict_convert_to_named_keys(log_index):
    from warpnet.wlan_exp_log.log_entries import wlan_exp_log_entry_types as log_entry_types

    for k_i in log_index.keys():
        log_type_l = log_entry_types.get_entry_types_from_id(k_i, strict=True)
        if(len(log_type_l) == 1):
            k_str = log_type_l[0]
        else:
            raise Exception('No entry type defined with ID %d' % k_i)

        #Keep the value, replace the key
        # Do this in place to avoid duplicating the whole log (potentially multi GB)
        log_index[k_str] = log_index.pop(k_i)

def gen_log_ndarrays(log_bytes, log_index, convert_keys=False):
    """Generate numpy structured arrays using the raw log and log
    index that was generated with gen_log_index().
    """
    import numpy as np
    from warpnet.wlan_exp_log.log_entries import wlan_exp_log_entry_types as log_entry_types

    entries_nd = dict()

    for k in log_index.keys():
        if(type(k) == type(1)): #is a number
            log_type_l = log_entry_types.get_entry_types_from_id(k, strict=True)
            if(len(log_type_l) == 1):
                log_type = log_entry_types.get_entry_from_type(log_type_l[0])
            else:
                continue
        else:
            log_type = log_entry_types.get_entry_from_type(k)
            if(not log_type):
                continue

        # Construct the list of byte ranges for this type of log entry
        index_iter = [log_bytes[o : o + log_type.fields_size] for o in log_index[k]]

        # Build a structured array with one element for each byte range enumerated above
        # Store each array in a dictionary indexed by the log entry type
        entries_nd[k] = np.fromiter(index_iter, np.dtype(log_type.fields_np_dt), len(log_index[k]))

    if(convert_keys):
        log_dict_convert_to_named_keys(entries_nd)

    return entries_nd

# End gen_log_ndarrays()

def gen_hdf5_file(filename, log_dict):
    import h5py

    hf = h5py.File(filename, mode='w', userblock_size=1024)

    dk = log_dict.keys()

    if(type(log_dict[dk[0]]) == type(dict())):
        #log_dict is dictionary-of-dictionaries
        # Create an HDF5 file with one group per value in log_dict
        #  with one dataset per value in log_dict[each key]
        #This is a good structure for one dictionary containing one key-value per parsed log file

        for file_k in log_dict.keys():
            #Create one group per log file, using log file name as group name
            grp = hf.create_group(file_k)

            for log_k in log_dict[file_k].keys():
                #Create one dataset per numpy array of log data
                grp.create_dataset(log_k, data=log_dict[file_k][log_k])
    else:
        #log_dict is dictionary-of-arrays
        # Create HDF5 file with datasets in root, one per log_dict[each key]

        for log_k in log_dict.keys():
            #Create one dataset per numpy array of log data
            hf.create_dataset(log_k, data=log_dict[log_k])

    hf.close()
    return

