%******************************************************************************
% WARPNet Node
%
% File   :	wn_node.m
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
% 2.00a ejw  9/05/13  Initial release
%
%
%******************************************************************************

classdef wn_node < handle_light
    %
    % The WARPNet node class represents one node in a WARPNet network.  wn_node is the 
    % primary interface for interacting with WARPNet nodes.  This class provides methods 
    % for sending commands and checking the status of WARPNet nodes.
    %
    % The wn_node class provides the base definition of a WARPNet node and it is expected
    % that it will be extened to incorporate additional functionality.  By default, the 
    % WARPNet node provdes a base node and transport components.
    %
    
    properties (SetAccess = protected)
        type;                  % Unique Type of the WARPNet node 
                               %   (this will be used for scripts to determine subclassed nodes)
        ID;                    % Unique identification for this node
        description;           % String description of this node (auto-generated)
        serialNumber;          % Node's serial number, read from EEPROM on hardware
        fpgaDNA;               % Node's FPGA's unique identification (on select hardware)
        hwVer;                 % WARP hardware version of this node
        wn_ver_major;          % WARPNet version running on this node
        wn_ver_minor;
        wn_ver_revision;
        
        transport;             % Node's transport object
    end
    properties (SetAccess = public)
        name;                  % User specified name for node; user scripts supply this
    end
    properties(Hidden = true,Constant = true)
        % Command Groups - Most Significant Byte of Command ID
        GRPID_WARPNET             = hex2dec('FF');
        GRPID_NODE                = hex2dec('00');
        GRPID_TRANS               = hex2dec('10');
    end
    properties(Hidden = true,Constant = true)
        % Define groups to help with calculating WARPNet commands
        GRP_WARPNET               = 'warpnet';
        GRP                       = 'node';
    
        % These constants define specific command IDs used by this object.
        % All nodes must implement these commands but can be extended to support others
        % Their C counterparts are found in *_node.h        
        CMD_WARPNET_TYPE          = hex2dec('FFFFFF');

        CMD_INFO                  = 1;
        CMD_IDENTIFY              = 2;
    end
    properties(Hidden = true,Constant = true)
        % These constants define the specific parameter identifiers used by this object.
        % All nodes must implement these parameters but can be extended to suppor others
        % Their C counterparts are found in *_node.h
        NODE_TYPE                 = 0;
        NODE_ID                   = 1;
        NODE_HW_GEN               = 2;
        NODE_DESIGN_VER           = 3;
        NODE_SERIAL_NUM           = 4;
        NODE_FPGA_DNA             = 5;
    end
    
    
    methods
        function obj = wn_node()
            % Initialize the version of the node
            
            [obj.wn_ver_major, obj.wn_ver_minor, obj.wn_ver_revision] = wn_ver();
            
        end
        
        function applyConfiguration(objVec, IDVec)
            % Do nothing for now

        end
              
        
        function out = wn_nodeCmd(obj, varargin)
            % Sends commands to the node object.
            %   This method is safe to call with multiple wn_node objects as its first argument. 
            %   For example, let node0 and node1 be wn_node objects:
            %
            %   wn_nodeCmd(node0, args )
            %   wn_nodeCmd([node0, node1], args )
            %   node0.wn_nodeCmd( args )
            %
            % are all valid ways to call this method. Note, when calling this method for 
            % multiple node objects, the interpretation of other vectored arguments is left 
            % up to the underlying components.
            %
            nodes = obj;
            numNodes = numel(nodes);
            
            for n = numNodes:-1:1
                currNode = nodes(n);
                out(n) = currNode.procCmd(n, currNode, varargin{:});
            end
            
            if(length(out)==1 && iscell(out))
               out = out{1}; % Strip away the cell if it's just a single element. 
            end
        end
        
        
        function out = wn_transportCmd(obj, varargin)
            % Sends commands to the transport object.
            %   This method is safe to call with multiple wn_node objects as its first argument. 
            %   For example, let node0 and node1 be wn_node objects:
            %
            %   wn_transportCmd(node0, args )
            %   wn_transportCmd([node0, node1], args )
            %   node0.wn_transportCmd( args )
            %
            % are all valid ways to call this method. Note, when calling this method for 
            % multiple node objects, the interpretation of other vectored arguments is left 
            % up to the underlying components.
            %
            nodes = obj;
            numNodes = numel(nodes);
            
            for n = numNodes:-1:1
                currNode = nodes(n);
                if(any(strcmp(superclasses(currNode.transport),'wn_transport')))
                    out(n) = currNode.transport.procCmd(n, currNode, varargin{:});
                else
                    error(generatemsgid('NoTransport'),'Node %d does not have an attached transport module',currNode.ID);
                end
                
            end
            
            if(length(out)==1 && iscell(out))
               out = out{1}; % Strip away the cell if it's just a single element. 
            end           
        end
        
        
        function delete(obj)
            %Clears the transport object to close any open socket
            %connections in the event that the node object is deleted.
            if(~isempty(obj.transport))
                obj.transport.delete();
                obj.transport = [];
            end
        end
    end
    
   
    methods(Static=true,Hidden=true)   
        function command_out = calcCmd_helper(group,command)
            % Performs the actual command calculation for calcCmd
            command_out = uint32(bitor(bitshift(group,24),command));
        end
    end
        
    methods(Hidden=true)
        % These methods are hidden because users are not intended to call
        % them directly from their WARPNet scripts.     
        
        function out = calcCmd(obj, grp, cmdID)
            % Takes a group ID and a cmd ID to form a single uint32 command. Every 
            % WARPNet module calls this method to construct their outgoing commands.
            switch(lower(grp))
                case 'warpnet'
                    out = obj.calcCmd_helper(obj.GRPID_WARPNET,cmdID);
                case 'node'
                    out = obj.calcCmd_helper(obj.GRPID_NODE,cmdID);
                case 'transport'
                    out = obj.calcCmd_helper(obj.GRPID_TRANS,cmdID);
                otherwise
                    error(generatemsgid('UnknownParameter'), 'Unknown Group: %d', grp);
            end
        end
        
        function out = procCmd(obj, nodeInd, node, cmdStr, varargin)
            %wn_node procCmd(obj, nodeInd, node, varargin)
            % obj: node object (when called using dot notation)
            % nodeInd: index of the current node, when wn_node is iterating over nodes
            % node: current node object (the owner of this baseband)
            % varargin:
            out = [];
            cmdStr = lower(cmdStr);
            switch(cmdStr)

                case 'get_hardware_info'
                    % Reads details from the WARP hardware and updates node object parameters
                    %
                    % Arguments: none
                    % Returns: none (access updated node parameters if needed)
                    %                    
                    myCmd = wn_cmd( node.calcCmd(obj.GRP,obj.CMD_INFO) );
                    resp  = node.sendCmd(myCmd);
                    resp  = resp.getArgs();
                    
                    % Process the response arguments
                    obj.processParameters( resp );

                case 'identify'
                    % Blinks the user LEDs on the WARP node, to help identify MATLAB node-to-hardware node mapping
                    %
                    % Arguments: none
                    % Returns: none
                    %
                    myCmd = wn_cmd(node.calcCmd(obj.GRP,obj.CMD_IDENTIFY));
                    node.sendCmd(myCmd);

                otherwise
                    error(generatemsgid('UnknownCommand'), 'unknown node command %s', cmdStr);
            end
            
            if(iscell(out)==0 && numel(out)~=1)
                out = {out}; 
            end     
        end
        
        
        function processParameters( obj, parameters )
                    
            % Each Parameter is of the form:
            %            | 31 ... 24 | 23 ... 16 | 15 ... 8 | 7 ... 0 |
            %     Word 0 | Reserved  | Group     | Length             |
            %     Word 1 | Parameter Identifier                       |
            %     Word 2 | Value 0 of Parameter                       |
            %     ...
            %     Word N | Value M of Parameter                       |
            %
            %  where the number of parameters, M, is equal to the Length field
            %
            
            param_start = 1;
            param_end   = length( parameters );
            
            while( param_start <= param_end ) 

                param_group       = double( bitand( bitshift( parameters(param_start), -16), 255));
                param_length      = double( bitand( parameters(param_start), 65535 ) );
                param_identifier  = parameters( param_start + 1 );
                
                value_start       = param_start + 2;                   % 
                value_end         = param_start + param_length + 1;    % End index is (param_length - 1)
                
                param_values      = parameters( value_start:value_end );

                obj.processParameterGroup( param_group, param_identifier, param_length, param_values );
                
                % Find the start of the next parameter
                param_start = param_start + param_length + 2;
            end                        
        end
        
        
        function processParameterGroup( obj, group, identifier, length, values )
            switch( group )
                case obj.GRPID_NODE
                    obj.processParameter( identifier, length, values );
                    
                case obj.GRPID_TRANS
                    obj.transport.processParameter( identifier, length, values );                    
                    
                otherwise
                    error(generatemsgid('UnknownParameter'), 'Unknown Parameter Group: %d', group);
            end
        end
        
        
        function processParameter( obj, identifier, length, values )        
            switch( identifier ) 
                case obj.NODE_TYPE
                    if ( length == 1 ) 
                        obj.type  = values(1);
                    else
                        error(generatemsgid('IncorrectParameter'), 'Node Type Parameter had incorrect length.  Should be 1, received %d', length);
                    end
                
                case obj.NODE_ID
                    if ( length == 1 ) 
                        obj.ID    = values(1);
                    else
                        error(generatemsgid('IncorrectParameter'), 'Node ID Parameter had incorrect length.  Should be 1, received %d', length);
                    end
                    
                case obj.NODE_HW_GEN
                    if ( length == 1 ) 
                        obj.hwVer = bitand( values(1), 255);
                    else
                        error(generatemsgid('IncorrectParameter'), 'Node HW Gen Parameter had incorrect length.  Should be 1, received %d', length);
                    end
                    
                case obj.NODE_DESIGN_VER
                    if ( length == 1 )                     
                        % Get the WARPNet version reported by the node
                        MAJOR    = double(bitand(bitshift(values(1), -16), 255));
                        MINOR    = double(bitand(bitshift(values(1), -8), 255));
                        REVISION = double(bitand(values(1), 255));

                        % Check the current WARPNet version against the node
                        if(obj.wn_ver_major ~= MAJOR || obj.wn_ver_minor ~= MINOR)
                            myErrorMsg = sprintf('Node %d reports WARPNet version %d.%d.%d while this PC is configured with %d.%d.%d', ...
                                obj.ID, MAJOR, MINOR, REVISION, obj.wn_ver_major, obj.wn_ver_minor, obj.wn_ver_revision );
                            error( generatemsgid('VersionMismatch'), myErrorMsg );
                        end

                        if(obj.wn_ver_revision ~= REVISION)
                            myWarningMsg = sprintf('Node %d reports WARPLab version %d.%d.%d while this PC is configured with %d.%d.%d', ...
                                obj.ID, MAJOR, MINOR, REVISION, obj.wn_ver_major, obj.wn_ver_minor, obj.wn_ver_revision );
                            warning( generatemsgid('VersionMismatch'), myWarningMsg );
                        end

                        % Set WARPNet version to that reported by the node
                        obj.wn_ver_major    = MAJOR;
                        obj.wn_ver_minor    = MINOR;
                        obj.wn_ver_revision = REVISION;
                    else
                        error(generatemsgid('IncorrectParameter'), 'Node Design Version Parameter had incorrect length.  Should be 1, received %d', length);
                    end
                    
                case obj.NODE_SERIAL_NUM
                    if ( length == 1 ) 
                        obj.serialNumber = values(1);
                    else
                        error(generatemsgid('IncorrectParameter'), 'Node Serial Number Parameter had incorrect length.  Should be 1, received %d', length);
                    end
                    
                case obj.NODE_FPGA_DNA
                    if ( length == 2 )
                        temp0 = uint64( values(1) );
                        temp1 = uint64( values(2) );
                        obj.fpgaDNA      = 2^32 * temp1 + temp0;
                    else
                        error(generatemsgid('IncorrectParameter'), 'Node FPGA DNA Parameter had incorrect length.  Should be 2, received %d', length);
                    end

                otherwise
                    error(generatemsgid('UnknownParameter'), 'Unknown Node Group Parameter Identifier: %d', identifier);
            end
        end

        
        function out = sendCmd(obj, cmd)
            %This method is responsible for serializing the command
            %objects provided by each of the components of WARPNet into a
            %vector of values that the transport object can send. This
            %method is used when the board must at least provide a
            %transport-level acknowledgement. If the response has actual
            %payload that needs to be provided back to the calling method,
            %that response is passed as an output of this method.
            resp = obj.transport.send(cmd.serialize());
            
            out = wn_resp(resp);
        end
        
        function sendCmd_noresp(obj, cmd)
            %This method is responsible for serializing the command
            %objects provided by each of the components of WARPNet into a
            %vector of values that the transport object can send. This 
            %method is used when a board should not send an immediate
            %response. The transport object is non-blocking -- it will send
            % the command and then immediately return.
            obj.transport.send(cmd.serialize(), 'noresp');
        end 
        
        function out = receiveResp(obj)
            %This method will return a vector of responses that are
            %sitting in the host's receive queue. It will empty the queue
            %and return them all to the calling method.
            resp = obj.transport.receive();
            if(~isempty(resp))
                %Create vector of response objects if received string of bytes
                %is a concatenation of many responses
                done = false;
                index = 1;
                while ~done
                    out(index) = wn_resp(resp);
                    currRespLen = length(out(index).serialize);
                    if(currRespLen<length(resp))
                        resp = resp((currRespLen+1):end);
                    else
                        done = true;
                    end
                    index = index+1;
                end
            else
                out = [];
            end
        end
        
        function disp(obj)
            % Do nothing for now
            
        end        
    end %end methods(Hidden)
end %end classdef
