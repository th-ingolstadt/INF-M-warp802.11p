%******************************************************************************
% WARPNet Java Ethernet UDP Transport
%
% File   :	wn_transport_eth_udp_java.m
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

classdef wn_transport_eth_udp_java < wn_transport_eth_udp
% Java-based Ethernet UDP Transport for unicast traffic
% User code should not use this object directly-- the parent wn_node will
%  instantiate the appropriate transport object for the hardware in use

%******************************** Properties **********************************

% Only uses the properties from the wn_transport_eth_udp parent class


%********************************* Methods ************************************

    methods
        function obj = wn_transport_eth_udp_java()
            obj = obj@wn_transport_eth_udp();
        end
        
        function checkSetup(obj)
            %TODO
            checkSetup@wn_transport_eth_udp(obj);
        end

        function open(obj,varargin)
            %varargin{1}: (optional) IP address
            %varargin{2}: (optional) port

            import java.io.*
            import java.net.DatagramSocket
            import java.net.DatagramPacket
            import java.net.InetAddress            
            if(nargin==3)
               if(ischar(varargin{1}))
                  obj.address = varargin{1};
               else
                  obj.address = obj.int2IP(varargin{1});
               end
               obj.port = varargin{2};
            end
            
            %obj.sock = DatagramSocket(obj.port);
            obj.sock = DatagramSocket();
            obj.sock.setSoTimeout(1);
            obj.sock.setReuseAddress(1);
            obj.sock.setSendBufferSize(2^22);
            REQUESTED_BUF_SIZE = 2^22; %2^22
            obj.sock.setReceiveBufferSize(REQUESTED_BUF_SIZE); 
            x = obj.sock.getReceiveBufferSize();
            obj.rxBufferSize = x;
            if(x < REQUESTED_BUF_SIZE)
                fprintf('OS reduced recv buffer size to %d\n', x);
            end
            obj.sock.setBroadcast(true);

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
                    obj.sock.close();
                catch closeError
                    warning(generatemsgid('CloseErr'), 'Error closing socket; java error was %s', closeError.message)
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
            %varargin{1}: data
            %varargin{2}: (optional) # of tranmission attempts, defaults to
            %2 if left unspecified
            %   OR
            %varargin{2}: (optional) 'noresp' to ignore any responses
            
            import java.io.*
            import java.net.DatagramSocket
            import java.net.DatagramPacket
            import java.net.InetAddress            

            payload = uint32(varargin{1});
            obj.hdr.msgLength = (length(payload))*4; %Length in bytes
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
               error('wn_transport_eth_java:send:incorrectArguments','incorrect arguments, please see help for wn_transport_eth_java/send for usage information'); 
            end
            
            if(robust)
               obj.hdr.flags = bitset(obj.hdr.flags,1,1);
            else
               obj.hdr.flags = bitset(obj.hdr.flags,1,0);
            end
            
            obj.hdr.increment;
            data = [obj.hdr.serialize, payload];
            
           % fprintf('pktLen: %u (bytes)\n',length(data)*4);
            
            reply = [];
            
            %%%%%%%
            %for index = 1:min([10,length(data)])
            %   fprintf('0x%x ',data(index)); 
            %end
            %fprintf('\n');
            %%%%%%%

            data8 = [zeros(1,2,'int8') typecast(swapbytes(uint32(data)),'int8')];
            jaddr = InetAddress.getByName(obj.address);
            pkt_send = DatagramPacket(data8, length(data8), jaddr, obj.port);
            obj.sock.send(pkt_send);
            
            %Wait to receive reply from the board                
            if(robust==1)
                MAX_PKT_LEN = obj.getMaxPayload() + 100;
                pkt_recv = DatagramPacket(zeros(1,MAX_PKT_LEN,'int8'), MAX_PKT_LEN);
                currTx = 1;
                receivedResponse = 0;
                currTime = tic;

                while (receivedResponse == 0)
                    try
                        obj.sock.receive(pkt_recv);
                        recv_len = pkt_recv.getLength;
                    catch receiveError
                        if ~isempty(strfind(receiveError.message,'java.net.SocketTimeoutException'))
                            recv_len = 0;
                         %Timeout receiving; do nothing
                        else
                           fprintf('%s.m--Failed to receive UDP packet.\nJava error message follows:\n%s',mfilename,receiveError.message);
                        end
                    end
                    
                    if(recv_len > 0)
                        recv_data8 = pkt_recv.getData;
                        reply8 = [recv_data8(3:recv_len) zeros(mod(-(recv_len)-2,4),1,'int8')];
                        reply = swapbytes(typecast(reply8, 'uint32'));
                                                
                        if(obj.hdr.isReply(reply(1:obj.hdr.length)))
                            reply = reply((obj.hdr.length+1):end);
                            if(isempty(reply))
                               reply = []; 
                            end
                            receivedResponse = 1;
                        end 
                    end
                    
                    %timeout = timeout + 1;
                    %if (timeout == 1000 && receivedResponse==0)
                    if (toc(currTime) >obj.timeout && receivedResponse==0)
                        if(currTx == maxAttempts)
                            error('wn_transport_eth_java:send:noReply','maximum number of retransmissions met without reply from node'); 
                        end
                    if(currTx == maxAttempts)
                       error('wn_transport_eth_java:send:noReply','maximum number of retransmissions met without reply from node'); 
                    end
                        obj.sock.send(pkt_send);
                        currTx = currTx + 1;
                        currTime = tic;
                    end                    
                end
            end
        end
        
        function resp = receive(obj)
            MAX_PKT_LEN = obj.getMaxPayload() + 100;
            import java.io.*
            import java.net.DatagramSocket
            import java.net.DatagramPacket
            import java.net.InetAddress            
            pkt_recv = DatagramPacket(zeros(1,MAX_PKT_LEN,'int8'), MAX_PKT_LEN);
            
            done = false;
            resp = [];
            while ~done
                try
                    obj.sock.receive(pkt_recv);
                    recv_len = pkt_recv.getLength;
                catch receiveError
                    if ~isempty(strfind(receiveError.message,'java.net.SocketTimeoutException'))
                        %Timeout receiving; do nothing
                        recv_len = 0;
                    else
                        fprintf('%s.m--Failed to receive UDP packet.\nJava error message follows:\n%s',mfilename,receiveError.message);
                    end
                end
                
                
                recv_data8 = pkt_recv.getData;
                
                if(recv_len > 0)
                    if(recv_len > 1000)
                        %keyboard
                    end
                    
                    reply8 = [recv_data8(3:recv_len); zeros(mod(-(recv_len-2),4),1,'int8')].';
                    reply = swapbytes(typecast(reply8, 'uint32'));
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



        function rx_bytes = receive_buffer( obj, cmd, id, flags, start_address, size )
            % Function will wait to receive 'size' bytes and return them in the 'bytes' array
            
            % Initialize loop variables
            rx_bytes         = uint8( zeros( size, 1 ) );     % Pre-allocate an array to hold retrieved bytes
            rx_bytes_valid   = logical( false( size, 1 ) );   % Allocate an array to track which bytes have been received from the node
            curr_buff        = wn_buffer();                % Create a wn_buffer object to deserialize the received samples packets
            start_time       = tic;                        % Start time of function for timeout
            numloops         = 0;                          % Detect if we lost packets

            % TODO:  Support Jumbo Frames
            maxPayload_uint8 = 1280;                       % Should match value in 'wlan_exp_node.c'

            % fprintf('receive_buffer args:   start = %d     size = %d \n', start_address, size );
            
            while ~all( rx_bytes_valid(:) )
            
                % We have been through this loop 'numloops' times
                numloops = numloops + 1;

                % Each iteration of this loop retrieves the first contiguous block of samples
                % that has not yet been received from the node
                
                % Find the index of the first byte not yet received (0-indexed)
                missing_bytes_hw_index       = start_address + find( ~rx_bytes_valid(:) ) - 1;
                first_missing_bytes_hw_index = missing_bytes_hw_index(1);
                
                % Find how many samples immediately after the first are not yet received
                bpoint = find( diff( missing_bytes_hw_index ) > 1 );

                if( isempty( bpoint ) )
                    % No samples after the starting sample have been received
                    bpoint = length( missing_bytes_hw_index );
                else
                    % Some later samples are already received; don't retrieve them again
                    bpoint = bpoint(1);
                end
                
                % Calculate how many samples to request in this batch
                num_bytes_this_req = missing_bytes_hw_index(bpoint) - first_missing_bytes_hw_index + 1;
                
                USEFULBUFFERSIZE = .9; %Assume 1-USEFULBUFFERSIZE is used for overhead;

                if( num_bytes_this_req > ( USEFULBUFFERSIZE * obj.rxBufferSize ) )
                    % If the number of bytes we need from the board exceeds the receive buffer size of our transport, we are going to drop
                    % packets. We should reduce our request to minimize dropping.
                    num_bytes_this_req = floor( ( USEFULBUFFERSIZE * obj.rxBufferSize ) / 4 ) * 4;
                end

                % Calculate how many transport packets are required
                num_pkts_required = ceil( double( num_bytes_this_req ) / double( maxPayload_uint8 ) );

                % fprintf('receive_buffer cmd args:   start = %d     size = %d \n', first_missing_bytes_hw_index, num_bytes_this_req );

                % Construct and send the argument to the node
                cmd.setArgs( id, flags, first_missing_bytes_hw_index, num_bytes_this_req );
                obj.send(cmd.serialize(), 'noresp');
                
                % Wait for the node to send all requested samples
                %   - Each packet received resets the timout timer
                %   - If a timout occurs (i.e. when a node-to-host packet is lost), the indexes of the lost samples
                %       are marked as invalid and will be re-requested on the next iteration of the outer loop
                
                num_resp_pkts = 0;
                curr_time     = tic;
                
                while( ( num_resp_pkts < num_pkts_required ) && ( toc( curr_time ) < obj.timeout ) )

                    resp = obj.receiveResp();
                    
                    if( ~isempty( resp ) )
                    
                        first_byte_received_hw_index = -1;
                        num_bytes_received           = -1;
                        
                        % Extract the received bytes and reset the timout timer
                        %   resp might contain multiple response packets, as obj.receiveResp() empties the incoming UDP buffer and returns everything
                        
                        for respIndex = 1:length( resp )
                        
                            curr_buff.deserialize( resp(respIndex).getArgs );

                            if( ( respIndex > 1 ) && ( curr_buff.start_byte > ( first_byte_received_hw_index + num_bytes_received + 1 ) ) )
                                fprintf('read_baseband_buffers: samples packet(s) dropped by transport; increase UDP receive buffer\n');
                            end

                            first_byte_received_hw_index = (curr_buff.start_byte);    % Hardware index of first byte received (0-indexed)
                            num_bytes_received           = (curr_buff.num_bytes);
                            
                            % Calculate indexes for output arrays for these bytes (first element of outout array is first requested byte,
                            %   even if first requested byte is not byte 0 in hardware)
                            output_byte_indexes = ( first_byte_received_hw_index - start_address ) + ( 1:num_bytes_received );

                            % Fill in the output arrays - output arrays are (num_requested_samples x num_interfaces)
                            rx_bytes( output_byte_indexes )       = curr_buff.getBytes();
                            rx_bytes_valid( output_byte_indexes ) = true;
                            if( max( output_byte_indexes ) > size )
                                error(generatemsgid('ReadBuffer_IndexError'), 'receive_buffers attempted to write an invalid byte index');
                            end
                            
                            % fprintf('Got bytes: Start=%d, Num=%d, (offset=%d)\n', curr_buff.start_byte, curr_buff.num_bytes, output_byte_indexes(1) )
                        end
                        
                        num_resp_pkts = num_resp_pkts + length(resp);
                        curr_time    = tic;
                    end
                end

                % Fail-safe timeout, in case indexing is broken (in m or C), to keep read_baseband_buffers from running forever
                if( toc( start_time ) > 30 )
                    error(generatemsgid('ReadBuffer_Timeout'), 'receive_buffers took too long to retrieve bytes; check for indexing errors in C and M code');
                end
            end %end while ~(all samples received)
            
            if( numloops > 1 )
               warning(generatemsgid('DroppedFrames'),'Dropped frames on fast read... took %d iterations', numloops); 
            end
            
        end % End receive_buffer


        % TODO:  FIX - THIS CODE IS REPEATED FROM WN_NODE.M        
        function out = receiveResp(obj)
            % This method will return a vector of responses that are sitting in the host's receive queue. It will empty the queue
            % and return them all to the calling method.
            resp = obj.receive();

            if(~isempty(resp))
                % Create vector of response objects if received string of bytes is a concatenation of many responses
                done = false;
                index = 1;
                while ~done
                    out(index)  = wn_resp(resp);
                    currRespLen = length(out(index).serialize);
                    if( currRespLen < length(resp) )
                        resp = resp( (currRespLen+1):end );
                    else
                        done = true;
                    end
                    index = index+1;
                end
            else
                out = [];
            end
        end

        
    end
end %classdef
