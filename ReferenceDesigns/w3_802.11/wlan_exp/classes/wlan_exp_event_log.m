%******************************************************************************
% WLAN Exp Event Log
%
% File   :	wlan_exp_event_log.m
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

classdef wlan_exp_event_log < handle_light
    %
    % The WLAN Exp Event Log will keep a cell array of all events that can be sorted / 
    % processed.
    %
    % The event log also implemented the functions to both read the event log from the 
    % node and process the event log.
    %
    
    properties (SetAccess = protected)
        node;                   % Node that owns this event log
        
        size;                   % Size in bytes of the event log
        event_list;             % Cell array of events
        
        curr_index;             % Current event log index on the node
        last_index;             % Last index read from the event log
        oldest_index;           % Oldest event log index on the node
    end

    properties(Hidden = true,Constant = true)
        % Event Types
        %  These correspond to the event types in wlan_mac_events.h
        EVENT_TYPE_DEFAULT             =  0;
        EVENT_TYPE_RX_OFDM             =  1;
        EVENT_TYPE_RX_DSSS             =  2;
        EVENT_TYPE_TX                  =  3;
        EVENT_TYPE_ERR_BAD_FCS_RX      =  4;


        % Magic number to deliniate events in the log
        EVENT_MAGIC_NUM           = 44269;              % 0xACED -> Should be the upper 16 bits of the timestamp
        
    end
    
    
    methods
        function obj = wlan_exp_event_log( varargin )
        
            % Initialize the array of nodes            
            if nargin == 1
                if ( ~any( strcmp( [superclasses(varargin{1}); class(varargin{1})] , 'wlan_exp_node' ) ) )
                    error(generatemsgid('UnknownArg'),'Unknown argument.  Argument is of type "%s", need "wlan_exp_node"', class(varargin{1}) );
                else
                    obj.node = varargin{1};
                end
            else
                error(generatemsgid('TooManyArguments'),'Too many arguments provided');
            end
            
            % Initialize local variables
            obj.event_list   = {};
            obj.size         = 0;
            obj.curr_index   = 0;
            obj.last_index   = 0;
            obj.oldest_index = 0;
        end

        
        function out = wlan_exp_event_logCmd(obj, varargin)
            % Processes
            %   This method is safe to call with multiple wlan_exp_event_log objects as its first argument. 
            %   For example, let log0 and log1 be wlan_exp_event_log objects:
            %
            %   wlan_exp_event_logCmd(log0, args )
            %   wlan_exp_event_logCmd([log0, log1], args )
            %   log0.wlan_exp_event_logCmd( args )
            %
            % are all valid ways to call this method. Note, when calling this method for 
            % multiple log objects, the interpretation of other vectored arguments is left 
            % up to the underlying components.
            %
            logs = obj;
            numLogs = numel(logs);
            
            for n = numLogs:-1:1
                currLog = logs(n);
                out(n) = currLog.procCmd(n, currLog, varargin{:});
            end
            
            if(length(out)==1 && iscell(out))
               out = out{1}; % Strip away the cell if it's just a single element. 
            end
        end


        function set_curr_index( obj, index )
            obj.curr_index = index;                        % Set the current index
        end


        function out = get_curr_index( obj )
            out = obj.curr_index;                          % Set the current index
        end
        
        
        function set_oldest_index( obj, index )
            obj.oldest_index = index;                      % Set the oldest index
        end


        function out = get_oldest_index( obj )
            out = obj.oldest_index;                        % Set the oldest index
        end

        
        function set_size( obj, size )
            obj.size = size;                               % Set the size of the event log
        end
        

        function out = get_last_index( obj )
            out = obj.last_index;                          % Set the last index
        end

        
        function out = process_events( obj, index, values )
            % Function will iterate through the array of doubles and attempt to create events 
            %   from the values.  By default, all events will have the following fields:
            %       Word [0]         - timestamp[31:0]
            %       Word [1]         - timestamp[63:32]
            %       Word [2]         - event_type[31:16]; event_length[15:0];
            %       Word [3]         - event_id
            % 
            % Arguments:
            %     index              - Byte index in the event log of the start of the events
            %
            event_header_size = 16;            
            size              = numel( values );
            
            if ( size < 1 )
                error(generatemsgid('BadValue'),'There is no information in the event log');
            end
            
            out         = {};
            
            switch ( class( values ) )
                case 'uint32'
                    words       = uint32( values );
                    bytes       = typecast( words, 'uint8' );
                
                case 'uint8'
                    bytes       = values;
                
                otherwise
                    error(generatemsgid('InvalidArgumentType'),'Argument was of class "%s".  Need either "uint32" or "uint8".', class( values ) );
            end

            num_bytes   = numel( bytes );

            % Initialize loop variables
            issue_warning = 1;
            event_index   = 1;
            bytes_index   = 1;

            while ( bytes_index < num_bytes )
                % Check that there enough bytes left to try the next event
                if ( ( bytes_index + 15 ) < num_bytes ) 
                
                    % Decode the event header
                    %
                    [ magic, timestamp, type, length, id ] = obj.extract_header( bytes( bytes_index : bytes_index + 15 ) );

                    % Check that this is a valid event
                    if ( magic ~= obj.EVENT_MAGIC_NUM )

                        % If we have not issued a warning then issue one until the next good event
                        if ( issue_warning == 1 )
                            % Give a warning since we don't know if this is an isolated event
                            warning(generatemsgid('EventInvalidHeader'),'Event header did not have the correct delimiter.');
                            
                            issue_warning = 0;
                        end
                        
                        bytes_index  = bytes_index + 1;
                    else
                        % If we have issued a warning and now have a valid event, then we can issue another warning
                        if ( issue_warning == 0 ) 
                            issue_warning = 1;
                        end
                    
                        % fprintf( 'Event %d:  Type = %d   Length = %d   Valid = %x \n', id, type, length, magic);
                    
                        % Calculate the beginning and end of the event
                        bytes_index_start = bytes_index + event_header_size;
                        bytes_index_end   = bytes_index_start + length - 1;
                        
                        % Check that there are enough bytes to create the event
                        if ( bytes_index_end < num_bytes ) 
                        
                            % Create the appropriate event
                            %
                            switch ( type ) 
                                case obj.EVENT_TYPE_DEFAULT
                                    out{ event_index } = { wlan_exp_event( obj.node, id, timestamp ) };
                                
                                case obj.EVENT_TYPE_RX_OFDM
                                    out{ event_index } = { wlan_exp_event_rx_ofdm( obj.node, id, timestamp, bytes( bytes_index_start : bytes_index_end ) ) };
                                
                                case obj.EVENT_TYPE_RX_DSSS
                                    out{ event_index } = { wlan_exp_event_rx_dsss( obj.node, id, timestamp, bytes( bytes_index_start : bytes_index_end ) ) };
                                
                                case obj.EVENT_TYPE_TX
                                    out{ event_index } = { wlan_exp_event_tx( obj.node, id, timestamp, bytes( bytes_index_start : bytes_index_end ) ) };
                                
                                case obj.EVENT_TYPE_ERR_BAD_FCS_RX
                                    out{ event_index } = { wlan_exp_event_err_bad_fcs_rx( obj.node, id, timestamp, bytes( bytes_index_start : bytes_index_end ) ) };
                                
                                otherwise
                                    warning(generatemsgid('EventInvalidType'),'Event type %d is unknown.', type);
                            end
                        end
                        
                        event_index  = event_index + 1;
                        bytes_index  = bytes_index + event_header_size + length;
                    end

                else
                    % Exit the loop
                    break;
                end
            end
            
            % Set the last_index property to keep track of where we are
            obj.last_index = index + (bytes_index - 1);

        end


        function reset(obj)
            % Reset local variables
            obj.event_list   = {};
            obj.curr_index   = 0;
            obj.last_index   = 0;
            obj.oldest_index = 0;
        end

        
        function delete(obj)
            obj.event_list   = {};
        end
    end
    
   
        
    methods(Hidden=true)
        % These methods are hidden because users are not intended to call
        % them directly from their WLAN Exp scripts.     
        
        function out = procCmd(obj, logInd, log, cmdStr, varargin)
            %wlan_exp_network procCmd(obj, logInd, log, varargin)
            % obj        : log object (when called using dot notation)
            % networkInd : index of the current log, when wlan_exp_event_log is iterating over logs
            % network    : current log object
            % varargin:
            out = [];
            cmdStr = lower(cmdStr);
            switch(cmdStr)

                case 'add_event'
                    % Add an event to the network
                    %
                    % Arguments: Event that is a child of wlan_exp_event
                    % Returns:   1 - Node added successfully
                    %            0 - Node not added
                    % 

                    numEvents      = numel( varargin{1} );
                    current_index  = numel( log.event_list );
                    log.event_list = [ log.event_list; varargin{1} ];
                    
                    % TODO:  Add some class checking
                    % if( strcmp( superclasses(  ),'wlan_exp_event') )
                    

                case 'get_events'
                    % Get all events of type 'class'
                    %
                    % Arguments: 'class'
                    % Returns:   vector of all nodes that correspond to 'class'
                    % 
                    numEvents    = numel( log.event_list );
                    search_class = varargin{1};
 
                    index        = 1;
                    resp         = {};
                    
                    if ( strcmp( class( varargin{1} ), 'char' ) )                     
                        for n = 1:numEvents
                            if ( strcmp( class( log.event_list{n} ), search_class ) )
                                resp( index ) = { log.event_list{n} };
                                index         = index + 1;
                            end
                        end
                        
                        if ( isempty( resp ) ) 
                            % TODO:  print warning if you did not get any nodes of the class
                            out = [];
                        else
                            out = resp{:};
                        end
                        
                    else
                        out = [];

                        error(generatemsgid('UnknownArg'),'Unknown argument.  Argument is of type "%s", need a class name that is "char" ', class(varargin{1}));
                    end


                otherwise
                    error(generatemsgid('UnknownCmd'),'Unknown command:  %s ', cmdStr);
            end
            
            if(iscell(out)==0 && numel(out)~=1)
                out = {out}; 
            end     
        end


        function [ magic, timestamp, type, length, id ] = extract_header( obj, bytes )
            % Function will iterate through an array of bytes and extract all the header fields. 
            % By default, all events will have the following fields:
            %       Word [0]         - timestamp[31:0]
            %       Word [1]         - timestamp[63:32]
            %       Word [2]         - event_type[31:16]; event_length[15:0];
            %       Word [3]         - event_id

            words       = swapbytes( typecast( bytes, 'uint32' ) );
            
            magic       = double( bitshift( bitand( words( 2 ), 4294901760 ), -16 ) );
            timestamp   = 2^32 * double( bitand( words( 2 ), 65535 ) ) + double( words( 1 ) );
            length      = double( bitshift( bitand( words( 3 ), 4294901760 ), -16 ) );
            type        = double( bitand( words( 3 ), 65535 ) );
            id          = double( words( 4 ) );
        end
        
        
        function disp(obj)
            % Call the display message for each node in the network
            numEvents    = numel( obj.event_list );
            
            fprintf('--------------------------------------------------------------------------------\n' );
            fprintf('Event Log of Node %d [%s]:  \n', obj.node.ID, obj.node.description );
            fprintf('--------------------------------------------------------------------------------\n' );
            
            for n = 1:numEvents
                fprintf('----------------------------------------\n' );
                fprintf('Event %d \n', n);
                cellfun( @(x) x.disp(), obj.event_list{n} );
            end
            
            fprintf('--------------------------------------------------------------------------------\n' );
            fprintf('End of Event Log \n' );
            fprintf('--------------------------------------------------------------------------------\n' );
        end
        
    end %end methods(Hidden)
end %end classdef






