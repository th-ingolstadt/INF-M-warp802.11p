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


__all__ = ['WlanExpLogEntryTypes', 
           'NodeInfo', 'ExpInfo', 'StationInfo', 'Temperature',
           'Rx', 'RxOFDM', 'RxDSSS', 'Tx', 'TxRxStats']




# WLAN Exp Event Log Constants
#   NOTE:  The C counterparts are found in wlan_mac_event_log.h
WLAN_EXP_LOG_DELIM = 0xACED


# WLAN Exp Log Entry Constants
#   NOTE:  The C counterparts are found in wlan_mac_entries.h
ENTRY_TYPE_NODE_INFO              = 1
ENTRY_TYPE_EXP_INFO               = 2
ENTRY_TYPE_STATION_INFO           = 3
ENTRY_TYPE_NODE_TEMPERATURE       = 4

ENTRY_TYPE_RX_OFDM                = 10
ENTRY_TYPE_RX_DSSS                = 11

ENTRY_TYPE_TX                     = 20

ENTRY_TYPE_TXRX_STATS             = 30



#-----------------------------------------------------------------------------
# Log Entry Type Base Class
#-----------------------------------------------------------------------------

class WlanExpLogEntryType(object):
    """Base class to define a log entry type."""
    _entry_type_id    = None
    _fields           = None
    name              = None
    fields_struct_fmt = None
    fields_size       = None
    fields_np_dt      = None


    def __init__(self):
        self._fields           = []
        self.fields_struct_fmt = ''
        self.fields_size       = 0
        self.fields_np_dt      = []

    def get_field_names(self):
        return [name for struct_type, np_type, name in self._fields]

    def get_field_struct_types(self):
        return [struct_type for struct_type, np_type, name in self._fields]

    def get_field_np_types(self):
        return [np_type for struct_type, np_type, name in self._fields]

    def get_field_info(self):
        return self._fields

    def set_field_info(self, field_info):
        self._fields = field_info
        self._update_fields()

    def append_field_info(self, field_info):
        if type(field_info) is list:
            self._fields.extend(field_info)
        else:
            self._fields.append(field_info)
        self._update_fields()
        
    def _update_fields(self):
        self.fields_struct_fmt = ' '.join(self.get_field_struct_types())
        self.fields_np_dt = [(name, np_type) for struct_type, np_type, name in self._fields]
        self.fields_size = calcsize(self.fields_struct_fmt)

    def is_entry_type_id(self, entry_type_id, strict=False):
        if (entry_type_id == self._entry_type_id):
            return True
        else:
            if strict:
                return False
            else:
                for entry_class in inheritors(self.__class__):
                    if (entry_type_id == entry_class._entry_type_id):
                        return True
                return False

    def deserialize(self, buffer):
        """Unpack the buffer of a single log entry in to a dictionary."""
        ret_dict = {}
        try:
            dataTuple = unpack(self.fields_struct_fmt, buffer)
            all_names = self.get_field_names()
            names = [x for x in all_names if x != 'padding']
            for idx, data in enumerate(dataTuple):
                ret_dict[names[idx]] = data
        except error as err:
            print("Error unpacking buffer: {0}".format(err))

        return ret_dict

    def __repr__(self):
        return 'WLAN_EXP_LOG_Entry_Type_' + self.name

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
        self.append_field_info([ 
            ('I',     'uint32',      'node_type'),
            ('I',     'uint32',      'node_id'),
            ('I',     'uint32',      'hw_generation'),
            ('I',     'uint32',      'design_ver'),
            ('I',     'uint32',      'serial_num'),
            ('Q',     'uint64',      'fpga_dna'),
            ('I',     'uint32',      'wlan_max_associations'),
            ('I',     'uint32',      'wlan_log_max_size'),
            ('I',     'uint32',      'wlan_max_stats')])

# End class 


class ExpInfo(WlanExpLogEntryType):
    """Experiment Info Log Entry Type."""
    _entry_type_id = ENTRY_TYPE_EXP_INFO
    name           = 'EXP_INFO'

    def __init__(self):
        super(ExpInfo, self).__init__()
        self.append_field_info([ 
            ('6s',    '6uint8',      'mac_addr'),
            ('Q',     'uint64',      'timestamp'),
            ('I',     'uint16',      'info_type'),
            ('I',     'uint16',      'length')])

