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


# Global Variables
wlan_exp_log_entry_types = {}


class WlanExpLogEntry:
    """Generic class to define a log entry."""
    _fields = list()
    name = ''
    fields_struct_fmt = ''
    fields_size = 0
    print_fmt = ''
    entry_type_ID = -1

    def __init__(self, entry_type_ID=-1, name=''):
        self.entry_type_ID = entry_type_ID
        self.name = name
        # Add object to global dictionary so only one object is ever created
        wlan_exp_log_entry_types[entry_type_ID] = self

    def get_field_names(self):
        return [name for type,np_type,name in self._fields]

    def get_field_struct_types(self):
        return [type for type,np_type,name in self._fields]

    def get_field_np_types(self):
        return [np_type for type,np_type,name in self._fields]

    def get_field_info(self):
        return self._fields

    def set_field_info(self, field_info):
        self._fields = field_info

        self.fields_struct_fmt = ' '.join([type for type,np_type,name in self._fields])
#        self.fields_np_dt = np.dtype([(name,np_type) for type,np_type,name in self._fields])
        self.fields_np_dt = [(name,np_type) for type,np_type,name in self._fields]
        self.fields_size = calcsize(self.fields_struct_fmt)
        return

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

# End class WlanExpLogEntry



# Initialize the global dictionary with all the log entry types defined in the 802.11 ref design
log_entry_node_info    = WlanExpLogEntry(ENTRY_TYPE_NODE_INFO, 'NODE_INFO')
log_entry_exp_info     = WlanExpLogEntry(ENTRY_TYPE_EXP_INFO, 'EXP_INFO')
log_entry_station_info = WlanExpLogEntry(ENTRY_TYPE_STATION_INFO, 'STATION_INFO')
log_entry_node_temperature    = WlanExpLogEntry(ENTRY_TYPE_NODE_TEMPERATURE, 'NODE_TEMPERATURE')
log_entry_rx_ofdm      = WlanExpLogEntry(ENTRY_TYPE_RX_OFDM, 'RX_OFDM')
log_entry_rx_dsss      = WlanExpLogEntry(ENTRY_TYPE_RX_DSSS, 'RX_DSSS')
log_entry_tx           = WlanExpLogEntry(ENTRY_TYPE_TX, 'TX')
log_entry_txrx_stats   = WlanExpLogEntry(ENTRY_TYPE_TXRX_STATS, 'TXRX_STATS')


# WLAN Exp Log Entry Definitions
#   NOTE:  The C counterparts are found in wlan_mac_entries.h

# Node info
log_entry_node_info.print_fmt = 'NODE_INFO'
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
log_entry_station_info.print_fmt = 'STATION_INFO'
log_entry_station_info.set_field_info( [
    ('Q',     'uint64',      'timestamp'),
    ('6s',    '6uint8',      'mac_addr'),
    ('16s',   '16uint8',     'host_name'),
    ('H',     'uint16',      'aid'),
    ('I',     'uint32',      'flags'),
    ('B',     'uint8',	     'rate'),
    ('B',     'uint8',	     'antenna_mode'),
    ('B',     'uint8',	     'max_retry')])


# Exp info
log_entry_exp_info.print_fmt = 'EXP_INFO'
log_entry_exp_info.set_field_info( [
    ('6s',    '6uint8',      'mac_addr'),
    ('Q',     'uint64',      'timestamp'),
    ('I',     'uint16',      'info_type'),
    ('I',     'uint16',      'length')])


# Node temperature
log_entry_node_temperature.print_fmt = 'NODE_TEMPERATURE'
log_entry_node_temperature.set_field_info( [
    ('Q',     'uint64',      'timestamp'),
    ('I',     'uint32',      'node_id'),
    ('I',     'uint32',      'serial_num'),
    ('I',     'uint32',      'temp_current'),
    ('I',     'uint32',      'temp_min'),
    ('I',     'uint32',      'temp_max')])


# OFDM transmissions
log_entry_tx.print_fmt = 'TX'
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
log_entry_rx_ofdm.print_fmt = 'RX_OFDM'
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
log_entry_rx_dsss.print_fmt = 'RX_DSSS'
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
log_entry_txrx_stats.print_fmt = 'TXRX_STATS'
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
