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

  hdf5           -- A data container format that we use to store log_data, 
                    log_data_index, and other user defined attributes.  You can 
                    find more documentation on HDF / HDF5 at:
                        http://www.hdfgroup.org/
                        http://www.h5py.org/

  numpy          -- A python package that allows easy and fast manipulation of 
                    large data sets.  You can find more documentaiton on numpy at:
                        http://www.numpy.org/

Functions (see below for more information):
    gen_log_data_index()               -- Generate a byte index given a WLAN Exp log file
    filter_log_index()                 -- Filter a log index with given parameters
    log_data_to_np_arrays()            -- Generate a numpy structured array (ndarray) of log entries
    np_arrays_to_hdf5()                -- Generate a HDF5 file based on numpy arrays
    log_data_to_hdf5()                 -- Generate an HDF5 file containing log_data
    hdf5_is_valid_log_data_container() -- Is the HDF5 file a valid wlan_exp_log_data_container
    hdf5_to_log_data()                 -- Extract the log data from an HDF5 file
    hdf5_to_log_data_index()           -- Extract the log_data_index from an HDF5 file
    hdf5_to_attr_dict()                -- Extract the attribute dictionary from an HDF5 file
    
"""

__all__ = ['gen_log_data_index', 
           'filter_log_index',
           'log_data_to_np_arrays',
           'np_arrays_to_hdf5',
           'log_data_to_hdf5',
           'hdf5_is_valid_log_data_container',
           'hdf5_to_log_data',
           'hdf5_to_log_data_index',
           'hdf5_to_attr_dict']


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
# WLAN Exp Log HDF5 file Utilities
#-----------------------------------------------------------------------------
def np_arrays_to_hdf5(filename, np_log_dict, attr_dict=None, compression=None):
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

    hf = h5py.File(filename, mode='w')

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

# End np_arrays_to_hdf5()



def log_data_to_hdf5(filename, log_data, attr_dict=None, gen_index=True, overwrite=False):
    """Create an HDF5 file that contains the log_data, a log_data_index, and any
    user attributes.

    If the requested filename already exists and overwrite==True this
    method will replace the existing file, destroying any data in the original file.

    If the filename already esists and overwrite==False this method will print a warning, 
    then create a new filename with a date-time suffix.
    
    For WLAN Exp log data manipulation, it is necessary to define a common file format
    so that it is easy for multiple consumers, both in python and other languages, to
    access the data.  To do this, we use HDF5 as the container format with a couple of 
    additional conventions to hold the log data as well as other pieces of information.
    Below are the rules that we follow to create an HDF5 file that will contain WLAN
    Exp log data:
    
    wlan_exp_log_data_container (equivalent to a HDF5 group):
       /: Root Group in HDF5 file
           |- Attributes:
           |      |- 'wlan_exp_log'         (1,)      bool
           |      |- 'wlan_exp_ver'         (3,)      uint32
           |      |- <user provided attributes in attr_dict>
           |- Datasets:
           |      |- 'log_data'             (1,)      voidN  (where N is the size of the data)
           |- Groups (created if gen_index==True):
                  |- 'log_data_index'
                         |- Datasets: 
                            (dtype depends if largest offset in log_data_index is < 2^32)
                                |- <int>    (N1,)     uint32/uint64
                                |- <int>    (N2,)     uint32/uint64
                                |- ...
    
    Attributes:
        filename   -- File name of HDF5 file to appear on disk.  If this file exists, then 
                      a new group will be added to the file with the specified group_name.  
                      If the file exists and group_name is not provided, then this function
                      will error out.
        log_data   -- Binary WLAN Exp log data
        attr_dict  -- An array of user provided attributes that will be added to the group.                      
        gen_index  -- Generate the 'log_data_index' from the log_data and store it in the 
                      file.
        overwrite  -- If true method will overwrite existing file with filename
    """
    import h5py

    # Process the inputs to generate any error
    (np_data, log_data_index) = _process_hdf5_log_data_inputs(log_data, gen_index)
    
    # Determine a safe filename for the output HDF5 file
    if overwrite:
        print("WARNING: overwriting existing file {0}".format(filename))
        h5_filename = filename
    else:
        h5_filename = _get_safe_filename(filename)

    # Open an HDF5 File Object in 'w' (Create file, truncate if exists) mode
    hf = h5py.File(h5_filename, mode='w')
    
    # Store all data in root group - h5py File object represents file and root group
    log_grp = hf

    # Create a wlan_exp_log_data_container in the root group
    _create_hdf5_log_data_container(log_grp, np_data, log_data_index)

    # Add the attribute dictionary to the group
    _add_attr_dict_to_group(log_grp, attr_dict)

    # Close the file 
    hf.close()

# End log_data_to_hdf5()



def log_data_to_hdf5_hier(filename, log_data, group_name, attr_dict=None, gen_index=True):
    """Add a wlan_exp_log_data_container with the given group_name to the HDF5
    file.  If the file does not exist, it will be created.
    
    Attributes:
        filename   -- File name of HDF5 file to appear on disk.  If this file exists, then 
                      a new group will be added to the file with the specified group_name.
                      Otherwise a new file will be created with the group.
        log_data   -- Binary WLAN Exp log data
        group_name -- Name of Group within the HDF5 file.  
        attr_dict  -- An array of user provided attributes that will be added to the group.                      
        gen_index  -- Generate the 'log_data_index' from the log_data and store it in the 
                      file.
    """
    import h5py
    
    # Process the inputs to generate any error
    (np_data, log_data_index) = _process_hdf5_log_data_inputs(log_data, gen_index)

    # Open a HDF5 File Object in 'a' (Read/Write if exists, create otherwise) mode
    hf = h5py.File(filename, mode='a')

    # Check if the group already exists
    if(group_name in hf.keys()):
        raise AttributeError("Group {0} already exists in file\n".format(group_name))
    
    # Store all data in the new group
    log_grp = hf.create_group(group_name)

    # Create a wlan_exp_log_data_container in the root group
    _create_hdf5_log_data_container(log_grp, np_data, log_data_index)

    # Add the attribute dictionary to the group
    _add_attr_dict_to_group(log_grp, attr_dict)
    
    # Close the file 
    hf.close()

# End log_data_to_hdf5_hier()



def hdf5_is_valid_log_data_container(filename, group_name=None):
    """Is the filename a valid wlan_exp_log_data_container

    Attributes:
        filename   -- File name of HDF5 file that appears on disk.  If this file 
                      does not exist, return False
        group_name -- Name of Group within the HDF5 file.  If not specified, then 
                      the group_name within the HDF5 file will be '/'.  If the 
                      group_name is not a valid group within the file, then the
                      function will return False
    
    Returns:
        True / False
    """
    try:
        (hf, grp) = _hdf5_extract(filename, group_name)
    except AttributeError:
        return False
    
    return True

# End hdf5_is_valid_log_data_container()



def hdf5_to_log_data(filename, group_name=None):
    """Extract the log_data from an HDF5 file that was created to the 
    wlan_exp_log_data_container format.

    Attributes:
        filename   -- File name of HDF5 file that appears on disk.  If this file 
                      does not exist, this function will raise an AttributeError
        group_name -- Name of Group within the HDF5 file.  If not specified, then 
                      the group_name within the HDF5 file will be '/'.  If the 
                      group_name is not a valid group within the file, then the
                      function will raise an AttributeError
    
    Returns:
        log_data from HDF5 file
    """
    import numpy as np

    (hf, grp) = _hdf5_extract(filename, group_name)
    
    # Get the log_data from the group data set
    try:
        ds          = grp['log_data']
        log_data_np = np.empty(shape=ds.shape, dtype=ds.dtype)
        ds.read_direct(log_data_np)
        log_data    = log_data_np.data
    except:
        msg  = "Group {0} of {1} is not a valid ".format(group_name, filename)
        msg += "wlan_exp_log_data_container."
        raise AttributeError(msg)
    
    hf.close()

    return log_data

# End log_data_to_hdf5()



def hdf5_to_log_data_index(filename, group_name=None, gen_index=True):
    """Extract the log_data from an HDF5 file that was created to the 
    wlan_exp_log_data_container format.

    Attributes:
        filename   -- File name of HDF5 file that appears on disk.  If this file 
                      does not exist, this function will raise an AttributeError
        group_name -- Name of Group within the HDF5 file.  If not specified, then 
                      the group_name within the HDF5 file will be '/'.  If the 
                      group_name is not a valid group within the file, then the
                      function will raise an AttributeError
        gen_index  -- Generate the 'log_data_index' from the log_data if the 
                      'log_data_index' is not in the file.
    
    Returns:
        log_data_index from HDF5 file
        generated log_data_index from log_data in HDF5 file
        None
    """
    import numpy as np

    error          = False
    log_data_index = {}

    (hf, grp) = _hdf5_extract(filename, group_name)
    
    # Get the log_data_index group from the specified group
    try:
        index_grp   = grp["log_data_index"]
        
        for k, v in index_grp.items():
            log_data_index[int(k)] = v.tolist()
    except:
        error = True

    # If there was an error getting the log_data_index from the file and 
    #   gen_index=True, then generate the log_data_index from the log_data
    #   in the file
    if error and gen_index:
        # Get the log_data from the group data set
        try:
            ds          = grp['log_data']
            log_data_np = np.empty(shape=ds.shape, dtype=ds.dtype)
            ds.read_direct(log_data_np)
            log_data    = log_data_np.data
        except:
            pass

        log_data_index = gen_log_data_index(log_data)
        error          = False

    if error:
        msg  = "Group {0} of {1} is not a valid ".format(group_name, filename)
        msg += "wlan_exp_log_data_container."
        raise AttributeError(msg)
    
    hf.close()

    return log_data_index

# End hdf5_to_log_data_index()



def hdf5_to_attr_dict(filename, group_name=None):
    """Extract the attributes from an HDF5 file that was created to the 
    wlan_exp_log_data_container format.

    Attributes:
        filename   -- File name of HDF5 file that appears on disk.  If this file 
                      does not exist, this function will raise an AttributeError
        group_name -- Name of Group within the HDF5 file.  If not specified, then 
                      the group_name within the HDF5 file will be '/'.  If the 
                      group_name is not a valid group within the file, then the
                      function will raise an AttributeError
    
    Returns:
        attribute dictionary in the HDF5 file
    """
    attr_dict = {}

    (hf, grp) = _hdf5_extract(filename, group_name)
    
    # Check the default attributes of the group
    try:
        for k, v in grp.attrs.items():
            attr_dict[k] = v        
    except:
        msg  = "Group {0} of {1} is not a valid ".format(group_name, filename)
        msg += "wlan_exp_log_data_container."
        raise AttributeError(msg)
    
    hf.close()

    return attr_dict

# End hdf5_to_log_data_index()



#-----------------------------------------------------------------------------
# Internal HDF5 file Utilities
#-----------------------------------------------------------------------------
def _process_hdf5_log_data_inputs(log_data, gen_index):
    """Process the log_data and gen_index inputs to create numpy data and a log_data_index."""
    import numpy as np
    
    # Try generating the index first
    #     This will catch any errors in the user-supplied log data before opening any files
    if gen_index:
        try:
            log_data_index = gen_log_data_index(log_data)
        except:
            msg  = "Unable to generate log_data_index\n"
            raise AttributeError(msg)
    else:
        log_data_index = None

    # Try creating the numpy array of binary data from log_data
    #    This will catch any errors before opening any files
    try:
        # Use numpy void datatype to store binary log data. This will assure HDF5 stores data with opaque type.
        # dtype spec is 'V100' for 100 bytes of log data
        np_dt     = np.dtype('V{0}'.format(len(log_data))) 
        np_data   = np.empty((1,), np_dt)

        # Redirect numpy array data pointer to the existing buffer object passed in by user
        np_data.data = log_data 
    except:
        msg  = "Invalid log_data object - unable to create numpy array for log_data.\n"
        raise AttributeError(msg)

    return (np_data, log_data_index)

# End _process_hdf5_log_data_inputs()



def _create_hdf5_log_data_container(group, np_data, log_data_index):
    """Create a wlan_exp_log_data_container in the given group."""
    import numpy as np
    import warpnet.wlan_exp.version as version

    # Add default attributes to the group
    group.attrs['wlan_exp_log'] = True
    group.attrs['wlan_exp_ver'] = np.array(version.wlan_exp_ver(output=False), dtype=np.uint32)

    if('log_data' in group.keys()):
        raise AttributeError("Dataset 'log_data' already exists in group {0}\n".format(group))
    
    # Create a data set for the numpy-formatted log_data (created above)
    group.create_dataset("log_data", data=np_data)

    # Add the index to the HDF5 file if necessary
    if not log_data_index is None:
        index_grp = group.create_group("log_data_index")

        for k, v in log_data_index.items():
            # Check if highest-valued entry index can be represented as uint32 or requires uint64
            if (v[-1] < 2**32):
                dtype = np.uint32
            else:
                dtype = np.uint64

            # Group names must be strings - keys here are known to be integers (entry_type_id values)
            index_grp.create_dataset(str(k), data=np.array(v, dtype=dtype))

# End _create_log_data_container()



def _add_attr_dict_to_group(group, attr_dict):
    """Add the attribute dictionary to the given group."""
    
    # Add user provided attributes to the group
    #     Try adding all attributes, even if some fail
    curr_attr_keys = group.attrs.keys()

    if not attr_dict is None:
        for k, v in attr_dict:
            try:
                if not k in curr_attr_keys:
                    if (type(k) is str):                    
                        group.attrs[k] = v
                    else:
                        print("WARNING: Converting '{0}' to string to add attribute.".format(k))
                        group.attrs[str(k)] = v
                else:
                    print("WARNING: Attribute '{0}' already exists, ignoring".format(k))
            except:
                print("WARNING: Could not add attribute '{0}' to group {1}".format(k, group))

# End _add_attr_dict_to_group()



def _hdf5_extract(filename, group_name=None):
    """Extract the file object and the group from the HDF5 file.

    Attributes:
        filename   -- File name of HDF5 file that appears on disk.  If this file 
                      does not exist, raise an AttributeError  
        group_name -- Name of Group within the HDF5 file.  If not specified, then 
                      the group_name within the HDF5 file will be '/'.  If the 
                      group_name is not a valid group within the file, then the
                      function will raise an AttributeError
    
    Returns:
        (HDF5 File object, HDF5 group)
    """
    import os
    import h5py
    import warpnet.wlan_exp.version as version
    
    # Error if inputs are not correct        
    if not os.path.isfile(filename):
        raise AttributeError("File {0} does not exist.".format(filename))
    
    # Open a HDF5 File Object in 'r' (Readonly) mode
    hf = h5py.File(filename, mode='r')
    
    # Find the group if it was specified
    try:
        if not group_name is None:
            grp        = hf[group_name]
        else:
            grp        = hf
            group_name = '/'
    except:
        raise AttributeError("Group {0} does not exist in {1}.".format(group_name, filename))


    # Check the default attributes of the group
    try:
        if grp.attrs['wlan_exp_log']:
            ver = grp.attrs['wlan_exp_ver']
            version.wlan_exp_ver_check(major=ver[0], minor=ver[1], revision=ver[2])
    except:
        msg  = "Group {0} of {1} is not a valid ".format(group_name, filename)
        msg += "wlan_exp_log_data_container."
        raise AttributeError(msg)
    
    return (hf, grp)

# End _hdf5_validation()



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



def _get_safe_filename(filename):
    """Create a 'safe' file name based on the current file name.
    
    Given the filename:  <name>.<ext>, the method will change the name the 
    filename to: <name>_<date>_<id>.<ext>, where <date> is a formatted
    string from time and <id> is a unique ID starting at zero if more 
    than one file is created in a given second.
    """
    import os
    import time

    if os.path.isfile(filename):
        # Already know it's a file, so fn_file is not ''
        (fn_fldr, fn_file) = os.path.split(filename)
        
        # Find the first '.' in the file and classify everything after that as the <ext>
        ext_i = fn_file.find('.')
        if (ext_i != -1):
            # Remember the original file extension
            fn_ext  = fn_file[ext_i:]
            fn_base = fn_file[0:ext_i]
        else:
            fn_ext  = ''
            fn_base = fn_file

        # Create a new filename
        i = 0
        while True:
            ext           = '_{0}_{1:02d}'.format(time.strftime("%Y%m%d_%H%M%S"), i)
            safe_filename = fn_fldr + fn_base + ext + fn_ext
            i            += 1

            # Break the loop if we found a unique file name
            if not os.path.isfile(safe_filename):
                break
    else:
        # File didn't exist - use name as provided
        safe_filename = filename

    return safe_filename

# End _get_safe_filename()



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





