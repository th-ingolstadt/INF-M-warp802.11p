import numpy as np
import wlan_exp.log.util as log_util
import wlan_exp.util as wlan_exp_util    

def _collision_idx_finder_l(src_ts, src_dur, int_ts, int_dur):
      
  
    num_src = src_ts.shape[0]
    num_int = src_ts.shape[0]
    
    src_coll_idx = np.zeros([num_src])
    int_coll_idx = np.zeros([num_int])
    
    int_ts_end = int_ts + int_dur

    num_coll = 0

    for src_idx in range(num_src):
        
        if src_idx % 1000 == 0:
            print("{0}/{1}".format(src_idx,num_src))
        
        curr_src_ts = src_ts[src_idx]
        curr_src_dur = src_dur[src_idx]        
        
        if np.any((int_ts < (curr_src_ts + curr_src_dur)) & (int_ts_end > curr_src_ts)):
            num_coll = num_coll+1

        
 #       for int_idx in range(num_int):
 #           curr_int_ts = int_ts[int_idx]
 #           curr_int_dur = int_dur[int_idx]
            
 #           if ( curr_int_ts < (curr_src_ts + curr_src_dur) ) and ( curr_src_ts < (curr_int_ts + curr_int_dur) ):
 #               src_coll_idx[src_idx] = src_idx
 #               int_coll_idx[src_idx] = int_idx                
 #               continue
                                                                
    print("num_coll = {0}".format(num_coll))    
    return (src_coll_idx,int_coll_idx)

