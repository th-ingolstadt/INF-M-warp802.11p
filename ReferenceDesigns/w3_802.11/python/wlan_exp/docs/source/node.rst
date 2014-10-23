.. _wlan_exp_node:

.. include:: globals.rst

WLAN Exp Node
-------------

The WLAN experiment node represents one node in a WLAN experiment network.  This class is the primary 
interface for interacting with nodes by providing methods for sending commands and checking the status 
of the node.  This is the base class for all nodes and provides the common functionality for all nodes.

Documentation of sub-classes of WlanExpNode will focus on additional functionality provided by that 
particular sub-class.  

.. toctree::
    :maxdepth: 1

    node_ap.rst
    node_ibss.rst
    node_sta.rst

Common Commands
```````````````

Class
.....
.. autoclass:: wlan_exp.node.WlanExpNode


Log Commands
............

These WlanExpNode commands are used to interact with the logging framework.  The log occupies
a large portion of DRAM which is set in C code during runtime (see `wlan_mac_high.h 
<http://warpproject.org/trac/browser/ReferenceDesigns/w3_802.11/c/wlan_mac_high_framework/include/wlan_mac_high.h>`_ 
for more information on the memory map; Use log_get_capacity() to see the number of bytes allocated for the log).

The log has the ability to wrap to enable longer experiments.  By enabling wrapping and periodically pulling
the logs, you can effectively log an experiment for an indefinite amount of time.  Also, the log can record the 
entire payload of the wireless packets.  Both the log wrapping and logging of full payloads is off by default and 
can be modified with the log_configure() command.

.. autoclass:: wlan_exp.node.WlanExpNode
   :members: log_configure, log_get, log_get_all_new, log_get_size, log_get_capacity, log_get_indexes, log_get_flags, log_is_full, log_write_exp_info, log_write_time, log_write_txrx_stats


Statistics Commands
...................

These WlanExpNode commands are used to to interact with the statistics framework.  Statistics are kept for for
each association (the statistics that are kept can be found as part of the `log entry types
<http://warpproject.org/trac/wiki/802.11/wlan_exp/log/entry_types>`_).  If promiscuous statistics are enabled, 
then the node will keep statistics for every MAC address overheard (whether the node is associated or not).  In 
order to keep the maximum number of statistics recorded on the node to a reasonable amount, there is a maximum number
of statistics implemented in the C code.  When that maximum is reached, then the oldest statistics structure of an 
unassociated node will be overwritten.

.. autoclass:: wlan_exp.node.WlanExpNode
   :members: stats_configure_txrx, stats_get_txrx


Local Traffic Generator (LTG) Commands
......................................

These WlanExpNode commands are used to interact with the LTG framework.  LTGs are used to have the node locally
generate different types of traffic for experiments.  See _wlan_exp_ltg for more information on LTGs.

.. note:: Please be careful that LTGs are removed when they are done being used.  By not removing unused LTGs, this
    will not allow memory to be freed within the heap on the node and can lead to memory allocation failures when
    trying to create new LTGs.

.. autoclass:: wlan_exp.node.WlanExpNode
   :members: ltg_configure, ltg_get_status, ltg_remove, ltg_start, ltg_stop, ltg_remove_all, ltg_start_all, ltg_stop_all


Node Commands
.............
These WlanExpNode commands are used to interact with the node and control parameters associated with the node operation.

.. autoclass:: wlan_exp.node.WlanExpNode
   :members: reset_all, reset, get_wlan_mac_address, set_time, get_time, set_low_to_high_rx_filter, set_channel, get_channel, set_tx_rate_unicast 


Association Commands
....................

Queue Commands
..............




Full List
.........

.. automodule:: wlan_exp.node
   :members:
   :inherited-members:
   :exclude-members: configure_node, check_wlan_exp_ver
