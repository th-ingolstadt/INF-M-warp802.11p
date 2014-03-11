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
    _cache           = None
    
    def __init__(self):
        pass

    def add_entry_type(self, entry_type):
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

    def print_entry_types(self):
        msg = "Entry Types:\n"
        for entry_type in self._log_entry_types.keys():
            msg += "    Type = {0}\n".format(entry_type)
            entry = self.get_entry_from_type(entry_type)
        print(msg)

# End class WlanExpLogEntries

# Global Variable
wlan_exp_log_entry_types     = WlanExpLogEntryTypes()

#-----------------------------------------------------------------------------
# Log Entry Type Base Class
#-----------------------------------------------------------------------------

class WlanExpLogEntryType(object):
    """Base class to define a log entry type."""
    _entry_type_id    = None
    _fields           = None
    _virtual_fields   = None

    entry_name        = None
    fields_size       = None
    fields_fmt_struct = None
    fields_fmt_np     = None


    def __init__(self):
        self._fields           = []
        self._virtual_fields   = []
        self.fields_fmt_struct = ''
        self.fields_size       = 0
        self.fields_fmt_np      = []

    #_fields is a list of 3-tuples:
    # (field_name, field_fmt_struct, field_fmt_np)

    #_virtual_fields is nominally a list of 3-tuples:
    # (field_name, field_fmt_np, field_byte_offset)


    def get_field_names(self):
        return [field_name for (field_name, field_fmt_struct, field_fmt_np) in self._fields]

    def get_field_struct_formats(self):
        return [field_fmt_struct for (field_name, field_fmt_struct, field_fmt_np) in self._fields]

    def get_field_defs(self):
        return self._fields

    def get_virtual_field_defs(self):
        return self._virtual_fields

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
        #fields_fmt_struct is a string suitable for use as the format string to struct.unpack
        # (Format characters: http://docs.python.org/2/library/struct.html#format-characters)
        self.fields_fmt_struct = ' '.join(self.get_field_struct_formats())
        self.fields_size = calcsize(self.fields_fmt_struct)

        #fields_np_dt is a numpy dtype, built using a dictionary of names/formats/sizes:
        # {'names':[field_names], 'formats':[field_formats], 'offsets':[field_offsets]}
        # We specify each field's byte offset explicitly. Byte offsets for fields in _fields
        #  are inferred, assuming tight C-struct-type packing (same assumption as struct.unpack)
        # _virtual_fields have explicit offsets that can refer to any bytes in the log entry
        
        get_np_size = (lambda f: np.dtype(f).itemsize)

        #Compute the offset of each real field, inferred by the sizes of all previous fields
        # This loop must look at all real fields, even ignored/padding fields
        sizes =  map(get_np_size, [field_fmt_np for (field_name, field_fmt_struct, field_fmt_np) in self._fields])
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

    def __repr__(self):
        return 'WLAN_EXP_LOG_Entry_Type_' + self.name

# End class 

#-----------------------------------------------------------------------------
# Virtual Log Entry Classes
#-----------------------------------------------------------------------------

class Rx(WlanExpLogEntryType):
    """Receive Virtual Log Entry Type."""
    name           = 'RX_ALL'

    def __init__(self):
        super(Rx, self).__init__()

        self.append_field_defs([ 
            ('timestamp',       'Q',    'uint64'),
            ('mac_header',      '24s',  '24uint8'),
            ('length',      'H',    'uint16'),
            ('rate',        'B',    'uint8'),
            ('power',       'b',    'int8'),
            ('fcs_result',      'B',    'uint8'),
            ('pkt_type',        'B',    'uint8'),
            ('chan_num',        'B',    'uint8'),
            ('ant_mode',        'B',    'uint8'),
            ('rf_gain',     'B',    'uint8'),
            ('bb_gain',     'B',    'uint8'),
            ('padding',     '2x',   'uint16')])

Rx()
# End class 


#-----------------------------------------------------------------------------
# Log Entry Type Classes
#-----------------------------------------------------------------------------

