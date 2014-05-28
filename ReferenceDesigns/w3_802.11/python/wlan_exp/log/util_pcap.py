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

__all__ = []


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
def _log_data_to_pcap(log_data, log_index, filename, overwrite=False):
    """Create an PCAP file that contains the log_data for the entries in log_index 

    If the requested filename already exists and overwrite==True this
    method will replace the existing file, destroying any data in the original file.

    If the filename already esists and overwrite==False this method will print a warning, 
    then create a new filename with a unique date-time suffix.
    
    NOTE:  Currently log_data_to_pcap only supports ['RX_DSSS', 'RX_OFDM', 'TX', 'TX_LOW']
    entry types.  If other entry types are contained within the log_index, they will
    be ignored but a warning will be printed.
    
    Attributes:
        log_data    -- Binary WLAN Exp log data
        log_index   -- Filtered log index
        filename    -- File name of PCAP file to appear on disk.
        overwrite   -- If true method will overwrite existing file with filename
    """
    import os
    from . import util as log_util

    # Process the inputs to generate any error
    log_pcap_index = _gen_pcap_log_index(log_index)
    
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
def _gen_pcap_log_index(log_index):
    """Uses a log index to create a pcap log index.

    For each supported entry in the log index, an entry is created in the
    pcap_log_index that is a list of tuples: 
    (entry_type_id, timestamp_offset, length_offset, payload_offset).

    Currently supported entry_types are:    
        RX_DSSS
        RX_OFDM
        TX
        TX_LOW
    """
    pcap_log_index = []

    # Create a list of entry type ids to filter the index
    from . import entry_types
    entry_type_offsets = {}

    supported_entry_types = ['RX_DSSS', 'RX_OFDM', 'TX', 'TX_LOW' ]
    
    for entry_type in supported_entry_types:
        try:
            entry         = entry_types.log_entry_types[entry_type]
            entry_offsets = entry.get_field_offsets()

            entry_type_offsets[entry_type] = [entry_offsets['timestamp'],
                                              entry_offsets['length'],
                                              entry_offsets['mac_payload_len']]
        except KeyError:
            print("Could not filter log data with event type: {0}".format(entry_type))


    # Create the raw pcap log index
    for entry_type in log_index.keys():
        try: 
            offsets = entry_type_offsets[entry_type]
            for offset in log_index[entry_type]:
                pcap_log_index.append((entry_type, offset + offsets[0], offset + offsets[1], offset + offsets[2]))
        except KeyError:
            print("Can not use entry type: {0} in PCAP generation.".format(entry_type))

    
    # Sort the PCAP data
    from operator import itemgetter
    
    sorted(pcap_log_index, key=itemgetter(1))

    return pcap_log_index

# End _gen_log_pcap_index()



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



def _covert_log_data_to_pcap(file, log_data, log_pcap_index):
    """ 
    
    """
    global pcap_global_header_fmt
    global pcap_global_header

    import struct
    time_factor = 1000000        # Timestamps are in # of microseconds (ie 10^(-6) seconds)

    # Write the Global header to the file
    file.write(_serialize_header(pcap_global_header, pcap_global_header_fmt))
    
    
    # TODO:  Need to re-order the index by timestamp so the events are in time order
    
    
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



























