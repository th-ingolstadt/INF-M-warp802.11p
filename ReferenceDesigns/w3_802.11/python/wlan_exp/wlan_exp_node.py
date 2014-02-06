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
import warpnet.wn_message as wn_message
import warpnet.wn_config as wn_config
import warpnet.wn_exception as ex

from . import wlan_exp_defaults
from . import wlan_exp_cmds
from . import wlan_exp_util


__all__ = ['WlanExpNode', 'WlanExpNodeFactory']



# WLAN Exp Node Parameter Identifiers (Extension of WARPNet Parameter Identifiers)
#   NOTE:  The C counterparts are found in *_node.h
NODE_WLAN_MAX_ASSN           = 6
NODE_WLAN_EVENT_LOG_SIZE     = 7
NODE_WLAN_MAX_STATS          = 8


class WlanExpNode(wn_node.WnNode):
    """Base Class for WLAN Experiment node.
    
    The WLAN experiment node represents one node in a WLAN experiment network.  
    This class is the primary interface for interacting with nodes by 
    providing method for sending commands and checking status of nodes.
    
    The base WLAN experiment nodes builds off the WARPNet node and utilizes
    the attributes of the WARPNet node.
    
    Attributes (inherited from WnNode):
        node_type -- Unique type of the WARPNet node
        node_id -- Unique identification for this node
        name -- User specified name for this node (supplied by user scripts)
        description -- String description of this node (auto-generated)
        serial_number -- Node's serial number, read from EEPROM on hardware
        fpga_dna -- Node's FPGA'a unique identification (on select hardware)
        hw_ver -- WARP hardware version of this node
        wn_ver_major -- WARPNet version running on this node
        wn_ver_minor
        wn_ver_revision
        transport -- Node's transport object
        transport_bcast -- Node's broadcast transport object

    New Attributes:
        max_associations -- Maximum associations of the node
        event_log_size -- Size of event log (in bytes)
        event_log -- Event log object

        wlan_exp_ver_major -- WLAN Exp version running on this node
        wlan_exp_ver_minor
        wlan_exp_ver_revision        
    """
    max_associations      = None
    max_statistics        = None
    event_log_size        = None
    event_log             = None

    wlan_exp_ver_major    = None
    wlan_exp_ver_minor    = None
    wlan_exp_ver_revision = None

    
    def __init__(self):
        super(WlanExpNode, self).__init__()
        
        (self.wlan_exp_ver_major, self.wlan_exp_ver_minor, 
                self.wlan_exp_ver_revision) = wlan_exp_util.wlan_exp_ver(output=0)
        
        self.node_type = wlan_exp_defaults.WLAN_EXP_BASE
        self.max_associations = 0
        self.max_statistics = 0
        self.event_log = None


    def configure_node(self, jumbo_frame_support=False):
        """Get remaining information from the node and set remaining parameters."""
        # Call WarpNetNode apply_configuration method
        super(WlanExpNode, self).configure_node(jumbo_frame_support)
        
        # Set description
        self.description = str("WLAN EXP " + self.description)


    #-------------------------------------------------------------------------
    # WLAN Exp Commands for the Node
    #-------------------------------------------------------------------------
    def get_log(self, file_name=None):
        """Get the entire log file as a WnBuffer.  
        
        Optionally, save the contents to the provided file name."""
        resp  = None
        start = self.get_log_start()
        end   = self.get_log_end()
        
        if (start < end):
            resp = self.get_log_events((end - start), start)
        else:
            resp = self.get_log_events((self.event_log_size - start), start)
            temp = self.get_log_events(start, 0)
            resp.append(temp)

        if not file_name is None:
            file_byte_array = resp.get_bytes()
            try:
                with open(file_name, 'wb') as data_file:
                    data_file.write(file_byte_array)
            except IOError as err:
                print("Error writing config file: {0}".format(err))

        return resp

    def get_log_events(self, size, start_byte=0):
        """Low level method to get part of the log file as a WnBuffer.
        
        Attributes:
            size -- Number of bytes to read from the log
            start_byte -- Starting byte to read from the log (optional)
        
        NOTE:  There is no guarentee that this will return data aligned to 
        event boundaries.  Use get_log_start() and get_log_end() to get 
        event aligned boundaries.
        
        NOTE:  Log reads are not destructive.  Log entries will only be
        destroyed by a log reset or if the log wraps.
        """
        return self.send_cmd(wlan_exp_cmds.WlanExpCmdLogGetEvents(size, start_byte))

    def reset_log(self):
        """Reset the event log on the node."""
        self.send_cmd(wlan_exp_cmds.WlanExpCmdResetLog())

    def get_log_start(self):
        """Get the index of the oldest event in the log."""
        return self.send_cmd(wlan_exp_cmds.WlanExpCmdGetLogOldestIdx())

    def get_log_end(self):
        """Get the index of the end of the log.
        
        NOTE:  As long as the log is recording, this will continue to 
        increment until the log is full.  If wrapping is enabled, then 
        eventually, the value of the log end will be less than log start.
        """
        return self.send_cmd(wlan_exp_cmds.WlanExpCmdGetLogCurrIdx())


    def get_statistics(self):
        """Get the statistics from the node.
        
        NOTE:  implement cmd as buffer cmd
        """
        raise NotImplementedError
    
    def write_statistics_to_log(self):
        """Write the current statistics to the log."""
        return self.send_cmd(wlan_exp_cmds.WlanExpCmdAddStatsToLog())
    
    def write_exp_info_to_log(self, reason, message=None):
        """Write the experiment information provided to the log.
        
        Attributes:
            reason -- Reason code for the experiment info.  This is an 
                      arbitrary 16 bit number chosen by the experimentor
            message -- Information to be placed in the event log
        """
        raise NotImplementedError


    def set_time(self, time):
        """Sets the time in microseconds on the node.
        
        Attributes:
            time -- Time to send to the board (either float in sec or int in us)
        """
        self.send_cmd(wlan_exp_cmds.WlanExpCmdNodeTime(time))
    

    def get_time(self):
        """Gets the time in microseconds from the node."""
        return self.send_cmd(wlan_exp_cmds.WlanExpCmdNodeTime(0x0000FFFF0000FFFF))


    def set_channel(self, channel):
        """Sets the channel of the node and returns the channel that was set."""
        return self.send_cmd(wlan_exp_cmds.WlanExpCmdNodeChannel(channel))
    

    def get_channel(self):
        """Gets the current channel of the node."""
        return self.send_cmd(wlan_exp_cmds.WlanExpCmdNodeChannel(0xFFFF))
        

    def stream_log_entries(self, port, ip_address=None, host_id=None):
        """Configure the node to stream log entries to the given port."""
        config       = wn_config.WnConfiguration()

        if (ip_address is None):
            ip_address = config.get_param('network', 'host_address')
            
        if (host_id is None):
            host_id = config.get_param('network', 'host_id')
        
        self.send_cmd(wlan_exp_cmds.WlanExpCmdStreamLogEntries(1, host_id, ip_address, port))
        print("Node {0}:".format(self.node_id),
              "Streaming Log Entries to {0} ({1})".format(ip_address, port))


    def disable_log_entries_stream(self):
        """Configure the node to disable log entries stream."""
        self.send_cmd(wlan_exp_cmds.WlanExpCmdStreamLogEntries(0, 0, 0, 0))
        print("Node {0}:".format(self.node_id), "Disable log entry stream.")


    def config_demo(self, flags=0, sleep_time=0):
        """Configure the demo on the node

        NOTE:  Defaults to demo mode deactivated.        
        """
        self.send_cmd(wlan_exp_cmds.WlanExpCmdConfigDemo(flags, sleep_time))
        print("Node {0}:".format(self.node_id), 
              "flags = {0}".format(flags), 
              "packet wait time = {0}".format(sleep_time))



    #-------------------------------------------------------------------------
    # WARPNet Parameter Framework
    #   Allows for processing of hardware parameters
    #-------------------------------------------------------------------------
    def process_parameter(self, identifier, length, values):
        """Extract values from the parameters"""
        if   (identifier == NODE_WLAN_MAX_ASSN):
            if (length == 1):
                self.max_associations = values[0]
            else:
                raise ex.WnParameterError("NODE_WLAN_MAX_ASSN", "Incorrect length")

        elif (identifier == NODE_WLAN_EVENT_LOG_SIZE):
            if (length == 1):
                self.event_log_size = values[0]
            else:
                raise ex.WnParameterError("NODE_WLAN_EVENT_LOG_SIZE", "Incorrect length")

        elif (identifier == NODE_WLAN_MAX_STATS):
            if (length == 1):
                self.max_statistics = values[0]
            else:
                raise ex.WnParameterError("NODE_WLAN_MAX_STATS", "Incorrect length")

        else:
            super(WlanExpNode, self).process_parameter(identifier, length, values)


    #-------------------------------------------------------------------------
    # Misc methods for the Node
    #-------------------------------------------------------------------------
    def check_ver(self):
        """Check the WLAN Exp version of the node against the current WLAN Exp
        version."""
        (major, minor, revision) = wlan_exp_util.wlan_exp_ver(output=0)
        
        # Node %d with Serial # %d has version "%d.%d.%d" which does not match WLAN Exp v%d.%d.%d
        output_str = str("Node {0} ".format(self.node_id) +
                         "with serial # W3-a-{0:05d} ".format(self.serial_number) +
                         "has version {0:d}.{1:d}.{2:d} ".format(self.wlan_exp_ver_major,
                                                                 self.wlan_exp_ver_minor,
                                                                 self.wlan_exp_ver_revision) +
                         "which does not match WLAN Exp " +
                         "v{0:d}.{1:d}.{2:d}".format(major, minor, revision))
        
        if (major != self.wlan_exp_ver_major) or (minor != self.wlan_exp_ver_minor):
            raise ex.WnVersionError(output_str)
        else:
            if (revision != self.wlan_exp_ver_revision):
                print("WARNING: " + output_str)


