%******************************************************************************
% WARPNet MEX Ethernet UDP Transport
%
% File   :	wn_transport_eth_udp_mex.m
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
% 1.00a ejw  7/12/13  Initial release
% 1.01a ejw  8/5/13   Update to use new WARPNet MEX transport;
%                     Removed references to java transport
%
%******************************************************************************

classdef wn_transport_eth_udp_mex < wn_transport_eth_udp
% Mex physical layer Ethernet UDP Transport for unicast traffic
% User code should not use this object directly-- the parent wn_node will
% instantiate the appropriate transport object for the hardware in use

%******************************** Properties **********************************

    properties (SetAccess = protected, Hidden = true)
        maxSamples;          %Maximum number of samples able to be transmitted (based on maxPayload)
    end


%********************************* Methods ************************************

    methods

        function obj = wn_transport_eth_udp_mex()
            obj = obj@wn_transport_eth_udp();
        end
        
        function checkSetup(obj)
            %%Check to make sure wn_mex_udp_transport exists and is is configured
            temp = which('wn_mex_udp_transport');
            if( isempty( temp ) )
                error('wn_transport_eth_udp_mex:constructor','WARPNet Mex UDP transport not found in Matlab''s path')             
            elseif( strcmp( temp( (end-length(mexext)+1):end ), mexext) ==0 )
                error('wn_transport_eth_udp_mex:constructor','WARPNet Mex UDP transport found, but it is not a compiled mex file');
            end
            
            checkSetup@wn_transport_eth_udp(obj);
        end
        
        function setMaxPayload(obj,value)
            obj.maxPayload = value;
        
            % Compute the maximum number of samples in each Ethernet packet
            %   - Start with maxPayload is the max number of bytes per packet (nominally the Ethernet MTU)
            %   - Subtract sizes of the transport header, command header and samples header
            [samples_path,ig,ig] = fileparts(which('wl_samples'));

            if(~isempty(samples_path))
                obj.maxSamples = double((floor(double(obj.maxPayload)/4) - sizeof(obj.hdr)/4 - sizeof(wn_cmd)/4) - (sizeof(wl_samples)/4));
            else
                obj.maxSamples = 0;
            end
        end
        
        function open(obj,varargin)
            %varargin{1}: (optional) IP address
            %varargin{2}: (optional) port

            if(nargin==3)
               if(ischar(varargin{1}))
                  obj.address = varargin{1};
               else
                  obj.address = obj.int2IP(varargin{1});
               end
               obj.port = varargin{2};
            end

            REQUESTED_BUF_SIZE = 2^22;
             
            % Call to 'init_sockets' will internall call setReuseAddress and setBroadcast
            obj.sock = wn_mex_udp_transport('init_socket');
            
            % fprintf('Opening Sock:  %d \n', obj.sock);
            
            wn_mex_udp_transport('set_so_timeout', obj.sock, 1);
            wn_mex_udp_transport('set_send_buf_size', obj.sock, REQUESTED_BUF_SIZE);
            wn_mex_udp_transport('set_rcvd_buf_size', obj.sock, REQUESTED_BUF_SIZE);

            x = wn_mex_udp_transport('get_rcvd_buf_size', obj.sock);            
            obj.rxBufferSize = x;

            if(x < REQUESTED_BUF_SIZE)
                fprintf('OS reduced recv buffer size to %d\n', x);
            end
            
            obj.status = 1;
        end
        
        function out = procCmd(obj,nodeInd,node,cmdStr,varargin)
            out = [];
            cmdStr = lower(cmdStr);
            switch(cmdStr)
                
                otherwise
                    out = procCmd@wn_transport_eth_udp(obj,nodeInd,node,cmdStr,varargin);
            end
            
            if(iscell(out)==0 && numel(out)~=1)
                out = {out}; 
            end     
        end
        
        function close(obj)            
            if(~isempty(obj.sock))
                try
                    % fprintf('Closing Sock:  %d \n', obj.sock);
                    wn_mex_udp_transport('close', obj.sock);
                catch closeError
                    warning(generatemsgid('CloseErr'), 'Error closing socket; mex error was %s', closeError.message)
                end
            end
            obj.status=0;
        end
        
        function delete(obj)
            obj.close();
        end
        
        function flush(obj)
            %TODO
            flush@wn_transport_eth_udp(obj);
        end
    end %methods

    
    methods (Hidden = true)

        function reply = send(obj,varargin)
            % varargin{1}: data
            % varargin{2}: (optional) # of tranmission attempts, defaults to 2 if left unspecified
            %   OR
            % varargin{2}: (optional) 'noresp' to ignore any responses
            
            payload = uint32(varargin{1});
            obj.hdr.msgLength = (length(payload))*4;       % Length in bytes
            throwError = 0;

            if (nargin==2)
               robust = 1;
               maxAttempts = 2;
            elseif (nargin==3)
               if(ischar(varargin{2}))
                   if(strcmpi(varargin{2},'noresp'))
                       robust = 0;
                   else
                       throwError = 1;
                   end
               else
                   maxAttempts = varargin{2};
               end
               
            else
               throwError = 1;
            end
            
            if(throwError)
               error('wn_transport_eth_mex:send:incorrectArguments','incorrect arguments, please see help for wn_transport_eth_mex/send for usage information'); 
            end
            
            if(robust)
               obj.hdr.flags = bitset(obj.hdr.flags,1,1);
            else
               obj.hdr.flags = bitset(obj.hdr.flags,1,0);
            end
            
            obj.hdr.increment;
            data  = [obj.hdr.serialize, payload];
            data8 = [zeros(1,2,'uint8') typecast(swapbytes(uint32(data)),'uint8')];
            reply = [];
            
            %%%%%%%
            %for index = 1:min([10,length(data)])
            %   fprintf('0x%x ',data(index)); 
            %end
            %fprintf('\n');
            %%%%%%%
            % fprintf('Sending to Address:  %s     Port:  %d \n', obj.address, obj.port);
            
            size = wn_mex_udp_transport('send', obj.sock, data8, length(data8), obj.address, obj.port);

            MAX_PKT_LEN = obj.getMaxPayload() + 100;
            
            if(robust==1)
                %Wait to receive reply from the board                
                currTx = 1;
                receivedResponse = 0;
                currTime = tic;

                while (receivedResponse == 0)
                    
                    try
                        [recv_len, recv_data8] = wn_mex_udp_transport('receive', obj.sock, MAX_PKT_LEN);

                    catch receiveError
                        fprintf('%s.m--Failed to receive UDP packet.\nError message follows:\n    %s\n',mfilename,receiveError.message);
                    end
                    
                    if(recv_len > 0)
                        reply8     = [recv_data8(3:recv_len) zeros(mod(-(recv_len)-2,4),1,'uint8')];
                        reply      = swapbytes(typecast(reply8, 'uint32'));
                        
                        if(obj.hdr.isReply(reply(1:obj.hdr.length)))
                            reply = reply((obj.hdr.length+1):end);
                            if(isempty(reply))
                               reply = []; 
                            end
                            receivedResponse = 1;
                        end 
                    end
                    
                    if (toc(currTime) > obj.timeout && receivedResponse==0)
                        if(currTx == maxAttempts)
                            error('wn_transport_eth_mex:send:noReply','maximum number of retransmissions met without reply from node'); 
                        end
                        if(currTx == maxAttempts)
                            error('wn_transport_eth_mex:send:noReply','maximum number of retransmissions met without reply from node'); 
                        end
                        size = wn_mex_udp_transport('send', obj.sock, data8, length(data8), obj.address, obj.port);
                        currTx = currTx + 1;
                        currTime = tic;
                    end                    
                end
            end
        end
        
        function resp = receive(obj)
            MAX_PKT_LEN = obj.getMaxPayload() + 100;            
            done        = false;
            resp        = [];
            
            while ~done
                try
                    [recv_len, recv_data8] = wn_mex_udp_transport('receive', obj.sock, MAX_PKT_LEN);

                catch receiveError
                    fprintf('%s.m--Failed to receive UDP packet.\nError message follows:\n    %s\n',mfilename,receiveError.message);
                end
                
                if(recv_len > 0)
                    reply8 = [recv_data8(3:recv_len) zeros(mod(-(recv_len-2),4),1,'uint8')].';
                    reply  = swapbytes(typecast(reply8, 'uint32'));

                    if(obj.hdr.isReply(reply(1:obj.hdr.length)))
                        reply = reply((obj.hdr.length+1):end);
                        if(isempty(reply))
                            reply = [];
                        end
                        resp = [resp,reply];
                    end
                else
                    done = true;
                end
            end
            
        end
        
        %----------------------------------------------------------------------
        % Special Transport functions for WARPLab
        %
        %
        
        function reply = print_cmd(obj, type, num_samples, start_sample, buffer_ids, command)
            fprintf('Command:  %s \n', type);
            fprintf('    # samples  = %d    start sample = %d \n', num_samples, start_sample);
            fprintf('    buffer IDs = ');
            for index = 1:length(buffer_ids)
                fprintf('%d    ', buffer_ids(index));
            end
            fprintf('\n    Command (%d bytes): ', length(command) );
            for index = 1:length(command)
                switch(index)
                    case 1
                        fprintf('\n        Padding  : ');
                    case 3
                        fprintf('\n        Dest ID  : ');
                    case 5
                        fprintf('\n        Src ID   : ');
                    case 7
                        fprintf('\n        Rsvd     : ');
                    case 8
                        fprintf('\n        Pkt Type : ');
                    case 9
                        fprintf('\n        Length   : ');
                    case 11
                        fprintf('\n        Seq Num  : ');
                    case 13
                        fprintf('\n        Flags    : ');
                    case 15
                        fprintf('\n        Command  : ');
                    case 19
                        fprintf('\n        Length   : ');
                    case 21
                        fprintf('\n        # Args   : ');
                    case 23
                        fprintf('\n        Args     : ');
                    otherwise
                        if ( index > 23 ) 
                            if ( ( mod( index + 1, 4 ) == 0 ) && not( index == length(command) ) )
                                fprintf('\n                   ');
                            end
                        end
                end
               fprintf('%2x ', command(index) );
            end
            fprintf('\n\n');
            
            reply = 0;
        end

        %-----------------------------------------------------------------
        % read_buffers
        %     Command to utilize additional functionality in the wn_mex_udp_transport C code in order to 
        %     speed up processing of 'readIQ' and 'readRSSI' commands
        % 
        % Supports the following calling conventions:
        %    - start_sample -> must be a single value
        %    - num_samples  -> must be a single value
        %    - buffer_ids   -> Can be a vector of single RF interfaces
        % 
        function reply = read_buffers(obj, func, num_samples, buffer_ids, start_sample, wn_command)
            % func           : Function within read_buffers to call
            % number_samples : Number of samples requested
            % buffer_ids     : Array of Buffer IDs
            % start_sample   : Start sample
            % wn_command     : Ethernet WARPNet command

            % Get the lowercase version of the function            
            func = lower(func);

            % Get the number of buffer ids in the array
            num_buffer_ids = length(buffer_ids);
            
            % Calculate how many transport packets are required
            numPktsRequired = ceil(double(num_samples)/double(obj.maxSamples));

            % Pre-allocate the samples array
            %     NOTE:  For RSSI the number of samples returned is twice the number requested
            %            since there are two 16 bit RSSI values per "sample" 
            switch(func)
                case 'iq'
                    rx_samples = zeros(num_samples, num_buffer_ids);
                case 'rssi'
                    rx_samples = zeros(num_samples * 2, num_buffer_ids);
                otherwise
                    error(generatemsgid('UnknownCmd'),'unknown command ''%s''',cmdStr);                    
            end
            
            % For each Buffer ID in the array of buffer IDs
            for index = 1:num_buffer_ids;
            
                buffer_id = buffer_ids(index);
                
                % Check to make sure buffer_ids is the correct dimensionality
                % if( ( length(strfind(dec2bin(buffer_ids(index)),'1'))==1 ) == 0 )
                if( ~( ( buffer_id == 1 ) || ( buffer_id == 2 ) || ( buffer_id == 4 ) || (buffer_id == 8) ) )
                    error(generatemsgid('InvalidBufferSelect'),'WL MEX transport: buffer selection must be singular. Use vector notation for reading from multiple buffers e.g. [RFA,RFB]');
                end
                
                % Command argumnets for the command will be set in the MEX function since it is faster
                % wn_command.setArgs(buffer_id, start_sample, num_samples, obj.maxSamples * 4, numPktsRequired);
                
                % Construct the WARPNet command that will be used used to get the samples
                %   NOTE:  Since we did not add the arguments of the command thru setArgs, we need to pad the structure so that 
                %          the proper amount of memory is allocated to be available to the MEX
                payload           = uint32( wn_command.serialize() );        % Convert command to uint32
                cmd_args_pad      = uint32( zeros(1, 5) );                   % Padding for command args
                obj.hdr.flags     = bitset(obj.hdr.flags,1,0);               % We do not need a response for the sent command
                obj.hdr.msgLength = ( ( length( payload ) ) + 5) * 4;        % Length in bytes

                data  = [obj.hdr.serialize, payload, cmd_args_pad];
                data8 = [zeros(1,2,'uint8') typecast(swapbytes(uint32(data)),'uint8')];

                
                switch(func)
                    case 'iq'
                        % Calls the MEX read_iq command
                        %
                        % obj.print_cmd('READ_IQ', num_samples, start_sample, buffer_ids, data8);
                        
                        [num_rcvd_samples, cmds_used, samples]  = wn_mex_udp_transport('read_iq', obj.sock, data8, length(data8), obj.address, obj.port, num_samples, buffer_id, start_sample, obj.maxSamples * 4, numPktsRequired);

                        % Code to test higher level matlab code without invoking the MEX transport
                        % 
                        % samples = zeros( num_samples, 1 );
                        % cmds_used = 0;
                        
                        rx_samples(:,index) = samples;

                    case 'rssi'
                        % Calls the MEX read_rssi command
                        %                        
                        % obj.print_cmd('READ_RSSI', num_samples, start_sample, buffer_ids, data8);

                        [num_rcvd_samples, cmds_used, samples]  = wn_mex_udp_transport('read_rssi', obj.sock, data8, length(data8), obj.address, obj.port, num_samples, buffer_id, start_sample, obj.maxSamples * 4, numPktsRequired);

                        % Code to test higher level matlab code without invoking the MEX transport
                        %
                        % samples = zeros( num_samples * 2, 1 );
                        % cmds_used = 0;
                        
                        rx_samples(:,index) = samples;

                    otherwise
                        error(generatemsgid('UnknownCmd'),'unknown command ''%s''',cmdStr);
                        
                end
                
                obj.hdr.increment(cmds_used);
            end
            
            reply = rx_samples;

        end
 
 
        %-----------------------------------------------------------------
        % write_buffers
        %     Command to utilize additional functionality in the wn_mex_udp_transport C code in order to 
        %     speed up processing of 'writeIQ' commands
        % 
        function write_buffers(obj, func, num_samples, samples_I, samples_Q, buffer_ids, start_sample, hw_ver, wn_command)
            % func           : Function within read_buffers to call
            % number_samples : Number of samples requested
            % samples_I      : Array of uint16 I samples
            % samples_Q      : Array of uint16 Q samples
            % buffer_ids     : Array of Buffer IDs
            % start_sample   : Start sample
            % hw_ver         : Hardware version of the Node
            % wn_command     : Ethernet WARPNet command
            
            %Calculate how many transport packets are required
            numPktsRequired = ceil(double(num_samples)/double(obj.maxSamples));

            % Construct the WARPNet command that will be used used to write the samples
            payload           = uint32( wn_command.serialize() );        % Convert command to uint32
            obj.hdr.flags     = bitset(obj.hdr.flags,1,0);                % We do not need a response for the sent command
            obj.hdr.msgLength = ( length( payload ) ) * 4;                % Length in bytes
            data              = [obj.hdr.serialize, payload];
            data8             = [zeros(1,2,'uint8') typecast(swapbytes(uint32(data)),'uint8')];
            
            func = lower(func);
            switch(func)
                case 'iq'
                    % Calls the MEX read_iq command
                    %
                    % Needs to handle all the array dimensionality; MEX will only process one buffer ID at a atime 
                    
                    for index = 1:(length(buffer_ids))                    
                        cmds_used = wn_mex_udp_transport('write_iq', obj.sock, data8, obj.getMaxPayload(), obj.address, obj.port, num_samples, samples_I(:,index), samples_Q(:,index), buffer_ids(index), start_sample, numPktsRequired, obj.maxSamples, hw_ver);
                        
                        % increment the transport header by cmds_used (ie number of commands used
                        obj.hdr.increment(cmds_used);
                    end
                    
                otherwise
                    error(generatemsgid('UnknownCmd'),'unknown command ''%s''',cmdStr);
                    
            end
            
        end
        
    end
end %classdef