# End class 


class StationInfo(WlanExpLogEntryType):
    """Station Info Log Entry Type."""
    _entry_type_id = ENTRY_TYPE_STATION_INFO
    name           = 'STATION_INFO'

    def __init__(self):
        super(StationInfo, self).__init__()
        self.append_field_info([ 
            ('Q',     'uint64',      'timestamp'),
            ('6s',    '6uint8',      'mac_addr'),
            ('16s',   '16uint8',     'host_name'),
            ('H',     'uint16',      'aid'),
            ('I',     'uint32',      'flags'),
            ('B',     'uint8',       'rate'),
            ('B',     'uint8',       'antenna_mode'),
            ('B',     'uint8',       'max_retry')])

# End class 


class Temperature(WlanExpLogEntryType):
    """Temperature Log Entry Type."""
    _entry_type_id = ENTRY_TYPE_NODE_TEMPERATURE
    name           = 'NODE_TEMPERATURE'

    def __init__(self):
        super(Temperature, self).__init__()
        self.append_field_info([ 
            ('Q',     'uint64',      'timestamp'),
            ('I',     'uint32',      'node_id'),
            ('I',     'uint32',      'serial_num'),
            ('I',     'uint32',      'temp_current'),
            ('I',     'uint32',      'temp_min'),
            ('I',     'uint32',      'temp_max')])

# End class 


class Rx(WlanExpLogEntryType):
    """Receive Log Entry Type."""
    name           = 'RX'

    def __init__(self):
        super(Rx, self).__init__()
        self.append_field_info([ 
            ('Q',     'uint64',      'timestamp'),
            ('24s',   '24uint8',     'mac_header'),
            ('H',     'uint16',      'length'),
            ('B',     'uint8',       'rate'),
            ('b',     'int8',        'power'),
            ('B',     'uint8',       'fcs_result'),
            ('B',     'uint8',       'pkt_type'),
            ('B',     'uint8',       'chan_num'),
            ('B',     'uint8',       'ant_mode'),
            ('B',     'uint8',       'rf_gain'),
            ('B',     'uint8',       'bb_gain'),
            ('2x',    'uint16',      'padding')])

# End class 


class RxOFDM(Rx):
    """Receive OFDM Log Entry Type."""
    _entry_type_id = ENTRY_TYPE_RX_OFDM
    name           = 'RX_OFDM'

    def __init__(self):
        super(RxOFDM, self).__init__()
        self.append_field_info([ 
            ('256B',  '(64,2)i2',    'chan_est')])

# End class 


class RxDSSS(Rx):
    """Receive DSSS Log Entry Type."""
    _entry_type_id = ENTRY_TYPE_RX_DSSS
    name           = 'RX_DSSS'

    def __init__(self):
        super(RxDSSS, self).__init__()

# End class 


class Tx(WlanExpLogEntryType):
    """Transmit Log Entry Type."""
    _entry_type_id = ENTRY_TYPE_TX
    name           = 'TX'

    def __init__(self):
        super(Tx, self).__init__()
        self.append_field_info([ 
            ('Q',     'uint64',      'timestamp'),
            ('I',     'uint32',      'time_to_accept'),
            ('I',     'uint32',      'time_to_done'),
            ('24s',   '24uint8',     'mac_header'),
            ('B',     'uint8',       'retry_count'),
            ('B',     'uint8',       'gain_target'),
            ('B',     'uint8',       'chan_num'),
            ('B',     'uint8',       'rate'),
            ('H',     'uint16',      'length'),
            ('B',     'uint8',       'result'),
            ('B',     'uint8',       'pkt_type'),
            ('B',     'uint8',       'ant_mode')])

# End class 


