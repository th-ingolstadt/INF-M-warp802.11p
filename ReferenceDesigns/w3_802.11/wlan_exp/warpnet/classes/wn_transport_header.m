classdef wn_transport_header < wn_msg_helper
    % contains data needed by the transport to ensure reliable transmission
    properties (SetAccess = protected)
        seqNum = uint16(0); %Sequence number for outgoing transmissions
        PKTTYPE_TRIGGER = uint8(0); 
        PKTTYPE_HTON_MSG = uint8(1); 
        PKTTYPE_NTOH_MSG = uint8(2); 
    end
    properties (Hidden = true)
        reserved = uint8(0);
    end
    properties (SetAccess = public)
        flags = uint16(0);
        msgLength = uint16(0);
        destID = uint16(0); %Destination ID for outgoing transmissions
        srcID = uint16(0); %Source ID for outgoing transmissions
        pktType = uint8(0);
    end
    methods
        function obj = wn_transport_header()
            % wn_transport_header Sets the default data type of the header to be 'uint32' and
            % initializes sequence number to 1.
            obj.seqNum = 0;
        end
        
        function len = length(obj)
            %length Returns header length in # of uint32s.
            %len = length(obj.serialize); 
            persistent transport_header_length;
            if(isempty(transport_header_length))
                transport_header_length = length(obj.serialize);
            end
            len = transport_header_length;
        end
        
        function out = sizeof(obj)
            out = length(obj)*4;
        end
        
        function reset(obj)
            %reset Resets the sequence number back to 1.
            obj.seqNum = 1;
        end

        function increment(obj,varargin)
            %increment Increments the sequence number.
            if( nargin == 1 )
                % obj.seqNum = mod( (obj.seqNum + 1), uint16(realmax) );   % This is much more expensive computationally.

                if(obj.seqNum ~= uint16(realmax))
                    obj.seqNum = obj.seqNum+1;    
                else
                    obj.seqNum = 0;    
                end

            else
                obj.seqNum = mod( (obj.seqNum + varargin{1}), uint16(realmax) );
            end
        end

        function output = deserialize(obj)
            %Un-used for transport_headers
        end

        function output = serialize(obj)
            %serialize Serializes the header to a vector
            %This function serializes the header to a vector 
            %for placing into outgoing packets by
            %wn_transport's children.
            %output = [obj.destID,obj.srcID,obj.seqNum,obj.flags];
%             typedef struct {
%                 u16 destID;
%                 u16 srcID;
%                 u8 reserved;
%                 u8 pktType;
%                 u16 length;
%                 u16 seqNum;
%                 u16 flags;
%             } wn_transport_header;
            %NOTES: calling cast(x,'uint32') is much slower than uint32(2)
            % bitshift/bitor are slower than explicit mults/adds
            w0 = 2^16*uint32(obj.destID) + uint32(obj.srcID);
            w1 = 2^24*uint32(obj.reserved) + 2^16*uint32(obj.pktType) + uint32(obj.msgLength);
            w2 = 2^16*uint32(obj.seqNum) + uint32(obj.flags);
            output = [w0 w1 w2];
        end
        
        function match = isReply(obj,input)
            %isReply Checks an input vector to see if it is a valid reply
            %to the last outgoing packet.
            %This function checks to make sure that the received
            %source address matches the previously sent destination
            %addresses and makes sure that the sequence numbers match.
         
            if(length(input)==length(obj.serialize))
                
                array16 = reshape(flipud(reshape(typecast(input(:)','uint16'),2,obj.length)),1,2*obj.length);
                replyDest = array16(1)==obj.srcID;
                replySrc = array16(2)==obj.destID;
                replySeq = array16(5)==obj.seqNum;
                match = min([replyDest,replySrc,replySeq]);
                if(match==0)
                    warning('wn_transport_header:setType', 'transport_header mismatch: [%d %d] [%d %d] [%d %d]\n',...
                        array16(1), obj.srcID, array16(2), obj.destID, array16(5), obj.seqNum);
                end
            else
                error('wn_transport_header:setType',sprintf('length of header did not match internal length of %d',obj.type))
            end
        end

        
    end %methods
end %classdef
