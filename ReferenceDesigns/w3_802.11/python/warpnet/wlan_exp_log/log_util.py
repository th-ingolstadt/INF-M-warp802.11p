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
    gen_log_index_raw() -- Generate a byte index given a WLAN Exp log file
    filter_log_index()  -- Filter a log index with given parameters
    gen_log_np_arrays()  -- Generate a numpy structured array (ndarray) of log entries
    gen_hdf5_file()     -- Generate a HDF5 file based on numpy arrays
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

    offset         = 0
    hdr_size       = 8
    log_len        = len(log_bytes)
    log_index      = dict()
    use_byte_array = 0

    # Need to determine if we are using byte arrays or strings for the
    # log_bytes b/c we need to handle the data differently
    try:
        byte_array_test = log_bytes[offset:offset+hdr_size]
        byte_array_test = ord(byte_array_test[0])
    except TypeError:
        use_byte_array  = 1


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

        if (use_byte_array):
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
    """Parses a log index to generate a filtered log index.

    Consumers, in general, cannot operate on a raw log index since that has
    not been converted in to log entry types.  The besides filtering a log
    index, this method will also convert any raw index entries (ie entries
    with keys of type int) in to the corresponding WlanExpLogEntryTypes.

    Attributes:
        include_only -- List of WlanExpLogEntryTypes to include in the output
                        log index.  This takes precedence over 'exclude'.
        exclude      -- List of WlanExpLogEntryTypes to exclude in the output
                        log index.  This will not be used if include != None.
        merge        -- A dictionary of the form:

    {'WlanExpLogEntryType name': [List of 'WlanExpLogEntryTypes name' to merge]}

    By using the 'merge', we are able to combine the indexes of
    WlanExpLogEntryTypes to create super-sets of entries.  For example,
    we could create a log index that contains all the receive events:
        {'RX_ALL': ['RX_OFDM', 'RX_DSSS']}
    as long as the names 'RX_ALL', 'RX_OFDM', and 'RX_DSSS' are valid
    WlanExpLogEntryTypes.

    The filter follows the following basic rules:
        1) Every requested output (either through 'include_only' or 'merge')
             has a key in the output dictionary
        2) All input and output keys must refer to the 'name' property of
             valid WlanExpLogEntryType instances

    For example, assume:
      - 'A', 'B', 'C', 'D', 'M' are valid WlanExpLogEntryType instance names
      - The log_index = {'A': [A0, A1, A2], 'B': [B0, B1], 'C': []}

    'include_only' behavior:

        x = filter_log_index(log_index, include_only=['A'])
        x == {'A': [A0, A1, A2]}

        x = filter_log_index(log_index, include_only=['A',B'])
        x == {'A': [A0, A1, A2], 'B': [B0, B1]}

        x = filter_log_index(log_index, include_only=['C'])
        x == {'C': []]}

        x = filter_log_index(log_index, include_only=['D'])
        x == {'D': []]}

    All names specified in 'include_only' are included as part of the
    output dictionary.  It is then up to the consumer to check if the
    number of entries for a given 'name' is zero (ie the list is empty).

    'exclude' behavior:

        x = filter_log_index(log_index, exclude=['B'])
        x == {'A': [A0, A1, A2]}, 'C': []}

        x = filter_log_index(log_index, exclude=['D'])
        WARNING:  D does not exist in log index.  Ignoring for exclude.
        x == {'A': [A0, A1, A2]}, 'B': [B0, B1], 'C': []}

    All names specified in 'exclude' are removed from the output dictionary.
    However, there is no guarentee what other WlanExpLogEntryTypes are in
    the output dictionary.  That depends on the entries in the input log index.

    'merge' behavior:

        x = filter_log_index(log_index, merge={'D': ['A', 'B']}
        x == {'A': [A0, A1, A2]},
              'B': [B0, B1],
              'C': [],
              'D': [A0, A1, A2, B0, B1]}

        x = filter_log_index(log_index, merge={'M': ['C', 'D']}
        x == {'A': [A0,A1,A2]},
              'B': [B0,B1],
              'C': [],
              'M': []}

    All names specified in the 'merge' are included as part of the output
    dictionary.  It is then up to the consumer to check if the number of
    entries for a given 'name' is zero (ie the list is empty).

    Combined behavior:

        x = filter_log_index(log_index, include_only=['M'], merge={'M': ['A','C']}
        x == {'M': [A0, A1, A2]}

        x = filter_log_index(log_index, include_only=['M'], merge={'M': ['A','D']}
        WARNING:  D does not exist in log index.  Ignoring for merge.
        x == {'M': [A0, A1, A2]}

        x = filter_log_index(log_index, include_only=['M'], merge={'M': ['C','D']}
        WARNING:  D does not exist in log index.  Ignoring for merge.
        x == {'M': []}
    """
    from warpnet.wlan_exp_log.log_entries import wlan_exp_log_entry_types as entry_types

    ret_log_index = {}

    if (include_only is not None) and (type(include_only) is not list):
        raise TypeError("Parameter 'include' must be a list.\n")

    if (exclude is not None) and (type(exclude) is not list):
        raise TypeError("Parameter 'exclude' must be a list.\n")

    if (merge is not None) and (type(merge) is not dict):
        raise TypeError("Parameter 'merge' must be a dictionary.\n")

    # Start by creating a new dictionary with the same values as the log_index input
    #  but using the WlanExpLogEntryType instances as keys
    ret_log_index = {entry_types[k] : log_index[k] for k in log_index.keys()}

    # Filter the log_index
    try:
        # Create any new log indexes through the merge dictionary
        if merge is not None:
            # For each new merged index output
            for k in merge.keys():
                new_index = []
                for v in merge[k]:
                    try:
                        new_index += ret_log_index[v]
                    except:
                        msg  = "WARNING:  {0} does ".format(v)
                        msg += "not exist in log index.  Ignoring for merge.\n"
                        print(msg)

                # Add the new merged index lists to the output dictionary
                # Use the type instance corresponding to the user-supplied string as the key
                ret_log_index[entry_types[k]] = sorted(new_index)

        # Filter the resulting log index by 'include' / 'exclude' lists
        if include_only is not None:
            new_log_index = {}

            for entry_name in include_only:
                new_log_index[entry_types[entry_name]] = []
                for k in ret_log_index.keys():
                    if k == entry_name:
                        new_log_index[k] = ret_log_index[k]

            ret_log_index = new_log_index
        else:
            if exclude is not None:
                for unwanted_key in exclude:
                    try:
                        del ret_log_index[unwanted_key]
                    except:
                        msg  = "WARNING:  {0} does ".format(unwanted_key)
                        msg += "not exist in log index.  Ignoring for exclude.\n"
                        print(msg)

    except KeyError as err:
        msg  = "Issue generating log_index:\n"
        msg += "    Could not find entry type with name:  {0}".format(err)
        print(msg)

    return ret_log_index

