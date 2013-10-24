%******************************************************************************
% WLAN Exp Setup
%
% File   :	wlan_exp_setup.m
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
%
%******************************************************************************

function wlan_exp_setup

fprintf('Setting up WLAN Exp Paths...\n');

%------------------------------------------------------------------------------
% Remove any paths from previous WARPNet installations

% NOTE:  We had to add in code involving regular expressions to catch a absolute vs 
%     relative path issue that we found.  For example, if ./util exists in the path, 
%     then the previous version of the code will run forever because it will find
%     the file using 'which' but then the absolute path returned will not be in the
%     actual path.  Now, we use regular expressions to make sure the path exists 
%     before removing it.


% Check on  .../M_Code_Reference/classes
done = false;
while(~done)
    [wl_path,ig,ig] = fileparts(which('wlan_exp_node'));
    if(~isempty(wl_path))
        fprintf('   removing path ''%s''\n',wl_path);

        my_path     = regexprep( sprintf('%s', path), ';', ' ' );
        search_term = regexprep( sprintf('%s ',wl_path), '\\', '\\\\' );
        path_comp   = regexp( sprintf('%s',my_path), sprintf('%s',search_term), 'match' );
                
        if ( ~isempty( path_comp ) )        
            rmpath(wl_path)
        else
            done = true;
        end
    else
        done = true;
    end
end

% Check on  .../M_Code_Reference/util
%   - Save the util directory path as a check for the inifile
util_path = '';
done = false;
while(~done)
    [wl_path,ig,ig] = fileparts(which('wlan_exp_ver'));
    if(~isempty(wl_path))
        fprintf('   removing path ''%s''\n',wl_path);

        my_path     = regexprep( sprintf('%s', path), ';', ' ' );
        search_term = regexprep( sprintf('%s ',wl_path), '\\', '\\\\' );
        path_comp   = regexp( sprintf('%s',my_path), sprintf('%s',search_term), 'match' );
        
        if ( ~isempty( path_comp ) )        
            rmpath(wl_path)
            util_path = wl_path;
        else
            done = true;
        end
    else
        done = true;
    end
end



%------------------------------------------------------------------------------
% Add new WLAN Exp paths 

% Add  .../M_Code_Reference/classes
myPath = sprintf('%s%sclasses',pwd,filesep);
fprintf('   adding path ''%s''\n',myPath);
addpath(myPath)

% Add  .../M_Code_Reference/util
myPath = sprintf('%s%sutil',pwd,filesep);
fprintf('   adding path ''%s''\n',myPath);
addpath(myPath)


% Deprecated paths in WARPLab 7.4.0
%     .../M_Code_Reference/util/inifile
%     .../M_Code_Reference/mex
%     .../M_Code_Reference/config


% Example of adding a user path to WARPLab setup:
%   In this example, C:\WARP_Repository\ResearchApps\PHY\WARPLAB\WARPLab7\M_Code_Reference\mex is added to the MATLAB path
%
% myPath = sprintf('C:%sWARP_Repository%sResearchApps%sPHY%sWARPLAB%sWARPLab7%sM_Code_Reference%smex',filesep,filesep,filesep,filesep,filesep,filesep,filesep);
% fprintf('   adding path ''%s''\n',myPath);
% addpath(myPath) 

fprintf('   saving path\n');
savepath



%------------------------------------------------------------------------------
% Check on WARPNet installation
%   - As of WARPLab 7.4.0 and later, WARPLab depends on the WARPNet framework.  This 
%     setup will check for existing installations of WARPNet and allow the user to 
%     select if you want to use the existing WARPNet framework or the WARPNet framework 
%     bundled with this WARPLab release.
%     


% Get current WARPNet paths
[wn_ver_path,ig,ig] = fileparts(which('wn_ver'));

% Get WARPNet paths from install
new_wn_path         = sprintf('%s%swarpnet',pwd,filesep);
new_wn_ver_path     = sprintf('%s%swarpnet%sutil',pwd,filesep,filesep);

% Do we need to setup WARPNet?
setup_warpnet  = 0;

