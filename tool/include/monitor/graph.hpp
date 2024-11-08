#ifndef GRAPH_H_
#define GRAPH_H_

#include "typedef.h"
#include "segmentizer.hpp"

struct candidate {
    val_t v;
    timestamp_t ts;
    bool operator<(candidate &o) { return ts < o.ts; }
    bool operator>(candidate &o) { return ts > o.ts; }
};

using opt_cand = std::optional<candidate>;

constexpr opt_cand min_cand(opt_cand &a, candidate &b) {
    if(!a.has_value())
        return b;
    if(a.value() < b)
        return a;
    return b;
}

constexpr opt_cand max_cand(opt_cand &a, candidate &b) {
    if(!a.has_value())
        return b;
    if(a.value() > b)
        return a;
    return b;
}


enum op_state : u_char {
NotStarted = 0,
Called = 1,
Returned = 2,
};

inline std::ostream &operator<<(std::ostream &os, const op_state o) {
    switch(o) {
        case NotStarted:
            os << "NotStarted";
            break;
        case Called:
            os << "Called";
            break;
        case Returned:
            os << "Returned";
            break;
    }
    return os;
}

struct node_t {

    seg_node seg;
    op_state add;
    op_state rmv;

    std::vector<optval_t> first, last;

    std::vector<index_t> succ;
    std::vector<index_t> pred;

    void add_candidate(tid_t thread, val_t v);
    void collect_candidates();
    constexpr bool added() { return add == Returned; }
    constexpr bool present() { return added() && rmv == NotStarted; }
    val_t val() const {
        return seg.original_val.value_or(val_t(seg.thread, seg.index));
    }



};

std::ostream &operator<<(std::ostream &os, const node_t &n);
std::ostream &operator<<(std::ostream &os, const node_t *n);

class ConstraintGraph {
    private:
        void ensure_size(tid_t thr, index_t idx);
        void ensure_thread(tid_t thr);

        tid_t current_max;

        ADT type;
        std::vector<std::vector<node_t>> nodes;

        long long height = 0;
        std::vector<bool> pemp_ok;
        std::vector<optval_t> active;
        std::vector<index_t> top;

        void add_stack_traps(val_t v);
        void add_queue_traps(val_t v);
        void check_stack_traps(val_t v);
        void check_queue_traps(val_t v);


        void add_candidates(tid_t thr, val_t v);
        void get_first(val_t v);

        void remove(val_t val);
        void join_path(val_t cause, tid_t t1, index_t i1, tid_t t2, index_t i2);
    public:
        void close_open(std::vector<val_t> &);
        std::vector<std::vector<seg_node>> segment_nodes();

        std::vector<node_t> &operator[](tid_t th) { return nodes[th]; }
        void add_call(val_t v, timestamp_t ts);
        void add_ret(val_t v, timestamp_t ts);
        void rmv_call(tid_t thr, val_t v,timestamp_t ts);
        void rmv_ret(tid_t thr, val_t v, timestamp_t ts);
        void rmvE_call(tid_t thr);
        void rmvE_ret(tid_t thr);

        ADT adt() const { return type; }

        ConstraintGraph(ADT type, tid_t thread_count) : type(type), current_max(0) {
            ensure_thread(thread_count);
        }
        ~ConstraintGraph() {}
};

#endif // GRAPH_H_
