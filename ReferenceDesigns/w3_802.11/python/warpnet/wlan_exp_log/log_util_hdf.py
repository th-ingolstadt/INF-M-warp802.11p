# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment Log HDF5 Utilities
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
1.00a ejw  03/21/14 Initial release

------------------------------------------------------------------------------

This module provides utility functions for HDF to handle WLAN Exp log data.

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
    np_arrays_to_hdf5()           -- Generate a HDF5 file based on numpy arrays
    log_data_to_hdf5()            -- Generate an HDF5 file containing log_data
    is_valid_log_data_container() -- Is the HDF5 group a valid wlan_exp_log_data_container
    hdf5_to_log_data()            -- Extract the log data from an HDF5 file
    hdf5_to_log_data_index()      -- Extract the log_data_index from an HDF5 file
    hdf5_to_attr_dict()           -- Extract the attribute dictionary from an HDF5 file
    
"""

__all__ = ['np_arrays_to_hdf5',
           'log_data_to_hdf5',
           'is_valid_log_data_container',
           'hdf5_to_log_data',
           'hdf5_to_log_data_index',
           'hdf5_to_attr_dict']


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



def log_data_to_hdf5(log_data, filename, attr_dict=None, gen_index=True, overwrite=False):
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
        filename   -- File name of HDF5 file to appear on disk.  
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
    if(attr_dict is not None):
        _add_attr_dict_to_group(log_grp, attr_dict)

    # Close the file 
    hf.close()

# End log_data_to_hdf5()



def log_data_to_hdf5_group(log_data, group_name, filename=None, h5_file=None, 
                           attr_dict=None, gen_index=True):
    """Add a wlan_exp_log_data_container with the given group_name to the HDF5
    file.  If the file does not exist, it will be created.
    
    Attributes:
        log_data   -- Binary WLAN Exp log data
        group_name -- Name of Group within the HDF5 file object
        filename   -- Name of HDF5 file to open as a h5py File object
        h5_file    -- h5py File object to use.
        attr_dict  -- Dictionary of user provided attributes that will be added 
                      to the group.                      
        gen_index  -- Generate the 'log_data_index' from the log_data and store 
                      it in the group.
    
    Either filename or h5_file must be present otherwise, the method will 
    raise an AttributeError.  Also, if both filename or h5_file is present,
    the method will raise an AttributeError.  h5py made a design decision that
    only allows a given OS file to be opened as one h5py File object.  
    Subsequent calls to open the file will result in h5py errors so we will
    not allow that to occur.
    """
    # Process the inputs to generate any error
    (np_data, log_data_index) = _process_hdf5_log_data_inputs(log_data, gen_index)

    # Open the file object
    hf    = _open_hdf5_file(filename, h5_file, readonly=False)

    # Create the new group
    try:
        log_grp = hf.create_group(group_name)
    except ValueError:
        raise AttributeError("Unable to create group {0} in file {1}\n".format(group_name, h5_file))

    # Create a wlan_exp_log_data_container in the group
    _create_hdf5_log_data_container(log_grp, np_data, log_data_index)

    # Add the attribute dictionary to the group
    if(attr_dict is not None):
        _add_attr_dict_to_group(log_grp, attr_dict)

    # Close the file if it was opened from the filename
    if filename is not None:
        hf.close()

# End log_data_to_hdf5_group()



def is_valid_log_data_container(group):
    """Is the HDF5 group a valid wlan_exp_log_data_container

    Attributes:
        group  -- HDF5 group that will be checked
    
    Returns:
        True / False
    """
    import numpy as np
    import warpnet.wlan_exp.version as version

    try:
        if group.attrs['wlan_exp_log']:
            ver = group.attrs['wlan_exp_ver']
            version.wlan_exp_ver_check(major=ver[0], minor=ver[1], revision=ver[2])
        
        if group['log_data']:
            if(group['log_data'].dtype.kind != np.dtype(np.void).kind):
                return False
    except:
        return False
    
    return True

# End is_valid_log_data_container()



def hdf5_to_log_data(filename=None, h5_file=None, group_name=None):
    """Extract the log_data from an HDF5 file that was created to the 
    wlan_exp_log_data_container format.

    Attributes:
        filename   -- Name of HDF5 file to open as a h5py File object
        h5_file    -- h5py File object to use.
        group_name -- Name of Group within the HDF5 file object
    
    Either filename or h5_file must be present.
    
    Returns:
        log_data from HDF5 file
    """
    import numpy as np

    # Open the file object
    hf    = _open_hdf5_file(filename, h5_file)

    # Extract the group
    group = _extract_hdf5_group(hf, group_name)

    # Check that the group is a valid container
    if not is_valid_log_data_container(group):
        msg  = "Group {0} of {1} is not a valid ".format(group_name, h5_file)
        msg += "wlan_exp_log_data_container."
        raise AttributeError(msg)
    
    # Get the log_data from the group data set
    ds          = group['log_data']
    log_data_np = np.empty(shape=ds.shape, dtype=ds.dtype)
    ds.read_direct(log_data_np)
    log_data    = log_data_np.data

    # Close the file if it was opened from the filename
    if filename is not None:
        hf.close()

    return log_data

# End log_data_to_hdf5()



def hdf5_to_log_data_index(filename=None, h5_file=None, group_name=None, gen_index=True):
    """Extract the log_data from an HDF5 file that was created to the 
    wlan_exp_log_data_container format.

    Attributes:
        filename   -- Name of HDF5 file to open as a h5py File object
        h5_file    -- h5py File object to use.
        group_name -- Name of Group within the HDF5 file object    
        gen_index  -- Generate the 'log_data_index' from the log_data if the 
                      'log_data_index' is not in the file.

    Either filename or h5_file must be present.
    
    Returns:
        log_data_index from HDF5 file
        generated log_data_index from log_data in HDF5 file
    """
    import warpnet.wlan_exp_log.log_util as log_util
    
    error          = False
    log_data_index = {}
    
    # Open the file object
    hf    = _open_hdf5_file(filename, h5_file)
    
    # Extract the group
    group = _extract_hdf5_group(hf, group_name)
    
    # Check that the group is a valid container
    if not is_valid_log_data_container(group):
        msg  = "Group {0} of {1} is not a valid ".format(group_name, h5_file)
        msg += "wlan_exp_log_data_container."
        raise AttributeError(msg)
        
    # Get the log_data_index group from the specified group
    try:
        index_group   = group["log_data_index"]
        
        for k, v in index_group.items():
            log_data_index[int(k)] = v.tolist()
    except:
        error = True
    
    # If there was an error getting the log_data_index from the file and 
    #   gen_index=True, then generate the log_data_index from the log_data
    #   in the file
    if error and gen_index:
        try:
            log_data       = hdf5_to_log_data(h5_file=hf, group_name=group_name)
            log_data_index = log_util.gen_log_data_index(log_data)
            error          = False
        except:
            log_data_index = None
    
    if error:
        msg  = "Group {0} of {1} is not a valid ".format(group_name, filename)
        msg += "wlan_exp_log_data_container."
        raise AttributeError(msg)
    
    # Close the file if it was opened from the filename
    if filename is not None:
        hf.close()
    
    return log_data_index

# End hdf5_to_log_data_index()



def hdf5_to_attr_dict(filename=None, h5_file=None, group_name=None):
    """Extract the attributes from an HDF5 file that was created to the 
    wlan_exp_log_data_container format.

    Attributes:
        filename   -- Name of HDF5 file to open as a h5py File object
        h5_file    -- h5py File object to use.
        group_name -- Name of Group within the HDF5 file object    

    Either filename or h5_file must be present.
    
    Returns:
        attribute dictionary in the HDF5 file
    """
    attr_dict = {}

    # Open the file object
    hf    = _open_hdf5_file(filename, h5_file)
    
    # Extract the group
    group = _extract_hdf5_group(hf, group_name)
    
    # Check that the group is a valid container
    if not is_valid_log_data_container(group):
        msg  = "Group {0} of {1} is not a valid ".format(group_name, h5_file)
        msg += "wlan_exp_log_data_container."
        raise AttributeError(msg)
    
    # Check the default attributes of the group
    try:
        for k, v in group.attrs.items():
            try:
                attr_dict[k] = v
            except:
                print("WARNING: Could not retreive attribute '{0}' from group {1}".format(k, group))
    except:
        msg  = "Group {0} of {1} is not a valid ".format(group_name, filename)
        msg += "wlan_exp_log_data_container."
        raise AttributeError(msg)
    
    # Close the file if it was opened from the filename
    if filename is not None:
        hf.close()

    return attr_dict

# End hdf5_to_log_data_index()



#-----------------------------------------------------------------------------
# Internal HDF5 file Utilities
#-----------------------------------------------------------------------------
def _process_hdf5_log_data_inputs(log_data, gen_index):
    """Process the log_data and gen_index inputs to create numpy data and a log_data_index."""
    import numpy as np
    import warpnet.wlan_exp_log.log_util as log_util
    
    # Try generating the index first
    #     This will catch any errors in the user-supplied log data before opening any files
    if gen_index:
        try:
            log_data_index = log_util.gen_log_data_index(log_data)
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
    import h5py
    import numpy as np
    import warpnet.wlan_exp.version as version

    # Add default attributes to the group
    group.attrs['wlan_exp_log'] = True
    group.attrs['wlan_exp_ver'] = np.array(version.wlan_exp_ver(), dtype=np.uint32)

    if('log_data' in group.keys() and type(group['log_data']) is h5py.Dataset):
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

    if (attr_dict is not None):
        for k, v in attr_dict.items():
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



def _open_hdf5_file(filename=None, h5_file=None, readonly=True):
    """Return an HDF5 File object.

    Attributes:    
        filename   -- Name of HDF5 file to open as a h5py File object
        h5_file    -- h5py File object to use.
        readonly   -- Open the file in 'r' (Readonly) mode or in 'a'
                      (Read/Write if exists, create otherwise) mode
                      This only applies if 'filename' is present.
    
      Either filename or h5_file must be present otherwise, the method will 
    raise an AttributeError.  Also, if both filename or h5_file is present,
    the method will raise an AttributeError.  h5py made a design decision that
    only allows a given OS file to be opened as one h5py File object.  
    Subsequent calls to open the file will result in h5py errors so we will
    not allow that to occur.        
    """
    import os
    import h5py    
    
    file_object = None

    # Check arguments
    msg  = "Invalid arguments.  Must pass either a filename or an HDF5 File object.\n"
    
    if ((filename is None) and (h5_file is None)):
        msg += "User provided neither."
        raise AttributeError(msg)
    
    if ((filename is not None) and (h5_file is not None)):
        msg += "User provided both."
        raise AttributeError(msg)

    # Process the filename if it is present
    if (filename is not None):
        if not os.path.isfile(filename):
            raise AttributeError("File {0} does not exist.".format(filename))
    
        if readonly:
            # Open a HDF5 File Object in 'r' (Readonly) mode
            file_object = h5py.File(filename, mode='r')
        else:
            # Open a HDF5 File Object in 'a' (Read/Write if exists, create otherwise) mode
            file_object = h5py.File(filename, mode='a')
    

    # Process the hdf5 File object if it is present
    if (h5_file is not None):
        # Use the HDF5 File Object
        file_object = h5_file
    
    return file_object

# End _open_hdf5_file_object()



def _extract_hdf5_group(file_object, group_name=None):
    """Extract the file object and the group from the HDF5 file.

    Attributes:
    
    Returns:
        HDF5 group
    """
    if group_name is not None:
        try:
            group = file_object[group_name]
        except:
            raise AttributeError("Group {0} does not exist in {1}.".format(group_name, file_object))
    else:
        group = file_object
    
    return group

# End _extract_hdf5_group()



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



