import numpy as np
import wlan_exp.log.util as log_util
import wlan_exp.util as wlan_exp_util    

def _collision_idx_finder_l(src_ts, src_dur, int_ts, int_dur):
     
    num_src = src_ts.shape[0]
    num_int = int_ts.shape[0]
    
    maxlen = np.max([num_src,num_int])
    src_coll_idx = np.zeros([maxlen],dtype=np.int)
    int_coll_idx = np.zeros([maxlen],dtype=np.int)
    
    int_idx_start = 0
    
    for src_idx in range(num_src):        
        
        curr_src_ts = src_ts[src_idx]
        curr_src_dur = src_dur[src_idx] 
        
        curr_src_tx_end = curr_src_ts + curr_src_dur
        
        num_iter = 0
        
#%% Inexplicably, this technique is quite a bit slower than the binary search.        
#        for int_idx in range(int_idx_start,num_int):
##            print("{0}:[{1} {2} {3}]".format(src_idx,int_idx_start,int_idx,num_int))
#            num_iter = num_iter+1
#            curr_int_ts = int_ts[int_idx]
#            curr_int_dur = int_dur[int_idx]
#
#            if ( curr_int_ts < (curr_src_tx_end) ) and ( curr_src_ts < (curr_int_ts + curr_int_dur) ):                
#                #Found an overlap
#                src_coll_idx[src_idx] = src_idx
#                int_coll_idx[src_idx] = int_idx 
#                int_idx_start = int_idx;
#                #TODO: We should check the next packet to see if it also collided        
#                break
#            elif ( curr_int_ts > (curr_src_tx_end) ):
#                int_idx_start = int_idx;
#                break

#%%
        int_idx = num_int/2
        int_idx_high = num_int-1
        int_idx_low = 0

        #start in middle and branch out   
        while 1:
            num_iter = num_iter+1
            if (int_idx == int_idx_low or int_idx == int_idx_high):
                #We're done. No overlap
                break            

#            print("{0}: [{1} {2} {3}] [{4}]".format(src_idx, int_idx_low, int_idx, int_idx_high, num_int))            
            
            curr_int_ts = int_ts[int_idx]
            curr_int_dur = int_dur[int_idx]
            
            if ( curr_int_ts < (curr_src_tx_end) ) and ( curr_src_ts < (curr_int_ts + curr_int_dur) ):
                #We found an overlap
                #TODO: keep iterating forward on int_idx until there isn't an overlap
                src_coll_idx[src_idx] = src_idx
                int_coll_idx[src_idx] = int_idx 
                break
            elif ( curr_int_ts < curr_src_ts ):
                #Overlap may be later -- move index forward
                int_idx_low = int_idx                
            else:
                #Overlap may be earlier -- move index back
                int_idx_high = int_idx

            int_idx = (int_idx_high - int_idx_low)/2 + int_idx_low
#%%        
#        print("{0}: num_iter = {1}".format(src_idx, num_iter))                                                                
        
    return (src_coll_idx,int_coll_idx)

