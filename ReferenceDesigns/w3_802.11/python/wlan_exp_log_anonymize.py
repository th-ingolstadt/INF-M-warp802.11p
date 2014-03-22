import sys
import os

import warpnet.wlan_exp_log.log_util as log_util
import warpnet.wlan_exp_log.log_util_hdf as hdf_util

all_addrs = list()
addr_idx_map = dict()

def do_replace_addr(addr):
    do_replace = True

    #Don't replace the broadcast address (FF-FF-FF-FF-FF-FF)
    if(addr == (0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF)):
        do_replace = False

    #Don't replace multicast IP addresses (01-00-5E-xx-xx-xx)
    if(addr[0:3] == (0x01, 0x00, 0x5E)):
        do_replace = False

    #Don't replace Mango addresses (40-D8-55-04-2X-XX)
    if(addr[0:4] == (0x40, 0xD8, 0x55, 0x04) and ((addr[4] & 0x20) == 0x20)):
        do_replace = False

    return do_replace


def addr_to_replace(addr, byte_index, addr_idx_map):
    global all_addrs
    if(do_replace_addr(addr)):
        if(addr not in all_addrs):
            all_addrs.append(addr)
        if addr not in addr_idx_map.keys():
            addr_idx_map[addr] = [byte_index,]
        else:
            addr_idx_map[addr].append(byte_index)
    return


def log_anonymize(filename):
    global all_addrs

    print("Anonymizing file %s" % (filename))
    # Get the log_data from the file
    log_bytes = bytearray(hdf_util.hdf5_to_log_data(filename=filename))

    # Get the log_data_index from the file
    log_data_index = hdf_util.hdf5_to_log_data_index(filename=filename)
    
    # Generate the index of log entry locations sorted by log entry type
    log_index     = log_util.filter_log_index(log_data_index, merge={'RX_ALL': ['RX_OFDM','RX_DSSS']})

    # Re-initialize the address-byteindex map per file using the running
    #   list of known MAC addresses
    addr_idx_map = dict()
    for addr in all_addrs:
        addr_idx_map[addr] = list()

    log_util.print_log_index_summary(log_index)

    # Step 1: Build a dictionary of all MAC addresses in the log, then
    #   map each addresses to a unique anonymous address
    #   Uses tuple(bytearray slice) since bytearray isn't hashable as-is

    #Rx entries
    try:
        for idx in log_index['RX_ALL']:
            #6-byte addresses at offsets 12, 18, 24
            for o in (12, 18, 24):
                addr_to_replace(tuple(log_bytes[idx+o:idx+o+6]), idx+o, addr_idx_map)
    except KeyError:
        pass

    #Tx entries
    try:
        for idx in log_index['TX']:
            #6-byte addresses at offsets 20, 26, 32
            for o in (20, 26, 32):
                addr_to_replace(tuple(log_bytes[idx+o:idx+o+6]), idx+o, addr_idx_map)
    except KeyError:
        pass

    #Station Info entries
    try:
        for idx in log_index['STATION_INFO']:
            #6-byte address at offsets 8
                o = 8
                addr_to_replace(tuple(log_bytes[idx+o:idx+o+6]), idx+o, addr_idx_map)
    except KeyError:
        pass

    #TODO: Add WN cmd entries, to cover LTG cmds that might have non-Mango MAC addresses?

    #####################
    #Step 2: Enumerate actual MAC addresses and their anonymous replacements
    addr_map = dict()
    for ii,addr in enumerate(all_addrs):
        anon_addr = (0xFF, 0xFF, 0xFF, 0xFF, (ii//256), (ii%256))
        addr_map[addr] = anon_addr

    #Step 3: Replace all MAC addresses in the log 
    for old_addr in addr_idx_map.keys():
        new_addr = bytearray(addr_map[old_addr])
        for byte_idx in addr_idx_map[old_addr]:
            log_bytes[byte_idx:byte_idx+6] = new_addr

    #Step 4: Other annonymization steps

    #Station info entries contain "hostname", the DHCP client hostname field
    # Replace these with a string version of the new anonymous MAC addr
    try:
        for idx in log_index['STATION_INFO']:
            #6-byte MAC addr (already anonymized) at offset 8
            #15 character ASCII string at offset 14
            addr_o = 8
            name_o = 14
            addr = log_bytes[idx+addr_o : idx+addr_o+16]
    
            new_name = "AnonNode %2x_%2x" % (addr[4], addr[5])
            log_bytes[idx+name_o : idx+name_o+15] = bytearray(new_name.encode("UTF-8"))
    except KeyError:
        pass


    #Write the modified log to a new HDF5 file
    (fn_fldr, fn_file) = os.path.split(filename)

    # Find the last '.' in the file name and classify everything after that as the <ext>
    ext_i = fn_file.rfind('.')
    if (ext_i != -1):
        # Remember the original file extension
        fn_ext  = fn_file[ext_i:]
        fn_base = fn_file[0:ext_i]
    else:
        fn_ext  = ''
        fn_base = fn_file

    newfilename = os.path.join(fn_fldr, fn_base + "_anon" + fn_ext)
    hdf_util.log_data_to_hdf5(log_bytes, newfilename)

    return
################################################################################


if(len(sys.argv) < 2):
    print("ERROR: must provide at least one log file input")
    sys.exit()
else:
    for filename in sys.argv[1:]:
        log_anonymize(filename)

print("MAC Address Mapping:")
for ii,addr in enumerate(all_addrs):
    anon_addr = (0xFF, 0xFF, 0xFF, 0xFF, (ii//256), (ii%256))
    print("%2d: %02x:%02x:%02x:%02x:%02x:%02x -> %02x:%02x:%02x:%02x:%02x:%02x" %
        (ii, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], 
         anon_addr[0], anon_addr[1], anon_addr[2], anon_addr[3], anon_addr[4], anon_addr[5]))
