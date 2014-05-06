#cython: boundscheck=False
#cython: wraparound=False

cimport cython

import numpy as np
cimport numpy as np

DTYPE = np.uint64
ctypedef np.uint64_t DTYPE_t

def _collision_idx_finder_l(np.ndarray[DTYPE_t, ndim=1] src_ts, np.ndarray[DTYPE_t, ndim=1] src_dur, np.ndarray[DTYPE_t, ndim=1] int_ts, np.ndarray[DTYPE_t, ndim=1] int_dur):
    
    import wlan_exp.log.util as log_util
    import wlan_exp.util as wlan_exp_util    
    
    assert src_ts.dtype == DTYPE and src_dur.dtype == DTYPE and int_ts.dtype == DTYPE and int_dur.dtype == DTYPE
  
    cdef int num_src = src_ts.shape[0]
    cdef int num_int = src_ts.shape[0]
    
    cdef np.ndarray[DTYPE_t, ndim=1] src_coll_idx = np.zeros([num_src], dtype=DTYPE)
    cdef np.ndarray[DTYPE_t, ndim=1] int_coll_idx = np.zeros([num_int], dtype=DTYPE)

    cdef DTYPE_t curr_src_ts
    cdef DTYPE_t curr_src_dur
    cdef DTYPE_t curr_int_ts
    cdef DTYPE_t curr_int_dur
    
    int_ts_end = int_ts + int_dur
    
    cdef int src_idx, int_idx, int_idx_start
    
    int_idx_start = 0
    
    cdef int int_idx_high, int_idx_low

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