# End Class WlanExpNode




class WlanExpNodeFactory(wn_node.WnNodeFactory):
    """Sub-class of WARPNet node factory used to help with node configuration 
    and setup.
        
    Attributes (inherited):
        wn_dict -- Dictionary of WARPNet Node Types to class names
    """
    def __init__(self):
        super(WlanExpNodeFactory, self).__init__()
        
        # Add default classes to the factory
        self.add_node_class(wlan_exp_defaults.WLAN_EXP_AP_TYPE, 
                            wlan_exp_defaults.WLAN_EXP_AP_CLASS)

        self.add_node_class(wlan_exp_defaults.WLAN_EXP_STA_TYPE, 
                            wlan_exp_defaults.WLAN_EXP_STA_CLASS)

    
    def eval_node_class(self, node_class):
        """Evaluate the node_class string to create a node.  
        
        NOTE:  This should be overridden in each sub-class with the same
        overall structure but a different import.  Please call the super
        class so that the calls will propagate to catch all node types.
        """
        import wlan_exp
        
        node = None

        try:
            node = eval(str(node_class + "()"), globals(), locals())
        except:
            pass
        
        if node is None:
            return super(WlanExpNodeFactory, self).eval_node_class(node_class)
        else:
            return node


# End Class WlanExpNodeFactory
