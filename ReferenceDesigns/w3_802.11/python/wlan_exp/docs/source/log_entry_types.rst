.. _log_entry_types:

.. include:: globals.rst


Log Entry
---------

The log defines a number of entry types to collect information.  The definition of the various entry types can 
be found in the :ug:`user guide <wlan_exp/log/entry_types>`.


Log Entry Class
...............
.. autoclass:: wlan_exp.log.entry_types.WlanExpLogEntryType
   :members: get_field_names, get_field_struct_formats, get_entry_type_id, append_field_defs, modify_field_def, add_gen_numpy_array_callback, deserialize, serialize


Log Entry Functions
...................

.. autofunction:: wlan_exp.log.entry_types.np_array_add_fields


Log Entry Variables
...................

.. autoattribute:: wlan_exp.log.entry_types.entry_node_info
    :annotation: = NODE_INFO log entry

.. autoattribute:: wlan_exp.log.entry_types.entry_exp_info_hdr
    :annotation: = EXP_INFO log entry

.. autoattribute:: wlan_exp.log.entry_types.entry_station_info
    :annotation: = STATION_INFO log entry

.. autoattribute:: wlan_exp.log.entry_types.entry_bss_info
    :annotation: = BSS_INFO log entry

.. autoattribute:: wlan_exp.log.entry_types.entry_wn_cmd_info
    :annotation: = WN_CMD_INFO log entry

.. autoattribute:: wlan_exp.log.entry_types.entry_time_info
    :annotation: = TIME_INFO log entry

.. autoattribute:: wlan_exp.log.entry_types.entry_node_temperature
    :annotation: = NODE_TEMPERATURE log entry

.. autoattribute:: wlan_exp.log.entry_types.entry_rx_ofdm
    :annotation: = RX_OFDM log entry

.. autoattribute:: wlan_exp.log.entry_types.entry_rx_ofdm_ltg
    :annotation: = RX_OFDM_LTG log entry

.. autoattribute:: wlan_exp.log.entry_types.entry_rx_dsss
    :annotation: = RX_DSSS log entry

.. autoattribute:: wlan_exp.log.entry_types.entry_tx
    :annotation: = TX log entry

.. autoattribute:: wlan_exp.log.entry_types.entry_tx_ltg
    :annotation: = TX_LTG log entry

.. autoattribute:: wlan_exp.log.entry_types.entry_tx_low
    :annotation: = TX_LOW log entry

.. autoattribute:: wlan_exp.log.entry_types.entry_tx_low_ltg
    :annotation: = TX_LOW_LTG log entry

.. autoattribute:: wlan_exp.log.entry_types.entry_txrx_stats
    :annotation: = TXRX_STATS log entry

