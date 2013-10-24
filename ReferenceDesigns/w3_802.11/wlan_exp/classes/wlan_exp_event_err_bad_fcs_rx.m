classdef wlan_exp_event_err_bad_fcs_rx < wlan_exp_event_err
    %The WLAN Exp event error bad fcs rx class encapsulates the values of an 802.11 bad fcs rx event
    % in a WLAN Exp network.
    %
    % Default Error event structure:
    %     typedef struct{
    %     	u8  rate;
    %     	u8  reserved;
    %     	u16  length;
    %     } bad_fcs_event;
    %
    
    properties (SetAccess = protected)
        rate;                     % Rate of transmission
        length;                   % Length of transmission
    end
    properties (SetAccess = public)
        % None
    end
    properties(Hidden = true,Constant = true)
        % Number of uint32 words in the event.  This corresponds to the size of the structure in "wlan_mac_events.h"
        EVENT_NUM_WORDS                = 1;
    end
    
    
    methods
        function obj = wlan_exp_event_err_bad_fcs_rx( node, id, timestamp, values )
            % The constructor sets all the fields after calling the parent constructor
            
            % Call the parent object
            obj = obj@wlan_exp_event_err( node, id, timestamp, values );

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
                obj.rate             = double( bitand( bitshift( words(index),   0 ),   255 ) );
                obj.length           = double( bitand( bitshift( words(index), -16 ), 65535 ) );

                % Call parent class in case something common is added to the event
                parse_event@wlan_exp_event_err( obj, bytes );
                
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
                fprintf('  %d: [%.3f] - Bad FCS RX Event \n', currEvent.id, ( currEvent.timestamp / 1000 ) );
                fprintf('      Length:   %8.0f\t  Rate:     %8.0f\n', currEvent.length, currEvent.rate);
            end
        end        
    end %end methods(Hidden)
end %end classdef


