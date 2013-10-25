%%
% Example Setup
%
%   This file provides an example setup and use of various WLAN Exp commands.
%
%   Please use this as a guide, but refer to the documentation for final usage of commands
%

clear all

% Please modify wlan_exp_nodesConfig_example.txt with your node setup

config  = wlan_exp_nodesConfig('read', 'wlan_exp_nodesConfig_example.txt');              % Get the node configuration
network = wlan_exp_initNodes( config );                                                  % Initialize the nodes in the config to create a network

network.disp();                                                                          % Display the network

ap_nodes    = wlan_exp_networkCmd(network, 'get_nodes', 'wlan_exp_node_ap');             % Get AP nodes
sta_nodes   = wlan_exp_networkCmd(network, 'get_nodes', 'wlan_exp_node_station');        % Get STA nodes

log_oldest  = wn_nodeCmd( ap_nodes(1), 'log_get_oldest_event_index');                    % Get the index of the oldest event
log_current = wn_nodeCmd( ap_nodes(1), 'log_get_current_event_index');                   % Get the index of the current event
log_size    = log_current - log_oldest;                                                  % Compute the size of the log

output      = wn_nodeCmd( ap_nodes(1), 'log_get_events', log_size );                     % Get event log 
wlan_exp_event_logCmd( ap_nodes(1).event_log, 'add_event', output );                     % Add events to node event log

% Get the different types of events
rx_ofdm_log    = wlan_exp_event_logCmd( ap_nodes(1).event_log, 'get_events', 'wlan_exp_event_rx_ofdm');
rx_dsss_log    = wlan_exp_event_logCmd( ap_nodes(1).event_log, 'get_events', 'wlan_exp_event_rx_dsss');
tx_log         = wlan_exp_event_logCmd( ap_nodes(1).event_log, 'get_events', 'wlan_exp_event_tx');
bad_fcs_rx_log = wlan_exp_event_logCmd( ap_nodes(1).event_log, 'get_events', 'wlan_exp_event_err_bad_fcs_rx');

% ap_nodes(1).event_log.disp()                                                   % Display event log



%%
%  Get AP Nodes
ap_nodes = wlan_exp_networkCmd(network, 'get_nodes', 'wlan_exp_node_ap');


%%
%  Get Station Nodes
sta_nodes = wlan_exp_networkCmd(network, 'get_nodes', 'wlan_exp_node_station');


%%
%  Common Commands
output = wlan_exp_networkCmd(network, 'node_config_reset' );

output = wlan_exp_networkCmd(network, 'association_get_status' );
output = wlan_exp_networkCmd(network, 'association_set_table' );
output = wlan_exp_networkCmd(network, 'association_reset_stats' );

output = wlan_exp_networkCmd(network, 'disassociate', 'all' );            % Can also provide an AID

output = wlan_exp_networkCmd(network, 'get_tx_power' );
output = wlan_exp_networkCmd(network, 'set_tx_power' );
output = wlan_exp_networkCmd(network, 'get_tx_rate' );
output = wlan_exp_networkCmd(network, 'set_tx_rate' );

output = wlan_exp_networkCmd(network, 'get_channel' );
output = wlan_exp_networkCmd(network, 'set_channel', 2 );                 % Double between 1 - 11

output = wlan_exp_networkCmd(network, 'ltg_config_cbr', 0, 1000, 100 );   % AID = 0; 1000 bytes / packet; 100 us interval
output = wlan_exp_networkCmd(network, 'ltg_start', 0 );
output = wlan_exp_networkCmd(network, 'ltg_stop', 0 );
output = wlan_exp_networkCmd(network, 'ltg_remove', 0 );

wlan_exp_networkCmd(network, 'log_reset');

output = wlan_exp_networkCmd(network, 'log_config' );
output = wlan_exp_networkCmd(network, 'log_get_current_event_index' );
output = wlan_exp_networkCmd(network, 'log_get_oldest_event_index' );
output = wlan_exp_networkCmd(network, 'log_get_events' );


%%
%  Command test - AP
output = wn_nodeCmd( ap_nodes, 'get_association_status' );
output = wn_nodeCmd( ap_nodes, 'get_channel' );
output = wn_nodeCmd( ap_nodes, 'set_channel', 2 );
output = wn_nodeCmd( ap_nodes, 'association_reset_stats' );

output = wn_nodeCmd( ap_nodes, 'set_ssid', 'WARP-AP' );
output = wn_nodeCmd( ap_nodes, 'disassociate', 'all');

output = wn_nodeCmd( ap_nodes, 'allow_associations' );
output = wn_nodeCmd( ap_nodes, 'allow_associations', 'permanent' );
output = wn_nodeCmd( ap_nodes, 'disallow_associations' );


%% 
%  Command test - Station
output = wn_nodeCmd( sta_nodes, 'association_get_status' );
output = wn_nodeCmd( sta_nodes, 'get_channel' );
output = wn_nodeCmd( sta_nodes, 'set_channel', 2 );
output = wn_nodeCmd( sta_nodes, 'association_reset_stats' );



