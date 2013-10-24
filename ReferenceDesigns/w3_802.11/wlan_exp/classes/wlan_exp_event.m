classdef wlan_exp_event < handle_light
    %The WLAN Exp event class encapsulates the base values of an 802.11 event
    % in a WLAN Exp network.
    %
    % Default event structure:
    %     typedef struct{
    %         u64 timestamp;
    %         u8  event_type;
    %         u8  reserved[11];
    %     } default_event;
    %
    
    properties (SetAccess = protected)
        node;                  % Node association came from

        id;                    % Event ID
        timestamp;             % Timestamp of Event
    end
    
    properties (SetAccess = public)
        % None
    end
    
    
    methods
        function obj = wlan_exp_event( node, id, timestamp )
            % The constructor sets common fields from the event header.  
            %
            obj.node        = node;
            obj.id          = id;
            obj.timestamp   = timestamp;
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

        
        function parse_event( obj, bytes ) 
            % There is nothing to parse in a default event.
            %   NOTE:  This is included since each class needs to call the parent in case there
            %          becomes something common to parse.
        end

        
        function disp(obj)
            %This is a "pretty print" method to summmarize details about event objects and 
            % print them in a table on Matlab's command line.
            
            events    = obj;
            numEvents = numel(events);
            
            for n = numEvents:-1:1
                currEvent = events(n);
            
                fprintf('---------------------------------------------------\n');
                fprintf('%s\n', currEvent.node.description);
                fprintf('  Event of type %.0f occurred at %.3f ms \n', currEvent.type, ( currEvent.timestamp / 1000 ) );
            end
        end        
    end %end methods(Hidden)
end %end classdef
