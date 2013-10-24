%==============================================================================
% Function wn_initNodes()
%
% Input:  
%     - Array of struct (derived from wn_nodesConfig())
%
% Output:
%     - Instance of wlan_exp_network which will manage WLAN Exp nodes
%
% Future:
%     - Single number                   (TBD - Need to implement discovery process)
%     - Cell array of wlan_exp_node_*   (TBD - Need to build a case of why we would need this)
% 
%==============================================================================

function network = wlan_exp_initNodes(varargin)


    %==========================================================================
    % Process input arguments
    %

    if nargin == 0
        error(generatemsgid('InsufficientArguments'),'Not enough arguments are provided');

    elseif nargin == 1
        switch( class(varargin{1}) ) 
            case 'struct'
                numNodes        = length(varargin{1});                 
                nodeIDs         = varargin{1};         % Node IDs are specified in the input structure
                
                % Instantiate the correct node based on the nodeClass
                for n = numNodes:-1:1
                    nodes{n} = eval( nodeIDs(n).nodeClass );
                end
                
            otherwise
                error(generatemsgid('UnknownArg'),'Unknown argument.  Argument is of type "%s", need "struct"', class(varargin{1}));
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
        case 'java'
            transport_unicast = wn_transport_eth_udp_java;
            transport_bcast   = wn_transport_eth_udp_java_bcast;
        case 'wn_mex_udp'
            transport_unicast = wn_transport_eth_udp_mex;
            transport_bcast   = wn_transport_eth_udp_mex_bcast;
    end    

    
    %==========================================================================
    % Process nodes
    %
    
    gen_error       = [];
    gen_error_index = 0;

    for n = numNodes:-1:1
        currNode = nodes{n};

        % If we are doing a network setup of the node based on the input structure then create the broadcast packet and send it 
        % Note: 
        %   - Node must be configured with dip switches 0xF
        %     - Node will initialize itself to IP address of 10.0.0.0 and wait for a broadcast message to configure itself
        %     - Node will check against the serial number (only last 5 digits; "W3-a-" is not stored in EEPROM)
        %
        if( strcmp( class( nodeIDs ), 'struct') )
        
            % Open a broadcast transport to send the configure command to the node
            transport_bcast.open();
            tempNode = wlan_exp_node;

            % Send serial number, node ID, and IP address
            myCmd         = wn_cmd( tempNode.calcCmd( tempNode.GRP, tempNode.CMD_IP_SETUP ) );
            myCmd.addArgs( uint32( sscanf( nodeIDs(n).serialNumber, 'W3-a-%d' ) ) );  
            myCmd.addArgs( nodeIDs(n).IDUint32 );  
            myCmd.addArgs( nodeIDs(n).ipAddressUint32 );
            
            unicast_port  = 9000;
            if ( ~strcmp( nodeIDs(n).unicastPort, '' ) )
                unicast_port = str2num( nodeIDs(n).unicastPort );      % Set to the value in the config file
            end
            myCmd.addArgs( unicast_port );

            bcast_port    = 10000;            
            if ( ~strcmp( nodeIDs(n).bcastPort, '' ) )
                bcast_port   = str2num( nodeIDs(n).bcastPort );        % Set to the value in the config file
            end
            myCmd.addArgs( bcast_port );

            transport_bcast.send('message',myCmd.serialize());

            
            % Set up a dummy unicast transport to determine the type of the node
            transport_unicast.open();
            
            transport_unicast.port       = unicast_port;
            transport_unicast.hdr.srcID  = hostID;
            transport_unicast.hdr.destID = nodeIDs(n).IDUint32;
            transport_unicast.address    = nodeIDs(n).ipAddress;
            
            myCmd = wn_cmd( tempNode.calcCmd( tempNode.GRP_WARPNET, tempNode.CMD_WARPNET_TYPE ) );
            resp  = wn_resp( transport_unicast.send( myCmd.serialize() ) );
            resp  = resp.getArgs();

            % TODO:  This should only be deleted for MEX transport
            % transport_unicast.delete();

            if ( resp ~= currNode.type ) 
                error(generatemsgid('TypeMismatch'),'Type mismatch.  Node reports type %d while %s is of type %d ', resp, nodeIDs(n).nodeClass, currNode.type);
            end
            
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
            fprintf('\n');
            fprintf('Error in node %d with ID = %d: ', n, nodeID );
            ME
            fprintf('Error message follows:\n%s\n', ME.message);
            gen_error_index              = gen_error_index + 1;
            gen_error( gen_error_index ) = nodeID;
            % error(generatemsgid('NodeConfigError'),'Node with ID = %d is not responding. Please ensure that the node has been configured with the WLAN Exp bitstream.',nodeID)
            fprintf('\n');
        end
    end  

    % Output an error if there was one
    if( gen_error_index ~= 0 ) 

        error_nodes = sprintf('[');
        for n = 1:gen_error_index      
            error_nodes = sprintf('%s %d', error_nodes, gen_error(n));
        end
        error_nodes = sprintf('%s ]', error_nodes);
        
        error(generatemsgid('NodeConfigError'),'The following nodes with IDs = %s are not responding. Please ensure that the nodes have been configured with the WARPLab bitstream.', error_nodes)
    end

    % Create WLAN Exp Network from the nodes we have configured    
    network = wlan_exp_network( nodes );
    
end