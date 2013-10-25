%******************************************************************************
% WARPNet Setup
%
% File   :	wn_setup.m
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

function wn_setup

fprintf('Setting up WARPNet Paths...\n');

%------------------------------------------------------------------------------
% Remove any paths from previous WARPNet installations

% NOTE:  We had to add in code involving regular expressions to catch a absolute vs 
%     relative path issue that we found.  For example, if ./util exists in the path, 
%     then the previous version of the code will run forever because it will find
%     the file using 'which' but then the absolute path returned will not be in the
%     actual path.  Now, we use regular expressions to make sure the path exists 
%     before removing it.

% Check on  .../classes
done = false;
while(~done)
    [wn_path,ig,ig] = fileparts(which('wn_node'));        
    if(~isempty(wn_path))
        fprintf('   removing path ''%s''\n',wn_path);

        my_path     = regexprep( sprintf('%s', path), ';', ' ' );
        search_term = regexprep( sprintf('%s ',wn_path), '\\', '\\\\' );
        path_comp   = regexp( sprintf('%s',my_path), sprintf('%s',search_term), 'match' );

        if ( ~isempty( path_comp ) )        
            rmpath(wn_path)
        else
            done = true;
        end
    else
        done = true;
    end
end

% Check on  .../config
done = false;
while(~done)
    [wn_path,ig,ig] = fileparts(which('wn_config.ini'));
    if(~isempty(wn_path))
        fprintf('   removing path ''%s''\n',wn_path);

        my_path     = regexprep( sprintf('%s', path), ';', ' ' );
        search_term = regexprep( sprintf('%s ',wn_path), '\\', '\\\\' );
        path_comp   = regexp( sprintf('%s',my_path), sprintf('%s',search_term), 'match' );

        if ( ~isempty( path_comp ) )        
            rmpath(wn_path)
        else
            done = true;
        end
    else
        done = true;
    end
end

% Check on  .../mex
done = false;
while(~done)
    [wn_path,ig,ig] = fileparts(which('wn_mex_udp_transport'));
    if(~isempty(wn_path))
        fprintf('   removing path ''%s''\n',wn_path);

        my_path     = regexprep( sprintf('%s', path), ';', ' ' );
        search_term = regexprep( sprintf('%s ',wn_path), '\\', '\\\\' );
        path_comp   = regexp( sprintf('%s',my_path), sprintf('%s',search_term), 'match' );

        if ( ~isempty( path_comp ) )        
            rmpath(wn_path)
        else
            done = true;
        end
    else
        done = true;
    end
end

% Check on  .../util
util_path = '';
done = false;
while(~done)
    [wn_path,ig,ig] = fileparts(which('wn_ver'));
    if(~isempty(wn_path))
        fprintf('   removing path ''%s''\n',wn_path);

        my_path     = regexprep( sprintf('%s', path), ';', ' ' );
        search_term = regexprep( sprintf('%s ',wn_path), '\\', '\\\\' );
        path_comp   = regexp( sprintf('%s',my_path), sprintf('%s',search_term), 'match' );

        if ( ~isempty( path_comp ) )        
            rmpath(wn_path)
            util_path = wn_path;
        else
            done = true;
        end
    else
        done = true;
    end
end

% Check on  .../util/inifile
done = false;
while(~done)
    [wn_path,ig,ig] = fileparts(which('inifile'));
    if(~isempty(wn_path))

        my_path     = regexprep( sprintf('%s', path), ';', ' ' );
        search_term = regexprep( sprintf('%s ',wn_path), '\\', '\\\\' );
        path_comp   = regexp( sprintf('%s',my_path), sprintf('%s',search_term), 'match' );

        if ( ~isempty( path_comp ) )
            
            % In order to not remove the wrong inifile path, make sure it is under the util 
            % directory
            
            search_path = sprintf('%s', path_comp{1});
            search_util = regexprep( sprintf('%s',util_path), '\\', '\\\\' );
            util_comp   = regexp( search_path, search_util, 'match' );
            
            if ( ~isempty( util_comp ) )
                fprintf('   removing path ''%s''\n',wn_path);
                rmpath(wn_path)
            else
                done = true;
            end
        else
            done = true;
        end
    else
        done = true;
    end
