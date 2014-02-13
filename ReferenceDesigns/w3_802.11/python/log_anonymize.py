import wlan_exp_log
from wlan_exp_log import log_util
from wlan_exp_log.log_entries import *
from wlan_exp_log.mac_hdr_util import *
import sys
import code
from pprint import pprint

#Excellent util function for dropping into interactive Python shell
# From http://vjethava.blogspot.com/2010/11/matlabs-keyboard-command-in-python.html
def debug_here(banner=None):
    ''' Function that mimics the matlab keyboard command '''
    # use exception trick to pick up the current frame
    try:
        raise None
    except:
        frame = sys.exc_info()[2].tb_frame.f_back
    print("# Use quit() to exit :) Happy debugging!")
    # evaluate commands in current namespace
    namespace = frame.f_globals.copy()
    namespace.update(frame.f_locals)
    try:
        code.interact(banner=banner, local=namespace)
    except SystemExit:
        return

#logfile = 'example_logs/ap_log_stats.bin'
logfile_in  = 'example_logs/sta_log_stats.bin'
logfile_out = logfile_in[:-3] + '_anon.bin'

with open(logfile_in, 'rb') as fh:
    log_b = bytearray(fh.read())

# Generate the index of log entry locations sorted by log entry type
log_index = log_util.gen_log_index(log_b)

#Step 1: Build a dictionary of all MAC addresses in the log, then
# map each addresses to a unique anonymous address
# Uses tuple(bytearray slice) since bytearray isn't hashable as-is

all_addrs = set()
addr_idx_map = dict()

#OFDM Rx entries
if(log_entry_rx_ofdm.entry_type_ID in log_index.keys()):
    for idx in log_index[log_entry_rx_ofdm.entry_type_ID]:
        #6-byte addresses at offsets 12, 18, 24
        for o in (12, 18, 24):
            addr = tuple(log_b[o:o+6])
            all_addrs.add(addr)

#DSSS Rx entries
if(log_entry_rx_dsss.entry_type_ID in log_index.keys()):
    for idx in log_index[log_entry_rx_dsss.entry_type_ID]:
        #6-byte addresses at offsets 12, 18, 24
        all_addrs.add(tuple(log_b[idx+12:idx+12+6]))
        all_addrs.add(tuple(log_b[idx+18:idx+18+6]))
        all_addrs.add(tuple(log_b[idx+24:idx+24+6]))

#Tx entries
if(log_entry_tx.entry_type_ID in log_index.keys()):
    for idx in log_index[log_entry_tx.entry_type_ID]:
        #6-byte addresses at offsets 20, 26, 32
        all_addrs.add(tuple(log_b[idx+20:idx+20+6]))
        all_addrs.add(tuple(log_b[idx+26:idx+26+6]))
        all_addrs.add(tuple(log_b[idx+32:idx+32+6]))

#Station Info entries
if(log_entry_tx.entry_type_ID in log_index.keys()):
    for idx in log_index[log_entry_tx.entry_type_ID]:
        #6-byte address at offsets 8
        all_addrs.add(tuple(log_b[idx+8:idx+8+6]))

#Step 2: Enumerate actual MAC addresses and their anonymous replacements
addr_map = dict()
print("MAC Address Mapping:")
for ii,addr in enumerate(all_addrs):
    #Don't replace the broadcast address
    if(addr != (0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF)):
        anon_addr = (0xFF, 0xFF, 0xFF, 0xFF, (ii/256), (ii%256))
        addr_map[addr] = anon_addr
        print("%02x:%02x:%02x:%02x:%02x:%02x -> %02x:%02x:%02x:%02x:%02x:%02x" %
            (addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], 
             anon_addr[0], anon_addr[1], anon_addr[2], anon_addr[3], anon_addr[4], anon_addr[5]))


#Step 3: Replace all MAC addresses in the log bytearray

#Step 4: Other annonymization steps


# Station info entries contain "hostname", the DHCP client hostname field
# Replace these with a string version of the new anonymous MAC addr

debug_here()
