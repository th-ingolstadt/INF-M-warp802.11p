.. _wlan_exp_node_ibss:

.. include:: globals.rst

WLAN Exp Node - IBSS (Ad hoc)
-----------------------------
Sub-class of WlanExpNode that adds 802.11 IBSS (Ad hoc) functionality.  

For more information please refer to the `IBSS documentation <http://warpproject.org/trac/wiki/802.11/MAC/Upper/IBSS>`_.


Class
.....
.. autoclass:: wlan_exp.node_ibss.WlanExpNodeIBSS


IBSS Specific Implementation of Node Commands
.............................................
These commands have IBSS specific implementations.

.. autoclass:: wlan_exp.node_ibss.WlanExpNodeIBSS
   :members: counts_get_txrx, configure_bss, disassociate

