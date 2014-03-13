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

    # _virtual_fields is nominally a list of 3-tuples:
    #     (field_name, field_fmt_np, field_byte_offset)
    _virtual_fields   = None

    entry_type_id     = None
    name              = None
    
    fields_np_dt      = None
    fields_fmt_struct = None
    fields_size       = None
    
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
        self._virtual_fields   = []
        
        # Initialize unpack variables
        self.fields_fmt_struct = ''
        self.fields_size       = 0


    #-------------------------------------------------------------------------
    # Accessor methods for the WlanExpLogEntryType
    #-------------------------------------------------------------------------
    def get_field_names(self):
        return [field_name for (field_name, field_fmt_struct, field_fmt_np) in self._fields]

    def get_field_struct_formats(self):
        return [field_fmt_struct for (field_name, field_fmt_struct, field_fmt_np) in self._fields]

    def get_field_defs(self):          return self._fields
    def get_virtual_field_defs(self):  return self._virtual_fields        
    def get_entry_type_id(self):       return self.entry_type_id
    def get_entry_type_size(self):     return self.fields_size

    def append_field_defs(self, field_info):
        if type(field_info) is list:
            self._fields.extend(field_info)
        else:
            self._fields.append(field_info)
        self._update_field_defs()

    def append_virtual_field_defs(self, field_info):
        if type(field_info) is list:
            self._virtual_fields.extend(field_info)
        else:
            self._virtual_fields.append(field_info)
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


    def deserialize(self, buf):
        """Unpack the buffer of a single log entry in to a dictionary."""
        ret_dict = {}
        try:
            dataTuple = unpack(self.fields_fmt_struct, buf)
            all_names = self.get_field_names()
            all_fmts  = self.get_field_struct_formats()

            #Filter out names for fields ignored during unpacking
            names = [n for (n,f) in zip(all_names, all_fmts) if 'x' not in f]

            return dict(zip(names, dataTuple))

        except error as err:
            print("Error unpacking buffer: {0}".format(err))
        
        return ret_dict


    #-------------------------------------------------------------------------
    # Internal methods for the WlanExpLogEntryType
    #-------------------------------------------------------------------------
    def _update_field_defs(self):
        """Internal method to update fields."""
        # fields_np_dt is a numpy dtype, built using a dictionary of names/formats/sizes:
        #     {'names':[field_names], 'formats':[field_formats], 'offsets':[field_offsets]}
        # We specify each field's byte offset explicitly. Byte offsets for fields in _fields
        # are inferred, assuming tight C-struct-type packing (same assumption as struct.unpack)
        # _virtual_fields have explicit offsets that can refer to any bytes in the log entry
        
        get_np_size = (lambda f: np.dtype(f).itemsize)

        # Compute the offset of each real field, inferred by the sizes of all previous fields
        #   This loop must look at all real fields, even ignored/padding fields
        sizes =  list(map(get_np_size, [field_fmt_np for (field_name, field_fmt_struct, field_fmt_np) in self._fields]))
        offsets_all = [sum(sizes[0:i]) for i in range(len(sizes))]

        # numpy processing ignores the same fields ignored by struct.unpack
        offsets = offsets_all
        np_fields = self._fields

        # !!!BAD!!! Doing this filtering breaks HDF5 export
        # offsets = [o for (o,f) in zip(offsets_all, self._fields) if 'x' not in f[1]]
        # np_fields = [f for f in self._fields if 'x' not in f[1]]

        # Append the explicitly defined offsets for the virtual fields
        offsets += [field_byte_offset for (field_name, field_fmt_np, field_byte_offset) in self._virtual_fields]

        names =  [field_name for (field_name, field_fmt_struct, field_fmt_np) in np_fields]
        names += [field_name for (field_name, field_fmt_np, field_byte_offset) in self._virtual_fields]

        formats =  [field_fmt_np for (field_name, field_fmt_struct, field_fmt_np) in np_fields]
        formats += [field_fmt_np for (field_name, field_fmt_np, field_byte_offset) in self._virtual_fields]

        #print("np_dtype for %s (%d B): %s" % (self.name, np.dtype({'names':names, 'formats':formats, 'offsets':offsets}).itemsize, zip(offsets,formats,names)))

        self.fields_np_dt = np.dtype({'names':names, 'formats':formats, 'offsets':offsets})

        # Update the unpack fields
        self.fields_fmt_struct = ' '.join(self.get_field_struct_formats())
        self.fields_size       = calcsize(self.fields_fmt_struct)


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
    """Subclass for Tx and Rx log entries to properly compute values
    for 48-bit address fields. Numpy uses u64 for these values, which includes
    two extra non-MAC-address bytes from the binary log. The overridden generate_numpy_array
    below applies a 48-bit mask to each virtual addrX field before returning
    """
    def __init__(self, *args, **kwargs):
        super(WlanExpLogEntry_TxRx, self).__init__(*args, **kwargs)

    def generate_numpy_array(self, log_bytes, byte_offsets):
        #Use super class method first (avoids duplicating code)
        np_arr = super(WlanExpLogEntry_TxRx, self).generate_numpy_array(log_bytes, byte_offsets)

        #Mask each addr field to 48 bits
        np_arr['addr1'] = np.bitwise_and(np_arr['addr1'], 2**48-1)
        np_arr['addr2'] = np.bitwise_and(np_arr['addr2'], 2**48-1)
        np_arr['addr3'] = np.bitwise_and(np_arr['addr3'], 2**48-1)

        return np_arr