end



%------------------------------------------------------------------------------
% Add new WARPNet paths 

% Add  .../M_Code_Reference/warpnet/classes
myPath = sprintf('%s%sclasses',pwd,filesep);
fprintf('   adding path ''%s''\n',myPath);
addpath(myPath)

% Add  .../M_Code_Reference/warpnet/config
config_path = sprintf('%s%sconfig',pwd,filesep);
fprintf('   adding path ''%s''\n',config_path);
addpath(config_path)

% Add  .../M_Code_Reference/warpnet/mex
myPath = sprintf('%s%smex',pwd,filesep);
fprintf('   adding path ''%s''\n',myPath);
addpath(myPath)

% Add  .../M_Code_Reference/warpnet/util
myPath = sprintf('%s%sutil',pwd,filesep);
fprintf('   adding path ''%s''\n',myPath);
addpath(myPath)

% Add  .../M_Code_Reference/warpnet/util/inifile
myPath = sprintf('%s%sutil%sinifile',pwd,filesep,filesep);
fprintf('   adding path ''%s''\n',myPath);
addpath(myPath)

% Save path
fprintf('   saving path\n\n\n');
savepath


%------------------------------------------------------------------------------
% Find Config File
%   - Config file is now part of the WARPNet installation
%   - Use the 'configPath' variable from above since wn_config.ini may not exist

configFile = sprintf('%s%swn_config.ini', config_path, filesep);


%------------------------------------------------------------------------------
% Read config file

IP           = '';
host         = '';
port_unicast = '';
port_bcast   = '';
transport    = '';
jumbo        = '';

if(exist(configFile, 'file')) %Read existing config file for defaults
    fprintf('A wn_config.ini file was found in your path. Values specified in this\n');
    fprintf('configuration file will be used as defaults in the construction of the\n');
    fprintf('new file.\n');
    readKeys = {'network','','host_address',''};
    IP = inifile(configFile,'read',readKeys);
    IP = IP{1};
    
    readKeys = {'network','','host_ID',''};
    host = inifile(configFile,'read',readKeys);
    host = host{1};
    
    readKeys = {'network','','unicast_starting_port',''};
    port_unicast = inifile(configFile,'read',readKeys);
    port_unicast = port_unicast{1}; 
    
    readKeys = {'network','','bcast_port',''};
    port_bcast = inifile(configFile,'read',readKeys);
    port_bcast = port_bcast{1}; 
    
    readKeys = {'network','','transport',''};
    transport = inifile(configFile,'read',readKeys);
    transport = transport{1}; 
    
    readKeys = {'network','','jumbo',''};
    jumbo = inifile(configFile,'read',readKeys);
    jumbo = jumbo{1}; 
end

%Sane Defaults
if(isempty(IP))
    IP = '10.0.0.250';
end    
if(isempty(host))
    host = '250';
end    
if(isempty(port_unicast))
    port_unicast = '9000';
end    
if(isempty(port_bcast))
    port_bcast = '10000';
end    
if(isempty(transport))
    transport = 'java';
end    
if(isempty(jumbo))
    jumbo = 'false';
end   

inifile(configFile,'new')

writeKeys = {'config_info','','date_created',date};
inifile(configFile,'write',writeKeys,'tabbed')

[MAJOR,MINOR,REVISION] = wn_ver();
writeKeys = {'config_info','','wn_ver',sprintf('%d.%d.%d',MAJOR,MINOR,REVISION)};
inifile(configFile,'write',writeKeys,'tabbed')

fprintf('\n\n')
fprintf('------------------------------------------------------------\n')
fprintf('Please enter a WARPNet Ethernet interface address.\n\n')
fprintf('Pressing enter without typing an input will use a default\n')
fprintf('IP address of: %s\n\n',IP);

