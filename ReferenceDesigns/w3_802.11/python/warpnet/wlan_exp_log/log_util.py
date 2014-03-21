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
1.00a pom  01/27/14 Initial release
1.10a ejw  03/21/14 Major restructuring / documenting

------------------------------------------------------------------------------

This module provides utility functions for handling WLAN Exp log data.

Naming convention:

  log_data       -- The binary data from a WLAN Exp node's log.
  
  log_data_index -- This is an index that has not been interpreted / filtered
                    and corresponds 1-to-1 with what is in given log_data.
                    The defining characteristic of a log_data_index is that
                    the dictionary keys are all integers:
                      { <int> : [<offsets>] }

  log_index      -- A log_index is any index that is not a log_data_index.  In
                    general, this will be a interpreted / filtered version of
                    a log_data_index.

  numpy          -- A python package that allows easy and fast manipulation of 
                    large data sets.  You can find more documentaiton on numpy at:
                        http://www.numpy.org/

Functions (see below for more information):
    gen_log_data_index()          -- Generate a byte index given a WLAN Exp log file
    filter_log_index()            -- Filter a log index with given parameters
    log_data_to_np_arrays()       -- Generate a numpy structured array (ndarray) of log entries
    
"""

__all__ = ['gen_log_data_index', 
           'filter_log_index',
           'log_data_to_np_arrays']


#-----------------------------------------------------------------------------
# WLAN Exp Log Utilities
#-----------------------------------------------------------------------------
def gen_log_data_index(log_data, return_valid_slice=False):
    """Parses binary WLAN Exp log data by recording the byte index of each
    entry. The byte indexes are returned in a dictionary with the entry
    type IDs as keys. This method does not unpack or interpret each log
    entry and does not change any values in the log file itself (the
    log_data array argument can be read-only).

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
    log_len        = len(log_data)
    log_index      = dict()
    use_byte_array = 0

    # Need to determine if we are using byte arrays or strings for the
    # log_bytes b/c we need to handle the data differently
    try:
        byte_array_test = log_data[offset:offset+hdr_size]
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
        hdr_b = log_data[offset:offset+hdr_size]

        if (use_byte_array):
            if( (bytearray(hdr_b[2:4]) != b'\xed\xac') ):
                raise Exception("ERROR: Log file didn't start with valid entry header (offset %d)!" % (offset))

            entry_type_id = (hdr_b[4] + (hdr_b[5] * 256))
            entry_size = (hdr_b[6] + (hdr_b[7] * 256))
        else:
            if( (hdr_b[2:4] != b'\xed\xac') ):
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
                    except KeyError:
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



def log_data_to_np_arrays(log_data, log_index):
    """Generate numpy structured arrays using log_data and a log_index."""
    entries_nd = dict()

    for k in log_index.keys():
        # Build a structured array with one element for each byte range enumerated above
        # Store each array in a dictionary indexed by the log entry type
        entries_nd[k] = k.generate_numpy_array(log_data, log_index[k])

    return entries_nd

# End gen_log_np_arrays()



#-----------------------------------------------------------------------------
# WLAN Exp Log Printing Utilities
#-----------------------------------------------------------------------------
def print_log_index_summary(log_index, title=None):
    """Prints a summary of the log_index."""
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

# End print_log_entries()



#-----------------------------------------------------------------------------
# Deprecated methods
#-----------------------------------------------------------------------------

def write_log_data_index_file(log_data_file, node=None):
    """Method to write a log data index to a file for easy retrieval.
    
    Attributes:
        log_data_file -- File name of the binary log_data file
        node          -- Optional node binary log data came from
    
    Also, a 'wlan_exp_log_index_info' entry will be created in the 
    index that captures information about the framework:
    
    'wlan_exp_log_index_info' = {'filename'           = <str>,
                                 'date'               = <str>,
                                 'wlan_exp_version'   = <str>}

    If node is present, then the following info will also be present:

    'wlan_exp_log_index_info' = {'node_serial_number' = <str>,
                                 'node_name'          = <str> }

    """
    import os
    import time
    import pickle
    import warpnet.wlan_exp.version as version

    # Make sure we have a valid file and create the index file name    
    if os.path.isfile(log_data_file):
        idx_log_file = log_data_file + ".idx"
    else:
        raise TypeError("Filename {0} is not valid.".format(log_data_file))

    # Read the binary log file
    with open(log_data_file, 'rb') as fh:
        log_b = fh.read()
    
    # Generate the raw index of log entry locations sorted by log entry type
    log_data_index = gen_log_data_index(log_b)

    log_data_index['wlan_exp_log_index_info'] = {
        'filename'         : log_data_file,
        'date'             : time.strftime("%d/%m/%Y  %H:%M:%S"),
        'wlan_exp_version' : version.wlan_exp_ver_str()
    }

    # If the node is present, then add the additional fields
    if not node is None:
        log_data_index['wlan_exp_log_index_info'] = {
            'node_serial_number' : node.sn_str,
            'node_name'          : node.name
        }
    
    pickle.dump(log_data_index, open(idx_log_file, "wb"))

# End gen_log_index_raw()



def read_log_data_index_file(log_data_index_file):
    import pickle
    import warpnet.wlan_exp.version as version
    import warpnet.wn_exception as wn_ex
    
    log_data_index = pickle.load(open(log_data_index_file, "rb"))

    # Check the version in the log index agains the current version
    try:
        version.wlan_exp_ver_check(log_data_index['wlan_exp_log_index_info']['wlan_exp_version'])
    except wn_ex.VersionError as err:
        print(err)

    return log_data_index
    
# End gen_log_index_raw()





