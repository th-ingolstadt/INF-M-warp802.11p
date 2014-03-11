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
1.00a pom  1/27/14  Initial release

------------------------------------------------------------------------------

This module provides definitions for WLAN Exp Log entries

Functions (see below for more information):
    WlanExpLogEntry() -- Constructor for a log entry

Integer constants:
    WLAN_EXP_LOG_DELIM -- Wlan Exp Log delimiter
    ENTRY_TYPE_* -- Wlan Exp Log entry types

"""
from struct import calcsize, unpack, error
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
# Log Entry Type Container Class
#-----------------------------------------------------------------------------

class WlanExpLogEntryTypes:
    """Class that maintains all log entry types."""
    _log_entry_types = {}
    
    def __init__(self):
        pass

    def keys(self):
        return self._log_entry_types.keys()

    def add_entry_type(self, entry_type):
        """Add a new entry type to the container.
        
        NOTE:  This method will raise a TypeError exception if either the
        entry_name 
        """
        # Check that we do not have a name collision
        if entry_type.name in self._log_entry_types.keys():
            msg  = "Duplicate entry name.  {0} already exists.".format(entry_type.name)
            raise TypeError(msg)
        
        # Check that we do not have an ID collision
        new_id = entry_type.get_entry_type_id()
        if not new_id is None:
            for key in self._log_entry_types.keys():
                if new_id == self._log_entry_types[key].get_entry_type_id():
                    msg  = "Duplicate entry ID.  {0} already exists.".format(new_id)
                    raise TypeError(msg)
        
        # Add the entry to the log        
        self._log_entry_types[entry_type.name] = entry_type
            
    def get_entry_type_for_id(self, entry_type_id):
        for k in self._log_entry_types.keys():
            if self._log_entry_types[k].is_entry_type_id(entry_type_id):
                return self._log_entry_types[k]
        return None

    def get_entry_type_for_name(self, entry_type_name):
        for k in self._log_entry_types.keys():
            if self._log_entry_types[k].name == entry_type_name:
                return self._log_entry_types[k]
        return None

    def get_entry_type_id_for_name(self, entry_type_name):
        entry = self.get_entry_type_for_name(entry_type_name)
        if not entry is None:
            return entry.get_entry_type_id()
        return None

    def print_entry_types(self):
        msg = "Entry Types:\n"
        for entry_type in self._log_entry_types.keys():
            msg += "    Type = {0}\n".format(entry_type)
        print(msg)

# End class WlanExpLogEntries

# Global Variable
wlan_exp_log_entry_types     = dict()

#-----------------------------------------------------------------------------
# Log Entry Type Base Class
#-----------------------------------------------------------------------------

class WlanExpLogEntryType(object):
    """Base class to define a log entry type."""
    #_fields is a list of 3-tuples:
    # (field_name, field_fmt_struct, field_fmt_np)

    #_virtual_fields is nominally a list of 3-tuples:
    # (field_name, field_fmt_np, field_byte_offset)
    _fields           = []
    _virtual_fields   = []

    entry_type_id     = None
    name              = None
    
    def __init__(self, name=None, entry_type_id=None):
        #Require valid name
        if(name):
            if(name in wlan_exp_log_entry_types.keys()):
                print("WARNING: replacing exisitng WlanExpLogEntryType with name %s" % name)
            self.name = name
            wlan_exp_log_entry_types[name] = self
        else:
            raise Exception("ERROR: new WlanExpLogEntryType instance must have valid name")

        #Entry type ID is optional
        if(entry_type_id):
            #entry_type_id must be int
            if(type(entry_type_id) is not int):
                raise Exception("ERROR: WlanExpLogEntryType entry_type_id must be int")
            else:
                if(entry_type_id in wlan_exp_log_entry_types.keys()):
                    print("WARNING: replacing exisitng WlanExpLogEntryType with ID %d" % entry_type_ID)
                self.entry_type_id = entry_type_id
                wlan_exp_log_entry_types[entry_type_id] = self

        #Initialize fields to empty lists
        self._fields           = []
        self._virtual_fields   = []

    def __eq__(self, other):
        if type(other) is str:
            return self.name == other
        else:
            return (isinstance(other, self.__class__) and (self.name == other.name))

    def __ne__(self, other):
        return not self.__eq__(other)
    
    def __hash__(self):
        return hash(self.name)

    def get_field_names(self):
        return [field_name for (field_name, field_fmt_struct, field_fmt_np) in self._fields]

    def get_field_struct_formats(self):
        return [field_fmt_struct for (field_name, field_fmt_struct, field_fmt_np) in self._fields]

    def get_field_defs(self):
        return self._fields

    def get_virtual_field_defs(self):
        return self._virtual_fields
        
    def get_entry_type_id(self):
        return self._entry_type_id

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
        
    def _update_field_defs(self):
        #fields_np_dt is a numpy dtype, built using a dictionary of names/formats/sizes:
        # {'names':[field_names], 'formats':[field_formats], 'offsets':[field_offsets]}
        # We specify each field's byte offset explicitly. Byte offsets for fields in _fields
        #  are inferred, assuming tight C-struct-type packing (same assumption as struct.unpack)
        # _virtual_fields have explicit offsets that can refer to any bytes in the log entry
        
        get_np_size = (lambda f: np.dtype(f).itemsize)

        #Compute the offset of each real field, inferred by the sizes of all previous fields
        # This loop must look at all real fields, even ignored/padding fields
        sizes =  list(map(get_np_size, [field_fmt_np for (field_name, field_fmt_struct, field_fmt_np) in self._fields]))
        offsets_all = [sum(sizes[0:i]) for i in range(len(sizes))]

        #numpy processing ignores the same fields ignored by struct.unpack
        offsets = offsets_all
        np_fields = self._fields

        #BAD! Doing this filtering breaks HDF5 export
        #offsets = [o for (o,f) in zip(offsets_all, self._fields) if 'x' not in f[1]]
        #np_fields = [f for f in self._fields if 'x' not in f[1]]

        #Append the explicitly defined offsets for the virtual fields
        offsets += [field_byte_offset for (field_name, field_fmt_np, field_byte_offset) in self._virtual_fields]

        names =  [field_name for (field_name, field_fmt_struct, field_fmt_np) in np_fields]
        names += [field_name for (field_name, field_fmt_np, field_byte_offset) in self._virtual_fields]

        formats =  [field_fmt_np for (field_name, field_fmt_struct, field_fmt_np) in np_fields]
        formats += [field_fmt_np for (field_name, field_fmt_np, field_byte_offset) in self._virtual_fields]

        #print("np_dtype for %s (%d B): %s" % (self.name, np.dtype({'names':names, 'formats':formats, 'offsets':offsets}).itemsize, zip(offsets,formats,names)))

        self.fields_np_dt = np.dtype({'names':names, 'formats':formats, 'offsets':offsets})

    def is_entry_type_id(self, entry_type_id, strict=False):
        return (entry_type_id == self._entry_type_id)

    def generate_numpy_array(self, log_bytes, byte_offsets):

        index_iter = [log_bytes[o : o + self.fields_np_dt.itemsize] for o in byte_offsets]
        return np.fromiter(index_iter, self.fields_np_dt, len(byte_offsets))

    def deserialize(self, buf):
        """Unpack the buffer of a single log entry in to a dictionary."""
        ret_dict = {}
        try:
            dataTuple = unpack(self.field_fmt_struct, buf)
            all_names = self.get_field_names()
            all_fmts = self.get_field_struct_formats()

            #Filter out names for fields ignored during unpacking
            names = [n for (n,f) in zip(all_names, all_fmts) if 'x' not in f]

            return dict(zip(names, dataTuple))

        except error as err:
            print("Error unpacking buffer: {0}".format(err))
        
        return ret_dict

    def __repr__(self):
        return self.name

# End class 

#-----------------------------------------------------------------------------
# Virtual Log Entry Classes
#-----------------------------------------------------------------------------

entry_rx_common = WlanExpLogEntryType(name='RX_ALL', entry_type_id=None)
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
# Log Entry Type Classes
#-----------------------------------------------------------------------------

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

entry_exp_info = WlanExpLogEntryType(name='EXP_INFO', entry_type_id=ENTRY_TYPE_EXP_INFO)
entry_exp_info.append_field_defs([ 
            ('mac_addr',               '6s',     '6uint8'),
            ('timestamp',              'Q',      'uint64'),
            ('info_type',              'I',      'uint16'),
            ('length',                 'I',      'uint16')])

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

entry_wn_cmd_info = WlanExpLogEntryType(name='WN_CMD_INFO', entry_type_id=ENTRY_TYPE_WN_CMD_INFO)
entry_wn_cmd_info.append_field_defs([ 
            ('timestamp',              'Q',      'uint64'),
            ('command',                'I',      'uint32'),
            ('rsvd',                   'H',      'uint16'),
            ('num_args',               'H',      'uint16'),
            ('args',                   '10I',    '10uint32')])

entry_node_temperature = WlanExpLogEntryType(name='NODE_TEMPERATURE', entry_type_id=ENTRY_TYPE_NODE_TEMPERATURE)
entry_node_temperature.append_field_defs([ 
            ('timestamp',              'Q',      'uint64'),
            ('node_id',                'I',      'uint32'),
            ('serial_num',             'I',      'uint32'),
            ('temp_current',           'I',      'uint32'),
            ('temp_min',               'I',      'uint32'),
            ('temp_max',               'I',      'uint32')])

entry_rx_ofdm = WlanExpLogEntryType(name='RX_OFDM', entry_type_id=ENTRY_TYPE_RX_OFDM)
entry_rx_ofdm.append_field_defs(entry_rx_common.get_field_defs())
entry_rx_ofdm.append_field_defs([ 
            ('chan_est',               '256B',   '(64,2)i2')])

entry_rx_dsss = WlanExpLogEntryType(name='RX_DSSS', entry_type_id=ENTRY_TYPE_RX_DSSS)
entry_rx_dsss.append_field_defs(entry_rx_common.get_field_defs())

entry_tx = WlanExpLogEntryType(name='TX', entry_type_id=ENTRY_TYPE_TX)
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
#entry_tx.append_virtual_field_defs([ 
#            ('addr1',                 'uint64',      20),
#            ('addr2',                 'uint64',      26),
#            ('addr3',                 'uint64',      32)])

entry_tx_low = WlanExpLogEntryType(name='TX_LOW', entry_type_id=ENTRY_TYPE_TX_LOW)
entry_tx_low.append_field_defs([ 
            ('timestamp',              'Q',      'uint64'),
            ('mac_header',             '24s',    '24uint8'),
            ('tx_count',               'B',      'uint8'),
            ('tx_power',               'b',      'int8'),
            ('chan_num',               'B',      'uint8'),
            ('rate',                   'B',      'uint8'),
            ('length',                 'H',      'uint16'),
            ('pkt_type',               'B',      'uint8'),
            ('ant_mode',               'B',      'uint8')])

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