ip_valid = 0;

while( ip_valid == 0 ) 

    temp = input('WARPNet Ethernet Interface Address: ','s');
    if( isempty(temp) )
       temp     = IP; 
       ip_valid = 1;
       fprintf('   defaulting to %s\n',temp);
    else
       if ( regexp( temp, '\d+\.\d+\.\d+\.\d+' ) == 1 ) 
           ip_valid = 1;
           fprintf('   setting to %s\n',temp)
       else
           fprintf('   %s is not a valid IP address.  Please enter a valid IP address.\n',temp); 
       end
    end
end

if(ispc)
   [status, tempret] = system('ipconfig /all');
elseif(ismac||isunix)
   [status, tempret] = system('ifconfig -a');
end
tempret = strfind(tempret,temp);

if(isempty(tempret))
   warning(generatemsgid('InvalidInterface'),'No interface found. Please ensure that your network interface is connected and configured with static IP %s',temp);
   pause(1)
end

writeKeys = {'network','','host_address',temp};
inifile(configFile,'write',writeKeys,'tabbed')

fprintf('\n\n')
fprintf('------------------------------------------------------------\n')
fprintf('Please enter a WARPNet host ID.\n')
fprintf('Valid host IDs are integers in the range of [200,254]\n\n')
fprintf('Pressing enter without typing an input will use a default\n')
fprintf('host ID of: %s\n\n',host);

temp = input('WARPNet Host ID: ','s');
if(isempty(temp))
   temp = host; 
   if(isempty(str2num(temp)))
        error(generatemsgid('InvalidHostID'),'Host ID must be an integer in the range of [0,254]');
   else
       if((str2num(temp)<200) || ((str2num(temp)>254)))
           error(generatemsgid('InvalidHostID'),'Host ID must be an integer in the range of [0,254]');
       end
   end
   fprintf('   defaulting to %s\n',temp);
else
   if(isempty(str2num(temp)))
        error(generatemsgid('InvalidHostID'),'Host ID must be an integer in the range of [0,254]');
   else
       if((str2num(temp)<200) || ((str2num(temp)>254)))
           error(generatemsgid('InvalidHostID'),'Host ID must be an integer in the range of [0,254]');
       end
   end
   
   fprintf('   setting to %s\n',temp); 
end

writeKeys = {'network','','host_ID',temp};
inifile(configFile,'write',writeKeys,'tabbed')

fprintf('\n\n')
fprintf('------------------------------------------------------------\n')
fprintf('Please enter a unicast starting port.\n\n')
fprintf('Pressing enter without typing an input will use a default\n')
fprintf('unicast starting port of: %s\n\n',port_unicast);

temp = input('Unicast Starting Port: ','s');
if(isempty(temp))
   temp = port_unicast; 
   fprintf('   defaulting to %s\n',temp);
else
   fprintf('   setting to %s\n',temp); 
end

writeKeys = {'network','','unicast_starting_port',temp};
inifile(configFile,'write',writeKeys,'tabbed')


fprintf('\n\n')
fprintf('------------------------------------------------------------\n')
fprintf('Please enter a broadcast port.\n\n')
fprintf('Pressing enter without typing an input will use a default\n')
fprintf('broadcast port of: %s\n\n',port_bcast);

temp = input('Broadcast Port: ','s');
if(isempty(temp))
   temp = port_bcast; 
   fprintf('   defaulting to %s\n',temp);
else
   fprintf('   setting to %s\n',temp); 
end

writeKeys = {'network','','bcast_port',temp};
inifile(configFile,'write',writeKeys,'tabbed')


fprintf('\n\n')
fprintf('------------------------------------------------------------\n')
%%%%%%%% Transport Setup %%%%%%%%
I_JAVA       = 1;
I_WN_MEX_UDP = 2;

