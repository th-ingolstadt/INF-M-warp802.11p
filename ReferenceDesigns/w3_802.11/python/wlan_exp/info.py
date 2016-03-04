# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Structured Information classes
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2015, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
MODIFICATION HISTORY:

Ver   Who  Date     Changes
----- ---- -------- -----------------------------------------------------
1.00a ejw  10/15/15 Initial release

------------------------------------------------------------------------------

This module provides class definitions for information classes.

Classes (see below for more information):
    Info()            -- Base class for information classes
    StationInfo()     -- Base class for station information
    BSSInfo()         -- Base class for basic service set (BSS) information
    CountsInfo()      -- Base class for information on node counts

"""
import struct

import wlan_exp.util as util

__all__ = ['StationInfo', 'BSSInfo', 'TxRxCounts']


#-------------------------------------------------------------------------
# Field definitions for Information structures
#
# The info_field_defs is a dictionary of field defintions whose key is the
# field name and whose value is an ordered list of tuples of the form:
#    (field name, struct format, numpy format, description)
#
# where:
#     field name     - Text name of the field
#     struct format  - Python struct format identifier
#     numpy format   - Numpy format identifier
#     description    - Text description of the field
#
# These tuples can be extended by sub-classes, but the first four fields are
# required in order to efficiently serialize / deserialize information objects
# for communication over the transport.
#
# NOTE:  This is to reduce the size of the individual objects and to make it
#     easier to maintain all the field defintiions
#
# NOTE:  These definitions match the corresponding definitions in the WLAN
#     Exp framework in C.
#
#-------------------------------------------------------------------------
info_field_defs = {
    'STATION_INFO' : [
        ('timestamp',                   'Q',      'uint64',  'Microsecond timer value at time of creation'),
        ('mac_addr',                    '6s',     '6uint8',  'MAC address of associated device'),
        ('aid',                         'H',      'uint16',  'Association ID (AID) of device'),
        ('host_name',                   '20s',    '20uint8', 'String hostname (19 chars max), taken from DHCP DISCOVER packets'),
        ('flags',                       'I',      'uint32',  'Association state flags: ???'),
        ('latest_activity_timestamp',   'Q',      'uint64',  'Microsecond timer value at time of last successful Rx from device'),
        ('rx_last_seq',                 'H',      'uint16',  'Sequence number of last packet received from device'),
        ('rx_last_power',               'b',      'int8',    'Rx power in dBm of last packet received from device'),
        ('padding0',                    'x',      'uint8',   ''),
        ('tx_phy_mcs',                  'B',      'uint8',   'Current PHY MCS index in [0:7] for new transmissions to device'),
        ('tx_phy_mode',                 'B',      'uint8',   'Current PHY mode for new transmissions to deivce'),
        ('tx_phy_antenna_mode',         'B',      'uint8',   'Current PHY antenna mode in [1:4] for new transmissions to device'),
        ('tx_phy_power',                'b',      'int8',    'Current Tx power in dBm for new transmissions to device'),
        ('tx_mac_flags',                'B',      'uint8',   'Flags for Tx MAC config for new transmissions to device'),
        ('padding1',                    '3x',     '3uint8',  '')],

    'BSS_INFO' : [
        ('timestamp',                   'Q',      'uint64',  'Microsecond timer value at time of creation'),
        ('bssid',                       '6s',     '6uint8',  'BSS ID'),
        ('chan_num',                    'B',      'uint8',   'Channel (center frequency) index of transmission'),
        ('flags',                       'B',      'uint8',   'BSS flags'),
        ('latest_activity_timestamp',   'Q',      'uint64',  'Microsecond timer value at time of last Tx or Rx event to node with address mac_addr'),
        ('ssid',                        '33s',    '33uint8', 'SSID (32 chars max)'),
        ('state',                       'B',      'uint8',   'State of the BSS'),
        ('capabilities',                'H',      'uint16',  'Supported capabilities of the BSS'),
        ('beacon_interval',             'H',      'uint16',  'Beacon interval - In time units of 1024 us'),
        ('padding0',                    'x',      'uint8',   ''),
        ('num_basic_rates',             'B',      'uint8',   'Number of basic rates supported'),
        ('basic_rates',                 '10s',    '10uint8', 'Supported basic rates'),
        ('phy_mode',                    'B',      'uint8',   'PHY Mode (Legacy, HT)'),
        ('rx_last_power',               'b',      'int8',    'Last observed Rx Power (dBm)')],

    'TXRX_COUNTS' : [
        ('timestamp',                   'Q',      'uint64',  'Microsecond timer value at time of creation'),
        ('mac_addr',                    '6s',     '6uint8',  'MAC address of remote node whose statics are recorded here'),
        ('associated',                  'B',      'uint8',   'Boolean indicating whether remote node is currently associated with this node'),
        ('padding',                     'x',      'uint8',   ''),
        ('data_num_rx_bytes',           'Q',      'uint64',  'Total number of bytes received in DATA packets from remote node'),
        ('data_num_tx_bytes_success',   'Q',      'uint64',  'Total number of bytes successfully transmitted in DATA packets to remote node'),
        ('data_num_tx_bytes_total',     'Q',      'uint64',  'Total number of bytes transmitted (successfully or not) in DATA packets to remote node'),
        ('data_num_rx_packets',         'I',      'uint32',  'Total number of DATA packets received from remote node'),
        ('data_num_tx_packets_success', 'I',      'uint32',  'Total number of DATA packets successfully transmitted to remote node'),
        ('data_num_tx_packets_total',   'I',      'uint32',  'Total number of DATA packets transmitted (successfully or not) to remote node'),
        ('data_num_tx_packets_low',     'I',      'uint32',  'Total number of PHY transmissions of DATA packets to remote node (includes re-transmissions)'),
        ('mgmt_num_rx_bytes',           'Q',      'uint64',  'Total number of bytes received in management packets from remote node'),
        ('mgmt_num_tx_bytes_success',   'Q',      'uint64',  'Total number of bytes successfully transmitted in management packets to remote node'),
        ('mgmt_num_tx_bytes_total',     'Q',      'uint64',  'Total number of bytes transmitted (successfully or not) in management packets to remote node'),
        ('mgmt_num_rx_packets',         'I',      'uint32',  'Total number of management packets received from remote node'),
        ('mgmt_num_tx_packets_success', 'I',      'uint32',  'Total number of management packets successfully transmitted to remote node'),
        ('mgmt_num_tx_packets_total',   'I',      'uint32',  'Total number of management packets transmitted (successfully or not) to remote node'),
        ('mgmt_num_tx_packets_low',     'I',      'uint32',  'Total number of PHY transmissions of management packets to remote node (includes re-transmissions)'),
        ('latest_txrx_timestamp',       'Q',      'uint64',  'Microsecond timer value (system time) of last transmission / reception')]

}


info_consts_defs = {
    'STATION_INFO' : util.consts_dict({
        'flags'         : util.consts_dict({
            'DISABLE_ASSOC_CHECK'      : 0x00000001,
            'DOZE'                     : 0x00000002,
            'DO_NOT_REMOVE'            : 0x80000000
        }),
        'tx_phy_mode'   : util.phy_modes,
        'tx_mac_flags'  : util.consts_dict()
    }),

    'BSS_INFO'     : util.consts_dict({
        'flags'         : util.consts_dict(),
        'state'         : util.consts_dict({
            'UNAUTHENTICATED'          : 0x0001,
            'AUTHENTICATED'            : 0x0002,
            'ASSOCIATED'               : 0x0004,
            'OWNED'                    : 0x0005
        }),
        'capabilities'  : util.consts_dict({
            'ESS'                      : 0x0001,
            'IBSS'                     : 0x0002,
            'PRIVACY'                  : 0x0010,
            'SHORT_PREAMBLE'           : 0x0020,
            'PBCC'                     : 0x0040,
            'CHAN_AGILITY'             : 0x0080,
            'SPEC_MGMT'                : 0x0100,
            'SHORT_TIMESLOT'           : 0x0400,
            'APSD'                     : 0x0800,
            'DSSS_OFDM'                : 0x2000,
            'DELAYED_BLOCK_ACK'        : 0x4000,
            'IMMEDIATE_BLOCK_ACK'      : 0x8000
        }),
        'phy_mode'      : util.consts_dict({
            'LEGACY'                   : 0x01,
            'HT'                       : 0x02
        })
    }),

    'TXRX_COUNTS'  : util.consts_dict()
}



# -----------------------------------------------------------------------------
# Information Structure Base Class
# -----------------------------------------------------------------------------

class InfoStruct(dict):
    """Base class for structured information classes

    This class provides the basic methods for setting / accessing information
    within the info struct object.  It also provides the base methods for
    serializing / deserializing the object for communication by the transport.

    An InfoStruct object represents structured data that is passed to/from the
    MAC C code. In the C code these objects are represented by struct defintions.
    The fields defined for an InfoStruct object must match the fields in the
    corresponding C struct. Conceptually these InfoStruct objects are very similar
    to log entries; they even share the log entry syntax for defining fields. By
    defining InfoStruct objects here, however, wlan_exp scripts can control and
    retrieve parameters encoded in non-log structs on the node without relying
    on any of wlan_exp's logging framework. The primary benefit of this separation
    is removing the numpy dependency when dealing with the non-log version of
    the info structures described here.
    """
    _field_name         = None         # Internal name for the info_field_defs entry
    _fields_struct_fmt  = None         # Internal string of field formats, in struct module format
    _consts             = None         # Internal container for user-defined, type-specific constants

    def __init__(self, field_name):
        super(InfoStruct, self).__init__()

        if(field_name not in info_field_defs.keys()):
            msg  = "Field name {0} does not exist in info_field_defs.".format(field_name)
            raise AttributeError(msg)
            
        if(field_name not in info_consts_defs.keys()):
            msg  = "Field name {0} does not exist in info_consts_defs.".format(field_name)
            raise AttributeError(msg)

        # Initialize variables
        self._field_name         = field_name
        self._fields_struct_fmt  = ''
        self._consts             = info_consts_defs[field_name]

        # Add and initialize all the fields in the info_field_defs
        for field in info_field_defs[field_name]:
            if 'x' not in field[1]:
                self[field[0]] = None

        # Update the meta-data about the fields
        self._update_field_defs()


    # -------------------------------------------------------------------------
    # Accessor methods for the Info Type
    # -------------------------------------------------------------------------
    def get_field_names(self):
        """Get the field names.

        Returns:
            names (list of str):  List of string field names for the entry
        """
        if(self._field_name in info_field_defs.keys()):
            return [f[0] for f in info_field_defs[self._field_name]]
        else:
            msg  = "Field name {0} does not exist in info_feild_defs.".format(self._field_name)
            raise AttributeError(msg)


    def get_field_struct_formats(self):
        """Get the Python struct format of the fields.

        Returns:
            formats (list of str):  List of Python struct formats for the fields
        """
        if(self._field_name in info_field_defs.keys()):
            return [f[1] for f in info_field_defs[self._field_name]]
        else:
            msg  = "Field name {0} does not exist in info_feild_defs.".format(self._field_name)
            raise AttributeError(msg)


    def get_field_defs(self):
        """Get the field definitions.

        Returns:
            fields (list of tuple):  List of tuples that describe each field
        """
        if(self._field_name in info_field_defs.keys()):            
            return info_field_defs[self._field_name]
        else:
            msg  = "Field name {0} does not exist in info_feild_defs.".format(self._field_name)
            raise AttributeError(msg)


    def get_consts(self):
        """Get all constants defined in the info struct object as a dictionary

        Returns:
            values (dict):  All constant values in the object
        """
        return self._consts.copy()


    def get_const(self, name):
        """Get a constant defined in the info struct object

        Returns:
            value (int or str):  Value associated with the constant
        """
        if (name in self._consts.keys()):
            return self._consts[name]
        else:
            msg  = "Constant {0} does not exist ".format(name)
            msg += "in {0}".format(self.__class__.__name__)
            raise AttributeError(msg)


    def sizeof(self):
        """Return the size of the object when being transmitted / received by
        the transport
        """
        return struct.calcsize(self._fields_struct_fmt)


    # -------------------------------------------------------------------------
    # Utility methods for the InfoStruct object
    # -------------------------------------------------------------------------
    def serialize(self):
        """Packs object into a data buffer

        Returns:
            data (bytearray):  Bytearray of packed binary data
        """
        # Pack the object into a single data buffer
        ret_val    = b''
        fields     = []
        field_type = []
        tmp_values = []
        used_field = []

        if(self._field_name not in info_field_defs.keys()):
            msg  = "Field name {0} does not exist in info_feild_defs.".format(self._field_name)
            raise AttributeError(msg)

        for field in info_field_defs[self._field_name]:
            if 'x' not in field[1]:
                fields.append(field[0])
                try:
                    tmp_values.append(self[field[0]])
                    field_type.append(type(self[field[0]]))
                    used_field.append(True)
                except KeyError:
                    tmp_values.append(0)
                    field_type.append(None)
                    used_field.append(False)

        try:
            ret_val += struct.pack(self._fields_struct_fmt, *tmp_values)
        except struct.error as err:
            print("Error serializing structure:\n\t{0}".format(err))
            print("Serialize Structure:")
            print(fields)
            print(field_type)
            print(used_field)
            print(tmp_values)
            print(self._fields_struct_fmt)
            raise RuntimeError("See above print statements to debug error.")
            

        if (self.sizeof()) != len(ret_val):
            msg  = "WARNING: Sizes do not match.\n"
            msg += "    Expected  {0} bytes".format(self.sizeof())
            msg += "    Buffer is {0} bytes".format(len(ret_val))
            print(msg)

        return ret_val


    def deserialize(self, buf):
        """Unpacks a buffer of data into the object

        Args:
            buf (bytearray): Array of bytes containing the values of an information object

        Returns:
             (Info Object):  Each Info object in the list has been filled in with the corresponding
                data from the buffer.
        """
        all_names   = self.get_field_names()
        all_fmts    = self.get_field_struct_formats()
        object_size = self.sizeof()

        try:
            data_tuple = struct.unpack(self._fields_struct_fmt, buf[0:object_size])

            # Filter out names for fields ignored during unpacking
            names = [n for (n,f) in zip(all_names, all_fmts) if 'x' not in f]

            # Populate object with data
            for i, name in enumerate(names):
                self[name] = data_tuple[i]

        except struct.error as err:
            print("Error unpacking {0} buffer with len {1}: {2}".format(self.name, len(buf), err))


    # -------------------------------------------------------------------------
    # Internal methods for the InfoStruct object
    # -------------------------------------------------------------------------
    def _update_field_defs(self):
        """Internal method to update meta-data about the fields."""
        # Update the fields format used by struct unpack/calcsize
        self._fields_struct_fmt = ' '.join(self.get_field_struct_formats())


    def __str__(self):
        """Pretty print info struct object"""
        msg = "{0}\n".format(self.__class__.__name__)

        for field in info_field_defs[self._field_name]:
            if 'x' not in field[1]:
                msg += "    {0:30s} = {1}\n".format(field[0], self[field[0]])

        return msg

    # Allow attribute (ie ".") notation to access contents of dictionary
    def __getattr__(self, name):
        if name in self:
            return self[name]
        else:
            super(InfoStruct, self).__getattr__(name)
    
    def __setattr__(self, name, value):
        if name in self:
            self[name] = value
        else:
            super(InfoStruct, self).__setattr__(name, value)


# End Class



# -----------------------------------------------------------------------------
# TX/RX Counts Class
# -----------------------------------------------------------------------------

class TxRxCounts(InfoStruct):
    """Class for TX/RX counts."""

    def __init__(self):
        super(TxRxCounts, self).__init__(field_name='TXRX_COUNTS')

        # NOTE:  To populate the TxRxCounts with information, use the
        #     deserialize() function on a proper buffer of data


    def serialize(self):
        # NOTE:  serialize() is currently not supported for TxRxCounts.  This
        #     is due to the fact that TxRxCounts information should only come
        #     directly from the node and should not be set to the node.
        #
        print("Error:  serialize() is not supported for TxRxCounts.")
        raise NotImplementedError

        # NOTE:  If serialize() needs to be supported in future version for
        #     TxRxCounts, below is the code to use:
        #
        # # Convert MAC address to byte string for transmit
        # mac_addr_tmp     = self['mac_addr']
        # self['mac_addr'] = util.str_to_mac_addr(self['mac_addr'])
        # self['mac_addr'] = util.mac_addr_to_byte_str(self['mac_addr'])
        #
        # ret_val = super(TxRxCounts, self).serialize()
        #
        # # Revert MAC address to a colon delimited string
        # self['mac_addr'] = mac_addr_tmp
        #
        # return ret_val


    def deserialize(self, buf):
        super(TxRxCounts, self).deserialize(buf)

        if (False):
            msg = "TX/RX Counts Data Buffer:\n"
            msg += util.buffer_to_str(buf)
            print(msg)

        # Clean up the values
        #   - Convert the MAC Address to a colon delimited string
        self['mac_addr'] = util.byte_str_to_mac_addr(self['mac_addr'])
        self['mac_addr'] = util.mac_addr_to_str(self['mac_addr'])


# End Class



# -----------------------------------------------------------------------------
# Station Info Class
# -----------------------------------------------------------------------------

class StationInfo(InfoStruct):
    """Class for Station Information."""

    def __init__(self):
        super(StationInfo, self).__init__(field_name='STATION_INFO')

        # NOTE:  To populate the TxRxCounts with information, use the
        #     deserialize() function on a proper buffer of data


    def serialize(self):
        # NOTE:  serialize() is currently not supported for StationInfo.  This
        #     is due to the fact that StationInfo information should only come
        #     directly from the node and should not be set to the node.
        #
        print("Error:  serialize() is not supported for StationInfo.")
        raise NotImplementedError

        # NOTE:  If serialize() needs to be supported in future version for
        #     StationInfo, below is the code to use:
        #
        # # Convert MAC address to byte string for transmit
        # mac_addr_tmp     = self['mac_addr']
        # self['mac_addr'] = util.str_to_mac_addr(self['mac_addr'])
        # self['mac_addr'] = util.mac_addr_to_byte_str(self['mac_addr'])
        #
        # ret_val = super(StationInfo, self).serialize()
        #
        # # Revert MAC address to a colon delimited string
        # self['mac_addr'] = mac_addr_tmp
        #
        # return ret_val


    def deserialize(self, buf):
        super(StationInfo, self).deserialize(buf)

        if (False):
            msg = "Station Info Data buffer:\n"
            msg += util.buffer_to_str(buf)
            print(msg)

        # Clean up the values
        #   - Remove extra characters in the SSID
        #   - Convert the MAC Address to a colon delimited string
        if (self['host_name'][0] == '\x00'):
            self['host_name'] = '\x00'
        else:
            import ctypes
            self['host_name'] = ctypes.c_char_p(self['host_name']).value

        self['mac_addr'] = util.byte_str_to_mac_addr(self['mac_addr'])
        self['mac_addr'] = util.mac_addr_to_str(self['mac_addr'])


# End Class



# -----------------------------------------------------------------------------
# BSS Info Class
# -----------------------------------------------------------------------------

class BSSInfo(InfoStruct):
    """Class for Basic Service Set (BSS) Information

    Attributes:
        ssid (str):   SSID string (Must be 32 characters or less)
        channel (int, dict in util.wlan_channel array): Channel on which the BSS operates
            (either the channel number as an it or an entry in the wlan_channel array)
        bssid (int, str):  40-bit ID of the BSS either as a integer or colon delimited
            string of the form:  XX:XX:XX:XX:XX:XX
        ibss_status (bool, optional): Status of the
            BSS:
                * **True**  --> Capabilities field = 0x2 (BSS_INFO is for IBSS)
                * **False** --> Capabilities field = 0x1 (BSS_INFO is for BSS)
        beacon_interval (int): Integer number of beacon Time Units in [1, 65535]
            (http://en.wikipedia.org/wiki/TU_(Time_Unit); a TU is 1024 microseconds)
    """
    def __init__(self, init_fields=False, bssid=None, ssid=None, channel=None, ibss_status=False, beacon_interval=None):
        super(BSSInfo, self).__init__(field_name='BSS_INFO')

        if init_fields:
            # Set default values for fields not set by this method
            self['timestamp']                  = 0
            self['latest_activity_timestamp']  = 0
            self['flags']                      = 0
            self['state']                      = self._consts.state.OWNED
            self['num_basic_rates']            = 0
            self['basic_rates']                = bytes()
            self['phy_mode']                   = 0

            # Set SSID
            if ssid is not None:
                # Check SSID
                if type(ssid) is not str:
                    raise ValueError("The SSID must be a string.")

                if len(ssid) > 32:
                    ssid = ssid[:32]
                    print("WARNING:  SSID must be 32 characters or less.  Truncating to {0}".format(ssid))

                try:
                    self['ssid']         = bytes(ssid, "UTF8")
                except:
                    self['ssid']         = ssid

            # Set Channel
            if channel is not None:
                channel_error = False

                # Check Channel
                #   - Make sure it is a valid channel; only store channel
                if type(channel) is int:
                    channel = util.find_channel_by_channel_number(channel)
                    if channel is None: channel_error = True

                elif type(channel) is dict:
                    pass

                else:
                    channel_error = True

                if not channel_error:
                    self['chan_num']    = channel['channel']
                else:
                    msg  = "The channel must either be a valid channel number or a wlan_exp.util.wlan_channel entry."
                    raise ValueError(msg)

            # Set the beacon interval
            if beacon_interval is not None:
                # Check beacon interval
                if type(beacon_interval) is not int:
                    beacon_interval = int(beacon_interval)
                    print("WARNING:  Beacon interval must be an interger number of time units.  Rounding to {0}".format(beacon_interval))

                if not ((beacon_interval > 0) and (beacon_interval < 2**16)):
                    msg  = "The beacon interval must be in [1, 65535] (ie 16-bit positive integer)."
                    raise ValueError(msg)

                self['beacon_interval'] = beacon_interval

            # Set the BSSID
            if bssid is not None:
                # Check IBSS status
                if type(ibss_status) is not bool:
                    raise ValueError("The ibss_status must be a boolean.")

                # Set BSSID, capabilities
                #   - If this is an IBSS, then set local bit to '1' and mcast bit to '0'
                #   - Set the appropriate capabilities (NOTE:  the 802.11 reference design only supports short timeslots (ie short = 9us))
                if ibss_status:
                    self['bssid']        = util.create_locally_administered_bssid(bssid)
                    self['capabilities'] = (self._consts.capabilities.IBSS | self._consts.capabilities.SHORT_TIMESLOT)
                else:
                    self['bssid']        = bssid
                    self['capabilities'] = (self._consts.capabilities.ESS | self._consts.capabilities.SHORT_TIMESLOT)

                # Convert BSSID to colon delimited string for internal storage
                if type(bssid) is int:
                    self['bssid']        = util.mac_addr_to_str(self['bssid'])


    def serialize(self):
        # Convert bssid to byte string for transmit
        bssid_tmp     = self['bssid']
        self['bssid'] = util.str_to_mac_addr(self['bssid'])
        self['bssid'] = util.mac_addr_to_byte_str(self['bssid'])

        ret_val = super(BSSInfo, self).serialize()

        # Revert bssid to colon delimited string
        self['bssid'] = bssid_tmp

        return ret_val


    def deserialize(self, buf):
        super(BSSInfo, self).deserialize(buf)

        if (False):
            msg  = "BSS Info Data buffer:\n"
            msg += util.buffer_to_str(buf)
            print(msg)

        # Clean up the values
        #   - Remove extra characters in the SSID
        #   - Convert the BSS ID to a colon delimited string for storage
        #
        # NOTE:  A BSS ID is a 40-bit integer and can be treated like a MAC
        #     address in the WLAN Exp framework (ie all the MAC address
        #     utility functions can be used on it.)
        #
        import ctypes

        self['ssid']  = ctypes.c_char_p(self['ssid']).value
        self['bssid'] = util.byte_str_to_mac_addr(self['bssid'])
        self['bssid'] = util.mac_addr_to_str(self['bssid'])


# End Class



# -----------------------------------------------------------------------------
# Misc Utilities
# -----------------------------------------------------------------------------
def deserialize_info_buffer(buffer, buffer_class):
    """Unpacks a buffer of data into a list of objects

    Args:
        buf (bytearray): Array of bytes containing 1 or more objects of the same type

    Returns:
        information objects (List of Info Objects):
            Each Info object in the list has been filled in with the corresponding
            data from the buffer.
    """
    ret_val     = []
    buffer_size = len(buffer)
    index       = 0
    object_size = 1

    try:
        tmp_obj     = eval(buffer_class, globals(), locals())
        object_size = tmp_obj.sizeof()
    except:
        print("ERROR:  Cannot create information object of class: {0}".format(buffer_class))

    while (index < buffer_size):
        # try:
        tmp_obj = eval(buffer_class, globals(), locals())
        tmp_obj.deserialize(buffer[index:index+object_size])
        # except:
        #     tmp_obj = None

        ret_val.append(tmp_obj)

        index += object_size

    return ret_val

# End def

