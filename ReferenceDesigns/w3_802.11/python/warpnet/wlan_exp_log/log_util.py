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

def gen_log_index_raw(log_bytes):
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

    offset    = 0
    hdr_size  = 8
    log_len   = len(log_bytes)
    log_index = dict()

    # Need to determine if we are using Python 2 or 3 b/c they handle binary
    # files differently
    import sys
    if (sys.version_info.major == 3):
        python3 = 1
    else:
        python3 = 0

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

        if( (hdr_b[2:4] != b'\xed\xac') ):
            raise Exception("ERROR: Log file didn't start with valid entry header (offset %d)!" % (offset))

        if (python3):
            entry_type_id = (hdr_b[4] + (hdr_b[5] * 256))
            entry_size = (hdr_b[6] + (hdr_b[7] * 256))
        else:
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

# End gen_log_index_raw()



def filter_log_index(log_index, include_only=None, exclude=None, merge=None):
    """Parses a log index to generate a final log index that will be used
    by any one of a number of consumers, for example gen_log_ndarrays().
    This method allows the user to filter the raw log and create new
    log entry types that will allow customized views in to the log data.
    
    Attributes:
        include_only -- List of WlanExpLogEntryTypes to include in the output
                        log index.  This takes precedence over 'exclude'.
        exclude      -- List of WlanExpLogEntryTypes to exclude in the output
                        log index.  This will not be used if include != None.
        merge        -- List of dictionaries of the form:
                        {<Output WlanExpLogEntryTypes Instance>: 
                            [List of WlanExpLogEntryTypes names to be merged]}
        
    By using the 'merge', we are able to combine the indexes of 
    WlanExpLogEntryTypes to create super-sets of entries.  For example, 
    we could create a log index that contains all the receive events:
        {Rx(): ['RX_OFDM', 'RX_DSSS']}
    Then a consumer of the log index will create a specific set of entries
    for all receive events.  Please note that while all events need not have
    the same fields, the created class will need to know how to create 
    things like the numpy array based on the  WlanExpLogEntryTypes that are 
    being merged.  Please see warpnet_example_wlan_log_chan_est.py for an 
    example.
    """
    from warpnet.wlan_exp_log.log_entries import wlan_exp_log_entry_types as types
    
    ret_log_index = {}

    if (not include_only is None) and (not type(include_only) is list):
        raise TypeError("Parameter 'include' must be a list.\n")

    if (not exclude is None) and not type(exclude) is list:
        raise TypeError("Parameter 'exclude' must be a list.\n")

    if (not merge is None) and not type(merge) is dict:
        raise TypeError("Parameter 'merge' must be a dictionary.\n")


    # Replace the entry type IDs in the raw_log_index with entry types
    ret_log_index = {types.get_entry_type_for_id(k):log_index[k] for k in log_index.keys() if type(k) is int}

    # Filter the log_index
    try:
        # Create any new log indexes through the merge dictionary
        if not merge is None:
            new_index = []
            for k in merge.keys():
                for v in merge[k]:
                    new_index += ret_log_index[v]
                ret_log_index[k] = sorted(new_index)
    
        # Filter the resulting log index by 'include' / 'exclude' lists
        if not include_only is None:
            new_log_index = {}
            
            for entry_name in include_only:
                for k in ret_log_index.keys():
                    if k == entry_name:
                        new_log_index[k] = ret_log_index[k]

            ret_log_index = new_log_index
        else:
            if not exclude is None:
                for unwanted_key in exclude:
                    del ret_log_index[unwanted_key]
    
    except KeyError as err:
        msg  = "Issue generating log_index:\n"
        msg += "    Could not find entry type with name:  {0}".format(err)
        print(msg)

    return ret_log_index

# End gen_log_index()



def log_index_print_summary(log_index, title=None):
    """Prints a summary of the log index generated by gen_log_index()."""
    total_len = 0

    if not title is None:
        print('Log Index Summary:\n')
    else:
        print(title)

    for k in log_index.keys():
        print('{0:10d} of Type {1}'.format(len(log_index[k]), k))
        total_len += len(log_index[k])

    print('--------------------------')
    print('{0:10d} total entries\n'.format(total_len))

# End log_index_print_summary()



def gen_log_ndarrays(log_bytes, log_index):
    """Generate numpy structured arrays using the raw log and log
    index that was generated with gen_log_index().
    """
    entries_nd = dict()

    for k in log_index.keys():
        # Build a structured array with one element for each byte range enumerated above
        # Store each array in a dictionary indexed by the log entry type
        entries_nd[k] = k.generate_numpy_array(log_bytes, log_index[k])

    return entries_nd

# End gen_log_ndarrays()



def gen_hdf5_file(filename, log_dict, compression=None):
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
                grp.create_dataset(log_k, data=log_dict[file_k][log_k], compression=compression)
    else:
        #log_dict is dictionary-of-arrays
        # Create HDF5 file with datasets in root, one per log_dict[each key]

        for log_k in log_dict.keys():
            #Create one dataset per numpy array of log data
            hf.create_dataset(log_k, data=log_dict[log_k])

    hf.close()
    return

# End gen_hdf5_file()


def debug_here():
    import code
    import sys
    ''' Function that mimics the matlab keyboard command '''
    # use exception trick to pick up the current frame
    try:
        raise None
    except:
        frame = sys.exc_info()[2].tb_frame.f_back
    print("# Use quit() to exit :) Happy debugging!")
    # evaluate commands in current namespace
    namespace = frame.f_globals.copy()
    namespace.update(frame.f_locals)
    try:
        code.interact(banner=None, local=namespace)
    except SystemExit:
        return

