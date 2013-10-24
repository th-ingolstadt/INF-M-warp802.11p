%******************************************************************************
% WARPNet Ethernet UDP Transport
%
% File   :	wn_transport_eth_udp.m
% Authors:	Chris Hunter (chunter [at] mangocomm.com)
%			Patrick Murphy (murphpo [at] mangocomm.com)
%           Erik Welsh (welsh [at] mangocomm.com)
% License:	Copyright 2013, Mango Communications. All rights reserved.
%			Distributed under the WARP license  (http://warpproject.org/license)
%
%******************************************************************************
% MODIFICATION HISTORY:
%
% Ver   Who  Date     Changes
% ----- ---- -------- -------------------------------------------------------
%
%******************************************************************************

classdef wn_transport_eth_udp < wn_transport & handle_light
% Ethernet UDP Transport parent class for unicast traffic
% User code should not use this object directly-- the parent wn_node will
%  instantiate the appropriate transport object for the hardware in use

%******************************** Properties **********************************

    properties (SetAccess = public)
        timeout;        % Maximum time spent waiting before retransmission
    end

    properties (SetAccess = protected, Hidden = true)
        type;           % WARPNet transport type
        sock;           % UDP socket
        status;         % Status of UDP socket
        maxPayload;     % Maximum payload size (e.g. MTU - ETH/IP/UDP headers)
    end

    properties (SetAccess = public)
        eth_MAC_addr;   % Node's MAC address
        address;        % IP address of destination
        port;           % Unicast port of destination
        bcast_port;     % Broadcast port of destination
        hdr;            % Transport header object
        group_id;       % Group ID of the node attached to the transport
        rxBufferSize;   % OS's receive buffer size in bytes
    end

    properties(Hidden = true,Constant = true)
        %These constants define specific command IDs used by this object.
        %Their C counterparts are found in wn_transport.h
        GRP = 'transport';
        CMD_PING            = 1;
        CMD_IP              = 2;
        CMD_PORT            = 3;
        CMD_PAYLOADSIZETEST = 4;
        CMD_NODEGRPID_ADD   = 5;
        CMD_NODEGRPID_CLEAR = 6;        
    end
    
    properties(Hidden = true,Constant = true)
        % These constants define the specific parameter identifiers used by this object.
        % All nodes must implement these parameters but can be extended to suppor others
        % Their C counterparts are found in *_transport.h
        TRANSPORT_TYPE            = 0;
        TRANSPORT_HW_ADDR         = 1;
        TRANSPORT_IP_ADDR         = 2;
        TRANSPORT_UNICAST_PORT    = 3;
        TRANSPORT_BCAST_PORT      = 4;
        TRANSPORT_GRP_ID          = 5;
    end
    
    
%********************************* Methods ************************************

    methods
        function obj = wn_transport_eth_udp()
            obj.hdr         = wn_transport_header;
            obj.hdr.pktType = obj.hdr.PKTTYPE_HTON_MSG;
            obj.status      = 0;
            obj.timeout     = 1;
            obj.maxPayload  = 1000; % Sane default. This gets overwritten by CMD_PAYLOADSIZETEST command.
            
            obj.checkSetup();
        end
        
        function checkSetup(obj)
            %TODO
        end

        function setMaxPayload(obj,value)
            obj.maxPayload = value;
        end
        
        function out = getMaxPayload(obj)
            out = obj.maxPayload;
        end

        function open(obj,varargin)
            % Must be implemented by child classes
            obj.status = 0;
        end
        
        function out = procCmd(obj,nodeInd,node,cmdStr,varargin)
            out = [];
            cmdStr = lower(cmdStr);
            switch(cmdStr)
                case 'ping'
                    % Test to make sure node can be accessed via this
                    % transport
                    %
                    % Arguments: none
                    % Returns: true if board responds; raises error otherwise
                    myCmd = wn_cmd(node.calcCmd(obj.GRP,obj.CMD_PING));
                    node.sendCmd(myCmd);
                    out = true; %sendCmd will timeout and raise error if board doesn't respond
                    
                case 'payload_size_test'
                    % Determine's objects maxPayload parameter
                    %
                    % Arguments: none
                    % Returns: none
                    %
                    configFile = which('wn_config.ini');
                    if(isempty(configFile))
                       error(generatemsgid('MissingConfig'),'cannot find wn_config.ini. please run wn_setup.m'); 
                    end
                    readKeys = {'network','','jumbo',''};
                    jumbo = inifile(configFile,'read',readKeys);
                    jumbo = jumbo{1};
                    
                    if(strcmp('true',jumbo))
                        payloadTestSizes = [1000 1470 5000 8966];    
                    else
                        payloadTestSizes = [1000 1470];    
                    end
                    
                    payloadTestSizes = floor((payloadTestSizes - ( sizeof(node.transport.hdr) + sizeof(wn_cmd) ) - 4)/4);
                         
                    for index = 1:length(payloadTestSizes)
                        myCmd = wn_cmd(node.calcCmd(obj.GRP,obj.CMD_PAYLOADSIZETEST));
                        myCmd.addArgs(1:payloadTestSizes(index));
                        try
                            resp = node.sendCmd(myCmd);
                            obj.setMaxPayload( resp.getArgs );
                            if(obj.getMaxPayload < payloadTestSizes(index)*4)
                               break; 
                            end
                        catch ME
                            break 
                        end
                    end
                    
                case 'get_warpnet_type'
                    % Gets the WARPNet type from a node
                    %
                    % Arguments: none
                    % Returns:   type
                    %
                    myCmd = wn_cmd( node.calcCmd( node.GRP_WARPNET, node.CMD_WARPNET_TYPE ) );
                    resp  = node.sendCmd(myCmd);
                    out   = resp.getArgs();
                
                case 'add_node_group_id'
                    % Adds a Node Group ID to the node so that it can
                    % process broadcast commands that are received from
                    % that node group.
                    %
                    % Arguments: (uint32 NODE_GRP_ID)
                    % Returns: none
                    %
                    % NODE_GRP_ID: ID provided by wn_node_grp
                    %                   
                    NODE_GRP_ID = varargin{1};
                    myCmd = wn_cmd(node.calcCmd(obj.GRP,obj.CMD_NODEGRPID_ADD));
                    myCmd.addArgs(NODE_GRP_ID);
                    node.sendCmd(myCmd);
                    
                case 'clear_node_group_id'
                    % Clears a Node Group ID from the node so it can ignore
                    % broadcast commands that are received from that node
                    % group.
                    %
                    % Arguments: (uint32 NODE_GRP_ID)
                    % Returns: none
                    %
                    % NODE_GRP_ID: ID provided by wn_node_grp
                    %
                    NODE_GRP_ID = varargin{1};
                    myCmd = wn_cmd(node.calcCmd(obj.GRP,obj.CMD_NODEGRPID_CLEAR));
                    myCmd.addArgs(NODE_GRP_ID);
                    node.sendCmd(myCmd);
                    
                otherwise
                    error(generatemsgid('UnknownCmd'),'unknown command ''%s''',cmdStr);
            end
            
            if(iscell(out)==0 && numel(out)~=1)
                out = {out}; 
            end     
        end
        
        function close(obj)
            obj.status = 0;
        end    

        function delete(obj)
            obj.close();
        end

        function flush(obj)
            %TODO
        end
    end %methods

    methods (Hidden = true)
        function reply = send(obj,varargin)
            % Must be implemented by the child class
            reply = [];
        end
        
        function resp = receive(obj)
            % Must be implemented by the child class
            resp = [];
        end

        
        % Process parameters
        function processParameter( obj, identifier, length, values )        
            switch( identifier ) 
                case obj.TRANSPORT_TYPE
                    if ( length == 1 ) 
                        obj.type = values(1);
                    else
                        error(generatemsgid('IncorrectParameter'), 'Transport Type Parameter had incorrect length.  Should be 1, received %d', length);
                    end
                
                case obj.TRANSPORT_HW_ADDR
                    if ( length == 2 ) 
                        obj.eth_MAC_addr = 2^32*double(bitand(values(1),2^16-1)) + double(values(2));
                    else
                        error(generatemsgid('IncorrectParameter'), 'Transport HW Address Parameter had incorrect length.  Should be 2, received %d', length);
                    end
                    
                case obj.TRANSPORT_IP_ADDR
                    if ( length == 1 )
                        obj.address = obj.int2IP( values(1) );
                    else
                        error(generatemsgid('IncorrectParameter'), 'Transport IP Address Parameter had incorrect length.  Should be 1, received %d', length);
                    end
                    
                case obj.TRANSPORT_UNICAST_PORT                    
                    if ( length == 1 )
                        obj.port = values(1);
                    else
                        error(generatemsgid('IncorrectParameter'), 'Transport Unicast Port Parameter had incorrect length.  Should be 1, received %d', length);
                    end
                    
                case obj.TRANSPORT_BCAST_PORT                    
                    if ( length == 1 ) 
                        obj.bcast_port = values(1);
                    else
                        error(generatemsgid('IncorrectParameter'), 'Transport Broadcast Port Parameter had incorrect length.  Should be 1, received %d', length);
                    end
                    
                case obj.TRANSPORT_GRP_ID
                    if ( length == 1 )
                        group_id = values(1);
                    else
                        error(generatemsgid('IncorrectParameter'), 'Transport Group ID Parameter had incorrect length.  Should be 1, received %d', length);
                    end

                otherwise
                    error(generatemsgid('UnknownParameter'), 'Unknown Transport Group Parameter Identifier: %d', identifier);
            end
            
        end
        
                
        function dottedIPout = int2IP(obj,intIn)
            addrChars(4) = mod(intIn, 2^8);
            addrChars(3) = mod(bitshift(intIn, -8), 2^8);
            addrChars(2) = mod(bitshift(intIn, -16), 2^8);
            addrChars(1) = mod(bitshift(intIn, -24), 2^8);
            dottedIPout = sprintf('%d.%d.%d.%d', addrChars);
        end
        

        function intOut = IP2int(obj,dottedIP)
            addrChars = sscanf(dottedIP, '%d.%d.%d.%d')';
            intOut = 2^0 * addrChars(4) + 2^8 * addrChars(3) + 2^16 * addrChars(2) + 2^24 * addrChars(1);
        end
    end
end %classdef
