classdef wlan_exp_node < wn_node
    %The WLAN_EXP node class represents one 802.11 node in a WARPNet network.
    % wlan_exp_node is the primary interface for interacting with 802.11 WARPNet nodes.
    % This class provides methods for sending commands and checking the status of an
    % 802.11 WARPNet nodes.
    
    properties (SetAccess = protected)
        wlan_exp_ver_major;            % WLAN EXP version running on this node
        wlan_exp_ver_minor;
        wlan_exp_ver_revision;
        
        max_associations;              % Maximum associations of the node

        event_log;                     % Event Log
        
    end
    properties (SetAccess = public)
        % None - See super class
    end
    properties(Hidden = true,Constant = true)
        % These constants define specific command IDs used by this object.
        % Their C counterparts are found in wlan_exp_node.h
        CMD_IP_SETUP                   =  3;
        CMD_NODE_CONFIG_RESET          =  4;

        CMD_ASSN_GET_STATUS            = 10;
        CMD_ASSN_SET_TABLE             = 11;
        CMD_ASSN_RESET_STATS           = 12;
        
        CMD_DISASSOCIATE               = 20;
        
        CMD_TX_POWER                   = 30;
        CMD_TX_RATE                    = 31;
        CMD_CHANNEL                    = 32;

        CMD_LTG_CONFIG_CBF             = 40;
        CMD_LTG_START                  = 41;
        CMD_LTG_STOP                   = 42;
        CMD_LTG_REMOVE                 = 43;
        
        CMD_LOG_RESET                  = 50;
        CMD_LOG_CONFIG                 = 51;
        CMD_LOG_GET_CURR_IDX           = 52;
        CMD_LOG_GET_OLDEST_IDX         = 53;
        CMD_LOG_GET_EVENTS             = 54;
        CMD_LOG_ADD_EVENT              = 55;
        CMD_LOG_ENABLE_EVENT           = 56;
        %  Be careful that new commands added to wlan_exp_node do not collide with child commands
        
        
        % These constants define the WARPNet type of the node.
        % Their C counterparts are found in wlan_exp_common.h
        WLAN_EXP_BASE         = 4096;            % 0x00001000

        
        % These constants define the specific parameter identifiers used by this object.
        % All nodes must implement these parameters but can be extended to suppor others
        % Their C counterparts are found in *_node.h
        NODE_WLAN_MAX_ASSN        = 6;
        NODE_WLAN_EVENT_LOG_SIZE  = 7;

        
        % These are constants for the Transmit rate for nodes
        WLAN_MAC_RATE_6M      = 1;
        WLAN_MAC_RATE_9M      = 2;
        WLAN_MAC_RATE_12M     = 3;
        WLAN_MAC_RATE_18M     = 4;
        WLAN_MAC_RATE_24M     = 5;
        WLAN_MAC_RATE_36M     = 6;
        WLAN_MAC_RATE_48M     = 7;
        WLAN_MAC_RATE_54M     = 8;
        
    end

    
    methods
        function obj = wlan_exp_node()
            % The constructor sets some constants:
            %   - The current version of the WLAN Exp framework
            %   - The WARPNet type
            % The objects are configured via the separate applyConfiguration method.
            
            [obj.wlan_exp_ver_major, obj.wlan_exp_ver_minor, obj.wlan_exp_ver_revision] = wlan_exp_ver();

            obj.type = obj.WLAN_EXP_BASE;

            % Initialize local variables            
            obj.max_associations       = 0;
            obj.event_log              = wlan_exp_event_log( obj );
        end
        
        function applyConfiguration(objVec, IDVec)

            % Apply Configuration only operates on one ojbect at a time
            if ( ( length( objVec ) > 1 ) || ( length( IDVec ) > 1 ) ) 
                error(generatemsgid('ParameterTooLarge'),'applyConfiguration only operates on a single object with a single ID.  Provided parameters with lengths: %d and %d', length(objVec), length(IDVec) ); 
            end
            
            
            % Apply configuration will finish setting properties for a specific hardware node
            obj    = objVec(1);
            currID = IDVec(1);

            % Get ini configuration file 
            configFile = which('wn_config.ini');
            if(isempty(configFile))
                error(generatemsgid('MissingConfig'),'cannot find wn_config.ini. please run wn_setup.m'); 
            end
                
            % Use Ethernet/UDP transport for all wn_nodes. The specific type of transport is specified 
            % in the user's wn_config.ini file that is created via wn_setup.m

            readKeys = {'network','','transport',''};
            transportType            = inifile(configFile,'read',readKeys);
            transportType            = transportType{1};

            switch(transportType)
                case 'java'
                    obj.transport = wn_transport_eth_udp_java;
                case 'wn_mex_udp'
                    obj.transport = wn_transport_eth_udp_mex;
            end
            
            readKeys = {'network','','host_address',''};
            IP                       = inifile(configFile,'read',readKeys);
            IP                       = IP{1};
            IP                       = sscanf(IP,'%d.%d.%d.%d');

            readKeys = {'network','','host_ID',''};
            hostID                   = inifile(configFile,'read',readKeys);
            hostID                   = hostID{1};
            hostID                   = sscanf(hostID,'%d');
                
            readKeys = {'network','','unicast_starting_port',''};
            unicast_starting_port    = inifile(configFile,'read',readKeys);
            unicast_starting_port    = unicast_starting_port{1};
            unicast_starting_port    = sscanf(unicast_starting_port,'%d');

            readKeys = {'network','','bcast_port',''};
            bcast_starting_port      = inifile(configFile,'read',readKeys);
            bcast_starting_port      = bcast_starting_port{1};
            bcast_starting_port      = sscanf(bcast_starting_port,'%d');

            
            
            % currID is a structure containing node information
            switch( class( currID ) ) 
                case 'struct'
                    if ( ~strcmp( currID.serialNumber, '' ) )
                        obj.serialNumber      = sscanf( currID.serialNumber, 'W3-a-%d' );  % Only store the last 5 digits ( "W3-a-" is not stored )
                    else
                        error(generatemsgid('UnknownArg'),'Unknown argument.  Serial Number provided is blank');
                    end

                    if ( ~strcmp( currID.ID, '' ) )
                        obj.ID                = sscanf( currID.ID, '%d');
                    else
                        error(generatemsgid('UnknownArg'),'Unknown argument.  Node ID provided is blank');
                    end
                
                    if ( ~strcmp( currID.ipAddress, '' ) )
                        obj.transport.address = currID.ipAddress;
                    else
                        error(generatemsgid('UnknownArg'),'Unknown argument.  IP Address provided is blank');
                    end
                    
                    if ( ~strcmp( currID.unicastPort, '' ) )
                        unicast_starting_port = str2num( currID.unicastPort );
                    end

                    if ( ~strcmp( currID.bcastPort, '' ) )
                        bcast_starting_port   = str2num( currID.bcastPort );
                    end

                    if ( ~strcmp( currID.name, '' ) ) % Name is an optional parameter in the structure
                        obj.name              = currID.name;
                    end
                    
                otherwise
                    error(generatemsgid('UnknownArg'),'Unknown argument.  IDVec is of type "%s", need "struct" ', class(currID));
            end        
            

            % Configure transport with settings associated with provided ID
            obj.transport.open( currID.ipAddress, unicast_starting_port );            
            % obj.transport.port       = unicast_starting_port;
            obj.transport.bcast_port = bcast_starting_port;
            obj.transport.hdr.srcID  = hostID;
            obj.transport.hdr.destID = obj.ID;
            
            obj.wn_transportCmd('ping');              % Confirm the WARP node is online
            
            obj.wn_transportCmd('payload_size_test'); % Perform a test to figure out max payload size
            
            obj.wn_nodeCmd('get_hardware_info');      % Read details from the hardware (serial number, 
                                                      %   etc) and save to local properties
                
            % Populate the description property with a human-readable description of the node
            %
            obj.description = sprintf('WLAN EXP WARP v%d Node - ID %d', obj.hwVer, obj.ID);
            
            % Call super class function
            applyConfiguration@wn_node(objVec, IDVec);
        end

        
        function delete(obj)
            delete@wn_node(obj);
        end
    end
    
   
    methods(Hidden=true)
        %These methods are hidden because users are not intended to call
        %them directly from their WARPLab scripts.     
        
        function out = procCmd(obj, nodeInd, node, cmdStr, varargin)
            %wl_node procCmd(obj, nodeInd, node, varargin)
            % obj: node object (when called using dot notation)
            % nodeInd: index of the current node, when wl_node is iterating over nodes
            % node: current node object (the owner of this baseband)
            % varargin:
            out = [];            
            cmdStr = lower(cmdStr);
            switch(cmdStr)

                %------------------------------------------------------------------------------------------------------
                case 'node_config_reset'
                    % Resets the HW state of the node to accept a new node configuration
                    %
                    % Arguments: none
                    % Returns: none
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_NODE_CONFIG_RESET));                    
                    myCmd.addArgs( uint32( node.serialNumber ) );  
                    node.sendCmd(myCmd);

                    
                %------------------------------------------------------------------------------------------------------
                case 'association_get_status'
                    % Gets the association table from the node
                    %
                    % Arguments: none
                    % Returns  : none
                    %
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_ASSN_GET_STATUS));
                    myCmd.addArgs( uint32( 65537 ) );
                    
                    % TODO:  Expand command to return tables at specific intervals
                    
                    resp  = node.sendCmd(myCmd);
                    resp  = resp.getArgs();
                    if ( resp(1) > 0 ) 
                        out   = obj.process_association_status( resp );
                    end

                    
                %------------------------------------------------------------------------------------------------------
                case 'association_set_table'
                    % Sets the association table of the node
                    %
                    % Arguments: association table 
                    % Returns  : none
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_ASSN_SET_TABLE));                    
                    node.sendCmd(myCmd);
                    
                    % TODO:  CURRENTLY NOT SUPPORTED
                    warning(generatemsgid('CommandNotSupported'),'Currently this command is not supported.');

                
                %------------------------------------------------------------------------------------------------------
                case 'association_reset_stats'
                    % Resets all the statistics of a node
                    %
                    % Arguments: none
                    % Returns:   none
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_ASSN_RESET_STATS));
                    node.sendCmd(myCmd);
                    
                    
                %------------------------------------------------------------------------------------------------------
                case 'disassociate'
                    % Diassociate either stations associated with an AP or a AP associated with a station
                    %   - For stations, the disassociate command will not look at the additional arguments
                    %
                    % Arguments: 'all' / <Association ID>
                    % Returns:   none
                    
                    aid = varargin{1}{1};

                    switch( class( aid ) ) 
                        case 'char'
                            if ( strcmp( aid, 'all' ) ) 
                                aid = uint32( 65535 );
                            else
                                error(generatemsgid('ParameterSyntaxError'),'Association ID not a valid number of <all>\n  Syntax:  wn_nodeCmd( nodes, <disassociate>, <association id> or <all> )');                            
                            end
                            
                        case 'double'
                            % Do nothing
                            
                        otherwise
                            error(generatemsgid('ParameterSyntaxError'),'Association ID not a valid number of <all>\n  Syntax:  wn_nodeCmd( nodes, <disassociate>, <association id> or <all> )');
                    end
                    
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_DISASSOCIATE));
                    myCmd.addArgs( aid );                                      % Send association id
                    resp  = node.sendCmd(myCmd);
                    resp  = resp.getArgs();
                    
                    if ( resp ~= aid )
                        if ( resp == 0 )                                       % Value returned by node to indicate association ID DNE
                            warning(generatemsgid('ResponseMismatch'),'Tried to disassociate id %d.  Association ID %d does not exist on the node.', aid, aid);
                        else
                            warning(generatemsgid('ResponseMismatch'),'Tried to disassociate id %d.  Node acutally disassociated id %d.', aid, resp);
                        end
                    end

                    
                %------------------------------------------------------------------------------------------------------
                case 'get_tx_power'
                    % Get the TX power of the node
                    %
                    % Arguments: none
                    % Returns:   none
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_TX_POWER));                    
                    myCmd.addArgs( uint32( 65535 ) );                          % Send special value to get the response  
                    resp  = node.sendCmd(myCmd);
                    out   = resp.getArgs();

                    % TODO:  Need to finish infrastructure inside node C code
                    warning(generatemsgid('CommandNotSupported'),'Currently this command is not supported.');
                    
                    
                %------------------------------------------------------------------------------------------------------
                case 'set_tx_power'
                    % Set the TX power of the node
                    %
                    % Arguments: Tx power of the node
                    % Returns:   none
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_TX_POWER));                    
                    myCmd.addArgs( 0 );  
                    node.sendCmd(myCmd);

                    % TODO:  Need to finish infrastructure inside node C code
                    warning(generatemsgid('CommandNotSupported'),'Currently this command is not supported.');
                    
                    
                %------------------------------------------------------------------------------------------------------
                case 'get_tx_rate'
                    % Get the TX rate of the node
                    %
                    % Arguments: none
                    % Returns:   Tx reate of the node
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_TX_RATE));                    
                    myCmd.addArgs( uint32( 65535 ) );                          % Send special value to get the response  
                    resp  = node.sendCmd(myCmd);
                    resp  = resp.getArgs();
                    
                    % TODO:  Need to finish infrastructure inside node C code
                    warning(generatemsgid('CommandNotSupported'),'Currently this command is not supported.');
                    
                    
                %------------------------------------------------------------------------------------------------------
                case 'set_tx_rate'
                    % Set the TX rate of the node
                    %
                    % Arguments: Tx rate of the node
                    % Returns:   none
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_TX_RATE));                    
                    myCmd.addArgs( uint32( node.WLAN_MAC_RATE_6M ) );  
                    node.sendCmd(myCmd);
                    
                    % TODO:  Need to finish infrastructure inside node C code
                    warning(generatemsgid('CommandNotSupported'),'Currently this command is not supported.');
                    
                    
                %------------------------------------------------------------------------------------------------------
                case 'get_channel'
                    % Get the channel of the AP
                    %
                    % Arguments: none
                    % Returns:   Channel
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_CHANNEL));                    
                    myCmd.addArgs( uint32( 65535 ) );                          % Send special value to get the response  
                    resp  = node.sendCmd(myCmd);
                    out   = resp.getArgs();


                %------------------------------------------------------------------------------------------------------
                case 'set_channel'
                    % Set the channel of the AP
                    %
                    % Arguments: Channel Number (double between 1 - 11)
                    % Returns:   none
                    
                    % First, we need to strip off two layers of cells to get to the contents of varargin
                    channel_cell = varargin{1}{1};

                    if ( numel( channel_cell ) ~= 1 ) 
                        error(generatemsgid('ParameterSyntaxError'),'Did not provide channel number.\n  Syntax:  wn_nodeCmd( nodes, <set_channel>, <channel number 1 - 11> )');
                    end

                    channel = channel_cell{1};
                    
                    if( ~strcmp( class( channel ), 'double') )
                        error(generatemsgid('ParameterSytnaxError'),'Channel number not valid double.\n  Syntax:  wn_nodeCmd( nodes, <set_channel>, <channel number 1 - 11> )');
                    end

                    if( ( channel < 1 ) || ( channel > 11 ) )
                        error(generatemsgid('ParameterSyntaxError'),'Channel number out of bounds.\n  Syntax:  wn_nodeCmd( nodes, <set_channel>, <channel number 1 - 11> )');
                    end 
                    
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_CHANNEL));                    
                    myCmd.addArgs( channel );                                  % Send special value to get the response  
                    resp  = node.sendCmd(myCmd);
                    resp  = resp.getArgs();
                    
                    if ( resp ~= channel )
                        warning(generatemsgid('ResponseMismatch'),'Tried to set channel to %d.  Node acutally set channel to %d.', channel, resp);
                    end
                    
                    
                %------------------------------------------------------------------------------------------------------
                case 'ltg_config_cbr'
                    % Configure the Local Traffic Generator
                    %   - Sets up a traffic flow to a given destination
                    %
                    % Arguments: LTG ID          - Destination LTG ID for the traffic
                    %            pkt_size        - Size of the traffic packet (bytes)
                    %            pkt_interval    - How often should the packet be sent (us)
                    %
                    % Returns:   none

                    ltg_id       = varargin{1}{1};
                    ltg_size     = varargin{1}{2};
                    ltg_interval = varargin{1}{3};

                    if ( ( strcmp( class( ltg_id ), 'double' ) == 0 ) || ( strcmp( class( ltg_size ), 'double' ) == 0 ) || ( strcmp( class( ltg_interval ), 'double' ) == 0 ) ) 
                        error(generatemsgid('ParameterSyntaxError'),'LTG ID not a valid value.\n  Syntax:  wn_nodeCmd( nodes, <ltg_config_cbr>, <ltg id>, <ltg_size>, <ltg_interval> )');
                    end

                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_LTG_CONFIG_CBR));                    
                    myCmd.addArgs( ltg_id );                                   % Send ltg id
                    myCmd.addArgs( ltg_size );                                 % Send ltg size (bytes)
                    myCmd.addArgs( ltg_interval );                             % Send ltg interval (us)
                    
                    resp  = node.sendCmd(myCmd);
                    resp  = resp.getArgs();
                    
                    if ( resp ~= 0 )                                           % Value returned by node 
                        warning(generatemsgid('CommandFailure'),'Failed to start LTG with id %d.', ltg_id);
                    end

                    
                %------------------------------------------------------------------------------------------------------
                case 'ltg_start'
                    % Starts the Local Traffic Generator
                    %   - This will immediately start all traffic flows configured with 'config_ltg'
                    %
                    % Arguments: LTG ID / 'all'
                    % Returns:   none

                    ltg_id = varargin{1}{1};

                    switch( class( ltg_id ) ) 
                        case 'char'
                            if ( strcmp( ltg_id, 'all' ) ) 
                                ltg_id = uint32( 4294967295 );
                            else
                                error(generatemsgid('ParameterSyntaxError'),'LTG ID not a valid string.\n  Syntax:  wn_nodeCmd( nodes, <ltg_start>, <ltg id> or <all> )');                            
                            end
                            
                        case 'double'
                            % Do nothing
                            
                        otherwise
                            error(generatemsgid('ParameterSyntaxError'),'LTG ID not a valid value.\n  Syntax:  wn_nodeCmd( nodes, <ltg_start>, <ltg id> or <all> )');
                    end
                    
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_LTG_START));
                    myCmd.addArgs( ltg_id );                                   % Send ltg id
                    resp  = node.sendCmd(myCmd);
                    resp  = resp.getArgs();
                    
                    if ( resp ~= 0 )                                           % Value returned by node 
                        warning(generatemsgid('CommandFailure'),'Failed to start LTG with id %d.', ltg_id);
                    end
                    
                    
                %------------------------------------------------------------------------------------------------------
                case 'ltg_stop'
                    % Stops the Local Traffic Generator
                    %   - This will immediately stop all local traffic generation.  This is a way to 
                    %     gracefully end infinite traffic flows.
                    %
                    %     As part of the config_ltg command, you can specify how long a given traffic flow will run.
                    %
                    % Arguments: LTG ID / 'all'
                    % Returns:   none
                    
                    ltg_id = varargin{1}{1};

                    switch( class( ltg_id ) ) 
                        case 'char'
                            if ( strcmp( ltg_id, 'all' ) ) 
                                ltg_id = uint32( 4294967295 );
                            else
                                error(generatemsgid('ParameterSyntaxError'),'LTG ID not a valid string.\n  Syntax:  wn_nodeCmd( nodes, <ltg_stop>, <ltg id> or <all> )');                            
                            end
                            
                        case 'double'
                            % Do nothing
                            
                        otherwise
                            error(generatemsgid('ParameterSyntaxError'),'LTG ID not a valid value.\n  Syntax:  wn_nodeCmd( nodes, <ltg_stop>, <ltg id> or <all> )');
                    end
                    
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_LTG_STOP));
                    myCmd.addArgs( ltg_id );                                   % Send ltg id
                    resp  = node.sendCmd(myCmd);
                    resp  = resp.getArgs();
                    
                    if ( resp ~= 0 )                                           % Value returned by node 
                        warning(generatemsgid('CommandFailure'),'Failed to stop LTG with id %d.', ltg_id);
                    end
                    
                    
                %------------------------------------------------------------------------------------------------------
                case 'ltg_remove'
                    % Stops the Local Traffic Generator
                    %   - This will immediately stop all local traffic generation.  This is a way to 
                    %     gracefully end infinite traffic flows.
                    %
                    %     As part of the config_ltg command, you can specify how long a given traffic flow will run.
                    %
                    % Arguments: LTG ID / 'all'
                    % Returns:   none
                    
                    ltg_id = varargin{1}{1};

                    switch( class( ltg_id ) ) 
                        case 'char'
                            if ( strcmp( ltg_id, 'all' ) ) 
                                ltg_id = uint32( 4294967295 );
                            else
                                error(generatemsgid('ParameterSyntaxError'),'LTG ID not a valid string.\n  Syntax:  wn_nodeCmd( nodes, <ltg_remove>, <ltg id> or <all> )');                            
                            end
                            
                        case 'double'
                            % Do nothing
                            
                        otherwise
                            error(generatemsgid('ParameterSyntaxError'),'LTG ID not a valid value.\n  Syntax:  wn_nodeCmd( nodes, <ltg_remove>, <ltg id> or <all> )');
                    end
                    
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_LTG_REMOVE));
                    myCmd.addArgs( ltg_id );                                   % Send ltg id
                    resp  = node.sendCmd(myCmd);
                    resp  = resp.getArgs();
                    
                    if ( resp ~= 0 )                                           % Value returned by node 
                        warning(generatemsgid('CommandFailure'),'Failed to ltg LTG with id %d.', ltg_id);
                    end
                    
                    
                %------------------------------------------------------------------------------------------------------
                case 'log_reset'
                    % Resets the event log
                    %
                    % Arguments: none
                    % Returns  : none

                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_LOG_RESET));
                    node.sendCmd(myCmd);

                    % Reset the log within the node
                    % obj.event_log.reset();

                    
                %------------------------------------------------------------------------------------------------------
                case 'log_config'

                %------------------------------------------------------------------------------------------------------
                case 'log_get_current_event_index'
                    % Gets the current event index in the event log
                    %
                    % Arguments: none
                    % Returns  : event index

                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_LOG_GET_CURR_IDX));
                    resp  = node.sendCmd(myCmd);
                    resp  = resp.getArgs();
                    
                    % Update internal state
                    obj.event_log.set_curr_index( resp );

                    % Set outputs
                    out   = resp;

                    
                %------------------------------------------------------------------------------------------------------
                case 'log_get_oldes_event_index'
                    % Gets the oldest event index in the event log
                    %
                    % Arguments: none
                    % Returns  : event index

                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_LOG_GET_OLDEST_IDX));                    
                    resp  = node.sendCmd(myCmd);
                    resp  = resp.getArgs();
                    
                    % Update internal state
                    obj.event_log.set_oldest_index( resp );

                    % Set outputs
                    out   = resp;

                    
                %------------------------------------------------------------------------------------------------------
                case 'log_get_events'
                    % Get all the traffic events from a node
                    %
                    % Arguments: none
                    % Returns:   array of events
                    %              - NOTE:  This command can take a long time b/c it can take multiple ethernet
                    %                       packets to transfer the data and then build the required classes
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_LOG_GET_EVENTS));

                    % Process arguments

                    
                    % bytes = receive_buffer( obj, cmd, id, flags, start_address, size )
                    resp = node.transport.receive_buffer( myCmd, 0, 0, 0, 1000 );
                    
                    % out = { obj.event_log.process_events( 0, resp( 5:end ) ) };
                    out = { obj.event_log.process_events( 0, resp ) };


                %------------------------------------------------------------------------------------------------------
                case 'log_add_event'
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_ADD_EVENT));
                    node.sendCmd(myCmd);
                    warning(generatemsgid('CommandNotSupported'),'Currently this command is not supported.');
                

                %------------------------------------------------------------------------------------------------------
                case 'log_enable_event'
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_LOG_GET_CURR_IDX));
                    node.sendCmd(myCmd);
                    warning(generatemsgid('CommandNotSupported'),'Currently this command is not supported.');

                    
                %------------------------------------------------------------------------------------------------------
                otherwise
                    out = procCmd@wn_node(obj, nodeInd, node, cmdStr, varargin);
            end
            
            if(iscell(out)==0 && numel(out)~=1)
                out = {out}; 
            end     
        end

        
        function processParameters( obj, parameters )
                    
            % Currently, we only implement the base WARPNet parameters
            processParameters@wn_node( obj, parameters );
            
        end

        
        function processParameterGroup( obj, group, identifier, length, values )
        
            % Currently, we only implement the base WARPNet parameter groups
            processParameterGroup@wn_node( obj, group, identifier, length, values );
                    
        end
        
        
        function processParameter( obj, identifier, length, values )
        
            switch( identifier ) 
                case obj.NODE_WLAN_MAX_ASSN
                    if ( length == 1 ) 
                        obj.max_associations = values(1);
                    else
                        error(generatemsgid('IncorrectParameter'), 'WLAN Exp Node Max Associations Parameter had incorrect length.  Should be 1, received %d', length);
                    end
                
                case obj.NODE_WLAN_EVENT_LOG_SIZE
                    if ( length == 1 ) 
                        obj.event_log.set_size( values(1) );
                    else
                        error(generatemsgid('IncorrectParameter'), 'WLAN Exp Node Event Log Size Parameter had incorrect length.  Should be 1, received %d', length);
                    end

                otherwise
                    % Call parent class to process common parameters
                    processParameter@wn_node( obj, identifier, length, values );
            end
        end
        

        function out = process_association_status( obj, values ) 
            % Function will populate the buffer with the following packet format:
            %   Word [0]             - number of stations
            %   Word [1 - max_words] - information for each station
            %            
            size = numel( values );
            
            if ( size < 1 )
                error(generatemsgid('BadValue'),'There is no information in association table array');                
            end
            
            num_associations = swapbytes( values(1) );
            
            if ( num_associations ~= ( 2^32 - 1 ) )

                index = 2;

                for n = 1:num_associations
                    out(n) = wlan_exp_association_status( obj, values( index : index+8 ) );
                    index  = index + 8;
                end
            else 
                out = [];
            end
        end

        
        function disp(obj)
            %This is a "pretty print" method to summmarize details about
            %wl_node objects and print them in a table on Matlab's command
            %line.
            
            extraTitle = '';
            extraLine = '';
            extraArgs = '';
            
                    fprintf('Displaying properties of %d wlan_exp_node objects:\n',length(obj));
                    fprintf('|  ID |  WNVER | WLANVER |  HWVER |    Serial # |  Ethernet MAC Addr |          Address | %s\n',extraTitle)
                    fprintf('-----------------------------------------------------------------------------------------%s\n',extraLine)
            for n = 1:length(obj)
                currObj = obj(n); 

                if(isempty(currObj.ID))
                    fprintf('|     N/A     Node object has not been initialized                            |%s\n',extraArgs)
                else
                    ID      = sprintf('%d',currObj.ID);
                    WNVER   = sprintf('%d.%d.%d',currObj.wn_ver_major, currObj.wn_ver_minor, currObj.wn_ver_revision);
                    WLANVER = sprintf('%d.%d.%d',currObj.wlan_exp_ver_major, currObj.wlan_exp_ver_minor, currObj.wlan_exp_ver_revision);
                    HWVER   = sprintf('%d',currObj.hwVer);
                    
                    if(currObj.hwVer == 3)
                        SN = sprintf('W3-a-%05d',currObj.serialNumber);
                        %DNA = sprintf('%d',currObj.fpgaDNA);
                    else
                        SN = 'N/A';
                        %DNA = 'N/A';
                    end
                    
                    if ( ~isempty( currObj.transport ) & ~isempty( currObj.transport.address ) )
                        temp = dec2hex(uint64(currObj.transport.eth_MAC_addr),12);
                        ADDR = currObj.transport.address;
                    else
                        temp = 0;
                        ADDR = '';
                    end                     
                        
                    MACADDR = sprintf('%2s-%2s-%2s-%2s-%2s-%2s',...
                        temp(1:2),temp(3:4),temp(5:6),temp(7:8),temp(9:10),temp(11:12));
                    
                    fprintf('|%4s |%7s |%8s |%7s |%12s |%19s |%17s |%s\n',ID,WNVER,WLANVER,HWVER,SN,MACADDR,ADDR,extraArgs);
                    
                end
                    fprintf('-----------------------------------------------------------------------------------------%s\n',extraLine)
            end
        end        
    end %end methods(Hidden)
end %end classdef