if(~isempty(wn_ver_path))
    % Get the version of the existing WARPNet install
    [old_wn_ver_major, old_wn_ver_minor, old_wn_ver_rev] = wn_ver();
    old_wn_ver = sprintf( '%d.%d.%d', old_wn_ver_major, old_wn_ver_minor, old_wn_ver_rev);

    % Get the version of the WARPNet release
    %   - Unfortunately, in order to run the newer wn_ver(), we cannot just run the command.
    %     From the MATLab forums:  
    %       "RUN specifically says it works for scripts. It doesn't work for functions."
    %       "You have three options:
    %         1) CD to the directory containing the function and run it.
    %         2) Add the directory containing the function to the path and run it.
    %         3) Do one of 1) or 2), but instead of running the function create a function 
    %            handle to the function. As long as you have that function handle around, 
    %            you will be able to call the function regardless of whether the function is 
    %            in the current directory or not.
    % 
    currPath        = pwd;        
    cd(new_wn_ver_path);
    [new_wn_ver_major, new_wn_ver_minor, new_wn_ver_rev] = wn_ver();
    new_wn_ver = sprintf( '%d.%d.%d', new_wn_ver_major, new_wn_ver_minor, new_wn_ver_rev);
    cd(currPath);
    
    % Get the base path of the existing WARPNet installation
    wn_path         = regexprep( sprintf('%s', wn_ver_path), '\\util', '' );
    
    % Check if we are going to use the same WARPNet framework we are already using
    if ( ~strcmp(wn_path,new_wn_path) || ~strcmp(old_wn_ver,new_wn_ver) ) 
    
        fprintf('\n------------------------------------------------------------\n')
        fprintf('Please select the WARPNet installation.\n\n')
        fprintf('Pressing enter without typing an input will use existing WARPNet installation:\n')
        fprintf('  [1] WARPNet version %s in %s \n', old_wn_ver, wn_path);
        fprintf('  [2] WARPNet version %s in %s \n', new_wn_ver, new_wn_path);

        temp = input('WARPNet selection: ','s');
        if(isempty(temp))
            temp = '1'; 
        else
            if ( ( temp ~= '1' ) && ( temp ~= '2' ) ) 
                fprintf('    Invalid selection defaulting to [1] \n');
                temp = '1';
            end
        end
    else
        temp = '1';
    end

    switch ( temp ) 
        case '1'

            % TODO: print defaults
            % Find Config File from WARPNet installation
            [config_path,ig,ig] = fileparts(which('wn_config.ini'));
            config_file = sprintf('%s%swn_config.ini',config_path,filesep);

            if(~exist(config_file, 'file'))
                fprintf('    Issue with existing WARPNet config file.  Setting up WARPNet from current release. \n');
                setup_warpnet = 1;

            else 

                fprintf('\nUsing WARPNet version %s in %s \n', old_wn_ver, wn_path);
                
                wn_setup_path = [wn_path, filesep, 'wn_setup'];

                fprintf('To change these settings run: \n    %s\n', wn_setup_path);

                IP           = '';
                host         = '';
                port_unicast = '';
                port_bcast   = '';
                transport    = '';
                jumbo        = '';

                readKeys = {'network','','host_address',''};
                IP = inifile(config_file,'read',readKeys);
                IP = IP{1};

                readKeys = {'network','','host_ID',''};
                host = inifile(config_file,'read',readKeys);
                host = host{1};

                readKeys = {'network','','unicast_starting_port',''};
                port_unicast = inifile(config_file,'read',readKeys);
                port_unicast = port_unicast{1}; 

                readKeys = {'network','','bcast_port',''};
                port_bcast = inifile(config_file,'read',readKeys);
                port_bcast = port_bcast{1}; 

                readKeys = {'network','','transport',''};
                transport = inifile(config_file,'read',readKeys);
                transport = transport{1}; 

                readKeys = {'network','','jumbo',''};
                jumbo = inifile(config_file,'read',readKeys);
                jumbo = jumbo{1}; 

                fprintf('WARPNet settings:\n');
                fprintf('  Host IP Address:  %s\n', IP);
                fprintf('  Host ID        :  %s\n', host);
                fprintf('  Unicast Port   :  %s\n', port_unicast);
                fprintf('  Broadcast Port :  %s\n', port_bcast);
                fprintf('  Transport Type :  %s\n', transport);
                fprintf('  Jumbo Frames   :  %s\n', jumbo);

            end

        case '2'
            setup_warpnet = 1;

        otherwise
            error(generatemsgid('InvalidSelection'),'Issue setting up WARPNet path.  Please check selection.');                
    end
else
    fprintf('\n    !!!! WARPLab 7.4.0 and later, is built on WARPNet.  No WARPNet setup found in path. !!!!\n');
    setup_warpnet = 1;
end


% Call WARPNet setup if necessary
if ( setup_warpnet == 1 )

    % TODO: CALL WARPNET
    fprintf('\nCalling WARPNet Setup ... \n\n');
        
    wl_setup_func = [new_wn_path filesep 'wn_setup'];
    
    run( wl_setup_func );
    
    fprintf('Resuming WARPLab Setup ... \n');
end


fprintf('\n')
fprintf('------------------------------------------------------------\n')
fprintf('WLAN Exp Setup Complete\nwlan_exp_ver():\n');
wlan_exp_ver()
fprintf('------------------------------------------------------------\n\n')
end