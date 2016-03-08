.. _wlan_exp_node_sta:

.. include:: globals.rst

WLAN Exp Node - Station (STA)
-----------------------------
Sub-class of WlanExpNode that adds 802.11 Station (STA) functionality.  

For more information please refer to the `STA documentation <http://warpproject.org/trac/wiki/802.11/MAC/Upper/STA>`_.


Class
.....
.. autoclass:: wlan_exp.node_sta.WlanExpNodeSta


STA Specific Implementation of Node Commands
............................................
These commands have Station specific implementations.

.. autoclass:: wlan_exp.node_ap.WlanExpNodeSta
   :members: configure_bss, disassociate


STA Commands
............
These commands are specific to STA nodes.

.. autoclass:: wlan_exp.node_sta.WlanExpNodeSta
   :members: set_aid, join_network, is_joining

