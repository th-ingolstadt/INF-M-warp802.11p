% WARPNet MEX UDP Transport
%
%     This function is a custom MEX UDP transport layer for WARPNet communication.
%     It can be called with different options / commands to perform different 
%     functions.
%
% -----------------------------------------------------------------------------
% Authors:	Chris Hunter (chunter [at] mangocomm.com)
%			Patrick Murphy (murphpo [at] mangocomm.com)
%           Erik Welsh (welsh [at] mangocomm.com)
% License:	Copyright 2013, Mango Communications. All rights reserved.
%			Distributed under the WARP license  (http://warpproject.org/license)
% -----------------------------------------------------------------------------
% Command Syntax
%
% Standard WARPNet transport functions: 
%     1.                  wn_mex_udp_transport('version') 
%     2. index          = wn_mex_udp_transport('init_socket') 
%     3.                  wn_mex_udp_transport('set_so_timeout', index, timeout) 
%     4.                  wn_mex_udp_transport('set_send_buf_size', index, size) 
%     5. size           = wn_mex_udp_transport('get_send_buf_size', index) 
%     6.                  wn_mex_udp_transport('set_rcvd_buf_size', index, size) 
%     7. size           = wn_mex_udp_transport('get_rcvd_buf_size', index) 
%     8.                  wn_mex_udp_transport('close', index) 
%     9. size           = wn_mex_udp_transport('send', index, buffer, length, ip_addr, port) 
%    10. [size, buffer] = wn_mex_udp_transport('receive', index, length ) 
% 
% Additional WARPNet MEX UDP transport functions: 
%     1. [num_samples, cmds_used, samples]  = wn_mex_udp_transport('read_rssi' / 'read_iq', 
%                                                 index, buffer, length, ip_addr, port, 
%                                                 number_samples, buffer_id, start_sample) 
%     2. cmds_used                          = wn_mex_udp_transport('write_iq', 
%                                                 index, cmd_buffer, max_length, ip_addr, port, 
%                                                 number_samples, sample_buffer, buffer_id, 
%                                                 start_sample, num_pkts, max_samples, hw_ver) 
% 
% Please refer to comments within wn_mex_udp_transport.c for more information.
% 
% -----------------------------------------------------------------------------
% Compiling MEX
% 
% Please see the on-line documentation for how to compile WARPNet MEX functions at:
%    http://warpproject.org/trac/wiki/WARPNet/MEX
%
% The compile command within MATLAB is:
%
%    Windows:
%        mex -g -O wn_mex_udp_transport.c -lwsock32 -lKernel32 -DWIN32
%
%    MAC / Unix:
%        mex -g -O wn_mex_udp_transport.c
%

if ( ispc ) 

    % Allow both manual and automatic compilation on Windows
    %
    fprintf('WARPNet wn_mex_udp_transport Setup\n');
    fprintf('    Would you like [1] Manual (default) or [2] Automatic compliation?\n');
    temp = input('    Selection: ');

    if(isempty(temp))
        temp = 1;
    end

    if( ( temp > 2 ) || ( temp < 1 ) )
        temp = 1;
        fprintf('Invalid selection.  Setting to Manual.\n\n'); 
    end

else
    
    % Only allow manual compilation on MAC / UNIX
    %
    temp = 1; 
end

switch( temp ) 
    case 1

        fprintf('\nManual Compliation Steps:  \n');
        fprintf('    1) Please familiarize yourself with the process at: \n');
        fprintf('         http://warpproject.org/trac/wiki/WARPNet/MEX \n');
        fprintf('       to make sure your environment is set up properly. \n');
        fprintf('    2) Set up mex to choose your compiler.  (mex -setup) \n');
        
        if ( ispc )
            fprintf('         1. Select "Yes" to have mex locate installed compilers \n');
            fprintf('         2. Select "Microsoft Visual C++ 2010 Express" \n');
            fprintf('         3. Select "Yes" to verify your selections \n');
        else
            fprintf('         1. Select "/Applications/MATLAB_R2013a.app/bin/mexopts.sh : \n');
            fprintf('                    Template Options file for building MEX-files" \n');
            fprintf('         2. If necessary, overwrite:  /Users/<username>/.matlab/R2013a/mexopts.sh \n');
        end
        
        fprintf('    3) Change directory to M_Code_Reference/mex (cd mex) \n');
        fprintf('    4) Run the compile command: \n');
        
        if ( ispc )
            fprintf('         mex -g -O wn_mex_udp_transport.c -lwsock32 -lKernel32 -DWIN32 \n');
        else
            fprintf('         mex -g -O wn_mex_udp_transport.c \n');
        end
        
        fprintf('    5) Re-run wn_setup and make sure that "WARPNet Mex UDP" is an available transport. \n');
        fprintf('\n');
        
    case 2
        fprintf('\nRunning Automatic Setup procedure: ...');

        % Procedure to compile mex
        cc = mex.getCompilerConfigurations('c');

        if( strcmp(cc.Name,'Microsoft Visual C++ 2010 Express') == 1 ) 
        
            % Since we are not guarenteed to be in the .../M_Code_Reference directory, 
            % save the current path and then cd using absolute paths
            %
            curr_path       = pwd;
            [wn_path,ig,ig] = fileparts(which('wn_mex_udp_transport'));
        
            cd(sprintf('%s',wn_path));
            
            mex -g -O wn_mex_udp_transport.c -lwsock32 -lKernel32 -DWIN32
            
            cd(sprintf('%s',curr_path));
            
            clear 
            fprintf('Done. \n\n');
            fprintf('***************************************************************\n');
            fprintf('***************************************************************\n');
            fprintf('Please re-run wn_setup to use your "WARPNet Mex UDP" transport.\n');
            fprintf('***************************************************************\n');
            fprintf('***************************************************************\n');
        else
            fprintf('\nMicrosoft Visual C++ 2010 Express compiler not found.  Running mex setup: \n');
            mex -setup
            cc = mex.getCompilerConfigurations('c');
            
            if( strcmp(cc.Name,'Microsoft Visual C++ 2010 Express') == 1 ) 
                fprintf('Please re-run wn_mex_udp_transport to compile the transport.\n');
            else
                error('Please install "Microsoft Visual C++ 2010 Express" and re-run wn_mex_udp_transport.  \nSee http://warpproject.org/trac/wiki/WARPNet/MEX for more information');

            end    
        end
    
    otherwise
        error('Invalid selection.');
end
