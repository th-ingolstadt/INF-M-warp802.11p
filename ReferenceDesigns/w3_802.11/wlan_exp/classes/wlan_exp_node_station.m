classdef wlan_exp_node_station < wlan_exp_node
    %The WLAN_EXP Station node class represents an Station 802.11 node in a WARPNet network.
    % wlan_exp_node_station is the primary interface for interacting with Station 802.11 WARPNet nodes.
    % This class provides methods for sending commands and checking the status of Station
    % 802.11 WARPNet nodes.
    
    properties (SetAccess = protected)
        % None - See super class
        
        % TODO - Add a list of APs that are kept by the station
    end
    properties (SetAccess = public)
        % None - See super class
    end
    properties(Hidden = true,Constant = true)
        % These constants define specific command IDs used by this object.
        % Their C counterparts are found in wlan_exp_node_station.h
        CMD_STA_GET_AP_LIST            = 100;
        CMD_STA_ASSOCIATE              = 101;
        
        
        % These constants define the WARPNet type of the node.
        % Their C counterparts are found in wlan_exp_common.h
        WLAN_EXP_STATION               = 2;
    end

    
    methods
        function obj = wlan_exp_node_station()
            % Initialize the type of the class
            %     Note:  this will call the parent constructor first so it will add the AP 
            %            value to the value defined in the parent class
            obj.type = obj.type + obj.WLAN_EXP_STATION;
        end
        
        function applyConfiguration(objVec, IDVec)

            % Apply Configuration only operates on one ojbect at a time
            if ( ( length( objVec ) > 1 ) || ( length( IDVec ) > 1 ) ) 
                error(generatemsgid('ParameterTooLarge'),'applyConfiguration only operates on a single object with a single ID.  Provided parameters with lengths: %d and %d', length(objVec), length(IDVec) ); 
            end
            
            % Apply configuration will finish setting properties for a specific hardware node
            obj    = objVec(1);
            currID = IDVec(1);
            
            % Call super class function
            applyConfiguration@wlan_exp_node(objVec, IDVec);
            
            % Populate the description property with a human-readable description of the node
            %
            obj.description = sprintf('WLAN EXP STA WARP v%d Node - ID %d', obj.hwVer, obj.ID);
        end

        
        function delete(obj)
            delete@wlan_exp_node(obj);
        end
    end
    
   
    methods(Hidden=true)
        %These methods are hidden because users are not intended to call
        %them directly from their WARPLab scripts.     
        
        function out = procCmd(obj, nodeInd, node, cmdStr, varargin)
            %wlan_exp_node_station procCmd(obj, nodeInd, node, varargin)
            % obj     : node object (when called using dot notation)
            % nodeInd : index of the current node, when wl_node is iterating over nodes
            % node    : current node object (the owner of this baseband)
            % varargin:
            out = [];            
            cmdStr = lower(cmdStr);
            switch(cmdStr)

                %------------------------------------------------------------------------------------------------------
                case 'sta_get_ap_list'
                    % Get the AP list that the station can see
                    %
                    % Arguments: SSID
                    % Returns:   none
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_STA_GET_AP_LIST));                    
                    myCmd.addArgs( uint32( node.serialNumber ) );  
                    node.sendCmd(myCmd);
                    

                %------------------------------------------------------------------------------------------------------
                case 'sta_associate'
                    % Associate the Station with the SSID
                    %
                    % Arguments: AP List index
                    % Returns:   none
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_STA_ASSOCIATE));                    
                    myCmd.addArgs( uint32( node.serialNumber ) );  
                    node.sendCmd(myCmd);
                    
                    
                %------------------------------------------------------------------------------------------------------
                otherwise
                    out = procCmd@wlan_exp_node(obj, nodeInd, node, cmdStr, varargin);
            end
            
            if(iscell(out)==0 && numel(out)~=1)
                out = {out}; 
            end     
        end

        
        function processParameters( obj, parameters )
                    
            % Currently, we only implement the base WARPNet parameters
            processParameters@wlan_exp_node( obj, parameters );
            
        end

        
        function processParameterGroup( obj, group, identifier, length, values )
        
            % Currently, we only implement the base WARPNet parameters
            processParameterGroup@wlan_exp_node( obj, group, identifier, length, values );
                    
        end
        
        
        function processParameter( obj, identifier, length, values )
        
            % Currently, we only implement the base WARPNet parameters
            processParameter@wlan_exp_node( obj, identifier, length, values );
                    
        end
        
        
        function disp(obj)
            %This is a "pretty print" method to summmarize details about
            %wl_node objects and print them in a table on Matlab's command
            %line.
            
            extraTitle = '';
            extraLine = '';
            extraArgs = '';
            
                    fprintf('Displaying properties of %d wlan_exp_node_station objects:\n',length(obj));
                    fprintf('|  ID |  WNVER | WLANVER |  HWVER |    Serial # |  Ethernet MAC Addr |          Address | %s\n',extraTitle)
                    fprintf('-----------------------------------------------------------------------------------------%s\n',extraLine)
            for n = 1:length(obj)
                currObj = obj(n); 

                if(isempty(currObj.ID))
                    fprintf('|     N/A     Node object has not been initialized                            |%s\n',extraArgs)
                else
                    ID      = sprintf('%d',currObj.ID);
                    WNVER   = sprintf('%d.%d.%d',currObj.wn_ver_major, currObj.wn_ver_minor, currObj.wn_ver_revision);
                    WLANVER = sprintf('%d.%d.%d',currObj.wlan_exp_ver_major, currObj.wlan_exp_ver_minor, currObj.wlan_exp_ver_revision);
                    HWVER   = sprintf('%d',currObj.hwVer);
                    
                    if(currObj.hwVer == 3)
                        SN = sprintf('W3-a-%05d',currObj.serialNumber);
                        %DNA = sprintf('%d',currObj.fpgaDNA);
                    else
                        SN = 'N/A';
                        %DNA = 'N/A';
                    end
                    
                    if ( ~isempty( currObj.transport ) & ~isempty( currObj.transport.address ) )
                        temp = dec2hex(uint64(currObj.transport.eth_MAC_addr),12);
                        ADDR = currObj.transport.address;
                    else
                        temp = 0;
                        ADDR = '';
                    end                     
                        
                    MACADDR = sprintf('%2s-%2s-%2s-%2s-%2s-%2s',...
                        temp(1:2),temp(3:4),temp(5:6),temp(7:8),temp(9:10),temp(11:12));
                    
                    fprintf('|%4s |%7s |%8s |%7s |%12s |%19s |%17s |%s\n',ID,WNVER,WLANVER,HWVER,SN,MACADDR,ADDR,extraArgs);
                    
                end
                    fprintf('-----------------------------------------------------------------------------------------%s\n',extraLine)
            end
        end        
    end %end methods(Hidden)
end %end classdef
