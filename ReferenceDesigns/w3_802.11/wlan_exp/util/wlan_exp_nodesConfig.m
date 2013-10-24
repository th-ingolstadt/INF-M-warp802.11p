%==============================================================================
% Function:  wlan_exp_nodesConfig
%
% Inputs:
%     'Command'       - Command for the script to perform
%     'Filename'      - File to either read, write, or validate
%     NodeArray       - Optional argument when trying to write the nodes configuration
%
% Commands:
%     'validate'  - Will validate the input file and return 1 if valid or generate an error message
%     'read'      - Will read the input file and return the structure of nodes
%     'write'     - Will take the array of nodes and write to the specified file and return 1 if successful
%
% File Format:
%    serialNumber,ID,ipAddress,unicastPort,bcastPort,className,name,(opt 1),(opt 2), ...,(opt N)
%
%  The serialNumber, ID, ipAddress, and className are required fields and wlan_exp_nodesConfig will error
%  out if those are not present.  If unicastPort or bcastPort are not specified, then the default values of 
%  unicastPort = 9000 and bcastPort = 10000 will be used.
%
% NOTE:  The ',' and '\t' are reserved characters and will be interpreted as the next field;
%        All spaces, ' ', are valid and are not removed from the field.  However, leading and trailing
%            spaces are removed from each field
%        If you add in a optional field, then you have to put the correct number of delimiters, ie ',',
%            to indicate fields are not used
%        There should be no spaces in the field names in the header row
%
% Example:
%  serialNumber,    ID,     ipAddress,  unicastPort,   bcastPort,               nodeClass,    name,    opt_1
%    W3-a-00006,     0,      10.0.0.0,         9000,       10000,        wlan_exp_node_ap,  Node 0,  primary
%    W3-a-00007,     1,      10.0.0.1,         9010,       10000,   wlan_exp_node_station,  Node 1
%    W3-a-00008,     2,    10.0.0.200,             ,            ,   wlan_exp_node_station,
%
%
%==============================================================================

function nodesInfo = wlan_exp_nodesConfig(varargin)

    % Validate input parameters

    if ( (nargin == 0) | (nargin == 1) )
        error(generatemsgid('InsufficientArguments'),'Not enough arguments are provided to function call');
    else
        % Check that the command is valid
        if ( ~strcmp(varargin{1}, 'validate') & ~strcmp(varargin{1}, 'read') & ~strcmp(varargin{1}, 'write') )
            error(generatemsgid('InputFailure'),'Command "%s" is not valid.  Please use "validate", "read", or "write"', varargin{1}) 
        end
    
        % Check that the filename is a character string
        if ( ~ischar( varargin{2} ) )
            error(generatemsgid('InputFailure'),'Filename is not valid') 
        end

        % Check the filename exists if the command is "read" or "validate"
        if ( strcmp(varargin{1}, 'validate') | strcmp(varargin{1}, 'read') )
            if ( ~exist( varargin{2} ) )
                error(generatemsgid('InputFailure'),'Filename "%s" does not exist', varargin{2}) 
            end
        end

        % Check that "write" command only has three arguments
        if ( (nargin == 2) & strcmp(varargin{1}, 'write') )
            error(generatemsgid('InsufficientArguments'),'Not enough arguments are provided to "write" function call (need 3, provided 2)');
        end
        
        % Check for optional array of nodes
        if ( (nargin == 3) )         
            if ( ~strcmp(varargin{1}, 'write') )
                error(generatemsgid('InputFailure'),'Too many arguments are provided to "read" or "validate" function call');    
            end
            
            if ( ~strcmp(class(varargin{3}),'wl_node') )
                error(generatemsgid('InputFailure'),'Argument must be an array of "wl_node".  Provided "%s"', class(varargin{3}));
            end
        end
        
        % Default for other arguments
        if ( (nargin > 3) )
            error(generatemsgid('InputFailure'),'Too many arguments are provided to function call');    
        end
    end

    % Process the command

    fileID = fopen(varargin{2});

    switch( varargin{1} ) 
        case 'validate'
            wl_nodesReadAndValidate( fileID );
            nodesInfo = 1;
        case 'read'
            nodesInfo = wl_nodesReadAndValidate( fileID );
        case 'write'
            error(generatemsgid('TODO'),'TODO:  The "write" command for wlan_exp_nodesConfig() has not been implemented yet.');    
        otherwise
            error(generatemsgid('UnknownCmd'),'unknown command ''%s''',cmdStr);
    end
 
end    
    