class NodeInfo(WlanExpLogEntryType):
    """Node Info Log Entry Type."""
    _entry_type_id = ENTRY_TYPE_NODE_INFO
    name           = 'NODE_INFO'
    
    def __init__(self):
        super(NodeInfo, self).__init__()
        wlan_exp_log_entry_types.add_entry_type(self)

        self.append_field_defs([ 
            ('node_type',       'I',    'uint32'),
            ('node_id',     'I',    'uint32'),
            ('hw_generation',       'I',    'uint32'),
            ('design_ver',      'I',    'uint32'),
            ('serial_num',      'I',    'uint32'),
            ('fpga_dna',        'Q',    'uint64'),
            ('wlan_max_associations',       'I',    'uint32'),
            ('wlan_log_max_size',       'I',    'uint32'),
            ('wlan_max_stats',      'I',    'uint32')])
NodeInfo()
# End class 


class ExpInfo(WlanExpLogEntryType):
    """Experiment Info Log Entry Type."""
    _entry_type_id = ENTRY_TYPE_EXP_INFO
    name           = 'EXP_INFO'

    def __init__(self):
        super(ExpInfo, self).__init__()
        wlan_exp_log_entry_types.add_entry_type(self)

        self.append_field_defs([ 
            ('mac_addr',        '6s',   '6uint8'),
            ('timestamp',       'Q',    'uint64'),
            ('info_type',       'I',    'uint16'),
            ('length',      'I',    'uint16')])
ExpInfo()
# End class 


class StationInfo(WlanExpLogEntryType):
    """Station Info Log Entry Type."""
    _entry_type_id = ENTRY_TYPE_STATION_INFO
    name           = 'STATION_INFO'

    def __init__(self):
        super(StationInfo, self).__init__()
        wlan_exp_log_entry_types.add_entry_type(self)

        self.append_field_defs([ 
            ('timestamp',       'Q',    'uint64'),
            ('mac_addr',        '6s',   '6uint8'),
            ('host_name',       '16s',  '16uint8'),
            ('aid',     'H',    'uint16'),
            ('flags',       'I',    'uint32'),
            ('rate',        'B',    'uint8'),
            ('antenna_mode',        'B',    'uint8'),
            ('max_retry',       'B',    'uint8')])
StationInfo()
# End class 


class WNCmdInfo(WlanExpLogEntryType):
    """WARPNet Command Info Log Entry Type."""
    _entry_type_id = ENTRY_TYPE_WN_CMD_INFO
    name           = 'WN_CMD_INFO'

    def __init__(self):
        super(WNCmdInfo, self).__init__()
        wlan_exp_log_entry_types.add_entry_type(self)

        self.append_field_defs([ 
            ('timestamp',       'Q',    'uint64'),
            ('command',     'I',    'uint32'),
            ('rsvd',        'H',    'uint16'),
            ('num_args',        'H',    'uint16'),
            ('args',        '10I',  '10uint32')])

WNCmdInfo()
# End class 


class Temperature(WlanExpLogEntryType):
    """Temperature Log Entry Type."""
    _entry_type_id = ENTRY_TYPE_NODE_TEMPERATURE
    name           = 'NODE_TEMPERATURE'

    def __init__(self):
        super(Temperature, self).__init__()
        wlan_exp_log_entry_types.add_entry_type(self)

        self.append_field_defs([ 
            ('timestamp',       'Q',    'uint64'),
            ('node_id',     'I',    'uint32'),
            ('serial_num',      'I',    'uint32'),
            ('temp_current',        'I',    'uint32'),
            ('temp_min',        'I',    'uint32'),
            ('temp_max',        'I',    'uint32')])

Temperature()
# End class 


