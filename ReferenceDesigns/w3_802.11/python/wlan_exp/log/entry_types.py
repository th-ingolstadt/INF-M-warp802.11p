"""
entry_types.py
==============

This module defines each type of log entry that may exist
in the event log of an 802.11 Reference Design Node.

The log entry definitions in this file must match the corresponding
definitions in the wlan_mac_entries.h header file in the C code
running on the node.

This module maintains a dictionary which contains a reference to each
known log entry type. This dictionary is stored in the variable
``wlan_exp_log_entry_types``. The :class:`WlanExpLogEntryType` constructor
automatically adds each log entry type definition to this dictionary. Users
may access the dictionary to view currently defined log entry types. But user code
should not modify the dictionary contents directly.

Custom Log Entry Types
----------------------
The :mod:`log_entries` module includes definitions for the log entry types implemented in
the current 802.11 Reference Design C code.

Log entry types defined here must match the corresponding entry definitions in the node C code.
Custom entries can be defined and added to the global dictionary by user scripts.

Log entry type definitions are instances of the :class:`WlanExpLogEntryType` class. The 
:class:`WlanExpLogEntryType` constructor requires two arguments: ``name`` and ``entry_type_id``.
Both the name and entry type ID **must** be unique relative to the existing entry types defined in
:mod:`log_entries`.

To define a custom log entry type::

    import wlan_exp.log.entry_types as entry_types
    
    #name and entry_type_id must not collide with existing log entry type definitions
    my_entry_type = entry_types.WlanExpLogEntryType(name='MY_ENTRY', entry_type_id=999)
    my_entry_type.append_field_defs([
            ('timestamp',              'Q',      'uint64'),
            ('field_A',                'H',      'uint16'),
            ('field_B',                'H',      'uint16')])

----
"""
from struct import unpack, calcsize, error
import numpy as np

# WLAN Exp Event Log Constants
#   NOTE:  The C counterparts are found in wlan_mac_event_log.h
WLAN_EXP_LOG_DELIM = 0xACED


# WLAN Exp Log Entry Constants
#   NOTE:  The C counterparts are found in wlan_mac_entries.h
ENTRY_TYPE_NULL                   = 0
ENTRY_TYPE_NODE_INFO              = 1
ENTRY_TYPE_EXP_INFO               = 2
ENTRY_TYPE_STATION_INFO           = 3
ENTRY_TYPE_NODE_TEMPERATURE       = 4
ENTRY_TYPE_WN_CMD_INFO            = 5
ENTRY_TYPE_TIME_INFO              = 6

ENTRY_TYPE_RX_OFDM                = 10
ENTRY_TYPE_RX_DSSS                = 11

ENTRY_TYPE_TX                     = 20
ENTRY_TYPE_TX_LOW                 = 21

ENTRY_TYPE_TXRX_STATS             = 30

#-----------------------------------------------------------------------------
# Log Entry Type Container
#-----------------------------------------------------------------------------

log_entry_types          = dict()

#-----------------------------------------------------------------------------
# Log Entry Type Base Class
#-----------------------------------------------------------------------------

class WlanExpLogEntryType(object):
    """Base class to define a log entry type."""
    # _fields is a list of 3-tuples:
    #     (field_name, field_fmt_struct, field_fmt_np)
    _fields             = None

    entry_type_id       = None #:Unique integer ID for entry type
    name                = None #:Unique string name for entry type
    description         = '' #:Description of log entry type, used for generating documentation

    fields_np_dt        = None #:numpy dtype object describing format
    fields_fmt_struct   = None #:List of field formats, in struct module format
    _field_offsets       = None

    gen_numpy_callbacks = None

    consts = None #:Container for user-defined, entry-specific constants

    def __init__(self, name=None, entry_type_id=None):
        # Require valid name
        if name is not None:
            if(name in log_entry_types.keys()):
                print("WARNING: replacing exisitng WlanExpLogEntryType with name {0}".format(name))
            self.name = name
            log_entry_types[name] = self
        else:
            raise Exception("ERROR: new WlanExpLogEntryType instance must have valid name")

        # Entry type ID is optional
        if entry_type_id is not None:
            # entry_type_id must be int
            if type(entry_type_id) is not int:
                raise Exception("ERROR: WlanExpLogEntryType entry_type_id must be int")
            else:
                if(entry_type_id in log_entry_types.keys()):
                    print("WARNING: replacing exisitng WlanExpLogEntryType with ID {0}".format(entry_type_id))
                self.entry_type_id = entry_type_id
                log_entry_types[entry_type_id] = self

        # Initialize fields to empty lists
        self._fields             = []

        # Initialize unpack variables
        self.fields_fmt_struct   = ''
        
        # Initialize callbacks
        self.gen_numpy_callbacks = []

        # Initialize dictionary to contain constants specific to each entry type 
        self.consts = dict()

        # Initialize variable that contains field names and byte offsets 
        self._field_offsets       = {} 


    #-------------------------------------------------------------------------
    # Accessor methods for the WlanExpLogEntryType
    #-------------------------------------------------------------------------
    def get_field_names(self):
        return [f[0] for f in self._fields]
