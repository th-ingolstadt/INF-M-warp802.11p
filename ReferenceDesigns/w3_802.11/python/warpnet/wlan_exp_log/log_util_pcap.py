# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment Log PCAP Utilities
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
1.00a ejw  03/31/14 Initial release

------------------------------------------------------------------------------

This module provides utility functions for PCAP to handle WLAN Exp log data.

Naming convention:

  log_data       -- The binary data from a WLAN Exp node's log.
  
  log_pcap_index -- This is an index that will be used for PCAP generation.
                    Based on the selected event types, this index is a list
                    of tuples of event type ids and offsets:
                      [ (<int>, <offset>) ]

  pcap           -- A packet capture format for capturing / processing network traffic
                        http://en.wikipedia.org/wiki/Pcap
                        http://wiki.wireshark.org/Development/LibpcapFileFormat

Functions (see below for more information):
    log_data_to_pcap()            -- Generate a PCAP file based on log_data
    
"""

__all__ = ['log_data_to_pcap']


#-----------------------------------------------------------------------------
# WLAN Exp Log PCAP constants
#-----------------------------------------------------------------------------

# Global header for pcap:
#     typedef struct pcap_hdr_s {
#             guint32 magic_number;   /* magic number */
#             guint16 version_major;  /* major version number */
#             guint16 version_minor;  /* minor version number */
#             gint32  thiszone;       /* GMT to local correction */
#             guint32 sigfigs;        /* accuracy of timestamps */
#             guint32 snaplen;        /* max length of captured packets, in octets */
#             guint32 network;        /* data link type */
#     } pcap_hdr_t;
#
# pcap version is 2.4
# most defaults are based on descriptions:
#     wiki.wireshark.org/Development/LibpcapFileFormat
# network default (http://www.tcpdump.org/linktypes.html):
#     LINKTYPE_IEEE802_11	105	
# 

pcap_global_header_fmt = '<I H H I I I I'
pcap_global_header     = [('magic_number',  0xa1b2c3d4),
                          ('version_major', 2),
                          ('version_minor', 4),
                          ('thiszone',      0),
                          ('sigfigs',       0),
                          ('snaplen',       65535),
                          ('network',       105)]


# Generic packet header for pcap:
#     typedef struct pcaprec_hdr_s {
#             guint32 ts_sec;         /* timestamp seconds */
#             guint32 ts_usec;        /* timestamp microseconds */
#             guint32 incl_len;       /* number of octets of packet saved in file */
#             guint32 orig_len;       /* actual length of packet */
#     } pcaprec_hdr_t;

pcap_packet_header_fmt = '<I I I I'
pcap_packet_header     = [('ts_sec',   0),
                          ('ts_usec',  0),
                          ('incl_len', 0),
                          ('orig_len', 0)]


#-----------------------------------------------------------------------------
# WLAN Exp Log PCAP file Utilities
#-----------------------------------------------------------------------------
def gen_log_pcap_index(log_data, event_types=None):
    """Parses binary WLAN Exp log data by recording the byte index of each
    entry payload. The byte indexes are returned in a list of tuples: 
    (entry_type_id, timestamp_offset, length_offset, payload_offset).
    
    This method will only add entrys to the index if they are listed in 
    the entry_types field (ie this method will filter out any entries 
    not part of the event_types field).  By default, if no event_types
    are specified, then the default list contains:
        RX_DSSS
        RX_OFDM
        TX
        TX_LOW
    entries.  This method does not change any values in the log file itself 
    (the log_data array argument can be read-only).

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
    log_index      = []
    use_byte_array = 0


    # Create a list of entry type ids to filter the index
    import warpnet.wlan_exp_log.log_entries as log_entries
    event_type_offsets = {}

    if event_types is None:
        event_types = ['RX_DSSS', 'RX_OFDM', 'TX', 'TX_LOW' ]

    if   type(event_types) is list:
        pass
    elif type(event_types) is str:
        event_types = [event_types]
    else:
        print("ERROR:  event_types is not a list or string.")
        return log_index
    
    for event_type in event_types:
        try:
            entry         = log_entries.wlan_exp_log_entry_types[event_type]
            entry_offsets = entry.get_field_offsets()

            event_type_offsets[entry.get_entry_type_id()] = [entry_offsets['timestamp'],
                                                             entry_offsets['length'],
                                                             entry_offsets['mac_payload_len']]
        except KeyError:
            print("Could not filter log data with event type: {0}".format(event_type))


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

        # Check if entry starts with valid header.  Follow the same rules as 
        # warpnet.wlan_exp_log.log_uitl.gen_log_pcap_index()

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
            offsets = event_type_offsets[entry_type_id]
            log_index.append((entry_type_id, offset + offsets[0], offset + offsets[1], offset + offsets[2]))
        except KeyError:
            pass

        # Increment the byte offset for the next iteration
        offset += entry_size

    return log_index

