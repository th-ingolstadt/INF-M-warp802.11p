classdef wlan_exp_event_rx < wlan_exp_event
    %The WLAN Exp event RX class encapsulates the values of an 802.11 RX event
    % in a WLAN Exp network.
    %
    % Default RX event structure:
    %
    %     typedef struct{
    %     	u8   state;
    %     	u8   AID;
    %     	s8   power;
    %     	u8   rate;
    %     	u16  length;
    %     	u16  seq;
    %     	u8   mac_type;
    %     	u8   flags;
    %     	u8   reserved[2];
    %     } rx_event;
    % 
    %  NOTE:  this is equivalent to rx_dsss_event in the C code.
    %
    
    properties (SetAccess = protected)
        state;                    % State of the node
        aid;                      % Association ID
        power;                    % Tx power
        rate;                     % Rate of transmission
        length;                   % Length of transmission
        seq;                      % Sequence number
        mac_type;                 % Mac type
        flags;                    % Flags
    end
    
    properties (SetAccess = public)
        % None
    end
    
    properties(Hidden = true,Constant = true)
        % Number of uint32 words in the event.  This corresponds to the size of the structure in "wlan_mac_events.h"
        EVENT_NUM_WORDS                = 3;
    end
    
    
    methods
        function obj = wlan_exp_event_rx( node, id, timestamp, values )
            % The constructor sets all the fields after calling the parent constructor
            
            % Call the parent object
            obj = obj@wlan_exp_event( node, id, timestamp );

            % Set all the fields
            obj.parse_event( values );
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


        function parse_event( obj, bytes ) 

            words                = swapbytes( typecast( bytes, 'uint32' ) );
            num_words            = length( words );
            index                = 1;
            
            if ( num_words >= obj.EVENT_NUM_WORDS )
            
                % Set the fields
                obj.state            = double( bitand( bitshift( words(index),   0 ), 255 ) );
                obj.aid              = double( bitand( bitshift( words(index),  -8 ), 255 ) );
                obj.power            = (-1 * double( bitand( bitshift( words(index), -16 ), 128 ) ) ) + double( bitand( bitshift( words(index), -16 ), 127 ) );
                obj.rate             = double( bitand( bitshift( words(index), -24 ), 255 ) );

                obj.length           = double( bitand( bitshift( words(index + 1),   0 ), 65535 ) );
                obj.seq              = double( bitand( bitshift( words(index + 1), -16 ), 65535 ) );

                obj.mac_type         = double( bitand( bitshift( swapbytes( words(index + 2) ),   0 ), 255 ) );
                obj.flags            = double( bitand( bitshift( swapbytes( words(index + 2) ),  -8 ), 255 ) );

                % Call parent class in case something common is added to the event
                parse_event@wlan_exp_event( obj, bytes );
                
            else
                warning(generatemsgid('EventInitFailure'),'%s - Received %d words.  Valid events require %d words.  See documentation for event structure.', class( obj ), num_words, obj.EVENT_NUM_WORDS);
            end
        end
        
        
        function disp(obj)
            %This is a "pretty print" method to summmarize details about event objects and 
            % print them in a table on Matlab's command line.
            
            events    = obj;
            numEvents = numel(events);
            
            for n = numEvents:-1:1
                currEvent = events(n);
            
                fprintf('  --------------------------------\n');
                fprintf('  %d: [%.3f] - Rx Event \n', currEvent.id, ( currEvent.timestamp / 1000 ) );
                fprintf('      AID:      %8.0f\t  Pow:      %8.0f\n', currEvent.aid, currEvent.power);
                fprintf('      Seq:      %8.0f\t  Rate:     %8.0f\n', currEvent.seq, currEvent.rate);
                fprintf('      Length:   %8.0f\t  State:    %8.0f\n', currEvent.length, currEvent.state);
                fprintf('      MAC Type: %8.0f\t  Flags:    %8.0f\n', currEvent.mac_type, currEvent.flags);
            end
        end        
    end %end methods(Hidden)
end %end classdef

    