#        return [field_name for (field_name, field_fmt_struct, field_fmt_np, desc or _) in self._fields]

    def get_field_struct_formats(self):
        return [f[1] for f in self._fields]
#        return [field_fmt_struct for (field_name, field_fmt_struct, field_fmt_np, desc or None) in self._fields]

    def get_field_defs(self):          return self._fields

    def get_field_offsets(self):       return self._field_offsets

    def get_entry_type_id(self):       return self.entry_type_id

    def append_field_defs(self, field_info):
        """Adds fields to the definition of the log entry type.

        Arg ``field_info`` must be a list of 3-tuples. Each 3-tuple must be of the form
        ``(field_name, field_type_struct, field_type_numpy)`` where:
        
        * ``field_name``: Name of field as string
        * ``field_type_struct``: Field type as string, using formats specified by ``struct`` module
        * ``field_type_numpy``: Field type as string, using formats specified by numpy ``dtype``
        * ``field_desc``: String describing the field's meaning, used to generate wiki docs
        """
        if type(field_info) is list:
            self._fields.extend(field_info)
        else:
            self._fields.append(field_info)
        self._update_field_defs()


    def add_gen_numpy_array_callback(self, callback):
        """Add callback that is run after the numpy array is generated from the entry type."""
        if callable(callback):
            self.gen_numpy_callbacks.append(callback)
        else:
            print("ERROR:  Callback must be callable function.")


    #-------------------------------------------------------------------------
    # Utility methods for the WlanExpLogEntryType
    #-------------------------------------------------------------------------
    def generate_numpy_array(self, log_bytes, byte_offsets):
        """Generate a NumPy array from the log_bytes of the given
        WlanExpLogEntryType instance at the given byte_offsets.
        """
        index_iter = [log_bytes[o : o + self.fields_np_dt.itemsize] for o in byte_offsets]
        np_arr = np.fromiter(index_iter, self.fields_np_dt, len(byte_offsets))

        if self.gen_numpy_callbacks:
            for callback in self.gen_numpy_callbacks:
                np_arr = callback(np_arr)

        return np_arr

    def generate_entry_wiki_doc(self):
        field_descs = list()
        for f in self._fields:
            #Field tuple is (name, struct_type, np_type, (optional)desc)
            # Construct new tuple of (name, np_type, desc ('' if not defined))
            try:
                field_descs.append( (f[0], f[2], f[3]))
            except IndexError:
                #Field missing description; use empty string
                field_descs.append( (f[0], f[2], ''))

        #Construct the Trac-wiki-style documentation string for this entry type
        doc_str = '=== Entry Type {0} ===\n'.format(self.name)

        doc_str += self.description + '\n\n'

        doc_str += 'Entry type ID: {0}\n\n'.format(self.entry_type_id)

        doc_str += '||=  Field Name  =||=  DataType  =||=  Description  =||\n'

        for fd in field_descs:
            fd_desc = fd[2].replace('\n', '[[BR]]')
            doc_str += '|| {0} ||  {1}  || {2} ||\n'.format(fd[0], fd[1], fd_desc)

        doc_str += '\n----\n\n'

        return doc_str

    def _entry_as_string(self, buf):
        """Generate a string representation of the entry from a buffer. This method sould only
        be used for debugging log data parsing and log index generation, not for general creation
        of text log files."""
        
        entry_size = calcsize(self.fields_fmt_struct)
        entry      = self.deserialize(buf[0:entry_size])[0]
                
        str_out = self.name + ': '

        for k in entry.keys():
            s = entry[k]
            if((type(s) is int) or (type(s) is long)):
                str_out += "\n    {0:30s} = {1:20d} (0x{1:16x})".format(k, s)
            elif(type(s) is str):
                s = map(ord, list(entry[k]))
                str_out += "\n    {0:30s} = [".format(k)
                for x in s:
                    str_out += "{0:d}, ".format(x)
                str_out += "\b\b]"

        str_out += "\n"

        return str_out


    def deserialize(self, buf):
        """Unpacks one or more raw log entries of the same type into a list of dictionaries

        Args:
            buf (bytearray): Array of raw log data containing 1 or more log entries
            of the same type.
        
        Returns:
            List of dictoinaries. Each dictionary has one value per field in the
            log entry definition using the field names as keys.

        """
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

                # Filter out names for fields ignored during unpacking
                names = [n for (n,f) in zip(all_names, all_fmts) if 'x' not in f]

                # Use OrderedDict to preserve user-specified field order
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
        sizes   = list(map(get_np_size, [f[2] for f in self._fields]))
        offsets = [sum(sizes[0:i]) for i in range(len(sizes))]

        np_fields = self._fields

        # numpy processing ignores the same fields ignored by struct.unpack
        # !!!BAD!!! Doing this filtering breaks HDF5 export
        # offsets = [o for (o,f) in zip(offsets_all, self._fields) if 'x' not in f[1]]
        # np_fields = [f for f in self._fields if 'x' not in f[1]]

        names   =  [f[0] for f in np_fields]
        formats =  [f[2] for f in np_fields]

        self.fields_np_dt = np.dtype({'names':names, 'formats':formats, 'offsets':offsets})

        # Update the field offsets 
        self._field_offsets = dict(zip(names, offsets))
        
        # Check our definitions of struct vs numpy are in sync
        struct_size = calcsize(self.fields_fmt_struct)
        np_size     = self.fields_np_dt.itemsize
        
        if (struct_size != np_size):
            msg  = "WARNING:  Definitions of struct {0} do not match.\n".format(self.name)
            msg += "    Struct size = {0}    Numpy size = {1}".format(struct_size, np_size)
            print(msg)
        

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