class TxRxStats(WlanExpLogEntryType):
    """Transmit Log Entry Type."""
    _entry_type_id = ENTRY_TYPE_TXRX_STATS
    name           = 'TXRX_STATS'

    def __init__(self):
        super(TxRxStats, self).__init__()
        self.append_field_info([ 
            ('Q',     'uint64',      'timestamp'),
            ('Q',     'uint64',      'last_timestamp'),
            ('6s',    '6uint8',      'mac_addr'),
            ('B',     'uint8',       'associated'),
            ('x',     'uint8',       'padding'),
            ('I',     'uint32',      'num_tx_total'),
            ('I',     'uint32',      'num_tx_success'),
            ('I',     'uint32',      'num_retry'),
            ('I',     'uint32',      'mgmt_num_rx_success'),
            ('I',     'uint32',      'mgmt_num_rx_bytes'),
            ('I',     'uint32',      'data_num_rx_success'),
            ('I',     'uint32',      'data_num_rx_bytes')])

# End class 


#-----------------------------------------------------------------------------
# Log Entry Type Container Class
#-----------------------------------------------------------------------------

class WlanExpLogEntryTypes:
    """Class that maintains all log entry types."""
    _log_entry_types = {}
    _cache           = None
    
    def __init__(self):
        self.clear_cache()

        self.add_entry_type(NodeInfo())
        self.add_entry_type(ExpInfo())
        self.add_entry_type(StationInfo())
        self.add_entry_type(Temperature())
        self.add_entry_type(Rx())
        self.add_entry_type(RxOFDM())
        self.add_entry_type(RxDSSS())
        self.add_entry_type(Tx())
        self.add_entry_type(TxRxStats())

    def clear_cache(self):
        self._cache = {}

    def add_entry_type(self, entry_type):
        self._log_entry_types[entry_type.name] = entry_type
            
    def get_entry_types_from_id(self, entry_type_id, strict=False):
        ret_val = []
        if (entry_type_id in self._cache.keys()):
            return self._cache[entry_type_id]
        else:
            for entry_type in self._log_entry_types:
                entry = self.get_entry_from_type(entry_type)
                if entry.is_entry_type_id(entry_type_id):
                    ret_val.append(entry.name)
        self._cache[entry_type_id] = ret_val
        return ret_val

    def get_entry_from_type(self, entry_type):
        return self._log_entry_types[entry_type]

    def print_entry_types(self):
        msg = "Entry Types:\n"
        for entry_type in self._log_entry_types.keys():
            msg += "    Type = {0}\n".format(entry_type)
            entry = self.get_entry_from_type(entry_type)
            for subclass in inheritors(entry.__class__):
                msg += "        Sub Type = {0}\n".format(subclass.name)
        print(msg)

# End class WlanExpLogEntries


# Global Variable
wlan_exp_log_entry_types     = WlanExpLogEntryTypes()

log_entry_node_info          = NodeInfo()
log_entry_exp_info           = ExpInfo()
log_entry_station_info       = StationInfo()
log_entry_node_temperature   = Temperature()
log_entry_rx_ofdm            = RxOFDM()
log_entry_rx_dsss            = RxDSSS()
log_entry_tx                 = Tx()
log_entry_txrx_stats         = TxRxStats()


#-----------------------------------------------------------------------------
# Helper Function to travers class heirarchy
#-----------------------------------------------------------------------------

def inheritors(class_def):
    """Will return all sub-classes of the class_def."""
    subclasses = set()
    work = [class_def]
    while work:
        parent = work.pop()
        for child in parent.__subclasses__():
            if child not in subclasses:
                subclasses.add(child)
                work.append(child)
    return subclasses





