%%
% Example Setup
clear all

config  = wlan_exp_nodesConfig('read', 'wlan_exp_nodesConfig.txt');
network = wlan_exp_initNodes( config );
network.disp();

ap_nodes = wlan_exp_networkCmd(network, 'get_nodes', 'wlan_exp_node_ap');        % Get AP nodes
output   = wn_nodeCmd( ap_nodes, 'log_get_events' );                             % Get event log (fixed size)
temp     = wlan_exp_event_logCmd( ap_nodes(1).event_log, 'add_event', output );  % Add events to node event log
ap_nodes(1).event_log.disp()                                                     % Display event log



%%
%  Get AP Nodes
ap_nodes = wlan_exp_networkCmd(network, 'get_nodes', 'wlan_exp_node_ap');


%%
%  Get Station Nodes
sta_nodes = wlan_exp_networkCmd(network, 'get_nodes', 'wlan_exp_node_station');


%%
%  Common Commands
output = wlan_exp_networkCmd(network, 'node_config_reset' );

output = wlan_exp_networkCmd(network, 'get_association_status' );

output = wlan_exp_networkCmd(network, 'disassociate', 'all' );
output = wlan_exp_networkCmd(network, 'get_tx_power' );
output = wlan_exp_networkCmd(network, 'set_tx_power' );
output = wlan_exp_networkCmd(network, 'get_tx_rate' );
output = wlan_exp_networkCmd(network, 'set_tx_rate' );

output = wlan_exp_networkCmd(network, 'get_channel' );
output = wlan_exp_networkCmd(network, 'set_channel', 2 );

output = wlan_exp_networkCmd(network, 'config_ltg' );
output = wlan_exp_networkCmd(network, 'start_ltg' );
output = wlan_exp_networkCmd(network, 'stop_ltg' );

output = wlan_exp_networkCmd(network, 'reset_statistics' );

output = wlan_exp_networkCmd(network, 'get_traffic_events' );


%%
%  Command test - AP
output = wn_nodeCmd( ap_nodes, 'get_association_status' );
output = wn_nodeCmd( ap_nodes, 'get_channel' );
output = wn_nodeCmd( ap_nodes, 'set_channel', 2 );
output = wn_nodeCmd( ap_nodes, 'reset_statistics' );


output = wn_nodeCmd( ap_nodes, 'set_ssid', 'WARP-AP-EJW' );
output = wn_nodeCmd( ap_nodes, 'disassociate', 'all');

output = wn_nodeCmd( ap_nodes, 'allow_associations' );
output = wn_nodeCmd( ap_nodes, 'allow_associations', 'permanent' );
output = wn_nodeCmd( ap_nodes, 'disallow_associations' );


%% 
%  Command test - Station
output = wn_nodeCmd( sta_nodes, 'get_association_table' );
output = wn_nodeCmd( sta_nodes, 'get_channel' );
output = wn_nodeCmd( sta_nodes, 'set_channel', 2 );
output = wn_nodeCmd( sta_nodes, 'reset_statistics' );



