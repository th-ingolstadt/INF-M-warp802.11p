classdef wlan_exp_event_err < wlan_exp_event
    %The WLAN Exp event error class encapsulates the values of an 802.11 error event
    % in a WLAN Exp network.
    %
    % Default Error event structure:
    %     Contains nothing
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
        function obj = wlan_exp_event_err( node, id, timestamp, values )
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

            % Call parent class in case something common is added to the event
            parse_event@wlan_exp_event( obj, bytes );
        end
        
        
        function disp(obj)
            %This is a "pretty print" method to summmarize details about event objects and 
            % print them in a table on Matlab's command line.
            
            events    = obj;
            numEvents = numel(events);
            
            for n = numEvents:-1:1
                currEvent = events(n);
            
                fprintf('  --------------------------------\n');
                fprintf('  %d: [%.3f] - Error Event \n', currEvent.id, ( currEvent.timestamp / 1000 ) );
            end
        end        
    end %end methods(Hidden)
end %end classdef