"""
# Initialize the global dictionary with all the log entry types defined in the 802.11 ref design


# WLAN Exp Log Entry Definitions
#   NOTE:  The C counterparts are found in wlan_mac_entries.h

# Node info
log_entry_node_info.set_field_info( [
    ('I',     'uint32',      'node_type'),
    ('I',     'uint32',      'node_id'),
    ('I',     'uint32',      'hw_generation'),
    ('I',     'uint32',      'design_ver'),
    ('I',     'uint32',      'serial_num'),
    ('Q',     'uint64',      'fpga_dna'),
    ('I',     'uint32',      'wlan_max_associations'),
    ('I',     'uint32',      'wlan_log_max_size'),
    ('I',     'uint32',      'wlan_max_stats')])


# Station info
#   - In the current implementation STATION_INFO_HOSTNAME_MAXLEN = 15 bytes
log_entry_station_info.set_field_info( [
    ('Q',     'uint64',      'timestamp'),
    ('6s',    '6uint8',      'mac_addr'),
    ('16s',   '16uint8',     'host_name'),
    ('H',     'uint16',      'aid'),
    ('I',     'uint32',      'flags'),
    ('B',     'uint8',	      'rate'),
    ('B',     'uint8',	      'antenna_mode'),
    ('B',     'uint8',	      'max_retry')])


# Exp info
log_entry_exp_info.set_field_info( [
    ('6s',    '6uint8',      'mac_addr'),
    ('Q',     'uint64',      'timestamp'),
    ('I',     'uint16',      'info_type'),
    ('I',     'uint16',      'length')])


# Node temperature
log_entry_node_temperature.set_field_info( [
    ('Q',     'uint64',      'timestamp'),
    ('I',     'uint32',      'node_id'),
    ('I',     'uint32',      'serial_num'),
    ('I',     'uint32',      'temp_current'),
    ('I',     'uint32',      'temp_min'),
    ('I',     'uint32',      'temp_max')])


# OFDM transmissions
log_entry_tx.set_field_info( [
    ('Q',     'uint64',      'timestamp'),
    ('I',     'uint32',      'time_to_accept'),
    ('I',     'uint32',      'time_to_done'),
    ('24s',   '24uint8',     'mac_header'),
    ('B',     'uint8',       'retry_count'),
    ('B',     'uint8',       'gain_target'),
    ('B',     'uint8',       'chan_num'),
    ('B',     'uint8',       'rate'),
    ('H',     'uint16',      'length'),
    ('B',     'uint8',       'result'),
    ('B',     'uint8',       'pkt_type'),
    ('B',     'uint8',       'ant_mode')])


# OFDM Receptions
log_entry_rx_ofdm.set_field_info( [
    ('Q',     'uint64',      'timestamp'),
    ('24s',   '24uint8',     'mac_header'),
    ('H',     'uint16',      'length'),
    ('B',     'uint8',       'rate'),
    ('b',     'int8',        'power'),
    ('B',     'uint8',       'fcs_result'),
    ('B',     'uint8',       'pkt_type'),
    ('B',     'uint8',       'chan_num'),
    ('B',     'uint8',       'ant_mode'),
    ('B',     'uint8',       'rf_gain'),
    ('B',     'uint8',       'bb_gain'),
    ('2x',    'uint16',      'padding'),
    ('256B',  '(64,2)i2',    'chan_est')])


# DSSS Receptions
log_entry_rx_dsss.set_field_info( [
    ('Q',     'uint64',      'timestamp'),
    ('24s',   '24uint8',     'mac_header'),
    ('H',     'uint16',      'length'),
    ('B',     'uint8',       'rate'),
    ('b',     'int8',        'power'),
    ('B',     'uint8',       'fcs_result'),
    ('B',     'uint8',       'pkt_type'),
    ('B',     'uint8',       'chan_num'),
    ('B',     'uint8',       'ant_mode'),
    ('B',     'uint8',       'rf_gain'),
    ('B',     'uint8',       'bb_gain'),
    ('2x',    'uint16',      'padding')])


# TX/RX Statistics
log_entry_txrx_stats.set_field_info( [
    ('Q',     'uint64',      'timestamp'),
    ('Q',     'uint64',      'last_timestamp'),
    ('6s',    '6uint8',      'mac_addr'),
    ('B',     'uint8',       'associated'),
    ('x',     'uint8',       'padding'),
    ('I',     'uint32',      'num_tx_total'),
    ('I',     'uint32',      'num_tx_successful'),
    ('I',     'uint32',      'num_tx_total_retry'),
    ('I',     'uint32',      'num_rx_successful'),
    ('I',     'uint32',      'num_rx_bytes')])
"""