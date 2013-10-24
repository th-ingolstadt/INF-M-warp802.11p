%******************************************************************************
% WARPNet Buffer 
%
% File   :	wn_buffer.m
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
classdef wn_buffer < wn_msg_helper   
% WARPNet Buffer for transferring information
%   This object will contain a byte array of information that can be decoded
%   by higher level functions.  This object is used as a container by the 
%   transport
%
% Structure:
%   buffer_id   - uint32
%   flags       - uint32
%   start_byte  - uint32
%   size        - uint32
%   bytes[]     - array of uint8
%

%******************************** Properties **********************************
    properties(SetAccess = public)
        buffer_id;
        flags;
        start_byte;
    end

    properties(SetAccess = protected, Hidden=true)
        bytes;
    end

    properties(SetAccess = protected)
        num_bytes;
    end


%********************************* Methods ************************************
    
    methods
        function obj = wn_buffer(varargin)
           obj.buffer_id   = uint32(0); 
           obj.flags       = uint32(0); 
           obj.start_byte  = uint32(0);
           obj.num_bytes   = uint32(0);
           obj.bytes       = [];
           
           if( length( varargin ) == 1 ) 
               obj.deserialize( varargin{1} );
           end
        end
        
        function setFlags(obj,flags)
           flags         = uint32(flags);
           obj.flags     = bitor( obj.flags, flags );
        end
        
        function clearFlags(obj,flags)
           flags         = uint32(flags);
           obj.flags     = bitand( obj.flags, bitcmp(flags) );
        end
        
        function setBytes(obj,bytes)
           obj.num_bytes = length( bytes );
           obj.bytes     = bytes(:).';
        end
        
        function output = getBytes(obj)
           output        = obj.bytes; 
        end
        
        function output = serialize(obj)
           output( 1) = bitshift( bitand( obj.buffer_id,  4278190080 ), -24 );
           output( 2) = bitshift( bitand( obj.buffer_id,    16711680 ), -16 );
           output( 3) = bitshift( bitand( obj.buffer_id,       65280 ),  -8 );
           output( 4) = bitand( obj.buffer_id,  255 );
           output( 5) = bitshift( bitand( obj.flags,      4278190080 ), -24 );
           output( 6) = bitshift( bitand( obj.flags,        16711680 ), -16 );
           output( 7) = bitshift( bitand( obj.flags,           65280 ),  -8 );
           output( 8) = bitand( obj.flags,      255 );
           output( 9) = bitshift( bitand( obj.start_byte, 4278190080 ), -24 );
           output(10) = bitshift( bitand( obj.start_byte,   16711680 ), -16 );
           output(11) = bitshift( bitand( obj.start_byte,      65280 ),  -8 );
           output(12) = bitand( obj.start_byte, 255 );
           output(13) = bitshift( bitand( obj.num_bytes,  4278190080 ), -24 );
           output(14) = bitshift( bitand( obj.num_bytes,    16711680 ), -16 );
           output(15) = bitshift( bitand( obj.num_bytes,       65280 ),  -8 );
           output(16) = bitand( obj.num_bytes,       255 );
           output     = cat( 2, output, obj.bytes);
        end
        
        function deserialize(obj,vec)
           vec = uint32( vec );
           obj.buffer_id   = vec(1);
           obj.flags       = vec(2); 
           obj.start_byte  = vec(3);
           obj.num_bytes   = vec(4);
           
           if( length( vec ) > 4 )
              obj.bytes = typecast( vec(5:end), 'uint8' );
           end
        end
        
        function output = sizeof(obj)
            persistent wn_buffer_length;
            if(isempty(wn_buffer_length))
                wn_buffer_length = length( obj.serialize );
            end
            output = wn_buffer_length;
        end
    end
end