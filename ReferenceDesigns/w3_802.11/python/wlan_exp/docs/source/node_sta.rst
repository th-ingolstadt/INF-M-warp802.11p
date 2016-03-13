.. _wlan_exp_node_sta:

.. include:: globals.rst


STA (STA) Node
--------------
Subclass of WlanExpNode that interfaces to an 802.11 Reference Design node running the
Access Point (AP) application in CPU High. An AP node supports all the :doc:`common node<node>`
methods plus the AP-specific methods described below.

STA Node Methods
................

.. automethod:: wlan_exp.node_sta.WlanExpNodeSta.configure_bss
.. automethod:: wlan_exp.node_sta.WlanExpNodeSta.disassociate
.. automethod:: wlan_exp.node_sta.WlanExpNodeSta.set_aid
.. automethod:: wlan_exp.node_sta.WlanExpNodeSta.join_network
.. automethod:: wlan_exp.node_sta.WlanExpNodeSta.is_joining