function nodeInfo = wl_nodesReadAndValidate( fid )

    % Parse the input file    
    inputParsing    = textscan(fid,'%s', 'CollectOutput', 1, 'Delimiter', '\n\r', 'MultipleDelimsAsOne', 1);
    inputArrayTemp  = inputParsing{:};

    % Remove any commented lines
    input_index = 0;
    inputArray  = cell(1, 1);
    for n = 1:size(inputArrayTemp)
        if ( isempty( regexp(inputArrayTemp{n}, '#', 'start' ) ) )
            input_index = input_index + 1;
            inputArray{input_index, 1} = inputArrayTemp{n};
        end
    end

    % Convert cells to character arrays and break apart on ',' or '\t'
    removeTabs  = regexprep( inputArray, '[\t]', '' );
    fieldsArray = regexp( removeTabs, '[,]', 'split' );
    
    % Remove leading / trailing spaces in each field
    %   - Note:  fieldsArray is a cell array of strings.  Therefore, we have to pull out the array of 
    %         strings for each row in order to pass each of those strings to strtrim to remove leading
    %         and trailing whitespace.
    %
    fields_size = size( fieldsArray );
    
    for m = 1:fields_size(1)
        % Pull out array of strings from cell array
        temp_array = fieldsArray{ m };
        temp_size  = size( temp_array );
        
        % Run strtrim on each string in the temporary array
        for n = 1:temp_size(2)
            temp_array(temp_size(1), n) = strtrim( temp_array(temp_size(1), n) );
        end
        
        % Convert back to a cell array for future processing
        fieldsArray(m) = {temp_array};
    end

    % Remove all spaces from the header row    
    headerRow   = regexprep( fieldsArray{1}, '[ ]', '' );
    
    % Determine size of the matrices
    headerSize = size(headerRow);        % Number of Columns
    fieldSize  = size(fieldsArray);      % Number of Rows (Nodes)

    % Pad columns in cell array 
    % Create structure from cell array
    for n = 1:(fieldSize(1) - 1)
        rowSize = size( fieldsArray{n+1} );
        if ( rowSize(2) > headerSize(2) )
            error(generatemsgid('InputFailure'),'Node %d has too many columns.  Header specifies %d, provided %d', n, headerSize(2), rowSize(2))  
        end
        if ( rowSize(2) < headerSize(2) )
            for m = (rowSize(2) + 1):headerSize(2)
                fieldsArray{n+1}{m} = '';
            end
        end
        nodesStruct(n) = cell2struct( fieldsArray{n + 1}, headerRow, 2);
    end
    
    % Determine if array has correct required fields
    if ( ~isfield(nodesStruct, 'serialNumber') | ~isfield(nodesStruct, 'ID') | ~isfield(nodesStruct, 'ipAddress') | ~isfield(nodesStruct, 'nodeClass') )
        error(generatemsgid('InputFailure'),'Does not have required fields: "serialNumber", "ID", "ipAddress", and "nodeClass" \n ') 
    end         

    % Convert to unit32 for serialNumber, ID, and ipAddress
    nodeSize = size(nodesStruct);

    for n = 1:(nodeSize(2))
        inputString = [nodesStruct(n).serialNumber,0]; %null-terminate
        padLength = mod(-length(inputString),4);
        inputByte = [uint8(inputString),zeros(1,padLength)];
        nodesStruct(n).serialNumberUint32 = typecast(inputByte,'uint32');

        nodesStruct(n).IDUint32 = uint32( str2num( nodesStruct(n).ID ) );

        addrChars = sscanf(nodesStruct(n).ipAddress,'%d.%d.%d.%d');

        if ( ~eq( length( addrChars ), 4 ) )
            error(generatemsgid('InputFailure'),'IP address of node must have form "w.x.y.z".  Provided "%s" \n', nodesStruct(n).ipAddress)         
        end

        if ( (addrChars(4) == 0) | (addrChars(4) == 255) )
            error(generatemsgid('InputFailure'),'IP address of node %d cannot end in .0 or .255 \n', n)         
        end
        nodesStruct(n).ipAddressUint32 =  uint32( 2^0 * addrChars(4) + 2^8 * addrChars(3) + 2^16 * addrChars(2) + 2^24 * addrChars(1) );
    end    
    
    % Determine if each input row is valid
    %   - All serial numbers must be unique
    %   - All UIDs must be unique
    %   - All IP Addresses must be unique
    
    for n = 1:(nodeSize(2) - 1)
        for m = n+1:(nodeSize(2))
            if ( strcmp( nodesStruct(n).serialNumber, nodesStruct(m).serialNumber ) )
                error(generatemsgid('InputFailure'),'Serial Number must be unique.  Nodes %d and %d have the same serial number %s \n', n, m, nodesStruct(n).serialNumber)
            end
            if ( strcmp( nodesStruct(n).ID, nodesStruct(m).ID ) )
                error(generatemsgid('InputFailure'),'Node ID must be unique.  Nodes %d and %d have the same node ID %d \n', n, m, nodesStruct(n).IDUint32)
            end
            if (nodesStruct(n).ipAddressUint32 == nodesStruct(m).ipAddressUint32)
                error(generatemsgid('InputFailure'),'IP Address must be unique.  Nodes %d and %d have the same IP Address %s \n', n, m, nodesStruct(n).ipAddress)
            end    
        end
    end

    % Determine if each input row is valid
    %   - All rows must have a nodeClass
    for n = 1:nodeSize(2)
        if ( strcmp( nodesStruct(n).nodeClass, '' ) )
            error(generatemsgid('InputFailure'),'Node class must be populated.  Node %d on row %d does not have a class. \n', nodesStruct(n).IDUint32, n )
        end
    end
    
    
    % Uncomment Section to Display final node contents       
    % for n = 1:(nodeSize(2))
    %     disp(nodesStruct(n))
    % end
    
    nodeInfo = nodesStruct;    
end