# End class 


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
entry_rx_common.append_virtual_field_defs([ 
            ('addr1',                  'uint64',      20),
            ('addr2',                  'uint64',      26),
            ('addr3',                  'uint64',      32)])


#-----------------------------------------------------------------------------
# Log Entry Type Instances
#-----------------------------------------------------------------------------

# Node Info
entry_node_info = WlanExpLogEntryType(name='NODE_INFO', entry_type_id=ENTRY_TYPE_NODE_INFO)
entry_node_info.append_field_defs([ 
            ('node_type',              'I',      'uint32'),
            ('node_id',                'I',      'uint32'),
            ('hw_generation',          'I',      'uint32'),
            ('design_ver',             'I',      'uint32'),
            ('serial_num',             'I',      'uint32'),
            ('fpga_dna',               'Q',      'uint64'),
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
            ('max_retry',              'B',      'uint8')])


# WARPNet Command Info
entry_wn_cmd_info = WlanExpLogEntryType(name='WN_CMD_INFO', entry_type_id=ENTRY_TYPE_WN_CMD_INFO)
entry_wn_cmd_info.append_field_defs([ 
            ('timestamp',              'Q',      'uint64'),
            ('command',                'I',      'uint32'),
            ('rsvd',                   'H',      'uint16'),
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

entry_rx_ofdm.append_virtual_field_defs(entry_rx_common.get_virtual_field_defs())

entry_rx_ofdm.append_field_defs(entry_rx_common.get_field_defs())
entry_rx_ofdm.append_field_defs([ 
            ('chan_est',               '256B',   '(64,2)i2')])


# Receive DSSS
entry_rx_dsss = WlanExpLogEntry_TxRx(name='RX_DSSS', entry_type_id=ENTRY_TYPE_RX_DSSS)

entry_rx_dsss.append_virtual_field_defs(entry_rx_common.get_virtual_field_defs())

entry_rx_dsss.append_field_defs(entry_rx_common.get_field_defs())


# Transmit
entry_tx = WlanExpLogEntry_TxRx(name='TX', entry_type_id=ENTRY_TYPE_TX)
entry_tx.append_field_defs([ 
            ('timestamp',              'Q',      'uint64'),
            ('time_to_accept',         'I',      'uint32'),
            ('time_to_done',           'I',      'uint32'),
            ('mac_header',             '24s',    '24uint8'),
            ('retry_count',            'B',      'uint8'),
            ('gain_target',            'B',      'uint8'),
            ('chan_num',               'B',      'uint8'),
            ('rate',                   'B',      'uint8'),
            ('length',                 'H',      'uint16'),
            ('result',                 'B',      'uint8'),
            ('pkt_type',               'B',      'uint8'),
            ('ant_mode',               'B',      'uint8')])
# Somehow this breaks HDF5 writing...
entry_tx.append_virtual_field_defs([ 
            ('addr1',                  'uint64',      20),
            ('addr2',                  'uint64',      26),
            ('addr3',                  'uint64',      32)])


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
            ('timestamp',              'Q',      'uint64'),
            ('last_timestamp',         'Q',      'uint64'),
            ('mac_addr',               '6s',     '6uint8'),
            ('associated',             'B',      'uint8'),
            ('padding',                'x',      'uint8'),
            ('num_tx_total',           'I',      'uint32'),
            ('num_tx_success',         'I',      'uint32'),
            ('num_retry',              'I',      'uint32'),
            ('mgmt_num_rx_success',    'I',      'uint32'),
            ('mgmt_num_rx_bytes',      'I',      'uint32'),
            ('data_num_rx_success',    'I',      'uint32'),
            ('data_num_rx_bytes',      'I',      'uint32')])
