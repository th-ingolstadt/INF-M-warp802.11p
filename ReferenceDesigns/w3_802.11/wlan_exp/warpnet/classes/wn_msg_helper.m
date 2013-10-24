classdef wn_msg_helper < handle_light
    properties (Abstract)
    end
    methods (Abstract)
        out = serialize(varargin);
        out = deserialize(varargin);
        out = sizeof(varargin);
    end 
end
