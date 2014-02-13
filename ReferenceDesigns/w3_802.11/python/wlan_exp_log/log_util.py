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
    gen_log_ndarrays() -- Generate an array of 

"""


def gen_log_index(log_bytes):
    """Parses a binary WARPnet log file recording the byte index of each 
    event. The byte indexes are returned in a dictionary with the event 
    type IDs as keys. This method does not unpack or interpret each log 
    entry and does not change any values in the log file itself (the 
    log_bytes array argument can be read-only).

    Format of log entry header:

        typedef struct{
            u32 entry_type_id;
            u16 entry_type;
            u16 entry_length;
        } entry_header;

    fmt_log_hdr = 'I H H' #if we were using struct.unpack
    """

    offset = 0
    hdr_size = 8
    num_entries = 0

    log_index = dict()

    while True:
        # Stop here if the next log entry header is incomplete
        if( (offset + hdr_size) > len(log_bytes)):
            break

        # Check if event starts with valid header.  struct.unpack is the 
        # natural way to interpret the entry header, but it's slower
        # than accessing the bytes directly.

        # hdr = unpack(fmt_log_hdr, log_bytes[offset:offset+hdr_size])
        # ltk = hdr[1]
        # if( (hdr[0] & wn_entries.WN_LOG_DELIM) != wn_entries.WN_LOG_DELIM):
        #     raise Exception("ERROR: Log file didn't start with valid event header!")

        # Use raw byte slicing for better performance
        # Values below are hard coded to match current WLAN Exp log entry formats
        hdr_b = log_bytes[offset:offset+hdr_size]

        if( (hdr_b[2] != 0xED) | (hdr_b[3] != 0xAC) ):
            raise Exception("ERROR: Log file didn't start with valid event header (offset %d)!" % (offset))

        entry_type_id = (hdr_b[4] + (hdr_b[5] * 256))
        entry_size = (hdr_b[6] + (hdr_b[7] * 256))
        offset += hdr_size

        # Stop here if the last log entry is incomplete
        if( (offset + entry_size) > len(log_bytes)):
            break
	
        # Record the starting byte offset of this entry in the log index
        if(entry_type_id in log_index.keys()):
            log_index[entry_type_id].append(offset)
        else:
            log_index[entry_type_id] = [offset]

        # Increment the byte offset for the next iteration
        offset += entry_size

        num_entries += 1

    print('Parsed %d entries:' % num_entries)
    for k in log_index.keys():
        print(' %5d of Type %d' % (len(log_index[k]), k))

    return log_index

# End gen_log_index()


def gen_log_ndarrays(log_bytes, log_index):
    """Generate structured arrays of data in numpy from the log and log 
    index that was generated with gen_log_index().
    """

    import numpy as np
    from wn_log import wn_entries

    entries_nd = dict()

    for k in log_index.keys():
        log_type = wn_entries.log_entry_types[k]

        # Construct the list of byte ranges for this type of log entry
        index_iter = [log_bytes[o : o + log_type.fields_size] for o in log_index[log_type.event_type_ID]]

        # Build a structured array with one element for each byte range enumerated above
        # Store each array in a dictionary indexed by the log entry type
        entries_nd[k] = np.fromiter(index_iter, log_type.fields_np_dt, len(log_index[log_type.event_type_ID]))
	
    return entries_nd

# End gen_log_ndarrays()

