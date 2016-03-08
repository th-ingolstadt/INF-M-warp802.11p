# -*- coding: utf-8 -*-
"""
.. ------------------------------------------------------------------------------
.. WLAN Experiment Node - Station (STA)
.. ------------------------------------------------------------------------------
.. Authors:   Chris Hunter (chunter [at] mangocomm.com)
..            Patrick Murphy (murphpo [at] mangocomm.com)
..            Erik Welsh (welsh [at] mangocomm.com)
.. License:   Copyright 2014-2015, Mango Communications. All rights reserved.
..            Distributed under the WARP license (http://warpproject.org/license)
.. ------------------------------------------------------------------------------
.. MODIFICATION HISTORY:
..
.. Ver   Who  Date     Changes
.. ----- ---- -------- -----------------------------------------------------
.. 1.00a ejw  1/23/14  Initial release
.. ------------------------------------------------------------------------------

"""

import wlan_exp.node as node
import wlan_exp.cmds as cmds


__all__ = ['WlanExpNodeSta']


class WlanExpNodeSta(node.WlanExpNode):
    """802.11 Station (STA) functionality for a WLAN Experiment node.
    
    Args:
        network_config (transport.NetworkConfiguration) : Network configuration of the node
        mac_type (int)                                  : CPU Low MAC type
    """
    
    #-------------------------------------------------------------------------
    # Override WLAN Exp Node Commands 
    #-------------------------------------------------------------------------
    def configure_bss(self, bssid=False, ssid=None, channel=None, beacon_interval=False, ht_capable=None):
        """Configure the BSS information of the node
        
        Each node is either a member of no BSS (colloquially "unassociated") 
        or a member of one BSS.  A node requires a minimum valid bss_info to 
        be a member of a BSS. The minimum valid bss_info has:
            #. BSSID: 48-bit MAC address
            #. Channel: Logical channel for Tx/Rx by BSS members
            #. SSID: Variable length string (ie the name of the network)
            
        This method is used to manipulate node parameters that affect BSS state
        
        Args:
            bssid (int, str):  48-bit ID of the BSS either as a integer or 
                colon delimited string of the form:  XX:XX:XX:XX:XX:XX
            ssid (str):  SSID string (Must be 32 characters or less)
            channel (int): Channel number on which the BSS operates
            beacon_interval (int): Not used by STA
            ht_capable (bool):  Is the PHY mode HTMF (True) or NONHT (False)?
        """
        # if beacon_interval:
        #     print("WARNING:  Beacon interval is ignored by STA.")
        
        self.send_cmd(cmds.NodeConfigBSS(bssid=bssid, ssid=ssid, channel=channel,
                                         ht_capable=ht_capable))


    def disassociate(self, device_list):
        """Remove associations of devices within the device_list from the association table

        .. note:: This command is not supported by IBSS nodes.  Please use diassociate_all().
            This function will raise a NotImplementedError.
        
        Attributes:
            device_list (list of WlanExpNode / WlanDevice):  List of 802.11 devices or single 
                802.11 device for which to disassociate
        """
        print("Error:  disassociate(device_list) is not supported for STA nodes.  Please use disassociate_all().")
        raise NotImplementedError



    #-------------------------------------------------------------------------
    # STA specific WLAN Exp Commands 
    #-------------------------------------------------------------------------
    def _check_allowed_rate(self, mcs, phy_mode, verbose=False):
        """Check that rate parameters are allowed

        Args:
            mcs (int):           Modulation and coding scheme (MCS) index
            phy_mode (str, int): PHY mode (from util.phy_modes)

        Returns:
            valid (bool):  Are all parameters valid?
        """
        # TODO: implement STA-specific rate checking here
        #  Allow all supported rates for now

        return self._check_supported_rate(mcs, phy_mode, verbose)



    #-------------------------------------------------------------------------
    # STA specific WLAN Exp Commands 
    #-------------------------------------------------------------------------
    def set_aid(self, aid):
        """Set the Association ID (AID) of the STA
        
        Args:
            aid (int): Association ID (must be in [1, 255])
        """
        self.send_cmd(cmds.NodeSTASetAID(aid))
    

    def join_network(self, ssid, bssid=None, channel=None, timeout=5.0):
        """Join the specified network (BSS).

        By specifying the SSID, the STA will try to join the given network.  If
        the bssid and channel of the network are also known, they can be 
        provided.  Otherwise, the STA will scan for the given SSID to find the 
        corresponding bssid and channel.  If the SSID is None, then any 
        on-going join process will be halted.  
        
        If the node is currently associated with the given BSS, then the node
        state is not changed and this method will return "success" immediately.
        
        By default, the join process has a timeout that will automatically
        halt the join process once the timeout is exceeded.  Depending on the 
        scan parameters, this timeout value may need to be adjusted to allow
        the STA to scan all channels before the timeout period is exceeded.  
        If the timeout is None, then the method will return immediately and
        the methods is_joining() and get_bss_info() can be used to determine 
        if the STA has joined the BSS.
        
        Args:
            ssid (str):  SSID string (Must be 32 characters or less); a value
                of None will stop any current join process.
            bssid (int, str):  48-bit ID of the BSS either as a integer or 
                colon delimited string of the form:  XX:XX:XX:XX:XX:XX
            channel (int): Channel number on which the BSS operates
            timeout (float):  Time to complete the join process; a value of 
                None will cause the method to return immediately and allow 
                the join process to continue forever until the node has joined
                the network or the join has failed.
                
        
        ..note::  This method will use the current scan parameters.
        
        """
        status = True
        
        # Check if node is currently associated with ssid / bssid
        #     - _check_associated_node() returns True if ssid is None
        if self._check_associated_node(ssid, bssid):
            # Still need to send the Join command to stop join process
            if ssid is None:
                self.send_cmd(cmds.NodeSTAJoin(ssid=None))
        else:
            # Perform the join process

            # Send the join command
            status = self.send_cmd(cmds.NodeSTAJoin(ssid=ssid, bssid=bssid, channel=channel))
            
            if timeout is not None:
                import time
                
                # Get the start time so that the timeout can be enforced
                start_time = time.time()

                # Check when join process completes that STA has joined BSS                
                while ((time.time() - start_time) < timeout):
                    if self.is_joining():
                        # Sleep for 0.1 seconds so we don't flood the node
                        time.sleep(0.1)
                    else:
                        status = self._check_associated_node(ssid, bssid)
                        break
        
        return status        


    def is_joining(self):
        """Is the node currently in the join process?
        
        Returns:
            status (bool):  
                * True      -- Inidcates node is currently in the join process
                * False     -- Indicates node is not currently in the join process
        """
        return self.send_cmd(cmds.NodeSTAJoinStatus())



    #-------------------------------------------------------------------------
    # Internal STA methods
    #-------------------------------------------------------------------------
    def _check_associated_node(self, ssid, bssid):
        """Check if node is currently associated to the given BSS
        
        This method only checks that the bss_info of the node matches the
        provided ssid / bssid and is not a substitute for is_associated().
        
        This method will only check the bssid after it matches the SSID.  If
        either argument is None, then it will not be checked.  If SSID is 
        None, then the method will return True.
        
        Args:
            ssid  (str):  SSID of the BSS
            bssid (str):  BSSID of the BSS

        Returns:
            associated (bool): Do the SSID / BSSID matche the BSS of the node?
        """
        if ssid is not None:
            bss_info = self.get_bss_info()
        
            if bss_info is None:
                return False
            
            if (bss_info['ssid'] != ssid):
                return False
        
            if bssid is not None:
                if (bss_info['bssid'] != bssid):
                    return False
        
        return True



    #-------------------------------------------------------------------------
    # Misc methods for the Node
    #-------------------------------------------------------------------------
    def __str__(self):
        """Pretty print WlanExpNodeSta object"""
        msg = ""

        if self.serial_number is not None:
            from wlan_exp.util import mac_addr_to_str
            msg += "WLAN EXP STA Node:\n"
            msg += "    WLAN MAC addr :  {0}\n".format(mac_addr_to_str(self.wlan_mac_address))
            msg += "    Node ID       :  {0}\n".format(self.node_id)
            msg += "    Serial #      :  {0}\n".format(self.sn_str)
            msg += "    HW version    :  WARP v{0}\n".format(self.hw_ver)
        else:
            msg += "Node not initialized."

        if self.transport is not None:
            msg += "WLAN EXP "
            msg += str(self.transport)

        return msg


    def __repr__(self):
        """Return node name and description"""
        msg = super(WlanExpNodeSta, self).__repr__()
        msg = "WLAN EXP STA  " + msg
        return msg

# End Class WlanExpNodeSta