# End filter_log_index()



def gen_log_np_arrays(log_bytes, log_index):
    """Generate numpy structured arrays using the raw log and log
    index that was generated with gen_log_index().
    """
    entries_nd = dict()

    for k in log_index.keys():
        # Build a structured array with one element for each byte range enumerated above
        # Store each array in a dictionary indexed by the log entry type
        entries_nd[k] = k.generate_numpy_array(log_bytes, log_index[k])

    return entries_nd

# End gen_log_np_arrays()

def print_log_entries(log_bytes, log_index, entries_slice=None):
    """Work in progress - built for debugging address issues, some variant of this will be useful
    for creating text version of raw log w/out requiring numpy"""

    from itertools import chain
    from warpnet.wlan_exp_log.log_entries import wlan_exp_log_entry_types as entry_types
    hdr_size = 8

    if(entries_slice is not None) and (type(entries_slice) is slice):
        log_slice = entries_slice
    else:
        #Use entire log index by default
        tot_entries = sum(map(len(log_index.values())))
        log_slice = slice(0, tot_entries)

    #Create flat list of all byte offsets in log_index, sorted by offset
    # See http://stackoverflow.com/questions/18642428/concatenate-an-arbitrary-number-of-lists-in-a-function-in-python
    log_index_flat = sorted(chain.from_iterable(log_index.values()))

    for ii,entry_offset in enumerate(log_index_flat[log_slice]):

        #Look backwards for the log entry header and extract the entry type ID and size
        hdr_b = log_bytes[entry_offset-hdr_size:entry_offset]
        entry_type_id = (ord(hdr_b[4]) + (ord(hdr_b[5]) * 256))
        entry_size = (ord(hdr_b[6]) + (ord(hdr_b[7]) * 256))

        #Lookup the corresponding entry object instance (KeyError here indicates corrupt log or index)
        entry_type = entry_types[entry_type_id]

        #Use the entry_type's class method to string-ify itself
        print(entry_type.entry_as_string(log_bytes[entry_offset : entry_offset+entry_size]))

    return

