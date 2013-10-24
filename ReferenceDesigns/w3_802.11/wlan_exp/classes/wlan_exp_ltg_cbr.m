classdef wlan_exp_event_tx < wlan_exp_event
    %The WLAN Exp event TX class encapsulates the values of an 802.11 TX event
    % in a WLAN Exp network.
    %
    % Default TX event structure:
    %     typedef struct{
    %         u64  timestamp;
    %         u8   event_type;
    %         u8   state;
    %         u8   AID;
    %         char power;
    %         u16  length;
    %         u8   rate;
    %         u8   mac_type;
    %         u16  seq;
    %         u8   retry_count;
    %         u8   reserved[1];
    %     } tx_event;
    % 
    
    properties (SetAccess = protected)
        aid;                      % Association ID
        state;                    % State of the node
        power;                    % Tx power
        length;                   % Length of transmission
        rate;                     % Rate of transmission
        mac_type;                 % Mac type
        seq;                      % Sequence number
        retry_count;              % Retry count
    end
    properties (SetAccess = public)
        % None
    end
    properties(Hidden = true,Constant = true)
        % None
        EVENT_TYPE_TX             =  2;
    end
    
    
    methods
        function obj = wlan_exp_event_tx( node, values )
            % The constructor sets all the fields after calling the parent constructor
            %
            
            obj = obj@wlan_exp_event( node, values );

            index = 1;
    
            type               = double( bitand( swapbytes( values(index + 2) ), 255 ) );

            if ( type == obj.EVENT_TYPE_TX ) 
                obj.type       = obj.EVENT_TYPE_TX;
            else
                error(generatemsgid('TypeMismatch'), 'Event type incorrect.  \nReceived event of type:  %.0f while TX event is of type %d', type, obj.EVENT_TYPE_TX);
            end
            
            obj.state          = double( bitand( bitshift( swapbytes( values(index + 2) ), -8 ), 255 ) );
            obj.aid            = double( bitand( bitshift( swapbytes( values(index + 2) ), -16 ), 255 ) );
            obj.power          = (-1 * double( bitand( bitshift( swapbytes( values(index + 2) ), -24 ), 128 ) ) ) + double( bitand( bitshift( swapbytes( values(index + 2) ), -24 ), 127 ) );
            
            obj.length         = double( bitand( swapbytes( values(index + 3) ), 65535 ) );
            obj.rate           = double( bitand( bitshift( swapbytes( values(index + 3) ), -16), 255 ) );
            obj.mac_type       = double( bitand( bitshift( swapbytes( values(index + 3) ), -24), 255 ) );
            
            obj.seq            = double( bitand( swapbytes( values(index + 4) ), 65535 ) );
            obj.retry_count    = double( bitand( bitshift( swapbytes( values(index + 4) ), -16), 255 ) );
    
        end
        
        
        
        function out = wlan_exp_eventCmd(obj, varargin)
            % Processes commands from the association object.
            %   This method is safe to call with multiple wlan_exp_association objects as its first argument. 
            %
            events    = obj;
            numEvents = numel(events);
            
            for n = numEvents:-1:1
                currEvent = events(n);
                out(n) = currEvent.procCmd(n, currEvent, varargin{:});
            end
            
            if(length(out)==1 && iscell(out))
               out = out{1}; % Strip away the cell if it's just a single element. 
            end
        end
    end
    
   
    methods(Hidden=true)
        %These methods are hidden because users are not intended to call them directly.
        
        function out = procCmd(obj, eventInd, event, cmdStr, varargin)
            %procCmd(obj, index, event, command _tring, varargin)
            % obj      : event object
            % eventInd : index of the current event, when wlan_exp_event is iterating over nodes
            % node     : current event object
            % varargin :
            out = [];            
            cmdStr = lower(cmdStr);
            switch(cmdStr)

                    
                otherwise
                    error(generatemsgid('UnknownCommand'), 'unknown node command %s', cmdStr);
            end
            
            if(iscell(out)==0 && numel(out)~=1)
                out = {out}; 
            end     
        end


        
        function disp(obj)
            %This is a "pretty print" method to summmarize details about event objects and 
            % print them in a table on Matlab's command line.
            
            events    = obj;
            numEvents = numel(events);
            
            fprintf('---------------------------------------------------\n');
            fprintf('%s\n', currEvent.node.description);
            fprintf('---------------------------------------------------\n');

            for n = numEvents:-1:1
                currEvent = events(n);
            
                fprintf('  --------------------------------\n');
                fprintf('  %d: [%.3f] - Tx Event \n', n, ( currEvent.rx_timestamp / 1000 );
                fprintf('      AID:      %.0f\t  Pow:      %.0f\n', currEvent.aid, currEvent.power);
                fprintf('      Seq:      %.0f\t  Rate:     %.0f\n', currEvent.seq, currEvent.rate);
                fprintf('      Length:   %.0f\t  State:    %.0f\n', currEvent.length, currEvent.state);
                fprintf('      MAC Type: %.0f\t  Retry:    %.0f\n', currEvent.mac_type, currEvent.retry_count);
            end
            
            fprintf('---------------------------------------------------\n');
        end        
    end %end methods(Hidden)
end %end classdef
