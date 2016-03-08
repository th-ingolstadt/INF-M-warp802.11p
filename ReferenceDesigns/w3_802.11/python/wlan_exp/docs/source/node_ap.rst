.. _wlan_exp_node_ap:

.. include:: globals.rst

WLAN Exp Node - Access Point (AP)
---------------------------------
Sub-class of WlanExpNode that adds 802.11 Access Point (AP) functionality.  

For more information please refer to the `AP documentation <http://warpproject.org/trac/wiki/802.11/MAC/Upper/AP>`_.


Class
.....
.. autoclass:: wlan_exp.node_ap.WlanExpNodeAp


AP Specific Implementation of Node Commands
...........................................
These commands have Access Point specific implementations.

.. autoclass:: wlan_exp.node_ap.WlanExpNodeAp
   :members: configure_bss, enable_beacon_mac_time_update


AP Commands
...........
These commands are specific to Access Points.

.. autoclass:: wlan_exp.node_ap.WlanExpNodeAp
   :members: enable_DTIM_multicast_buffering, set_authentication_address_filter, add_association








