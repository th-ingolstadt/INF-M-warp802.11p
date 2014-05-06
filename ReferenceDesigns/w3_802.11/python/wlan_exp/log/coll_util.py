import numpy as np
import wlan_exp.log.util as log_util
import wlan_exp.util as wlan_exp_util    

def _collision_idx_finder_l(src_ts, src_dur, int_ts, int_dur):
     
    num_src = src_ts.shape[0]
    num_int = src_ts.shape[0]
    
    src_coll_idx = np.zeros([num_src],dtype=np.int)
    int_coll_idx = np.zeros([num_int],dtype=np.int)
    
    for src_idx in range(num_src):        
        
        curr_src_ts = src_ts[src_idx]
        curr_src_dur = src_dur[src_idx]    

        int_idx = num_int/2
        int_idx_high = num_int-1
        int_idx_low = 0

        #start in middle and branch out   
        while 1:
            if (int_idx == int_idx_low or int_idx == int_idx_high):
                #We're done. No overlap
                break            
            
            curr_int_ts = int_ts[int_idx]
            curr_int_dur = int_dur[int_idx]
            
            if ( curr_int_ts < (curr_src_ts + curr_src_dur) ) and ( curr_src_ts < (curr_int_ts + curr_int_dur) ):
                #We found an overlap
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
        
                                                                
        
    return (src_coll_idx,int_coll_idx)