def np_array_add_MAC_addr_fields(np_arr_orig):
        # Extend the default np_arr with convenience fields for MAC header addresses
        # IMPORTANT: np_arr uses the original bytearray as its underlying data
        # We must operate on a copy to avoid clobbering log entries adjacent to the
        #  Tx or Rx entries being extended

        # Create a new numpy dtype with additional fields
        dt_new = extend_np_dt(np_arr_orig.dtype,
                {'names': ('addr1', 'addr2', 'addr3', 'mac_seq'),
                 'formats': ('uint64', 'uint64', 'uint64', 'uint16')})

        #Initialize the output array (same shape, new dtype)
        np_arr_out = np.zeros(np_arr_orig.shape, dtype=dt_new)

        #Copy data from the base numpy array into the output array
        for f in np_arr_orig.dtype.names:
            #TODO: maybe don't copy fields that are ignored in the struct format?
            # problem is non-TxRx entries would still have these fields in their numpy versions
            np_arr_out[f] = np_arr_orig[f]

        #Helper array of powers of 2
        # this array arranges bytes such that they match other u64 representations of MAC addresses
        #  elsewhere in the framework
        addr_conv_arr = np.uint64(2)**np.array(range(40,-1,-8), dtype='uint64')

        #Extract all MAC headers (each header is 24-entry uint8 array)
        mac_hdrs = np_arr_orig['mac_payload']

        #Compute values for address-as-int fields using numpy's dot-product routine
        # MAC header offsets here select the 3 6-byte address fields
        np_arr_out['addr1'] = np.dot(addr_conv_arr, np.transpose(mac_hdrs[:, 4:10]))
        np_arr_out['addr2'] = np.dot(addr_conv_arr, np.transpose(mac_hdrs[:,10:16]))
        np_arr_out['addr3'] = np.dot(addr_conv_arr, np.transpose(mac_hdrs[:,16:22]))

        np_arr_out['mac_seq'] = np.dot(mac_hdrs[:,22:24], [1, 256]) // 16

        return np_arr_out

