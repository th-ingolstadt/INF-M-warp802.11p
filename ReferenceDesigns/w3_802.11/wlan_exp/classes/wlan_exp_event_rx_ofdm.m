classdef wlan_exp_event_rx_ofdm < wlan_exp_event_rx
    %The WLAN Exp event RX class encapsulates the values of an 802.11 TX event
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
    %     	u32	 channel_est[64];
    %     } rx_ofdm_event;
    %
    
    properties (SetAccess = protected)
        channel_est;                             % Channel Estimates
    end
    
    properties (SetAccess = public)
        % None
    end
    
    properties(Hidden = true,Constant = true)
        % Number of uint32 words in the event.  This corresponds to the size of the structure in "wlan_mac_events.h"
        EVENT_NUM_WORDS_OFDM           = 67;
    end
    
    
    methods
        function obj = wlan_exp_event_rx_ofdm( node, id, timestamp, values )
            % The constructor sets all the fields after calling the parent constructor
            
            % Call the parent object
            obj = obj@wlan_exp_event_rx( node, id, timestamp, values );

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
            
            if ( num_words >= obj.EVENT_NUM_WORDS_OFDM )
            
                % Set the unique fields of the event
                obj.channel_est      = words( (index + 3):end );

                % Call parent class in case something common is added to the event
                parse_event@wlan_exp_event_rx( obj, bytes );
                
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
                fprintf('  %d: [%.3f] - Rx OFDM Event \n', currEvent.id, ( currEvent.timestamp / 1000 ) );
                fprintf('      AID:      %8.0f\t  Pow:      %8.0f\n', currEvent.aid, currEvent.power);
                fprintf('      Seq:      %8.0f\t  Rate:     %8.0f\n', currEvent.seq, currEvent.rate);
                fprintf('      Length:   %8.0f\t  State:    %8.0f\n', currEvent.length, currEvent.state);
                fprintf('      MAC Type: %8.0f\t  Flags:    %8.0f\n', currEvent.mac_type, currEvent.flags);
                fprintf('      Channel Estimates:\n');
                
                num_channel_est = length( currEvent.channel_est );
                
                for m = 1:num_channel_est
                    fprintf('          0x%8x\n', currEvent.channel_est(m));
                end
            end
        end        
    end %end methods(Hidden)
end %end classdef

    
