classdef wlan_exp_association_status < handle_light
    %The WLAN Exp association status class encapsulates all the status values of an 802.11 assocation
    % in a WLAN Exp network.
    
    properties (SetAccess = protected)
        node;                  % Node association came from
    
        aid;                   % Assocation ID
        seq;                   % Sequence number
        address;               % MAC address of the association

        tx_rate;               % TX rate of the association

        rx_power;              % RX power of the association (of last recieved packet)
        rx_timestamp;          % Timestamp of last received packet
        
        tx_total;              % Total number of transmitted packets
        tx_success;            % Total number of successfully transmitted packets
        tx_bytes;              % Total number of bytes transmitted
        
        rx_total;              % Total number of received packets
        rx_success;            % Total number of successfully received packets
        rx_bytes;              % Total number of bytes received
        rx_error;              % Total number of packets that failed during signal packet phase        
    end
    properties (SetAccess = public)
        % None
    end
    properties(Hidden = true,Constant = true)
        % None
    end

    
    methods
        function obj = wlan_exp_association_status( node, values )
            % The constructor sets all the fields of the association.
            %
            % It takes a set of 32 bit values that have the following structure  
            %       typedef struct{
            %           u16  AID;
            %           u16  seq;
            %           u8   addr[6];
            %           u8   tx_rate;
            %           char last_rx_power;      <-- Signed byte
            %           u64  rx_timestamp;
            %           u32  num_tx_total;
            %           u32  num_tx_success;
            %           u32  num_rx_success;
            %           u32  num_rx_bytes;
            %       } station_info;

            obj.node           = node;
            
            index = 1;
            
            obj.aid            = double( bitand( swapbytes( values(index) ), 65535 ) );
            obj.seq            = double( bitand( bitshift( swapbytes( values(index) ), -16), 65535 ) );
            obj.address        = 2^32*double( bitand( bitshift( values(index + 1), -16), 2^16-1) ) + double( bitshift( bitand( values(index + 1), 2^16-1), 16 ) + bitand( bitshift( values(index + 2), -16), 2^16-1) );

            obj.tx_rate        = double( bitand( bitshift( values(index + 2), -8), 255 ) );
            
            obj.rx_power       = (-1 * double( bitand( values(index + 2), 128 ) ) ) + double( bitand( values(index + 2), 127 ) );
            obj.rx_timestamp   = 2^32*double( swapbytes( values(index + 4)) ) + double( swapbytes( values(index + 3) ) );

            obj.tx_total       = double( swapbytes( values(index + 5) ) );
            obj.tx_success     = double( swapbytes( values(index + 6) ) );
            obj.tx_bytes       = 0;          % TODO:  Add this to association information

            obj.rx_total       = 0;          % TODO:  Add this to association information
            obj.rx_success     = double( swapbytes( values(index + 7) ) );
            obj.rx_bytes       = double( swapbytes( values(index + 8) ) );
            obj.rx_error       = 0;          % TODO:  Add this to association information
        end
        
        
        
        function out = wlan_exp_association_statusCmd(obj, varargin)
            % Processes commands from the association object.
            %   This method is safe to call with multiple wlan_exp_association objects as its first argument. 
            %
            associations    = obj;
            numAssociations = numel(associations);
            
            for n = numAssociations:-1:1
                currAssociation = associations(n);
                out(n) = currAssociation.procCmd(n, currAssociation, varargin{:});
            end
            
            if(length(out)==1 && iscell(out))
               out = out{1}; % Strip away the cell if it's just a single element. 
            end
        end
    end
    
   
    methods(Hidden=true)
        %These methods are hidden because users are not intended to call
        %them directly from their WARPLab scripts.     
        
        function out = procCmd(obj, assnInd, association, cmdStr, varargin)
            %wl_node procCmd(obj, nodeInd, node, varargin)
            % obj: node object (when called using dot notation)
            % nodeInd: index of the current node, when wl_node is iterating over nodes
            % node: current node object (the owner of this baseband)
            % varargin:
            out = [];            
            cmdStr = lower(cmdStr);
            switch(cmdStr)

                case 'get_aid'
                    % Get the AID of the association
                    %
                    % Arguments: none
                    % Returns:   AID
                    %
                    out = association.aid;
                    
                    
                otherwise
                    error(generatemsgid('UnknownCommand'), 'unknown node command %s', cmdStr);
            end
            
            if(iscell(out)==0 && numel(out)~=1)
                out = {out}; 
            end     
        end


        
        function disp(obj)
            %This is a "pretty print" method to summmarize details about association objects and 
            % print them in a table on Matlab's command line.
            
            associations    = obj;
            numAssociations = numel(associations);
            
            for n = numAssociations:-1:1
                currAssociation = associations(n);
            
                temp           = dec2hex(uint64( currAssociation.address ),12);
                
                fprintf('---------------------------------------------------\n');
                fprintf('%s\n', currAssociation.node.description);
                fprintf('  AID: %02x -- MAC Addr: %02s:%02s:%02s:%02s:%02s:%02s\n', currAssociation.aid, temp(1:2), temp(3:4), temp(5:6), temp(7:8), temp(9:10), temp(11:12) );
                fprintf('     - Last heard on %.3f ms with sequence number %.0f \n', ( currAssociation.rx_timestamp / 1000 ), currAssociation.seq );
                fprintf('     - Last Rx Power : %.0f dBm\n', currAssociation.rx_power);
                fprintf('     - Tx Rate       : %.0f \n', currAssociation.tx_rate);
                fprintf('     - # Tx MPDUs    : %.0f (%.0f successful)\n', currAssociation.tx_total, currAssociation.tx_success);
                fprintf('     - # Rx MPDUs    : %.0f (%.0f bytes)\n', currAssociation.rx_success, currAssociation.rx_bytes);
            end
            
        end        
    end %end methods(Hidden)
end %end classdef
