# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment Node
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
MODIFICATION HISTORY:

Ver   Who  Date     Changes
----- ---- -------- -----------------------------------------------------
1.00a ejw  1/23/14  Initial release

------------------------------------------------------------------------------

This module provides class definition for WLAN Exp Node.

Functions (see below for more information):
    WlanExpNode() -- Base class for WLAN Exp node
    WlanExpNodeFactory() -- Base class for creating WLAN Exp nodes

Integer constants:
    NODE_WLAN_MAX_ASSN, NODE_WLAN_EVENT_LOG_SIZE, NODE_WLAN_MAX_STATS
        -- Node hardware parameter constants 

If additional hardware parameters are needed for sub-classes of WlanExpNode, 
pleasemake sure that the values of these hardware parameters are not reused.

"""


import warpnet.wn_node as wn_node
import warpnet.wn_exception as wn_ex

from . import version
from . import defaults
from . import cmds
from . import util


__all__ = ['WlanExpNode', 'WlanExpNodeFactory']



# WLAN Exp Node Parameter Identifiers (Extension of WARPNet Parameter Identifiers)
#   NOTE:  The C counterparts are found in *_node.h
NODE_WLAN_EXP_DESIGN_VER     = 6
NODE_WLAN_MAX_ASSN           = 7
NODE_WLAN_EVENT_LOG_SIZE     = 8
NODE_WLAN_MAX_STATS          = 9
NODE_WLAN_MAC_ADDR           = 10


class WlanExpNode(wn_node.WnNode):
    """Base Class for WLAN Experiment node.
    
    The WLAN experiment node represents one node in a WLAN experiment network.  
    This class is the primary interface for interacting with nodes by 
    providing method for sending commands and checking status of nodes.
    
    The base WLAN experiment nodes builds off the WARPNet node and utilizes
    the attributes of the WARPNet node.
    
    Attributes (inherited from WnNode):
        node_type       -- Unique type of the WARPNet node
        node_id         -- Unique identification for this node
        name            -- User specified name for this node (supplied by user scripts)
        description     -- String description of this node (auto-generated)
        serial_number   -- Node's serial number, read from EEPROM on hardware
        fpga_dna        -- Node's FPGA'a unique identification (on select hardware)
        hw_ver          -- WARP hardware version of this node
        wn_ver_major    -- WARPNet version running on this node
        wn_ver_minor
        wn_ver_revision
        transport       -- Node's transport object
        transport_bcast -- Node's broadcast transport object

    New Attributes:
        max_associations     -- Maximum associations of the node
        log_max_size         -- Maximum size of event log (in bytes)
        log_total_bytes_read -- Number of bytes read from the event log
        log_num_wraps        -- Number of times the event log has wrapped
        log_next_read_index  -- Index in to event log of next read

        wlan_exp_ver_major   -- WLAN Exp version running on this node
        wlan_exp_ver_minor
        wlan_exp_ver_revision        
    """
    wlan_mac_address      = None
    max_associations      = None
    max_statistics        = None
    log_max_size          = None
    log_total_bytes_read  = None
    log_num_wraps         = None
    log_next_read_index   = None

    wlan_exp_ver_major    = None
    wlan_exp_ver_minor    = None
    wlan_exp_ver_revision = None

    
    def __init__(self, host_config=None):
        super(WlanExpNode, self).__init__(host_config)
        
        (self.wlan_exp_ver_major, self.wlan_exp_ver_minor, 
                self.wlan_exp_ver_revision) = version.wlan_exp_ver(output=False)
        
        self.node_type            = defaults.WLAN_EXP_BASE
        self.max_associations     = 0
        self.max_statistics       = 0

        self.log_max_size         = 0
        self.log_total_bytes_read = 0
        self.log_num_wraps        = 0
        self.log_next_read_index  = 0


    def configure_node(self, jumbo_frame_support=False):
        """Get remaining information from the node and set remaining parameters."""
        # Call WarpNetNode apply_configuration method
        super(WlanExpNode, self).configure_node(jumbo_frame_support)
        
        # Set description
        self.description = str("WLAN EXP " + self.description)


    #-------------------------------------------------------------------------
    # WLAN Exp Commands for the Node
    #-------------------------------------------------------------------------


    #--------------------------------------------
    # Log Commands
    #--------------------------------------------
    def log_reset(self):
        """Reset the event log on the node."""
        # Update the internal state to stay consistent with the node.
        self.log_total_bytes_read = 0
        self.log_num_wraps        = 0
        self.log_next_read_index  = 0
        
        self.send_cmd(cmds.LogReset())


    def log_configure(self, flags):
        """Configure log with the given flags.
        
        Flags (32 bits):
            [0] - Allow log to wrap (1 - Enabled / 0 - Disabled )
            [1] - Log events (1 - Enabled / 0 - Disabled)
        """
        self.send_cmd(cmds.LogConfigure(flags))


    def log_get(self, size, offset=0):
        """Low level method to get part of the log file as a WnBuffer.
        
        Attributes:
            size -- Number of bytes to read from the log
            offset -- Starting byte to read from the log (optional)
        
        NOTE:  There is no guarentee that this will return data aligned to 
        event boundaries.  Use log_get_start() and log_get_end() to get 
        event aligned boundaries.
        
        NOTE:  Log reads are not destructive.  Log entries will only be
        destroyed by a log reset or if the log wraps.
        """
        return self.send_cmd(cmds.LogGetEvents(size, offset))


    def log_get_all_new(self, log_tail_pad=500):
        """Get all "new" entries in the log.

        Attributes:
            log_tail_pad  -- Number of bytes from the current end of the 
                               "new" entries that will not be read during 
                               the call.  This is to deal with the case that
                               the node is processing the last log entry so 
                               it contains incomplete data and should not be
                               read.
        
        Returns:
           WARPNet Buffer that contains all entries since the last time the 
             log was read.
        """
        return_val = b''
        (next_index, oldest_index, num_wraps) = self.log_get_indexes()

        if (num_wraps == self.log_num_wraps):
            if (next_index > (self.log_next_read_index + log_tail_pad)):
                return_val = self.log_get(offset=self.log_next_read_index, 
                                          size=(next_index - self.log_next_read_index - log_tail_pad))
                self.log_next_read_index = next_index
        else:
            if ((next_index != 0) or self.log_is_full()):
                return_val = self.log_get(offset=self.log_next_read_index, 
                                          size=cmds.LOG_GET_ALL_ENTRIES)
                self.log_next_read_index = 0
                self.log_num_wraps       = num_wraps

        return return_val


    def log_get_size(self):
        """Get the size of the log (in bytes)."""
        (capacity, size)  = self.send_cmd(cmds.LogGetCapacity())

        # Check the maximum size of the log and update the node state
        if (self.log_max_size != capacity):
            msg  = "EVENT LOG WARNING:  Log capacity changed.\n"
            msg += "    Went from {0} bytes to ".format(self.log_max_size)
            msg += "{0} bytes.\n".format(capacity)
            print(msg)
            self.log_max_size = capacity

        return size


    def log_get_capacity(self):
        """Get the capacity of the log (in bytes)."""
        return self.log_max_size


    def log_get_indexes(self):
        """Get the indexes that describe the state of the event log.
        
        Returns a tuple:
            (oldest_index, next_index, num_wraps)        
        """
        (next_index, oldest_index, num_wraps, _) = self.send_cmd(cmds.LogGetStatus())
        
        # Check that the log is in a good state
        if ((num_wraps < self.log_num_wraps) or 
            ((num_wraps == self.log_num_wraps) and 
             (next_index < self.log_next_read_index))):
            msg  = "\n!!! Event Log Corrupted.  Please reset the log. !!!\n"
            print(msg)
        
        return (next_index, oldest_index, num_wraps)


    def log_get_flags(self):
        """Get the flags that describe the event log configuration."""
        (_, _, _, flags) = self.send_cmd(cmds.LogGetStatus())

        return flags        


    def log_is_full(self):
        """Return whether the log is full or not."""
        (next_index, oldest_index, num_wraps, flags) = self.send_cmd(cmds.LogGetStatus())
        
        if (((flags & cmds.LOG_CONFIG_FLAG_WRAP) != cmds.LOG_CONFIG_FLAG_WRAP) and
            ((next_index == 0) and (oldest_index == 0) and (num_wraps == (self.log_num_wraps + 1)))):
            return True
        else:
            return False


    def log_enable_stream(self, port, ip_address=None, host_id=None):
        """Configure the node to stream log entries to the given port."""

        if (ip_address is None):
            ip_address = self.host_config.get_param('network', 'host_interfaces')[0]
            
        if (host_id is None):
            host_id = self.host_config.get_param('network', 'host_id')
        
        self.send_cmd(cmds.LogStreamEntries(1, host_id, ip_address, port))
        msg  = "{0}:".format(self.name) 
        msg += "Streaming Log Entries to {0} ({1})".format(ip_address, port)
        print(msg)


    def log_disable_stream(self):
        """Configure the node to disable log entries stream."""
        self.send_cmd(cmds.LogStreamEntries(0, 0, 0, 0))
        msg  = "{0}:".format(self.name) 
        msg += "Disable log entry stream."
        print(msg)


    def log_write_exp_info_to_log(self, reason, message=None):
        """Write the experiment information provided to the log.
        
        Attributes:
            reason -- Reason code for the experiment info.  This is an 
                      arbitrary 16 bit number chosen by the experimentor
            message -- Information to be placed in the event log
        """
        raise NotImplementedError


    #--------------------------------------------
    # Statistics Commands
    #--------------------------------------------
    def stats_configure_txrx(self, flags):
        """Configure statistics collection on the node.
        
        Flags (32 bits):
            [0] - Collect promiscuous statistics (1 - Enabled / 0 - Disabled )
        """
        self.send_cmd(cmds.StatsConfigure(flags))


    def stats_get_flags(self):
        """Get the configuration of the statistics collection on the node."""
        return self.send_cmd(cmds.StatsConfigure(cmds.STATS_RSVD_CONFIG))


    def stats_get_txrx(self, node_list=None):
        """Get the statistics from the node.
        
        Returns a list of statistic dictionaries.

        If a node_list is provided, then the command will only return
        statistics for the given nodes.  Otherwise, it will return 
        all the statistics on the node.
        """
        ret_stats = []
        if not node_list is None:
            if (type(node_list) is list):
                for node in node_list:
                    stats = self.send_cmd(cmds.StatsGetTxRx(node))
                    ret_stats.append(stats)
            else:
                ret_stats = self.send_cmd(cmds.StatsGetTxRx(node_list))
        else:
            ret_stats = self.send_cmd(cmds.StatsGetAllTxRx())
        
        return ret_stats
    

    def stats_write_txrx_to_log(self):
        """Write the current statistics to the log."""
        return self.send_cmd(cmds.StatsAddTxRxToLog())


    def stats_reset_txrx(self):
        """Reset the statistics on the node."""
        self.send_cmd(cmds.StatsResetTxRx())
        

    #--------------------------------------------
    # Local Traffic Generation (LTG) Commands
    #--------------------------------------------
    def ltg_to_node_configure(self, node_list, traffic_flow):
        """Configure the node for the given traffic flow to the given nodes."""
        if (type(node_list) is list):
            for node in node_list:
                status = self.send_cmd(cmds.LTGConfigure(node, traffic_flow))
                self._print_ltg_status('configure', status, node.name)
        else:
            status = self.send_cmd(cmds.LTGConfigure(node_list, traffic_flow))
            self._print_ltg_status('configure', status, node_list.name)


    def ltg_to_node_remove(self, node_list):
        """Remove the LTG flow to the given nodes from the node."""
        if (type(node_list) is list):
            for node in node_list:
                status = self.send_cmd(cmds.LTGRemove(node))
                self._print_ltg_status('remove', status, node.name)
        else:
            status = self.send_cmd(cmds.LTGRemove(node_list))
            self._print_ltg_status('remove', status, node_list.name)
        

    def ltg_to_node_start(self, node_list):
        """Start the LTG flow to the given nodes."""
        if (type(node_list) is list):
            for node in node_list:
                status = self.send_cmd(cmds.LTGStart(node))
                self._print_ltg_status('start', status, node.name)
        else:
            status = self.send_cmd(cmds.LTGStart(node_list))
            self._print_ltg_status('start', status, node_list.name)


    def ltg_to_node_stop(self, node_list):
        """Stop the LTG flow to the given nodes."""
        if (type(node_list) is list):
            for node in node_list:
                status = self.send_cmd(cmds.LTGStop(node))
                self._print_ltg_status('stop', status, node.name)
        else:
            status = self.send_cmd(cmds.LTGStop(node_list))
            self._print_ltg_status('stop', status, node_list.name)


    def ltg_remove_all(self):
        """Stops and removes all LTG flows on the node."""
        status = self.send_cmd(cmds.LTGRemove())
        if (status == cmds.LTG_ERROR):
            print("LTG ERROR: Could not remove all LTGs on {0}".format(self.name))
        

    def ltg_start_all(self):
        """Start all LTG flows on the node."""
        status = self.send_cmd(cmds.LTGStart())
        if (status == cmds.LTG_ERROR):
            print("LTG ERROR: Could not start all LTGs on {0}".format(self.name))


    def ltg_stop_all(self):
        """Stop all LTG flows on the node."""
        status = self.send_cmd(cmds.LTGStop())
        if (status == cmds.LTG_ERROR):
            print("LTG ERROR: Could not stop all LTGs on {0}".format(self.name))


    def _print_ltg_status(self, ltg_cmd_type, status, node_name):
        if (status == cmds.LTG_ERROR):
            print("LTG ERROR: Could not {0} LTG".format(ltg_cmd_type))
            print("    from {0} to {1}".format(self.name, node_name))


    #--------------------------------------------
    # Configure Node Attribute Commands
    #--------------------------------------------
    def node_is_associated(self, node_list):
        """Returns a either a boolean if the node list is a single node,
        or a list of booleans the same dimension as the node_list.  To 
        return True, both the current node and the node in the node_list 
        must be associated with each other.
        """
        ret_val = []
        
        if not node_list is None:
            if (type(node_list) is list):
                for idx, node in enumerate(node_list):
                    my_info   = self.node_get_station_info(node)
                    node_info = node.send_cmd(cmds.NodeGetStationInfo(self))
                    
                    # If the lists are not empty, then the nodes are associated
                    if my_info and node_info:
                        ret_val.append(True)
                    else:
                        ret_val.append(False)
            else:
                my_info   = self.node_get_station_info(node_list)
                node_info = node_list.send_cmd(cmds.NodeGetStationInfo(self))

                if my_info and node_info:
                    ret_val = True
                else:
                    ret_val = False
        else:
            ret_val = False
        
        return ret_val


    def node_get_station_info(self, node_list=None):
        """Get the station info from the node.
        
        Returns a list of station info dictionaries.

        If a node_list is provided, then the command will only return
        the station info for the given nodes.  Otherwise, it will return 
        all the station infos on the node.
        """
        ret_info = []
        if not node_list is None:
            if (type(node_list) is list):
                for node in node_list:
                    stats = self.send_cmd(cmds.NodeGetStationInfo(node))
                    ret_info.append(stats)
            else:
                ret_info = self.send_cmd(cmds.NodeGetStationInfo(node_list))
        else:
            ret_info = self.send_cmd(cmds.NodeGetAllStationInfo())
        
        return ret_info
    

    def node_set_time(self, time):
        """Sets the time in microseconds on the node.
        
        Attributes:
            time -- Time to send to the board (either float in sec or int in us)
        """
        self.send_cmd(cmds.NodeProcTime(time))
    

    def node_get_time(self):
        """Gets the time in microseconds from the node."""
        return self.send_cmd(cmds.NodeProcTime(cmds.RSVD_TIME))


    def node_set_channel(self, channel):
        """Sets the channel of the node and returns the channel that was set."""
        return self.send_cmd(cmds.NodeProcChannel(channel))
    

    def node_get_channel(self):
        """Gets the current channel of the node."""
        return self.send_cmd(cmds.NodeProcChannel(cmds.RSVD_CHANNEL))


    def node_set_tx_rate_unicast(self, rate, node_list=None):
        """Sets the unicast transmit rate of the node to the given nodes in 
        the node_list.  If the node list is empty, then it sets the default
        unicast transmit rate for future associations.  This returns a list 
        of unicast transmit rates that were set.  The rate provided must be
        an entry in the rates table found in wlan_exp.util
        """
        ret_val = []
        
        if (type(node_list) is list):
            for node in node_list:
                val = self.send_cmd(cmds.NodeProcTxRate(cmds.NODE_UNICAST, rate, node))
                ret_val.append(val)
        else:
            val = self.send_cmd(cmds.NodeProcTxRate(cmds.NODE_UNICAST, rate, node_list))
            ret_val.append(val)

        return ret_val


    def node_get_tx_rate_unicast(self, node_list=None):
        """Gets the unicast transmit rate of the node to the given nodes in 
        the node_list.  If the node list is empty, then it gets the default 
        unicast transmit rate for future associations.  All rates are returned
        as an index in to the rates table found in wlan_exp.util
        """
        ret_val = []
        
        if (type(node_list) is list):
            for node in node_list:
                val = self.send_cmd(cmds.ProcNodeTxRate(cmds.NODE_UNICAST, cmds.RSVD_TX_RATE, node))
                ret_val.append(val)
        else:
            val = self.send_cmd(cmds.ProcNodeTxRate(cmds.NODE_UNICAST, cmds.RSVD_TX_RATE, node_list))
            ret_val.append(val)

        return ret_val


    def node_set_tx_rate_multicast_data(self, rate):
        """Sets the multicast transmit rate for a node and returns the rate 
        that was set.  The rate provided must be an entry in the rates table 
        found in wlan_exp.util
        """
        return self.send_cmd(cmds.NodeProcTxRate(cmds.NODE_MULTICAST, rate))


    def node_get_tx_rate_multicast_data(self):
        """Gets the current multicast transmit rate for a node as an index
        in to the rates table found in wlan_exp.util
        """
        return self.send_cmd(cmds.NodeProcTxRate(cmds.NODE_MULTICAST, cmds.RSVD_TX_RATE))


    def node_set_tx_ant_mode_unicast(self, ant_mode, node_list=None):
        """Sets the unicast transmit antenna mode of the node to the given 
        nodes in the node_list.  If the node list is empty, then it sets 
        the default antenna mode for future associations.  This returns a 
        list of antenna modes that were set.
        """
        ret_val = []
        
        if (type(node_list) is list):
            for node in node_list:
                val = self.send_cmd(cmds.ProcNodeTxAntMode(cmds.NODE_UNICAST, ant_mode, node))
                ret_val.append(val)
        else:
            val = self.send_cmd(cmds.ProcNodeTxAntMode(cmds.NODE_UNICAST, ant_mode, node_list))
            ret_val.append(val)

        return ret_val


    def node_get_tx_ant_mode_unicast(self, node_list=None):
        """Gets the unicast transmit antenna mode of the node to the given 
        nodes in the node_list and returns a list of those antenna modes.
        """
        ret_val = []
        
        if (type(node_list) is list):
            for node in node_list:
                val = self.send_cmd(cmds.ProcNodeTxAntMode(cmds.NODE_UNICAST, cmds.RSVD_TX_ANT_MODE, node))
                ret_val.append(val)
        else:
            val = self.send_cmd(cmds.ProcNodeTxAntMode(cmds.NODE_UNICAST, cmds.RSVD_TX_ANT_MODE, node_list))
            ret_val.append(val)

        return ret_val


    def node_set_tx_ant_mode_multicast(self, ant_mode):
        """Sets the multicast transmit antenna mode for a node and returns the 
        antenna mode that was set.
        """
        return self.send_cmd(cmds.NodeProcTxAntMode(cmds.NODE_MULTICAST, ant_mode))


    def node_get_tx_ant_mode_multicast(self):
        """Gets the current multicast transmit antenna mode for a node."""
        return self.send_cmd(cmds.NodeProcTxAntMode(cmds.NODE_MULTICAST, cmds.RSVD_TX_ANT_MODE))


    def node_set_rx_ant_mode(self, ant_mode):
        """Sets the receive antenna mode for a node and returns the 
        antenna mode that was set.
        """
        return self.send_cmd(cmds.NodeProcRxAntMode(ant_mode))


    def node_get_rx_ant_mode(self):
        """Gets the current receive antenna mode for a node."""
        return self.send_cmd(cmds.NodeProcRxAntMode(cmds.RSVD_RX_ANT_MODE))


    def node_set_tx_power(self, power):
        """Sets the transmit power of the node and returns the power that was set."""
        return self.send_cmd(cmds.NodeProcTxPower(power))


    def node_get_tx_power(self):
        """Gets the current transmit power of the node."""
        return self.send_cmd(cmds.NodeProcTxPower(cmds.RSVD_TX_POWER))



    #--------------------------------------------
    # Queue Commands
    #--------------------------------------------
    def queue_tx_data_purge_all(self):
        """Purges all data transmit queues on the node."""
        self.send_cmd(cmds.QueueTxDataPurgeAll())



    #-------------------------------------------------------------------------
    # WARPNet Parameter Framework
    #   Allows for processing of hardware parameters
    #-------------------------------------------------------------------------
    def process_parameter(self, identifier, length, values):
        """Extract values from the parameters"""
        if (identifier == NODE_WLAN_EXP_DESIGN_VER):
            if (length == 1):                
                self.wlan_exp_ver_major = (values[0] & 0x00FF0000) >> 16
                self.wlan_exp_ver_minor = (values[0] & 0x0000FF00) >> 8
                self.wlan_exp_ver_revision = (values[0] & 0x000000FF)                
                
                # Check to see if there is a version mismatch
                self.check_wlan_exp_ver()
            else:
                raise wn_ex.ParameterError("NODE_DESIGN_VER", "Incorrect length")

        elif (identifier == NODE_WLAN_MAX_ASSN):
            if (length == 1):
                self.max_associations = values[0]
            else:
                raise wn_ex.ParameterError("NODE_WLAN_MAX_ASSN", "Incorrect length")

        elif (identifier == NODE_WLAN_EVENT_LOG_SIZE):
            if (length == 1):
                self.log_max_size = values[0]
            else:
                raise wn_ex.ParameterError("NODE_WLAN_EVENT_LOG_SIZE", "Incorrect length")

        elif (identifier == NODE_WLAN_MAX_STATS):
            if (length == 1):
                self.max_statistics = values[0]
            else:
                raise wn_ex.ParameterError("NODE_WLAN_MAX_STATS", "Incorrect length")

        elif   (identifier == NODE_WLAN_MAC_ADDR):
            if (length == 2):
                self.wlan_mac_address = ((2**32) * (values[0] & 0xFFFF) + values[1])
            else:
                raise wn_ex.ParameterError("NODE_WLAN_MAC_ADDR", "Incorrect length")

        else:
            super(WlanExpNode, self).process_parameter(identifier, length, values)


    #-------------------------------------------------------------------------
    # Misc methods for the Node
    #-------------------------------------------------------------------------
    def check_wlan_exp_ver(self):
        """Check the WLAN Exp version of the node against the current WLAN Exp
        version."""
        version.wlan_exp_ver_check(major=self.wlan_exp_ver_major,
                                   minor=self.wlan_exp_ver_minor,
                                   revision=self.wlan_exp_ver_revision)

# End Class WlanExpNode




class WlanExpNodeFactory(wn_node.WnNodeFactory):
    """Sub-class of WARPNet node factory used to help with node configuration 
    and setup.
        
    Attributes (inherited):
        wn_dict -- Dictionary of WARPNet Node Types to class names
    """
    def __init__(self, host_config=None):
        super(WlanExpNodeFactory, self).__init__(host_config)
        
        # Add default classes to the factory
        self.node_add_class(defaults.WLAN_EXP_AP_TYPE, 
                            defaults.WLAN_EXP_AP_CLASS)

        self.node_add_class(defaults.WLAN_EXP_STA_TYPE, 
                            defaults.WLAN_EXP_STA_CLASS)

    
    def node_eval_class(self, node_class, host_config):
        """Evaluate the node_class string to create a node.  
        
        NOTE:  This should be overridden in each sub-class with the same
        overall structure but a different import.  Please call the super
        class so that the calls will propagate to catch all node types.
        """
        from . import node_ap
        from . import node_sta
        
        node = None

        try:
            full_node_class = node_class + "(host_config)"
            node = eval(full_node_class, globals(), locals())
        except:
            pass
        
        if node is None:
            return super(WlanExpNodeFactory, self).node_eval_class(node_class, 
                                                                   host_config)
        else:
            return node


# End Class WlanExpNodeFactory