class RxOFDM(WlanExpLogEntryType):
    """Receive OFDM Log Entry Type."""
    _entry_type_id = ENTRY_TYPE_RX_OFDM
    name           = 'RX_OFDM'

    def __init__(self):
        super(RxOFDM, self).__init__()
        wlan_exp_log_entry_types.add_entry_type(self)

        #Reuse the fields from the common Rx definition
        self.append_field_defs(Rx.get_field_defs(Rx()))

        self.append_field_defs([ 
            ('chan_est',        '256B', '(64,2)i2')])

RxOFDM()
# End class 


class RxDSSS(WlanExpLogEntryType):
    """Receive DSSS Log Entry Type."""
    _entry_type_id = ENTRY_TYPE_RX_DSSS
    name           = 'RX_DSSS'

    def __init__(self):
        super(RxDSSS, self).__init__()
        
        #Reuse the fields from the common Rx definition
        self.append_field_defs(Rx.get_field_defs(Rx()))

        wlan_exp_log_entry_types.add_entry_type(self)

RxDSSS()
# End class 


class Tx(WlanExpLogEntryType):
    """Transmit Log Entry Type."""
    _entry_type_id = ENTRY_TYPE_TX
    name           = 'TX'

    def __init__(self):
        super(Tx, self).__init__()
        wlan_exp_log_entry_types.add_entry_type(self)

        self.append_field_defs([ 
            ('timestamp',       'Q',    'uint64'),
            ('time_to_accept',      'I',    'uint32'),
            ('time_to_done',        'I',    'uint32'),
            ('mac_header',      '24s',  '24uint8'),
            ('retry_count',     'B',    'uint8'),
            ('gain_target',     'B',    'uint8'),
            ('chan_num',        'B',    'uint8'),
            ('rate',        'B',    'uint8'),
            ('length',      'H',    'uint16'),
            ('result',      'B',    'uint8'),
            ('pkt_type',        'B',    'uint8'),
            ('ant_mode',        'B',    'uint8')])

# Somehow this breaks HDF5 writing...
#        self.append_virtual_field_defs([ 
#            ('addr1', 'uint64', 20),
#            ('addr2', 'uint64', 26),
#            ('addr3', 'uint64', 32)])

Tx()
# End class 

class TxLow(WlanExpLogEntryType):
    """Transmit Log Entry Type."""
    _entry_type_id = ENTRY_TYPE_TX_LOW
    name           = 'TX_LOW'

    def __init__(self):
        super(TxLow, self).__init__()
        wlan_exp_log_entry_types.add_entry_type(self)

        self.append_field_defs([ 
            ('timestamp',       'Q',    'uint64'),
            ('mac_header',      '24s',  '24uint8'),
            ('tx_count',        'B',    'uint8'),
            ('tx_power',        'b',    'int8'),
            ('chan_num',        'B',    'uint8'),
            ('rate',        'B',    'uint8'),
            ('length',      'H',    'uint16'),
            ('pkt_type',        'B',    'uint8'),
            ('ant_mode',        'B',    'uint8')])

TxLow()
# End class 


class TxRxStats(WlanExpLogEntryType):
    """Transmit Log Entry Type."""
    _entry_type_id = ENTRY_TYPE_TXRX_STATS
    name           = 'TXRX_STATS'

    def __init__(self):
        super(TxRxStats, self).__init__()
        wlan_exp_log_entry_types.add_entry_type(self)

        self.append_field_defs([ 
            ('timestamp',       'Q',    'uint64'),
            ('last_timestamp',      'Q',    'uint64'),
            ('mac_addr',        '6s',   '6uint8'),
            ('associated',      'B',    'uint8'),
            ('padding',     'x',    'uint8'),
            ('num_tx_total',        'I',    'uint32'),
            ('num_tx_success',      'I',    'uint32'),
            ('num_retry',       'I',    'uint32'),
            ('mgmt_num_rx_success',     'I',    'uint32'),
            ('mgmt_num_rx_bytes',       'I',    'uint32'),
            ('data_num_rx_success',     'I',    'uint32'),
            ('data_num_rx_bytes',       'I',    'uint32')])

TxRxStats()
# End class 

