classdef wlan_exp_node_ap < wlan_exp_node
    %The WLAN_EXP AP node class represents an AP 802.11 node in a WARPNet network.
    % wlan_exp_node_ap is the primary interface for interacting with AP 802.11 WARPNet nodes.
    % This class provides methods for sending commands and checking the status of AP
    % 802.11 WARPNet nodes.
    
    properties (SetAccess = protected)
        % None - See super class
    end
    properties (SetAccess = public)
        % None - See super class
    end
    properties(Hidden = true,Constant = true)
        % These constants define specific command IDs used by this object.
        % Their C counterparts are found in wlan_exp_node_ap.h
        CMD_AP_ALLOW_ASSOCIATIONS      = 100;
        CMD_AP_DISALLOW_ASSOCIATIONS   = 101;
        CMD_AP_GET_SSID                = 102;
        CMD_AP_SET_SSID                = 103;

        
        % These constants define the WARPNet type of the node.
        % Their C counterparts are found in wlan_exp_common.h
        WLAN_EXP_AP                    = 1;

        
        % These constants define are for the status of allowing associations
        % Their C counterparts are found in wlan_mac_ap.h
        ASSOCIATION_ALLOW_NONE         = 0;
        ASSOCIATION_ALLOW_TEMPORARY    = 1;
        ASSOCIATION_ALLOW_PERMANENT    = 3;

    end


    methods
        function obj = wlan_exp_node_ap()
            % Initialize the type of the class
            %     Note:  this will call the parent constructor first so it will add the AP 
            %            value to the value defined in the parent class
            obj.type = obj.type + obj.WLAN_EXP_AP;
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
            obj.description = sprintf('WLAN EXP AP WARP v%d Node - ID %d', obj.hwVer, obj.ID);
        end

        
        function delete(obj)
            delete@wlan_exp_node(obj);
        end
    end
    
   
    methods(Hidden=true)
        %These methods are hidden because users are not intended to call
        %them directly from their WARPLab scripts.     
        
        function out = procCmd(obj, nodeInd, node, cmdStr, varargin)
            %wlan_exp_node_ap procCmd(obj, nodeInd, node, varargin)
            % obj     : node object (when called using dot notation)
            % nodeInd : index of the current node, when wl_node is iterating over nodes
            % node    : current node object (the owner of this baseband)
            % varargin:
            out = [];            
            cmdStr = lower(cmdStr);
            switch(cmdStr)

                %------------------------------------------------------------------------------------------------------
                case 'ap_allow_associations'
                    % AP will allow associations 
                    %
                    % Arguments: none
                    %            'permanent'  - Will permanently allow associations
                    %
                    % Returns:   status       - Association allow status:
                    %                               ASSOCIATION_ALLOW_NONE
                    %                               ASSOCIATION_ALLOW_TEMPORARY
                    %                               ASSOCIATION_ALLOW_PERMANENT
                    %
                    
                    % Value of argument to pass to the command
                    arg = 0;
                    
                    % If there is an argument, then process it
                    if ( numel( varargin ) > 0 ) 
                    
                        arg = varargin{1};

                        switch( class( arg ) ) 
                            case 'char'
                                if ( strcmp( arg, 'permanent' ) ) 
                                    arg = uint32( 65535 );
                                else
                                    error(generatemsgid('ParameterSyntaxError'),'Allow association argument not a valid type.\n  Syntax:  wn_nodeCmd( nodes, <allow_associations>, <permanent> or nothing )');                            
                                end
                                
                            otherwise
                                error(generatemsgid('ParameterSyntaxError'),'Association ID not a valid number of <all>\n  Syntax:  wn_nodeCmd( nodes, <disassociate>, <association id> or <all> )');
                        end
                    end
                        
                    % Set up command to send to node
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_ALLOW_ASSOCIATIONS));                    
                    myCmd.addArgs( arg );                                      % Send argument
                    resp  = node.sendCmd(myCmd);
                    out   = resp.getArgs();
                    
                    
                %------------------------------------------------------------------------------------------------------
                case 'ap_disallow_associations'
                    % AP will not allow associations
                    %
                    % Arguments: none
                    % Returns  : status       - Association allow status:
                    %                               ASSOCIATION_ALLOW_NONE
                    %                               ASSOCIATION_ALLOW_TEMPORARY
                    %                               ASSOCIATION_ALLOW_PERMANENT
                    % 
                    
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_DISALLOW_ASSOCIATIONS));                    
                    resp  = node.sendCmd(myCmd);
                    resp  = resp.getArgs();
                    
                    if ( resp ~= obj.ASSOCIATION_ALLOW_NONE ) 
                        warning(generatemsgid('ResponseMismatch'),'Failed to disallow associations.  Please check that associations are not permanently allowed.');
                    end
                    
                    
                %------------------------------------------------------------------------------------------------------
                case 'ap_get_ssid'
                    % Get SSID of the AP
                    %
                    % Arguments: none
                    % Returns:   SSID
                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_GET_SSID));                    
                    resp  = node.sendCmd(myCmd);
                    resp  = resp.getArgs();

                    id_len     = resp(1);
                    char_index = 1;
                    ssid       = [''];

                    idx        = 2;
                    position   = 0;
                    
                    for n = 1:id_len
                        
                        ssid( char_index ) = char( bitand( bitshift( swapbytes( resp( idx ) ), position ), 255 ) );
                        position     = position - 8;
                        char_index   = char_index + 1;
                        
                        if ( mod( n, 4 ) == 0 )
                            idx      = idx + 1;
                            position = 0;
                        end
                    end
                    
                    out = ssid;
                    
            
                %------------------------------------------------------------------------------------------------------
                case 'ap_set_ssid'
                    % Sets the SSID of the AP
                    %
                    % Arguments: SSID
                    % Returns:   none
                    
                    % First, we need to strip off two layers of cells to get to the contents of varargin
                    ssid = varargin{1};
                    
                    if( ~strcmp( class( ssid ), 'char') )
                        error(generatemsgid('ParameterSytnaxError'),'SSID not valid string.\n  Syntax:  wn_nodeCmd( nodes, <set_ssid>, <SSID name> )');
                    end

                    myCmd = wn_cmd(node.calcCmd(obj.GRP, obj.CMD_SET_SSID));
                    
                    % Set number of characters in the new SSID
                    id_len     = length( ssid );
                    myCmd.addArgs( uint32( id_len ) );  

                    % Create an argument array for the new SSID
                    %   NOTE:  The characters must be arranged such that only a straight memcpy can be used on the node 
                    %       (ie there is no processing of the character array done on the node)                    
                    bytes   = unicode2native( ssid );
                    
                    % Pad the array so that it fits in an integer number of uint32 elements 
                    padding = 4 - mod( id_len, 4 );
                    bytes   = [bytes zeros(1, padding,'uint8')];
                    
                    % Swap the characters within each uint32 to get the correct order
                    args  = swapbytes( typecast( bytes, 'uint32' ) );
                    
                    myCmd.addArgs( args );  
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
            
                    fprintf('Displaying properties of %d wlan_exp_node_ap objects:\n',length(obj));
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
