%******************************************************************************
% WARPNet Version
%
% File   :	wn_ver.m
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
% 2.00a ejw  9/05/13  Initial release
%
%******************************************************************************

function varargout = wn_ver(varargin)
    
    MAJOR       = 2;
    MINOR       = 0;
    REVISION    = 0;
    XTRA        = '';
    RELEASE     = 1;
    
    if ( RELEASE == 0 ) 
        fprintf('*********************************************************************\n');
        fprintf('You are running a version of WARPNet that may not be compatible\n');
        fprintf('with released WARPNet bitstreams. Users should run wn_setup.m from\n');
        fprintf('the zip file provided in the release download.\n');
        fprintf('*********************************************************************\n\n');
    end

    outputs = {MAJOR,MINOR,REVISION,XTRA};

    % Process the inputs:
    %   0 => Print standard WARPNet output
    %   1 => Takes a argument string: 'x.y.z'; 
    %          Issues a warning if current framework is newer. 
    %          Issues an error if current framework is older.
    %        Take wn_node objects as argument
    %          Issues an error if MAJOR and MINOR do not match
    %
    if nargin == 0
        % Print standard WARPNet output
        if(nargout == 0)
            fprintf('WARPNet v%d.%d.%d %s\n\n',MAJOR,MINOR,REVISION,XTRA);
            fprintf('Framework Location:\n')
            [wn_path,ig,ig] = fileparts(which('wn_ver'));
            I = strfind(wn_path,sprintf('%sutil',filesep));
            wn_path = wn_path(1:I);
            fprintf('%s\n',wn_path);        
        else
            varargout(1:nargout) = outputs(1:nargout);
        end
        
    elseif nargin == 1
        switch( class(varargin{1}) ) 
            case 'wn_node'
                nodes    = varargin{1};
                numNodes = length(nodes);
                
                for n = numNodes:-1:1
                    currNode = nodes(n);

                    % Error if MAJOR and MINOR versions do not match
                    if ( ~((currNode.wn_ver_major == MAJOR) && (currNode.wn_ver_minor == MINOR)) )
                        error(generatemsgid('VersionMismatch'),'Node %d with Serial # %d has version "%d.%d.%d" which does not match WARPNet v%d.%d.%d', currNode.ID, currNode.serialNumber, currNode.wnVer_major, currNode.wnVer_minor, currNode.wnVer_revision, MAJOR, MINOR, REVISION);
                    end

                    % Print a warning if MAJOR, MINOR, and REVISION do not match                    
                    if ( ~((currNode.wn_ver_major == MAJOR) && (currNode.wn_ver_minor == MINOR) && (currNode.wn_ver_revision == REVISION) ) )
                        warning(generatemsgid('VersionMismatch'),'Node %d with Serial # %d has version "%d.%d.%d" which does not match WARPNet v%d.%d.%d', currNode.ID, currNode.serialNumber, currNode.wnVer_major, currNode.wnVer_minor, currNode.wnVer_revision, MAJOR, MINOR, REVISION);
                    end
                end
                
            case 'char'
                version         = varargin{1}; 
                
                if ( ~(strcmp(version,sprintf('%d.%d.%d', MAJOR,MINOR,REVISION))))

                    [VER, count] = sscanf(version, '%d.%d.%d');
                    
                    if ( ~(count == 3) ) 
                        error(generatemsgid('UnknownArg'),'Unknown argument.  Argument provided is: "%s", need "x.y.z" where x, y, and z are integers specifying the WARPNet version.', version);
                    end
                    
                    if ( VER(1) == MAJOR ) 
                        if ( VER(2) == MINOR ) 
                            if ( VER(3) == REVISION ) 
                                % Versions are equal do nothing
                            else
                                % Versions are not equal; Since MAJOR and MINOR versions match, only print a warning
                                if ( lt(VER(3),REVISION) ) 
                                    warning(generatemsgid('VersionMismatch'),'Specified version "%s" is older than WARPNet v%d.%d.%d', version, MAJOR, MINOR, REVISION);
                                else
                                    warning(generatemsgid('VersionMismatch'),'Specified version "%s" is newer than WARPNet v%d.%d.%d', version, MAJOR, MINOR, REVISION);
                                end                            
                            end
                        else
                            % Versions are not equal
                            if ( lt(VER(2),MINOR) ) 
                                warning(generatemsgid('VersionMismatch'),'Specified version "%s" is older than WARPNet v%d.%d.%d', version, MAJOR, MINOR, REVISION);
                            else
                                error(generatemsgid('VersionMismatch'),'Specified version "%s" is newer than WARPNet v%d.%d.%d', version, MAJOR, MINOR, REVISION);
                            end
                        end                        
                    else 
                        % Versions are not equal
                        if ( lt(VER(1),MAJOR) ) 
                            warning(generatemsgid('VersionMismatch'),'Specified version "%s" is older than WARPNet v%d.%d.%d', version, MAJOR, MINOR, REVISION);
                        else
                            error(generatemsgid('VersionMismatch'),'Specified version "%s" is newer than WARPNet v%d.%d.%d', version, MAJOR, MINOR, REVISION);                            
                        end
                    end
                end
                
            otherwise
                error(generatemsgid('UnknownArg'),'Unknown argument.  Argument is of type "%s", need "wn_node" or "char"', class(varargin{1}));
        end        
    else
        error(generatemsgid('TooManyArguments'),'Too many arguments provided');
    end
    
    
    
    
    
end