# End def


def extend_np_dt(dt_orig, new_fields=None):
    """Extends a numpy dtype object with additional fields. new_fields input must be dictionary
    with keys 'names' and 'formats', same as when specifying new dtype objects. The return
    dtype will *not* contain byte offset values for existing or new fields, even if exisiting
    fields had specified offsets. Thus the original dtype should be used to interpret raw data buffers
    before the extended dtype is used to add new fields.
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

# End def


#-----------------------------------------------------------------------------
# NULL Log Entry Instance
#-----------------------------------------------------------------------------

# The NULL entry is used to "remove" an existing entry within the log.
#   By replacing the current entry type with the NULL entry type and zeroing
#   out all data following the header, we can effectively remove an entry
#   without changing the memory footprint of the log.  NULL entries will 
#   be filtered and never show up in the raw_log_index.

entry_null = WlanExpLogEntryType(name='NULL', entry_type_id=ENTRY_TYPE_NULL)


#-----------------------------------------------------------------------------
# Virtual Log Entry Instances
#-----------------------------------------------------------------------------

entry_rx_common = WlanExpLogEntryType(name='RX_ALL', entry_type_id=None)
entry_rx_common.append_field_defs([
            ('timestamp',              'Q',      'uint64',  'Microsecond timer value at PHY Rx start'),
            ('length',                 'H',      'uint16',  'Length of payload in bytes'),
            ('rate',                   'B',      'uint8',   'PHY rate index, in [1:8]'),
            ('power',                  'b',      'int8',    'Rx power in dBm'),
            ('fcs_result',             'B',      'uint8',   'Checksum status, 0 = no errors'),
            ('pkt_type',               'B',      'uint8',   'Packet type: 1 = other data, 2 = encapsulated Ethernet, 3 = LTG, 11 = management, 21 = control'),
            ('chan_num',               'B',      'uint8',   'Channel (center frequency) index'),
            ('ant_mode',               'B',      'uint8',   'Antenna mode: [1,2,3,4] for SISO Rx on RF [A,B,C,D]'),
            ('rf_gain',                'B',      'uint8',   'AGC RF gain setting: [1,2,3] for [0,15,30]dB gain'),
            ('bb_gain',                'B',      'uint8',   'AGC BB gain setting: [0:31] for approx [0:63]dB gain'),
            ('flags',                  'H',      'uint16',  'Bit OR\'d flags: 0x1 = Rx was duplicate of previous Rx')])

#-----------------------------------------------------------------------------
# Log Entry Type Instances
#-----------------------------------------------------------------------------
# Node Info

entry_node_info = WlanExpLogEntryType(name='NODE_INFO', entry_type_id=ENTRY_TYPE_NODE_INFO)
entry_node_info.description = 'Details about the node hardware and its configuration. Node info values are static after boot.'
_node_info_node_types =  'Node type as 4 byte value: [b0 b1 b2 b3]:\n'
_node_info_node_types += ' b0: Always 0x00\n'
_node_info_node_types += ' b1: Always 0x01 for 802.11 ref design nodes\n'
_node_info_node_types += ' b2: CPU High application: 0x1 = AP, 0x2 = STA\n'
_node_info_node_types += ' b3: CPU Low application: 0x1 = DCF'

entry_node_info.append_field_defs([
            ('timestamp',              'Q',      'uint64',  'Microsecond timer value at time of log entry creation'),
            ('node_type',              'I',      'uint32',  _node_info_node_types),
            ('node_id',                'I',      'uint32',  'Node ID, as set during wlan_exp init'),
            ('hw_generation',          'I',      'uint32',  'WARP hardware generation: 3 for WARP v3'),
            ('wn_ver',                 'I',      'uint32',  'WARPnet version, as packed bytes [0 major minor rev]'),
            ('fpga_dna',               'Q',      'uint64',  'DNA value of node FPGA'),
            ('serial_num',             'I',      'uint32',  'Serial number of WARP board'),
            ('wlan_exp_ver',           'I',      'uint32',  'wlan_exp version, as packed values [(u8)major (u8)minor (u16)rev]'),
            ('wlan_max_associations',  'I',      'uint32',  'Maximum number of wireless associations allowed by node'),
            ('wlan_log_max_size',      'I',      'uint32',  'Maximum size in bytes of node\'s event log'),
            ('wlan_mac_addr',          'Q',      'uint64',  'Node MAC address, 6 bytes in lower 48-bits of u64'),
            ('wlan_max_stats',         'I',      'uint32',  'Maximum number of statistics structs maintained by node'),
            ('ltg_resolution',         'I',      'uint32',  'Minimum interval in microseconds of LTG schedules')])

# Experiment Info header - actual exp_info contains a "message" field that
#  follows this header. Since the message is variable length it is not described
#  in the fields list below. Full exp_info entries (header + message) must be extracted
#  directly by a user script.
entry_exp_info_hdr = WlanExpLogEntryType(name='EXP_INFO', entry_type_id=ENTRY_TYPE_EXP_INFO)
entry_exp_info_hdr.description = 'Header for generic experiment info entries created by the user application. '
entry_exp_info_hdr.description += 'The payload of the EXP_INFO entry is not described by the Python entry type. User '
entry_exp_info_hdr.description += 'code must access the payload in the binary log data directly.'
entry_exp_info_hdr.append_field_defs([
            ('timestamp',              'Q',      'uint64',  'Microsecond timer value at time of log entry creation'),
            ('info_type',              'H',      'uint16',  'Exp info type (arbitrary value supplied by application'),
            ('length',                 'H',      'uint16',  'Exp info length (describes arbitrary payload supplied by application')])


# Station Info
entry_station_info = WlanExpLogEntryType(name='STATION_INFO', entry_type_id=ENTRY_TYPE_STATION_INFO)
entry_station_info.description  = 'Information about an 802.11 association. At the AP one STATION_INFO is created '
entry_station_info.description += 'for each associated STA and is logged whenever the STA association state changes. '
entry_station_info.description += 'At the STA one STATION_INFO is logged whenever the STA associaiton state changes.'

entry_station_info.append_field_defs([
            ('timestamp',              'Q',      'uint64',  'Microsecond timer value at time of log entry creation'),
            ('mac_addr',               '6s',     '6uint8',  'MAC address of associated device'),
            ('aid',                    'H',      'uint16',  'Association ID (AID) of device'),
            ('host_name',              '20s',    '20uint8', 'String hostname (19 chars max), taken from DHCP DISCOVER packets'),
            ('flags',                  'I',      'uint32',  'Association state flags: ???'),
            ('rx_last_timestamp',      'Q',      'uint64',  'Microsecond timer value at time of last successful Rx from device'),
            ('rx_last_seq',            'H',      'uint16',  'Sequence number of last packet received from device'),
            ('rx_last_power',          'b',      'int8',    'Rx power in dBm of last packet received from device'),
            ('rx_last_rate',           'B',      'uint8',   'PHY rate index in [1:8] of last packet received from device'),
            ('tx_phy_rate',            'B',      'uint8',   'Current PHY rate index in [1:8] for new transmissions to device'),
            ('tx_phy_antenna_mode',    'B',      'uint8',   'Current PHY antenna mode in [1:4] for new transmissions to device'),
            ('tx_phy_power',           'b',      'int8',    'Current Tx power in dBm for new transmissions to device'),
            ('tx_phy_flags',           'B',      'uint8',   'Flags for Tx PHY config for new transmissions to deivce'),
            ('tx_mac_num_tx_max',      'B',      'uint8',   'Maximum number of transmissions (original Tx + re-Tx) per MPDU to device'),
            ('tx_mac_flags',           'B',      'uint8',   'Flags for Tx MAC config for new transmissions to device'),
            ('padding',                '2x',     'uint16',  '')])

# WARPNet Command Info
entry_wn_cmd_info = WlanExpLogEntryType(name='WN_CMD_INFO', entry_type_id=ENTRY_TYPE_WN_CMD_INFO)
entry_wn_cmd_info.description  = 'Record of a WARPnet / wlan_exp command received by the node. The full command payload '
entry_wn_cmd_info.description += 'is logged, including any (possibly personal-info-carrying) parameters like MAC addresses.'
entry_wn_cmd_info.append_field_defs([
            ('timestamp',              'Q',      'uint64', 'Microsecond timer value at time of log entry creation'),
            ('command',                'I',      'uint32', 'WARPnet / wlan_exp command ID'),
            ('src_id',                 'H',      'uint16',  'Node ID of device sending command'),
            ('num_args',               'H',      'uint16',  'Number of arguments supplied in command'),
            ('args',                   '10I',    '10uint32','Command arguments')])

# Time Info
entry_time_info = WlanExpLogEntryType(name='TIME_INFO', entry_type_id=ENTRY_TYPE_TIME_INFO)
entry_time_info.description  = 'Record of a time base event at the node. This log entry is used to enable parsing of log data '
entry_time_info.description +='recored before and after changes to the node\'s microsecond timer. This entry also allows a wlan_exp controler to write the current '
entry_time_info.description +='absolute time to the node log without affecting the node\'s timer value. This enables adjustment of log entry timestamps to '
entry_time_info.description +='real timestamps in post-proessing.'
entry_time_info.append_field_defs([
            ('timestamp',              'Q',      'uint64', 'Microsecond timer value at time of log entry creation'),
            ('time_id',                'I',      'uint32', 'Random ID value included in wlan_exp TIME_INFO command; used to find common entries across nodes'), 
            ('reason',                 'I',      'uint32', 'Reason code for TIME_INFO log entry creation'), 
            ('new_time',               'Q',      'uint64',  'New value of microsecond timer value; 0xFFFFFFFFFFFFFFFF if timer was not changed'),
            ('abs_time',               'Q',      'uint64',  'Absolute time in microseconds-since-epoch; 0xFFFFFFFFFFFFFFFF if unknown')])

# Temperature
entry_node_temperature = WlanExpLogEntryType(name='NODE_TEMPERATURE', entry_type_id=ENTRY_TYPE_NODE_TEMPERATURE)
entry_node_temperature.description  = 'Record of the FPGA system monitor die temperature. This entry is only created when directed by a wlan_exp command. Temperature '
entry_node_temperature.description +='values are stored as 32-bit unsigned integers. To convert to degrees Celcius, apply (((float)temp_u32)/(65536.0*0.00198421639)) - 273.15'
entry_node_temperature.append_field_defs([
            ('timestamp',              'Q',      'uint64', 'Microsecond timer value at time of log entry creation'),
            ('node_id',                'I',      'uint32', 'wlan_exp node ID'),
            ('serial_num',             'I',      'uint32', 'Node serial number'),
            ('temp_current',           'I',      'uint32', 'Current FPGA die temperature'),
            ('temp_min',               'I',      'uint32', 'Minimum FPGA die temperature since FPGA configuration or sysmon reset'),
            ('temp_max',               'I',      'uint32', 'Maximum FPGA die temperature since FPGA configuration or sysmon reset')])

# Receive OFDM
entry_rx_ofdm = WlanExpLogEntryType(name='RX_OFDM', entry_type_id=ENTRY_TYPE_RX_OFDM)
entry_rx_ofdm.append_field_defs(entry_rx_common.get_field_defs())
entry_rx_ofdm.add_gen_numpy_array_callback(np_array_add_MAC_addr_fields)
entry_rx_ofdm.append_field_defs([
            ('chan_est',               '256B',   '(64,2)i2',    'OFDM Rx channel estimates, packed as [(uint16)I (uint16)Q] values, one per subcarrier'), 
            ('mac_payload_len',        'I',      'uint32',      'Length in bytes of MAC payload recorded in log for this packet'),
            ('mac_payload',            '24s',    '24uint8',     'First 24 bytes of MAC payload, typically the 802.11 MAC header')])
entry_rx_ofdm.consts['FCS_GOOD'] = 0
entry_rx_ofdm.consts['FCS_BAD'] = 1
entry_rx_ofdm.consts['FLAG_DUP'] = 0x4


# Receive DSSS
entry_rx_dsss = WlanExpLogEntryType(name='RX_DSSS', entry_type_id=ENTRY_TYPE_RX_DSSS)
entry_rx_dsss.append_field_defs(entry_rx_common.get_field_defs())
entry_rx_dsss.add_gen_numpy_array_callback(np_array_add_MAC_addr_fields)
entry_rx_dsss.append_field_defs([          
            ('mac_payload_len',        'I',      'uint32',      'Length in bytes of MAC payload recorded in log for this packet'),
            ('mac_payload',            '24s',    '24uint8',     'First 24 bytes of MAC payload, typically the 802.11 MAC header')])
entry_rx_dsss.consts['FCS_GOOD'] = 0

# Transmit
entry_tx = WlanExpLogEntryType(name='TX', entry_type_id=ENTRY_TYPE_TX)
entry_tx.description  = 'Tx events in CPU High, logged for each MPDU frame created and enqueued in CPU High. See TX_LOW for log entries of '
entry_tx.description += 'actual Tx events, including re-transmissions. The time values in this log entry can be used to determine time in queue '
entry_tx.description += '(time_to_accept), time taken by CPU Low for all Tx attempts (time_to_done) and total time from creation to completion '
entry_tx.description += '(time_to_accept+time_to_done).'

entry_tx.add_gen_numpy_array_callback(np_array_add_MAC_addr_fields)

entry_tx.append_field_defs([
            ('timestamp',              'Q',      'uint64',  'Microsecond timer value at time packet was created, immediately before it was enqueued'),
            ('time_to_accept',         'I',      'uint32',  'Time duration in microseconds between packet creation and packet acceptance by CPU Low'),
            ('time_to_done',           'I',      'uint32',  'Time duration in microseconds between packet acceptance by CPU Low and Tx completion in CPU Low'),
            ('uniq_seq',               'Q',      'uint64',  'Unique sequence number for Tx packet; 12 LSB of this used for 802.11 MAC header sequence number'),
            ('num_tx',                 'B',      'uint8',   'Number of actual PHY Tx events which were used to transmit the MPDU (first Tx + all re-Tx)'),
            ('tx_power',               'b',      'int8',    'Tx power in dBm of final Tx attempt'),
            ('chan_num',               'B',      'uint8',   'Channel (center frequency) index of transmission'),
            ('rate',                   'B',      'uint8',   'PHY rate index in [1:8] of final Tx attempt'),
            ('length',                 'H',      'uint16',  'Length in bytes of MPDU; includes MAC header, payload and FCS'),
            ('result',                 'B',      'uint8',   'Tx result; 0 = ACK received or not required'),
            ('pkt_type',               'B',      'uint8',   'Packet type: 1 = other data, 2 = encapsulated Ethernet, 3 = LTG, 11 = management, 21 = control'),
            ('ant_mode',               'B',      'uint8'    'PHY antenna mode of final Tx attempt'),
            ('queue_id',               'B',      'uint8',   'Tx queue ID from which the packet was retrieved'),
            ('padding',                '2x',     '2uint8',  ''),
            ('mac_payload_len',        'I',      'uint32',      'Length in bytes of MAC payload recorded in log for this packet'),
            ('mac_payload',            '24s',    '24uint8',     'First 24 bytes of MAC payload, typically the 802.11 MAC header')])
entry_tx.consts['SUCCESS'] = 0

# Transmit from CPU Low
entry_tx_low = WlanExpLogEntryType(name='TX_LOW', entry_type_id=ENTRY_TYPE_TX_LOW)
entry_tx_low.description  = 'Record of actual PHY transmission. At least one TX_LOW will be logged for every TX entry. Multiple TX_LOW entries may be created '
entry_tx_low.description += 'for the same TX entry if the low-level MAC re-transmitted the frame. The uniq_seq fields can be match between TX and TX_LOW entries '
entry_tx_low.description += 'to find records common to the same MPUD.'

entry_tx_low.add_gen_numpy_array_callback(np_array_add_MAC_addr_fields)
entry_tx_low.append_field_defs([
            ('timestamp',              'Q',      'uint64',  'Microsecond timer value at time packet transmission actually started (PHY TX_START time)'),
            ('uniq_seq',               'Q',      'uint64',  'Unique sequence number of original MPDU'),
            ('rate',                   'B',      'uint8',   'PHY rate index in [1:8]'),
            ('ant_mode',               'B',      'uint8'    'PHY antenna mode in [1:4]'),
            ('tx_power',               'b',      'int8',    'Tx power in dBm'),
            ('flags',                  'B',      'uint8',   'Tx PHY flags'),
            ('tx_count',               'B',      'uint8',   'Transmission index for this attempt; 0 = initial Tx, 1+ = subsequent re-transmissions'),
            ('chan_num',               'B',      'uint8',   'Channel (center frequency) index'),
            ('length',                 'H',      'uint16',  'Length in bytes of MPDU; includes MAC header, payload and FCS'),
            ('num_slots',              'H',      'uint16',  'Number of backoff slots allotted prior to this transmission; may not have been used for initial Tx (tx_count==0)'),
            ('cw',                     'H',      'uint16',  'Contention window value at time of this Tx'),
            ('pkt_type',               'B',      'uint8',   'Packet type: 1 = other data, 2 = encapsulated Ethernet, 3 = LTG, 11 = management, 21 = control'),
            ('padding',                '3x',     '3uint8'), ''
            ('mac_payload_len',        'I',      'uint32',  'Length in bytes of MAC payload recorded in log for this packet'),
            ('mac_payload',            '24s',    '24uint8', 'First 24 bytes of MAC payload, typically the 802.11 MAC header')])

# Tx / Rx Statistics
entry_txrx_stats = WlanExpLogEntryType(name='TXRX_STATS', entry_type_id=ENTRY_TYPE_TXRX_STATS)
entry_txrx_stats.description  = 'Copy of the Tx/Rx statistics struct maintained by CPU High. If promiscuous statistics mode is Tx/Rx stats structs will be maintained '
entry_txrx_stats.description += 'for every unique source MAC address, up to the max_stats value. Otherwise statistics are maintaind only associated nodes.'
entry_txrx_stats.append_field_defs([
            ('timestamp',                      'Q',      'uint64',  'Microsecond timer value at time of log entry creation'),
            ('last_timestamp',                 'Q',      'uint64',  'Microsecond timer value at time of last Tx or Rx event to node with address mac_addr'),
            ('mac_addr',                       '6s',     '6uint8',  'MAC address of remote node whose statics are recorded here'),
            ('associated',                     'B',      'uint8',   'Boolean indicating whether remote node is currently associated with this node'),
            ('padding',                        'x',      'uint8',   ''),
            ('data_num_rx_bytes',              'Q',      'uint64',  'Total number of bytes received in DATA packets from remote node'),
            ('data_num_tx_bytes_success',      'Q',      'uint64',  'Total number of bytes successfully transmitted in DATA packets to remote node'),
            ('data_num_tx_bytes_total',        'Q',      'uint64',  'Total number of bytes transmitted (successfully or not) in DATA packets to remote node'),
            ('data_num_rx_packets',            'I',      'uint32',  'Total number of DATA packets received from remote node'),
            ('data_num_tx_packets_success',    'I',      'uint32',  'Total number of DATA packets successfully transmitted to remote node'),
            ('data_num_tx_packets_total',      'I',      'uint32',  'Total number of DATA packets transmitted (successfully or not) to remote node'),
            ('data_num_tx_packets_low',        'I',      'uint32',  'Total number of PHY transmissions of DATA packets to remote node (includes re-transmissions)'),
            
            ('mgmt_num_rx_bytes',              'Q',      'uint64',  'Total number of bytes received in management packets from remote node'),),
            ('mgmt_num_tx_bytes_success',      'Q',      'uint64',  'Total number of bytes successfully transmitted in management packets to remote node'),
            ('mgmt_num_tx_bytes_total',        'Q',      'uint64',  'Total number of bytes transmitted (successfully or not) in management packets to remote node'),
            ('mgmt_num_rx_packets',            'I',      'uint32',  'Total number of management packets received from remote node'),
            ('mgmt_num_tx_packets_success',    'I',      'uint32',  'Total number of management packets successfully transmitted to remote node'),
            ('mgmt_num_tx_packets_total',      'I',      'uint32',  'Total number of management packets transmitted (successfully or not) to remote node'),
            ('mgmt_num_tx_packets_low',        'I',      'uint32',  'Total number of PHY transmissions of management packets to remote node (includes re-transmissions)')])