# End gen_log_pcap_index()



def log_data_to_pcap(log_data, filename, event_types=None, overwrite=False):
    """Create an PCAP file that contains the log_data for the events in 

    If the requested filename already exists and overwrite==True this
    method will replace the existing file, destroying any data in the original file.

    If the filename already esists and overwrite==False this method will print a warning, 
    then create a new filename with a unique date-time suffix.

    This will iterate through the log_data to generate a pcap file based on 
    the event_types requested.
    
    Attributes:
        log_data    -- Binary WLAN Exp log data
        filename    -- File name of PCAP file to appear on disk.
        event_types -- List of WLAN Exp log event types that should appear in the file
        overwrite   -- If true method will overwrite existing file with filename
    """
    import os
    import warpnet.wlan_exp_log.log_util as log_util

    # Process the inputs to generate any error
    log_pcap_index = _process_pcap_log_data_inputs(log_data, event_types)
    
    # Determine a safe filename for the output PCAP file
    if overwrite:
        if os.path.isfile(filename):
            print("WARNING: overwriting existing file {0}".format(filename))

        pcap_filename = filename
    else:
        pcap_filename = log_util._get_safe_filename(filename)


    # Open an PCAP file in 'wb' mode    
    pf = open(pcap_filename, "wb")
    
    # Process the log data to populate the pcap file with data
    _covert_log_data_to_pcap(pf, log_data, log_pcap_index)

    # Close the file 
    pf.close()

# End log_data_to_pcap()



#-----------------------------------------------------------------------------
# Internal PCAP file Utilities
#-----------------------------------------------------------------------------
def _serialize_header(header, fmt):
    """Use the struct library to serialize the header dictionary using the given format."""
    import struct
    ret_val    = b''
    header_val = []

    for data in header:
        header_val.append(data[1])

    try:
        ret_val = struct.pack(fmt, *header_val)
    except struct.error as err:
        print("Error packing header: {0}".format(err))
    
    return ret_val

# End def



def _process_pcap_log_data_inputs(log_data, event_types=None):
    """Process the log_data and gen_index inputs to create numpy data and a log_data_index."""
    
    # Try generating the pcap index first
    #     This will catch any errors in the user-supplied log data before opening any files
    try:
        log_pcap_index = gen_log_pcap_index(log_data, event_types)
    except:
        msg  = "Unable to generate log_pcap_index"
        raise AttributeError(msg)

    return log_pcap_index

# End def



def _covert_log_data_to_pcap(file, log_data, log_pcap_index):
    """ 
    
    """
    global pcap_global_header_fmt
    global pcap_global_header

    import struct
    time_factor = 1000000        # Timestamps are in # of microseconds (ie 10^(-6) seconds)

    # Write the Global header to the file
    file.write(_serialize_header(pcap_global_header, pcap_global_header_fmt))
    
    # Iterate through the index and create a pcap entry for each item
    for item in log_pcap_index:
        # Get the timestamp (# of microseconds)
        timestamp    = struct.unpack("<Q", log_data[item[1]:(item[1] + 8)])[0]
        
        # Get the length
        orig_len     = struct.unpack("<H", log_data[item[2]:(item[2] + 2)])[0]
        
        # Get the payload
        incl_len_end = item[3] + 4
        incl_len     = struct.unpack("<I", log_data[item[3]:incl_len_end])[0]
        payload      = log_data[incl_len_end:(incl_len_end + incl_len)]
        
        # Populate the packet header
        ts_sec       = int(timestamp / time_factor)
        ts_usec      = int(timestamp % time_factor)

        fmt = '<I I I I'

        try:
            file.write(struct.pack(fmt, ts_sec, ts_usec, incl_len, orig_len))
            file.write(payload)
        except struct.error as err:
            print("Error packing packet: {0}".format(err))

# End def



























