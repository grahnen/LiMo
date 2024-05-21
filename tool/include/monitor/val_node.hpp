#ifndef PUSH_NODE_H_
#define PUSH_NODE_H_

#include "typedef.h"
#include "logic.hpp"
#include <cassert>
#include "exception.h"
#include "pop_node.hpp"
#include <type_traits>

#define NODE_TEMPLATE template<typename T, std::derived_from<ConstraintSystem<T>> CS>


template <typename T>
class ConstraintSystem;

enum op_state : u_char {
NOT_STARTED = 0,
CALLED = 1,
RETURNED = 2
};


struct candidate {
        val_t v;
        timestamp_t ts;

        bool operator<(candidate &o) {
            return ts < o.ts;
        }

        bool operator>(candidate &o) {
            return ts > o.ts;
        }
    };

using opt_cand = std::optional<candidate>;

inline opt_cand min_candidate(opt_cand &a, candidate b) {
    if(!a.has_value())
        return b;

    if(a.value() < b)
        return a;
    return b;
}

inline opt_cand max_candidate(opt_cand &a, candidate b) {
    if(!a.has_value())
        return b;

    if(a.value() > b)
        return a;
    return b;
}

inline void add_if_exists(std::set<val_t> &vls, std::optional<candidate> &c) {
    if(c.has_value())
        vls.insert(c->v);
}
using WP = index_t;

NODE_TEMPLATE
struct val_node {
    using Node = val_node<T, CS>;
    using NP = val_node<T,CS> *;
    using PushVec = std::vector<std::vector<NP>>;
    using ConcMap = std::vector<std::vector<std::set<NP>>>;

    timestamp_t push_call_time, pop_call_time, push_ret_time, pop_ret_time;
    tid_t thread;
    index_t index;
    T constraints;
    bool concurrent = false;
    op_state push;
    op_state pop;

    optval_t original_value;

    struct conc_ptr {
        val_t val;
        timestamp_t ts;

        bool operator<(conc_ptr &o) {
            return ts < o.ts;
        }
    };



    std::vector<std::optional<candidate>> fst_push;
    std::vector<std::optional<candidate>> lst_push;
    std::vector<std::optional<candidate>> fst_pop;
    std::vector<std::optional<candidate>> lst_pop;



    std::vector<WP> succ;
    std::vector<WP> pred;
    std::vector<WP> traps;



    std::set<val_t> saved_conc;
    constexpr bool inserted() {
        return push == RETURNED;
    }

    constexpr bool present() {
        return inserted() && pop == NOT_STARTED;
    }

    constexpr bool in_progress() {
        return push == CALLED;
    }

    constexpr bool invisible() {
        return concurrent;
    }

    val_t val() const {
        return original_value.value_or(val_t(thread, index));
    }

    void push_call(timestamp_t, PushVec &, std::vector<index_t> &, optval_t original_value = std::nullopt);
    void push_return(timestamp_t, PushVec &, std::vector<index_t> &);

    void add_traps(PushVec &, std::vector<index_t> &);

    //void check_trapped_own_thread(std::vector<NP> &pushes_own_thread, WP &traps_to_check) const;
    void check_trapped(PushVec &pushes) const;

    void pop_call(timestamp_t, PushVec &, std::vector<index_t> &);

    void pop_return(timestamp_t, PushVec &, std::vector<index_t> &);


    void join_path(tid_t t1, index_t i1, tid_t t2, index_t i2, std::vector<std::vector<val_node<T, CS>::NP>> &pushes);
    std::set<val_t> get_concurrent(PushVec &pushes, std::vector<index_t> &top);

    void remove(PushVec &pushes, std::vector<index_t> &top);

    static std::string render_dot(const PushVec &pushes, const std::vector<index_t> &top);

    static NP create(tid_t thread, index_t idx, tid_t n_threads) {
        NP node(new val_node());
        node->thread = thread;
        node->index = idx;
        node->concurrent = false;
        node->push = NOT_STARTED; // This is remedied in push_call()
        node->pop = NOT_STARTED;
        node->succ.resize(n_threads, MAXIDX);
        node->pred.resize(n_threads, MINIDX);

        node->push_call_time = POSINF;
        node->pop_call_time = POSINF;
        node->push_ret_time = POSINF;
        node->pop_ret_time = POSINF;

        node->constraints = CS::init();


        node->fst_push.resize(n_threads,  {});
        node->lst_push.resize(n_threads,  {});
        node->fst_pop.resize(n_threads,  {});
        node->lst_pop.resize(n_threads,  {});

        return node;
    }

private:
    val_node() = default;
};



NODE_TEMPLATE
std::ostream &operator<<(std::ostream &os, const val_node<T, CS> *v);

// NODE_TEMPLATE
// void print_traps(std::vector<std::vector<val_node<T, CS>::NP>> pushes) {
//     for(tid_t t = 0; t < pushes.size(); t++) {
//         for(index_t i = 0; i < pushes[t].size(); i++) {
//             for(int t2 = 0; t2 < pushes[t][i]->succ.size(); t2++) {
//                 std::cout << t << i << " -> " << t2 << pushes[t][i]->succ[t2] << ";" << std::endl;
//             }
//         }
//     }
// };


inline std::string name(tid_t th, index_t idx) {
    std::stringstream ss;
    ss << "n" << th << "_" << idx;
    return ss.str();
}

inline std::string name(const val_t v) {
    return name(v.thread, v.idx);
}

template<typename T>
class ConstraintSystem {
public:
    static T init(void);
    static T merge(T,T);
    static bool sat(T &);
    static T from_intervals(AtomicInterval, AtomicInterval);
    static std::pair<T, T> compare(T &,T &);
    static T push_call(T &s, timestamp_t ts);
    static T push_ret(T &s, timestamp_t ts);
    static T pop_call(T &s, timestamp_t ts);
    static T pop_ret(T &s, timestamp_t ts);
    static void post_process(std::vector<val_t> pop_order, std::function<T &(tid_t, timestamp_t)> pushes, std::function<std::set<val_t>&(tid_t, timestamp_t)> conc_map);
};

#endif // PUSH_NODE_H_
