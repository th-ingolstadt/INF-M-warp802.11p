classdef wn_transport < handle_light
%Abstract class of for WARPLab transports
properties (Abstract = true)
    hdr;
end

methods (Abstract = true)
    reply = send
end

end