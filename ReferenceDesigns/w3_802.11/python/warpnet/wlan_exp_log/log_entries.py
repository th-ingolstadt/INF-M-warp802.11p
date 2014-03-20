# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment Log Entries
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
1.00a pom   1/27/14 Initial release
1.01a pom   3/11/14 Major structural updates to allow better long term
                      flexibility and maintainability

------------------------------------------------------------------------------

This module provides definitions for WLAN Exp Log entries

Functions (see below for more information):
    WlanExpLogEntry()        -- Base class for a log entry

Integer constants:
    WLAN_EXP_LOG_DELIM       -- Wlan Exp Log delimiter
    ENTRY_TYPE_*             -- Wlan Exp Log entry types

Global variables:
    wlan_exp_log_entry_types -- Dictionary of 'name' -> 'type' mappings

    entry_rx_common          -- Common receive fields
    entry_node_info          -- Node Information entry
    entry_station_info       -- Station Information entry
    entry_wn_cmd_info        -- WARPNet Command Information entry
    entry_node_temperature   -- Temperature entry
    entry_rx_ofdm            -- Receive OFDM entry
    entry_rx_dsss            -- Receive DSSS entry
    entry_tx                 -- Transmit entry (CPU High)
    entry_tx_low             -- Transmit entry (CPU Low)
    entry_txrx_stats         -- Transmit / Receive Statistics entry

