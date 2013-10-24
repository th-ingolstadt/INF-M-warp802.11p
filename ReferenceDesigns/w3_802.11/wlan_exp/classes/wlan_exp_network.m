%******************************************************************************
% WLAN Exp Network
%
% File   :	wlan_exp_network.m
% Authors:	Chris Hunter (chunter [at] mangocomm.com)
%			Patrick Murphy (murphpo [at] mangocomm.com)
%           Erik Welsh (welsh [at] mangocomm.com)
% License:	Copyright 2013, Mango Communications. All rights reserved.
%			Distributed under the WARP license  (http://warpproject.org/license)
%
%******************************************************************************
% MODIFICATION HISTORY:
%
% Ver   Who  Date     Changes
% ----- ---- -------- -------------------------------------------------------
%
%******************************************************************************

classdef wlan_exp_network < handle_light
    %
    % The WARPNet node class represents one node in a WARPNet network.  wn_node is the 
    % primary interface for interacting with WARPNet nodes.  This class provides methods 
    % for sending commands and checking the status of WARPNet nodes.
    %
    % The wn_node class provides the base definition of a WARPNet node and it is expected
    % that it will be extened to incorporate additional functionality.  By default, the 
    % WARPNet node provdes a base node and transport components.
    %
    
    properties (SetAccess = protected)
        nodes;                 % Cell array of nodes
    end
    properties (SetAccess = public)
        name;                  % User specified name for node; user scripts supply this
    end
    
    
    methods
        function obj = wlan_exp_network( varargin )
            % Initialize the array of nodes            
            if nargin == 0
                obj.nodes = {};

            elseif nargin == 1
                switch( class(varargin{1}) ) 
                    case 'cell'
                        obj.nodes = varargin{1};
                    otherwise
                        error(generatemsgid('UnknownArg'),'Unknown argument.  Argument is of type "%s", need "cell"', class(varargin{1}));
                end
            else
                error(generatemsgid('TooManyArguments'),'Too many arguments provided');
            end
        end

        
        function out = wlan_exp_networkCmd(obj, varargin)
            % Processes
            %   This method is safe to call with multiple wlan_exp_network objects as its first argument. 
            %   For example, let network0 and network1 be wlan_exp_network objects:
            %
            %   wlan_exp_networkCmd(network0, args )
            %   wlan_exp_networkCmd([network0, network1], args )
            %   network0.wlan_exp_networkCmd( args )
            %
            % are all valid ways to call this method. Note, when calling this method for 
            % multiple network objects, the interpretation of other vectored arguments is left 
            % up to the underlying components.
            %
            networks = obj;
            numNetworks = numel(networks);
            
            for n = numNetworks:-1:1
                currNetwork = networks(n);
                out(n) = currNetwork.procCmd(n, currNetwork, varargin{:});
            end
            
            if(length(out)==1 && iscell(out))
               out = out{1}; % Strip away the cell if it's just a single element. 
            end
        end
        
        
        function delete(obj)
            obj.nodes = {};
        end
    end
    
   
        
    methods(Hidden=true)
        % These methods are hidden because users are not intended to call
        % them directly from their WLAN Exp scripts.     
        
        function out = procCmd(obj, networkInd, network, cmdStr, varargin)
            %wlan_exp_network procCmd(obj, nodeInd, node, varargin)
            % obj        : network object (when called using dot notation)
            % networkInd : index of the current network, when wlan_exp_network is iterating over networks
            % network    : current network object
            % varargin:
            out = [];
            cmdStr = lower(cmdStr);
            switch(cmdStr)

                case 'add_node'
                    % Add a node to the network
                    %
                    % Arguments: Node that is a child of wlan_exp_node
                    % Returns:   1 - Node added successfully
                    %            0 - Node not added
                    % 
                    
                    
                    % if( strcmp( superclasses(  ),'wlan_exp_node') )

                    % TODO
                    
                    

                case 'get_nodes'
                    % Get all nodes of type 'class'
                    %
                    % Arguments: 'class'
                    % Returns:   vector of all nodes that correspond to 'class'
                    % 
                    numNodes     = numel( network.nodes );
                    search_class = varargin{1};
 
                    index        = 1;
                    resp         = {};
                    
                    if ( strcmp( class( varargin{1} ), 'char' ) )                     
                        for n = 1:numNodes
                            if ( strcmp( class( network.nodes{n} ), search_class ) )
                                resp( index ) = { network.nodes{n} };
                                index         = index + 1;
                            end
                        end
                        
                        if ( isempty( resp ) ) 
                            % TODO:  print warning if you did not get any nodes of the class
                            out = [];
                        else
                            out = resp{:};
                        end
                        
                    else
                        out = [];

                        error(generatemsgid('UnknownArg'),'Unknown argument.  Argument is of type "%s", need a class name that is "char" ', class(varargin{1}));
                    end


                case 'get_node'
                    % Get a node of with the given ID 
                    %
                    % Arguments: 'ID' or ID
                    % Returns:   a node that corresponds to that 'ID'
                    % 
                    numNodes     = numel( network.nodes );
                    search_id    = varargin{1};
                    
                    resp         = {};

                    switch( class( varargin{1} ) )
                        case 'char'
                            search_id = str2num( varargin{1} );
                        
                        case 'double'
                            search_id = varargin{1};
                        
                        otherwise
                            error(generatemsgid('UnknownArg'),'Unknown argument.  Argument is of type "%s", need a class name that is "char" ', class(varargin{1}));
                            out = [];
                    end
                    
                    for n = 1:numNodes
                        if ( network.nodes{n}.ID == search_id )
                            resp = { network.nodes{n} };
                        end
                    end
                        
                    if ( isempty( resp ) ) 
                        % TODO:  print warning if you did not get any nodes of the class
                        out = [];
                    else
                        out = resp{:};
                    end


                case 'get_node_ids'
                    % Get all node ids of type 'class'
                    %
                    % Arguments: 'class'
                    % Returns:   vector of all node ids that correspond to 'class'
                    % 
                    numNodes     = numel( network.nodes );
                    search_class = varargin{1};
 
                    index        = 1;
                    resp         = {};
                    
                    if ( strcmp( class( varargin{1} ), 'char' ) )                     
                        for n = 1:numNodes
                            if ( strcmp( class( network.nodes{n} ), search_class ) )
                                resp( index ) = { network.nodes{n}.ID };
                                index         = index + 1;
                            end
                        end
                        
                        if ( isempty( resp ) ) 
                            % TODO:  print warning if you did not get any nodes of the class
                            out = [];
                        else
                            out = resp{:};
                        end
                        
                    else
                        out = [];

                        error(generatemsgid('UnknownArg'),'Unknown argument.  Argument is of type "%s", need a class name that is "char" ', class(varargin{1}));
                    end


                otherwise
                    % Default behavior is to call the wn_node command for each node using the command string
                    % 
                    numNodes     = numel( network.nodes );                    
                    resp         = {};
                    
                    for n = 1:numNodes
                        resp(n)  = { wn_nodeCmd( network.nodes{n}, cmdStr, varargin ) };
                    end
                    
                    out = resp;                
            end
            
            if(iscell(out)==0 && numel(out)~=1)
                out = {out}; 
            end     
        end
        
        
        function disp(obj)
            % Call the display message for each node in the network
            numNodes     = numel( obj.nodes );
            
            for n = 1:numNodes
                obj.nodes{n}.disp();
            end
        end
        
        
    end %end methods(Hidden)
end %end classdef
