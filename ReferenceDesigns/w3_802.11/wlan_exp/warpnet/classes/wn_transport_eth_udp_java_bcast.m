%******************************************************************************
% WARPNet Java Ethernet UDP Broadcast Transport
%
% File   :	wn_transport_eth_udp_java_bcast.m
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

classdef wn_transport_eth_udp_java_bcast < wn_transport_eth_udp
% Java physical layer Ethernet UDP Transport for broadcast traffic
% User code should not use this object directly-- the parent wn_node will
% instantiate the appropriate transport object for the hardware in use

%******************************** Properties **********************************

    properties (SetAccess = protected, Hidden = true)
        isopen;
    end

%********************************* Methods ************************************

    methods
        function obj = wn_transport_eth_udp_java_bcast()
            obj = obj@wn_transport_eth_udp();
            
            % At the moment, a trigger is the only type of broadcast packet.
            % In a future release this type will be exposed to objects that
            % create the broadcast transport object.
            obj.hdr.pktType = obj.hdr.PKTTYPE_TRIGGER;

            configFile = which('wn_config.ini');
            if(isempty(configFile))
                error(generatemessageid('MissingConfig'),'cannot find wn_config.ini. please run wn_setup.m'); 
            end
            readKeys = {'network','','host_address',''};
            IP = inifile(configFile,'read',readKeys);
            IP = IP{1};
            IP = sscanf(IP,'%d.%d.%d.%d');

            readKeys = {'network','','host_ID',[]};
            hostID = inifile(configFile,'read',readKeys);
            hostID = hostID{1};
            hostID = sscanf(hostID,'%d');
            
            readKeys = {'network','','bcast_port',[]};
            bcastport = inifile(configFile,'read',readKeys);
            bcastport = bcastport{1};
            bcastport = sscanf(bcastport,'%d');
            
            
            obj.address    = sprintf('%d.%d.%d.%d',IP(1),IP(2),IP(3),255);
            obj.port       = bcastport;
            obj.bcast_port = bcastport;
            obj.hdr.srcID  = hostID;
            obj.hdr.destID = 65535;    % Changed from 255 in WARPNet 7.1.0            
            obj.maxPayload = 1000;     % Default value;  Can explictly set a different maxPayload if 
                                       % you are certain that all nodes support a larger packet size.
        end

        function checkSetup(obj)
            %TODO
            checkSetup@wn_transport_eth_udp(obj);
        end

        function setMaxPayload(obj,value)
            obj.maxPayload = value;
        end
        
        function out = getMaxPayload(obj)
            out = obj.maxPayload;
        end

        function open(obj,varargin)
            %varargin{1}: (optional) IP address
            %varargin{2}: (optional) port
            
            import java.io.*
            import java.net.DatagramSocket
            import java.net.DatagramPacket
            import java.net.InetAddress            

            if(isempty(obj.isopen))
                if(nargin==3)
                   if(ischar(varargin{1}))
                      obj.address = varargin{1}; 
                   else
                      obj.address = obj.int2IP(varargin{1});
                   end
                   obj.port = varargin{2};
                end
                
                obj.sock = DatagramSocket();
                obj.sock.setSoTimeout(2000);
                obj.sock.setReuseAddress(1);
                obj.sock.setSendBufferSize(2^20);
                REQUESTED_BUF_SIZE = 2^22;
                obj.sock.setReceiveBufferSize(REQUESTED_BUF_SIZE); 
                x = obj.sock.getReceiveBufferSize();
                obj.rxBufferSize = x;
                if(x < REQUESTED_BUF_SIZE)
                    fprintf('OS reduced recv buffer size to %d\n', x);
                end
                obj.sock.setBroadcast(true);

                obj.status = 1;
                obj.isopen = 1;              
            end
        end

        function close(obj)
            try
                obj.sock.close();
            catch closeError
                warning(generatemsgid('CloseErr'), 'Error closing socket; java error was %s', closeError.message)
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
        function send(obj,type,varargin)
            %varargin{1}: data    

            import java.io.*
            import java.net.DatagramSocket
            import java.net.DatagramPacket
            import java.net.InetAddress            

            switch(lower(type))
                case 'trigger'
                    bitset(obj.hdr.flags,1,0); %no response
                    obj.hdr.pktType = obj.hdr.PKTTYPE_TRIGGER;
                case 'message'
                    obj.hdr.pktType = obj.hdr.PKTTYPE_HTON_MSG;
            end
            data = uint32(varargin{1});
            obj.hdr.msgLength = (length(data))*4; %Length in bytes
            obj.hdr.flags = bitset(obj.hdr.flags,1,0);
            obj.hdr.increment;
            data = [obj.hdr.serialize,data];
    
            data8 = [zeros(1,2,'uint8') typecast(swapbytes(uint32(data)),'uint8')];
            jaddr = InetAddress.getByName(obj.address);
            pkt_send = DatagramPacket(data8, length(data8), jaddr, obj.port);
            obj.sock.send(pkt_send);
        end
        
    end
end %classdef
