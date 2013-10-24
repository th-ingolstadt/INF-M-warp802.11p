%==============================================================================
% Function wn_initNodes()
%
% Input:  
%     - Single number
%     - Array of struct (derived from wn_nodesConfig())
%     - Array of wn_nodes; Either optional array of numbers, or array of struct 
%     - Cell Array of value pairs:
%         {'node_type', 'class name of node'}, where node_type is a unique 32 bit number
%
% Output:
%     - Array of wn_nodes
% 
%==============================================================================

function nodes = wn_initNodes(varargin)


    %==========================================================================
    % Process input arguments
    %

    if nargin == 0
        error(generatemsgid('InsufficientArguments'),'Not enough arguments are provided');

    elseif nargin == 1
        switch( class(varargin{1}) ) 
            case 'wn_node'
                nodes           = varargin{1};
                numNodes        = length(nodes);
                nodeIDs         = 0:(numNodes-1);      % Node IDs default:  0 to (number of nodes in the array)
            case 'struct'
                numNodes        = length(varargin{1}); 
                nodes(numNodes) = wn_node;
                nodeIDs         = varargin{1};         % Node IDs are specified in the input structure
            case 'double'
                numNodes        = varargin{1}; 
                nodes(numNodes) = wn_node;
                nodeIDs         = 0:(numNodes-1);     % Node IDs default:  0 to (number of nodes specified)
            otherwise
                error(generatemsgid('UnknownArg'),'Unknown argument.  Argument is of type "%s", need "wn_node", "struct", or "double"', class(varargin{1}));
        end
        
    elseif nargin == 2
        switch( class(varargin{1}) ) 
            case 'wn_node'
                nodes           = varargin{1};
                numNodes        = length(nodes);
            case 'double'
                numNodes        = varargin{1}; 
                nodes(numNodes) = wn_node;
            otherwise
                error(generatemsgid('UnknownArg'),'Unknown argument.  Argument is of type "%s", need "wn_node" or "double"', class(varargin{1}));
        end        

        switch( class(varargin{2}) ) 
            case 'struct'
                nodeIDs         = varargin{2};         % Node IDs are specified in the input structure
            case 'double'
                nodeIDs         = varargin{2};        % Node IDs default:  0 to (number of nodes specified)
            otherwise
                error(generatemsgid('UnknownArg'),'Unknown argument.  Argument is of type "%s", need "struct", or "double"', class(varargin{2}));
        end        
        
        if(length(nodeIDs)~=numNodes)
            error(generatemsgid('DimensionMismatch'),'Number of nodes does not match ID vector');
        end     
    else
        error(generatemsgid('TooManyArguments'),'Too many arguments provided');
    end

    
    %==========================================================================
    % Process config file
    %
    
    configFile = which('wn_config.ini');
    if(isempty(configFile))
       error(generatemsgid('wn_node:MissingConfig'),'cannot find wn_config.ini. please run wn_setup.m'); 
    end
    
    readKeys  = {'network','','host_ID',''};
    hostID    = inifile(configFile,'read',readKeys);
    hostID    = hostID{1};
    hostID    = sscanf(hostID,'%d');
    
    readKeys  = {'network','','transport',''};
    transport = inifile(configFile,'read',readKeys);
    transport = transport{1};
    
    switch(transport)
        case 'pnet'
            transport_bcast   = wn_transport_eth_udp_pnet_bcast.instance();
        case 'java'
            transport_bcast   = wn_transport_eth_udp_java_bcast;
        case 'wn_mex_udp'
            transport_bcast   = wn_transport_eth_udp_mex_bcast;
    end    

    transport_bcast_open = 0;

    
    %==========================================================================
    % Process nodes
    %
    
    for n = numNodes:-1:1
        currNode = nodes(n);

        % If we are doing a network setup of the node based on the input structure then create the broadcast packet and send it 
        % Note: 
        %   - Node must be configured with dip switches 0xF
        %     - Node will initialize itself to IP address of 10.0.0.0 and wait for a broadcast message to configure itself
        %     - Node will check against the serial number (only last 5 digits; "W3-a-" is not stored in EEPROM)
        %
        if( strcmp( class( nodeIDs ), 'struct') )
        
            % Open a broadcast transport to send the configure command to the node
            transport_bcast.open();
            tempNode      = wn_node;

            % Send serial number, node ID, and IP address
            myCmd         = wn_cmd( tempNode.calcCmd( tempNode.GRP, tempNode.CMD_IP_SETUP ) );
            myCmd.addArgs( uint32( sscanf( nodeIDs(n).serialNumber, 'W3-a-%d' ) ) );  
            myCmd.addArgs( nodeIDs(n).IDUint32 );  
            myCmd.addArgs( nodeIDs(n).ipAddressUint32 );  
            transport_bcast.send('message',myCmd.serialize());
            
            transport_bcast_open = 1;
        end

        % Error check to make sure that no node in the network has the same ID as this host.
        %
        nodeID = 0;
        switch(class( nodeIDs(n) ))
            case 'double'
                nodeID = nodeIDs(n);
                if( hostID == nodeID )
                    error(generatemsgid('InvalidNodeID'),'Host ID is set to %d and must be unique. No node in the network can share this ID',hostID); 
                end
                
            case 'struct'
                nodeID = nodeIDs(n).IDUint32;
                if( hostID == nodeID )
                    error(generatemsgid('InvalidNodeID'),'Host ID is set to %d and must be unique. No node in the network can share this ID',hostID); 
                end
        end
        
        % Now that the node has a valid IP address, we can apply the configuration
        %
        try
            currNode.applyConfiguration(nodeIDs(n));
        catch ME
            error( generatemsgid('NodeConfigError'),'Node with ID = %d is not responding. Please ensure that the node has been configured with the WARPLab bitstream.',nodeID )
        end    
    end
    
    
    % Send a test broadcast trigger command
    %
%    if( 0 )
    if(strcmp(class(nodes(1).trigger_manager),'wn_trigger_manager_proc'))
        
        if ( transport_bcast_open == 0 )
            transport_bcast.open();
        end
        
        tempNode      = wn_node;
        bcastTestFlag = uint32( round( 2^32 * rand ) );
        
        myCmd         = wn_cmd( tempNode.calcCmd( wn_trigger_manager_proc.GRP, wn_trigger_manager_proc.CMD_TEST_TRIGGER ) );
        myCmd.addArgs(bcastTestFlag); %Signals to the board that this is writing the received trigger test variable
        transport_bcast.send('message',myCmd.serialize());

        resp = wn_triggerManagerCmd( nodes, 'test_trigger' );

        trigger_failure = find( resp ~= bcastTestFlag );
        for nodeIndex = trigger_failure
           warning(generatemsgid('TriggerFailure'),'Node %d with Serial # %d failed trigger test',nodes(nodeIndex).ID,nodes(nodeIndex).serialNumber) 
        end

        if( any( resp ~= bcastTestFlag ) )
           error(generatemsgid('TriggerFailure'),'Broadcast triggers are not working. Please verify your ARP table has an entry for the broadcast address on your WARPLab subnet') 
        end

    end

    % Finalize the node initialization
    %
    nodes.wn_nodeCmd('initialize');
    
end