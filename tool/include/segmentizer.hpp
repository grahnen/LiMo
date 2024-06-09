#ifndef SEGMENTIZER_H_
#define SEGMENTIZER_H_
#include <set>
#include <functional>
#include "typedef.h"
#include "segment.hpp"

struct seg_node {
    tid_t thread;
    index_t index;
    timestamp_t add_call, add_ret, rmv_call, rmv_ret;
    std::set<val_t> conc; // Save None to skip removal complexity
    optval_t original_val;
    AtomicInterval addI() { return AtomicInterval::closed(add_call, add_ret); }
    AtomicInterval rmvI() { return AtomicInterval::closed(rmv_call, rmv_ret); }
    seg_node(tid_t t, index_t i) : thread(t), index(i), add_call(POSINF), add_ret(POSINF), rmv_call(POSINF), rmv_ret(POSINF), original_val({}) {}
    seg_node() : seg_node(0,0){}
    
    int overlaps(seg_node &v){
        AtomicInterval
            ai = addI(),
            ri = rmvI(),
            av = v.addI(),
            rv = v.rmvI();

        // Counts the number of overlaps
        return
            ((int)ai.overlaps(av)) +
            ((int)ai.overlaps(rv)) +
            ((int)ri.overlaps(av)) +
            ((int)ri.overlaps(rv));
            
    }
    
    std::vector<timestamp_t> timestamps() const;
};



using Accessor = std::vector<std::vector<seg_node>>;

using SegmentAccessor = std::vector<std::vector<Segmentation *>>;

SegmentAccessor segmentize(std::vector<val_t> order, Accessor acc);
void check_segments(ADT a, const std::vector<val_t> &order, const Accessor &acc, const SegmentAccessor &segs);



#endif // SEGMENTIZER_H_