TRANS_NAME_LONG{I_JAVA}        = 'Java UDP';
TRANS_NAME_SHORT{I_JAVA}       = 'java';
TRANS_NAME_LONG{I_WN_MEX_UDP}  = 'WARPNet Mex UDP';
TRANS_NAME_SHORT{I_WN_MEX_UDP} = 'wn_mex_udp';

TRANS_AVAIL(I_JAVA)        = true;
TRANS_AVAIL(I_WN_MEX_UDP)  = false;

fprintf('Select from the following available transports:\n')

if(isempty('wn_mex_udp')==0)
    try        
        version = wn_mex_udp_transport('version');
        version = sscanf(version,'%d.%d.%d%c');
        
        version_req = [1,0,0,sscanf(sprintf('%d','a'),'%d')];
        
        if(version(1)>version_req(1))
            TRANS_AVAIL(I_WN_MEX_UDP) = true;
        elseif(version(1)==version_req(1) && version(2)>version_req(2))
            TRANS_AVAIL(I_WN_MEX_UDP) = true;
        elseif(version(1)==version_req(1) && version(2)==version_req(2) && version(3)>version_req(2))
            TRANS_AVAIL(I_WN_MEX_UDP) = true;
        elseif(version(1)==version_req(1) && version(2)==version_req(2) && version(3)==version_req(3) && version(4)>=version_req(4))
            TRANS_AVAIL(I_WN_MEX_UDP) = true;
        end
        
    catch me
        fprintf('   For better transport performance, please setup the WARPNet Mex UDP transport: \n')
        fprintf('      http://warpproject.org/trac/wiki/WARPNet/MEX \n')
        TRANS_AVAIL(I_WN_MEX_UDP) = false;
    end
end

if(any(TRANS_AVAIL)==0)
   error('no supported transports were found installed on your computer'); 
end

defaultSel = 1;
sel        = 1;
for k = 1:length(TRANS_AVAIL)
    if(TRANS_AVAIL(k))
        if(strcmp(TRANS_NAME_SHORT{k},transport))
            defaultSel = sel;
            fprintf('[%d] (default) %s\n',sel,TRANS_NAME_LONG{k})
        else
            fprintf('[%d]           %s\n',sel,TRANS_NAME_LONG{k})
        end
        selectionToIndex(sel) = k;
        sel = sel+1;
    end
end

temp = input('Selection: ');
if(isempty(temp))
    temp = defaultSel;
end

if(temp>(sel-1))
   error('invalid selection') 
end

transport = TRANS_NAME_SHORT{selectionToIndex(temp)};

fprintf('   setting to %s\n',transport);

writeKeys = {'network','','transport',transport};
inifile(configFile,'write',writeKeys,'tabbed')

fprintf('\n\n')
fprintf('------------------------------------------------------------\n')
%%%%%%%% Transport Setup %%%%%%%%
NAME_SHORT{1} = 'false';
NAME_SHORT{2} = 'true';


fprintf('Enable jumbo frame support? (experimental)\n')

sel = 1;
for k = 1:2
        if(strcmp(NAME_SHORT{k},jumbo))
            defaultSel = sel;
            fprintf('[%d] (default) %s\n',sel,NAME_SHORT{k})
        else
            fprintf('[%d]           %s\n',sel,NAME_SHORT{k})
        end
        selectionToIndex(sel) = k;
        sel = sel+1;    
end

temp = input('Selection: ');
if(isempty(temp))
   temp = defaultSel;
end

if(temp>(sel-1))
   error('invalid selection') 
end

jumbo = NAME_SHORT{selectionToIndex(temp)};

fprintf('   setting to %s\n',jumbo);

writeKeys = {'network','','jumbo',jumbo};
inifile(configFile,'write',writeKeys,'tabbed')


fprintf('\n')
fprintf('------------------------------------------------------------\n')
fprintf('WARPNet Setup Complete\nwn_ver():\n');
wn_ver()
fprintf('------------------------------------------------------------\n\n')

end