def gen_hdf5_file(filename, np_log_dict, attr_dict=None, compression=None):
    """Generate an HDF5 file from numpy arrays. The np_log_dict input must be either:
    (a) A dictionary with numpy record arrays as values; each array will
        be a dataset in the HDF5 file root group
    (b) A dictionary of dictionaries like (a); each top-level value will
        be a group in the root HDF5 group, each numpy array will be a
        dataset in the group.

    attr_dict is optional. If provied, values in attr_dict will be copied to HDF5 
      group and dataset attributes. attr_dict values with keys matching np_log_dict keys
      will be used as dataset attributes named '<the_key>_INFO'.
      attr_dict entries may have an extra value with key '/', which will be used 
      as the value for a group attribute named 'INFO'.

      See examples below for supported np_log_dict and attr_dict structures.

    Examples:
    #No groups - all datasets in root group
        np_log_dict = {
            'RX_OFDM':  np_array_of_rx_etries,
            'TX':       np_array_of_tx_entries
        }

        attr_dict = {
            '/':        'Data from some_log_file.bin, node serial number W3-a-00001, written on 2014-03-18',
            'RX_OFDM':  'Filtered Rx OFDM events, only good FCS receptions',
            'TX':       'Filtered Tx events, only DATA packets'
        }

    #Two groups, with two datasets in each group
        np_log_dict = {
            'Log_Node_A': {
                'RX_OFDM':  np_array_of_rx_etries_A,
                'TX':       np_array_of_tx_entries_A
            },
            'Log_Node_B': {
                'RX_OFDM':  np_array_of_rx_etries_B,
                'TX':       np_array_of_tx_entries_B
            }
        }

        attr_dict = {
            '/':        'Written on 2014-03-18',
            'Log_Node_A': {
                '/':        'Data from node_A_log_file.bin, node serial number W3-a-00001',
                'RX_OFDM':  'Filtered Rx OFDM events, only good FCS receptions',
                'TX':       'Filtered Tx events, only DATA packets'
            }
            'Log_Node_B': {
                '/':        'Data from node_B_log_file.bin, node serial number W3-a-00002',
                'RX_OFDM':  'Filtered Rx OFDM events, only good FCS receptions',
                'TX':       'Filtered Tx events, only DATA packets'
            }
        }
    """
    import h5py

    hf = h5py.File(filename, mode='w', userblock_size=1024)

    dk = np_log_dict.keys()

    try:
        #Copy any user-supplied attributes to root group
        # h5py uses the h5py.File handle to access the file itself and the root group
        hf.attrs['INFO'] = attr_dict['/']
    except:
        pass

    if type(np_log_dict[dk[0]]) is dict:
        # np_log_dict is dictionary-of-dictionaries
        # Create an HDF5 file with one group per value in np_log_dict
        #   with one dataset per value in np_log_dict[each key]
        # This is a good structure for one dictionary containing one key-value
        #   per parsed log file, where the key is the log file name and the
        #   value is another dictionary containing the log entry arrays

        for grp_k in np_log_dict.keys():
            #Create one group per log file, using log file name as group name
            grp = hf.create_group(grp_k)
            
            try:
                grp.attrs['INFO'] = attr_dict[grp_k]['/']
            except:
                pass

            for arr_k in np_log_dict[grp_k].keys():
                #Create one dataset per numpy array of log data
                ds = grp.create_dataset(arr_k, data=np_log_dict[grp_k][arr_k], compression=compression)
                
                try:
                    ds.attrs[arr_k + '_INFO'] = attr_dict[grp_k][arr_k]
                except:
                    pass

    else:
        # np_log_dict is dictionary-of-arrays
        #   Create HDF5 file with datasets in root, one per np_log_dict[each key]

        for arr_k in np_log_dict.keys():
            # Create one dataset per numpy array of log data
            ds = hf.create_dataset(arr_k, data=np_log_dict[arr_k])
            
            try:
                ds.attrs[arr_k + '_INFO'] = attr_dict[arr_k]
            except:
                pass
    hf.close()
    return

# End gen_hdf5_file()



def log_index_print_summary(log_index, title=None):
    """Prints a summary of the log index generated by gen_log_index()."""
    total_len = 0

    if title is None:
        print('Log Index Summary:\n')
    else:
        print(title)

    for k in log_index.keys():
        print('{0:10d} of Type {1}'.format(len(log_index[k]), k))
        total_len += len(log_index[k])

    print('--------------------------')
    print('{0:10d} total entries\n'.format(total_len))

# End log_index_print_summary()



def mac_addr_int_to_str(mac_address):
    msg = ""
    addr = int(mac_address)
    if mac_address is not None:
        msg += "{0:02x}:".format(addr & 0xFF)
        msg += "{0:02x}:".format((addr >>  8) & 0xFF)
        msg += "{0:02x}:".format((addr >> 16) & 0xFF)
        msg += "{0:02x}:".format((addr >> 24) & 0xFF)
        msg += "{0:02x}:".format((addr >> 32) & 0xFF)
        msg += "{0:02x}".format((addr >> 40) & 0xFF)
    return msg