"""
from struct import unpack, calcsize, error
import numpy as np

# WLAN Exp Event Log Constants
#   NOTE:  The C counterparts are found in wlan_mac_event_log.h
WLAN_EXP_LOG_DELIM = 0xACED


# WLAN Exp Log Entry Constants
#   NOTE:  The C counterparts are found in wlan_mac_entries.h
ENTRY_TYPE_NODE_INFO              = 1
ENTRY_TYPE_EXP_INFO               = 2
ENTRY_TYPE_STATION_INFO           = 3
ENTRY_TYPE_NODE_TEMPERATURE       = 4
ENTRY_TYPE_WN_CMD_INFO            = 5

ENTRY_TYPE_RX_OFDM                = 10
ENTRY_TYPE_RX_DSSS                = 11

ENTRY_TYPE_TX                     = 20
ENTRY_TYPE_TX_LOW                 = 21

ENTRY_TYPE_TXRX_STATS             = 30

#-----------------------------------------------------------------------------
# Log Entry Type Container
#-----------------------------------------------------------------------------

wlan_exp_log_entry_types          = dict()

#-----------------------------------------------------------------------------
# Log Entry Type Base Class
#-----------------------------------------------------------------------------

class WlanExpLogEntryType(object):
    """Base class to define a log entry type."""
    # _fields is a list of 3-tuples:
    #     (field_name, field_fmt_struct, field_fmt_np)
    _fields           = None

    entry_type_id     = None
    name              = None

    fields_np_dt      = None
    fields_fmt_struct = None

    def __init__(self, name=None, entry_type_id=None):
        # Require valid name
        if name is not None:
            if(name in wlan_exp_log_entry_types.keys()):
                print("WARNING: replacing exisitng WlanExpLogEntryType with name {0}".format(name))
            self.name = name
            wlan_exp_log_entry_types[name] = self
        else:
            raise Exception("ERROR: new WlanExpLogEntryType instance must have valid name")

        # Entry type ID is optional
        if entry_type_id is not None:
            # entry_type_id must be int
            if type(entry_type_id) is not int:
                raise Exception("ERROR: WlanExpLogEntryType entry_type_id must be int")
            else:
                if(entry_type_id in wlan_exp_log_entry_types.keys()):
                    print("WARNING: replacing exisitng WlanExpLogEntryType with ID {0}".format(entry_type_id))
                self.entry_type_id = entry_type_id
                wlan_exp_log_entry_types[entry_type_id] = self

        # Initialize fields to empty lists
        self._fields           = []

        # Initialize unpack variables
        self.fields_fmt_struct = ''


    #-------------------------------------------------------------------------
    # Accessor methods for the WlanExpLogEntryType
    #-------------------------------------------------------------------------
    def get_field_names(self):
        return [field_name for (field_name, field_fmt_struct, field_fmt_np) in self._fields]

    def get_field_struct_formats(self):
        return [field_fmt_struct for (field_name, field_fmt_struct, field_fmt_np) in self._fields]

    def get_field_defs(self):          return self._fields

    def get_entry_type_id(self):       return self.entry_type_id

    def append_field_defs(self, field_info):
        if type(field_info) is list:
            self._fields.extend(field_info)
        else:
            self._fields.append(field_info)
        self._update_field_defs()

    #-------------------------------------------------------------------------
    # Generator methods for the WlanExpLogEntryType
    #-------------------------------------------------------------------------
    def generate_numpy_array(self, log_bytes, byte_offsets):
        """Generate a NumPy array from the log_bytes of the given
        WlanExpLogEntryType instance at the given byte_offsets.
        """
        index_iter = [log_bytes[o : o + self.fields_np_dt.itemsize] for o in byte_offsets]
        return np.fromiter(index_iter, self.fields_np_dt, len(byte_offsets))

    def entry_as_string(self, buf):
        """Generate a string representation of the entry from a buffer."""
        entry_size = calcsize(self.fields_fmt_struct)
        entry      = self.deserialize(buf[0:entry_size])[0]
                
        str_out = self.name + ': '

        for k in entry.keys():
            s = entry[k]
            if((type(s) is int) or (type(s) is long)):
                str_out += "\n    {0:25s} = {1:20d} (0x{1:16x})".format(k, s)
            elif(type(s) is str):
                s = map(ord, list(entry[k]))
                str_out += "\n    {0:25s} = [".format(k)
                for x in s:
                    str_out += "{0:d}, ".format(x)
                str_out += "\b\b]"

        str_out += "\n"

        return str_out

    def deserialize(self, buf):
        """Unpack the buffer of log entries into a list of dictionaries."""
        from collections import OrderedDict

        ret_val    = []
        buf_size   = len(buf)
        entry_size = calcsize(self.fields_fmt_struct)
        index      = 0

        while (index < buf_size):
            try:
                dataTuple = unpack(self.fields_fmt_struct, buf[index:index+entry_size])
                all_names = self.get_field_names()
                all_fmts  = self.get_field_struct_formats()

                #Filter out names for fields ignored during unpacking
                names = [n for (n,f) in zip(all_names, all_fmts) if 'x' not in f]

                #Use OrderedDict to preserve user-specified field order
                ret_val.append(OrderedDict(zip(names, dataTuple)))

            except error as err:
                print("Error unpacking {0} buffer with len {1}: {2}".format(self.name, len(buf), err))

            index += entry_size

        return ret_val


    #-------------------------------------------------------------------------
    # Internal methods for the WlanExpLogEntryType
    #-------------------------------------------------------------------------
    def _update_field_defs(self):
        """Internal method to update fields."""
        # Update the fields format used by struct unpack/calcsize
        self.fields_fmt_struct = ' '.join(self.get_field_struct_formats())

        # Update the numpy dtype definition
        # fields_np_dt is a numpy dtype, built using a dictionary of names/formats/offsets:
        #     {'names':[field_names], 'formats':[field_formats], 'offsets':[field_offsets]}
        # We specify each field's byte offset explicitly. Byte offsets for fields in _fields
        # are inferred, assuming tight C-struct-type packing (same assumption as struct.unpack)

        get_np_size = (lambda f: np.dtype(f).itemsize)

        # Compute the offset of each real field, inferred by the sizes of all previous fields
        #   This loop must look at all real fields, even ignored/padding fields
        sizes =  list(map(get_np_size, [field_fmt_np for (field_name, field_fmt_struct, field_fmt_np) in self._fields]))
        offsets_all = [sum(sizes[0:i]) for i in range(len(sizes))]

        offsets = offsets_all
        np_fields = self._fields

        # numpy processing ignores the same fields ignored by struct.unpack
        # !!!BAD!!! Doing this filtering breaks HDF5 export
        # offsets = [o for (o,f) in zip(offsets_all, self._fields) if 'x' not in f[1]]
        # np_fields = [f for f in self._fields if 'x' not in f[1]]

        names =  [field_name for (field_name, field_fmt_struct, field_fmt_np) in np_fields]

        formats =  [field_fmt_np for (field_name, field_fmt_struct, field_fmt_np) in np_fields]

        #print("np_dtype for %s (%d B): %s" % (self.name, np.dtype({'names':names, 'formats':formats, 'offsets':offsets}).itemsize, zip(offsets,formats,names)))

        self.fields_np_dt = np.dtype({'names':names, 'formats':formats, 'offsets':offsets})

    def __eq__(self, other):
        """WlanExpLogEntryType are equal if their names are equal."""
        if type(other) is str:
            return self.name == other
        else:
            return (isinstance(other, self.__class__) and (self.name == other.name))

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        return hash(self.name)

    def __repr__(self):
        return self.name

# End class


class WlanExpLogEntry_TxRx(WlanExpLogEntryType):
    """Subclass for Tx and Rx log entries which computes extra fields from the raw MAC headers
       and adds them to the numpy array. These fields must be calculated and appened to
       avoid modifying the source log bytes that the normal numpy array output references
    """
    def __init__(self, *args, **kwargs):
        super(WlanExpLogEntry_TxRx, self).__init__(*args, **kwargs)

    def generate_numpy_array(self, log_bytes, byte_offsets):
        #Use super class method first (avoids duplicating code)
        np_arr_orig = super(WlanExpLogEntry_TxRx, self).generate_numpy_array(log_bytes, byte_offsets)

        # Extend the default np_arr with convenience fields for MAC header addresses
        # IMPORTANT: np_arr uses the original bytearray as its underlying data
        # We must operate on a copy to avoid clobbering log entries adjacent to the
        #  Tx or Rx entries being extended

        # Create a new numpy dtype with additional fields
        dt_new = extend_np_dt(np_arr_orig.dtype,
                {'names': ('addr1', 'addr2', 'addr3'),
                 'formats': ('uint64', 'uint64', 'uint64')})

        #Initialize the output array (same shape, new dtype)
        np_arr_out = np.zeros(np_arr_orig.shape, dtype=dt_new)

        #Copy data from the base numpy array into the output array
        for f in np_arr_orig.dtype.names:
            #TODO: maybe don't copy fields that are ignored in the struct format?
            # problem is non-TxRx entries would still have these fields in their numpy versions
            np_arr_out[f] = np_arr_orig[f]

        #Helper array of powers of 2
        addr_conv_arr = np.uint64(2)**np.array(range(0,48,8), dtype='uint64')

        #Extract all MAC headers (each header is 24-entry uint8 array)
        mac_hdrs = np_arr_orig['mac_header']

        #Compute values for address-as-int fields using numpy's dot-product routine
        # MAC header offsets here select the 3 6-byte address fields
        np_arr_out['addr1'] = np.dot(addr_conv_arr, np.transpose(mac_hdrs[:, 4:10]))
        np_arr_out['addr2'] = np.dot(addr_conv_arr, np.transpose(mac_hdrs[:,10:16]))
        np_arr_out['addr3'] = np.dot(addr_conv_arr, np.transpose(mac_hdrs[:,16:22]))

        '''
        A quick benchmark of this implelemtation with simple log_index_gen -> numpy_array_gen
         using only ofdm_rx events and 1GB log file:
          Using superclass generate_numpy_array (i.e. no extra fields): 10.8sec
          Using this generate_numpy_array: 13.3sec
          Using this generate_numpy_array but with np_arr_out['addrX'] calls removed: 12.3sec

         The loop above to copy existing data into np_arr_out is more expensive than calculating
          new addr uint64's, but neither is hugely expensive.
        '''

        return np_arr_out

# End class

def extend_np_dt(dt_orig, new_fields=None):
    """Extends a numpy dtype object with additional fields. new_fields input must be dictionary
    with keys 'names' and 'formats', same as when specifying new dtype objects
    """

    from collections import OrderedDict

    if(type(dt_orig) is not np.dtype):
        raise Exception("ERROR: extend_np_dt requires valid numpy dtype as input")
    else:
        #Use ordered dictionary to preserve original field order (not required, just convenient)
        dt_ext = OrderedDict()

        #Extract the names/formats/offsets dictionary for the base dtype
        # dt.fields returns dictionary with field names as keys and
        #  values of (dtype, offset). The dtype objects in the first tuple field
        #  can be used as values in the 'formats' entry when creating a new dtype
        # This approach will preserve the types and dimensions of scalar and non-scalar fields
        dt_ext['names'] = list(dt_orig.names)
        dt_ext['formats'] = [dt_orig.fields[f][0] for f in dt_orig.names]

        if(type(new_fields) is dict):
            #Add new fields to the extended dtype
            dt_ext['names'].extend(new_fields['names'])
            dt_ext['formats'].extend(new_fields['formats'])
        elif type(new_fields) is not None:
            raise Exception("ERROR: new_fields argument must be dictionary with keys 'names' and 'formats'")

        #Construct and return the new numpy dtype object
        dt_new = np.dtype(dt_ext)

        return dt_new

#-----------------------------------------------------------------------------
# Virtual Log Entry Instances
#-----------------------------------------------------------------------------

entry_rx_common = WlanExpLogEntry_TxRx(name='RX_ALL', entry_type_id=None)
entry_rx_common.append_field_defs([
            ('timestamp',              'Q',      'uint64'),
            ('mac_header',             '24s',    '24uint8'),
            ('length',                 'H',      'uint16'),
            ('rate',                   'B',      'uint8'),
            ('power',                  'b',      'int8'),
            ('fcs_result',             'B',      'uint8'),
            ('pkt_type',               'B',      'uint8'),
            ('chan_num',               'B',      'uint8'),
            ('ant_mode',               'B',      'uint8'),
            ('rf_gain',                'B',      'uint8'),
            ('bb_gain',                'B',      'uint8'),
            ('padding',                '2x',     'uint16')])

#-----------------------------------------------------------------------------
# Log Entry Type Instances
#-----------------------------------------------------------------------------

# Node Info
entry_node_info = WlanExpLogEntryType(name='NODE_INFO', entry_type_id=ENTRY_TYPE_NODE_INFO)
entry_node_info.append_field_defs([
            ('node_type',              'I',      'uint32'),
            ('node_id',                'I',      'uint32'),
            ('hw_generation',          'I',      'uint32'),
            ('wn_ver',                 'I',      'uint32'),
            ('fpga_dna',               'Q',      'uint64'),
            ('serial_num',             'I',      'uint32'),
            ('wlan_exp_ver',           'I',      'uint32'),
            ('wlan_max_associations',  'I',      'uint32'),
            ('wlan_log_max_size',      'I',      'uint32'),
            ('wlan_max_stats',         'I',      'uint32')])


# Experiment Info
entry_exp_info = WlanExpLogEntryType(name='EXP_INFO', entry_type_id=ENTRY_TYPE_EXP_INFO)
entry_exp_info.append_field_defs([
            ('mac_addr',               '6s',     '6uint8'),
            ('timestamp',              'Q',      'uint64'),
            ('info_type',              'I',      'uint16'),
            ('length',                 'I',      'uint16')])


# Station Info
entry_station_info = WlanExpLogEntryType(name='STATION_INFO', entry_type_id=ENTRY_TYPE_STATION_INFO)
entry_station_info.append_field_defs([
            ('timestamp',              'Q',      'uint64'),
            ('mac_addr',               '6s',     '6uint8'),
            ('host_name',              '16s',    '16uint8'),
            ('aid',                    'H',      'uint16'),
            ('flags',                  'I',      'uint32'),
            ('rate',                   'B',      'uint8'),
            ('antenna_mode',           'B',      'uint8'),
            ('max_retry',              'B',      'uint8'),
            ('padding',                'x',      'uint8')])


# WARPNet Command Info
entry_wn_cmd_info = WlanExpLogEntryType(name='WN_CMD_INFO', entry_type_id=ENTRY_TYPE_WN_CMD_INFO)
entry_wn_cmd_info.append_field_defs([
            ('timestamp',              'Q',      'uint64'),
            ('command',                'I',      'uint32'),
            ('src_id',                 'H',      'uint16'),
            ('num_args',               'H',      'uint16'),
            ('args',                   '10I',    '10uint32')])


# Temperature
entry_node_temperature = WlanExpLogEntryType(name='NODE_TEMPERATURE', entry_type_id=ENTRY_TYPE_NODE_TEMPERATURE)
entry_node_temperature.append_field_defs([
            ('timestamp',              'Q',      'uint64'),
            ('node_id',                'I',      'uint32'),
            ('serial_num',             'I',      'uint32'),
            ('temp_current',           'I',      'uint32'),
            ('temp_min',               'I',      'uint32'),
            ('temp_max',               'I',      'uint32')])


# Receive OFDM
entry_rx_ofdm = WlanExpLogEntry_TxRx(name='RX_OFDM', entry_type_id=ENTRY_TYPE_RX_OFDM)
entry_rx_ofdm.append_field_defs(entry_rx_common.get_field_defs())
entry_rx_ofdm.append_field_defs([
            ('chan_est',               '256B',   '(64,2)i2')])


# Receive DSSS
entry_rx_dsss = WlanExpLogEntry_TxRx(name='RX_DSSS', entry_type_id=ENTRY_TYPE_RX_DSSS)
entry_rx_dsss.append_field_defs(entry_rx_common.get_field_defs())


# Transmit
entry_tx = WlanExpLogEntry_TxRx(name='TX', entry_type_id=ENTRY_TYPE_TX)
entry_tx.append_field_defs([
            ('timestamp',              'Q',      'uint64'),
            ('time_to_accept',         'I',      'uint32'),
            ('time_to_done',           'I',      'uint32'),
            ('mac_header',             '24s',    '24uint8'),
            ('num_tx',                 'B',      'uint8'),
            ('tx_power',               'b',      'int8'),
            ('chan_num',               'B',      'uint8'),
            ('rate',                   'B',      'uint8'),
            ('length',                 'H',      'uint16'),
            ('result',                 'B',      'uint8'),
            ('pkt_type',               'B',      'uint8'),
            ('ant_mode',               'B',      'uint8'),
            ('padding',                '3x',     '3uint8')])

# Transmit from CPU Low
entry_tx_low = WlanExpLogEntryType(name='TX_LOW', entry_type_id=ENTRY_TYPE_TX_LOW)
entry_tx_low.append_field_defs([
            ('timestamp',              'Q',      'uint64'),
            ('mac_header',             '24s',    '24uint8'),
            ('rate',                   'B',      'uint8'),
            ('ant_mode',               'B',      'uint8'),
            ('tx_power',               'b',      'int8'),
            ('flags',                  'B',      'uint8'),
            ('tx_count',               'B',      'uint8'),
            ('chan_num',               'B',      'uint8'),
            ('length',                 'H',      'uint16'),
            ('num_slots',              'H',      'uint16'),
            ('pkt_type',               'B',      'uint8'),
            ('padding',                'x',      'uint8')])


# Tx / Rx Statistics
entry_txrx_stats = WlanExpLogEntryType(name='TXRX_STATS', entry_type_id=ENTRY_TYPE_TXRX_STATS)
entry_txrx_stats.append_field_defs([
            ('timestamp',                      'Q',      'uint64'),
            ('last_timestamp',                 'Q',      'uint64'),
            ('mac_addr',                       '6s',     '6uint8'),
            ('associated',                     'B',      'uint8'),
            ('padding',                        'x',      'uint8'),
            ('data_num_rx_bytes',              'Q',      'uint64'),
            ('data_num_tx_bytes_success',      'Q',      'uint64'),
            ('data_num_tx_bytes_total',        'Q',      'uint64'),
            ('data_num_rx_packets',            'I',      'uint32'),
            ('data_num_tx_packets_success',    'I',      'uint32'),
            ('data_num_tx_packets_total',      'I',      'uint32'),            
            ('data_num_tx_packets_low',        'I',      'uint32'),
            ('mgmt_num_rx_bytes',              'Q',      'uint64'),
            ('mgmt_num_tx_bytes_success',      'Q',      'uint64'),
            ('mgmt_num_tx_bytes_total',        'Q',      'uint64'),
            ('mgmt_num_rx_packets',            'I',      'uint32'),
            ('mgmt_num_tx_packets_success',    'I',      'uint32'),
            ('mgmt_num_tx_packets_total',      'I',      'uint32'),            
            ('mgmt_num_tx_packets_low',        'I',      'uint32')])
