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

.. autoclass:: wlan_exp.node.WlanExpNode
   :members: 




Statistics Commands
...................

Local Traffic Generator (LTG) Commands
......................................

Node Commands
.............

Association Commands
....................

Queue Commands
..............




Full List
.........

.. autoclass:: wlan_exp.node.WlanExpNode
   :members:
   :inherited-members:
   :exclude-members: configure_node, check_wlan_exp_ver
