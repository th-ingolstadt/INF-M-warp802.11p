classdef wlan_exp_event_rx_dsss < wlan_exp_event_rx
    %The WLAN Exp event RX DSSS class encapsulates the values of an 802.11 RX DSSS event
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
    %     	u8   rf_gain;
    %     	u8   bb_gain;
    %     } rx_dsss_event;
    % 
    %  NOTE:  this is equivalent to wlan_exp_event_rx in the M code.
    %
    
    properties (SetAccess = protected)
        % None
    end
    
    properties (SetAccess = public)
        % None
    end
    
    properties(Hidden = true,Constant = true)
        % None
    end
    
    
    methods
        function obj = wlan_exp_event_rx_dsss( node, id, timestamp, values )
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
            % Since there are no new fields in this class, just call the parent class
            parse_event@wlan_exp_event_rx( obj, bytes );
        end
        
        
        function disp(obj)
            %This is a "pretty print" method to summmarize details about event objects and 
            % print them in a table on Matlab's command line.
            
            events    = obj;
            numEvents = numel(events);
            
            for n = numEvents:-1:1
                currEvent = events(n);
            
                fprintf('  --------------------------------\n');
                fprintf('  %d: [%.3f] - Rx DSSS Event \n', currEvent.id, ( currEvent.timestamp / 1000 ) );
                fprintf('      AID:      %8.0f\t  Pow:      %8.0f\n', currEvent.aid, currEvent.power);
                fprintf('      Seq:      %8.0f\t  Rate:     %8.0f\n', currEvent.seq, currEvent.rate);
                fprintf('      Length:   %8.0f\t  State:    %8.0f\n', currEvent.length, currEvent.state);
                fprintf('      MAC Type: %8.0f\t  Flags:    %8.0f\n', currEvent.mac_type, currEvent.flags);
            end
        end        
    end %end methods(Hidden)
end %end classdef

    
