#ifndef INTERVALMONITOR_H_
#define INTERVALMONITOR_H_
#include "segment.hpp"
#include "monitor/val_node.hpp"
#include "monitor.hpp"
#include "relation.h"
#include "logic.hpp"
#include "pop_node.hpp"
#include <cassert>
#include <memory>

#include "../monitor/src/val_node.cpp"// Include the source file since it contains template functions
#define DEBUGGING 1

class BoundSystem : public ConstraintSystem<TP>  {
public:
        static TP init(){ return std::make_shared<Const>(true);}
        static TP merge(TP a, TP b) { return std::make_shared<Conj>(std::vector { a, b })->simpl(); }
        static bool sat(TP &a) { return !(a->isConst() && a->value() == false); }
        static TP from_intervals(AtomicInterval push, AtomicInterval pop) { return std::make_shared<Bound>(push, pop); }
        static std::pair<TP, TP> compare(TP &a, TP &b);
        static TP push_call(TP &s, timestamp_t ts);
        static TP push_ret(TP &s, timestamp_t ts);
        static TP pop_call(TP &s, timestamp_t ts);
        static TP pop_ret(TP &s, timestamp_t ts);
        static void post_process(std::vector<val_t> pop_order, std::function<TP &(tid_t, timestamp_t)> pushes, std::function<std::set<val_t> &(tid_t, timestamp_t)> conc_map);
};

class SegmentSystem : public ConstraintSystem<SegmentBuilder> {
public:
        static SegmentBuilder init();
        static SegmentBuilder merge(SegmentBuilder a, SegmentBuilder b);
        static bool sat(SegmentBuilder &a);
        static SegmentBuilder from_intervals(AtomicInterval, AtomicInterval);
        static std::pair<SegmentBuilder, SegmentBuilder> compare(SegmentBuilder &a, SegmentBuilder &b);
        static SegmentBuilder push_call(SegmentBuilder &s, timestamp_t ts);
        static SegmentBuilder push_ret(SegmentBuilder &s, timestamp_t ts);
        static SegmentBuilder pop_call(SegmentBuilder &s, timestamp_t ts);
        static SegmentBuilder pop_ret(SegmentBuilder &s, timestamp_t ts);
        static void post_process(std::vector<val_t> pop_order, std::function<SegmentBuilder &(tid_t, timestamp_t)> pushes, std::function<std::set<val_t> &(tid_t, timestamp_t)> conc_map);
};

template class val_node<SegmentBuilder, SegmentSystem>;
template class val_node<TP, BoundSystem>;

using WP = index_t;

NODE_TEMPLATE
class ConstraintMonitor : public Monitor {
        using Node = val_node<T, CS>;
        using NP = typename Node::NP;
        uint64_t cover_depth = 0;
        std::vector<std::vector<std::set<val_t>>> conc_map;
        std::vector<val_t> pop_order;
        tid_t thread_count = 0;
        std::vector<std::optional<PopEmptyNode>> popEmpties;
        std::vector<index_t> n_finished;

        std::vector<index_t> active_push;
        std::vector<std::vector<NP>> pushes;

        std::vector<bool> pop_exists;
        std::vector<PopNode> pops;

        std::vector<index_t> heads;
        std::vector<index_t> roots;

        DECLHANDLER(popEmpty)
        DECLHANDLER(push)
        DECLHANDLER(pop)

public:

        #if DEBUGGING
        int largest_iter_count = 0;
        int num_contained = 0;
        int largest_conc = 0;
        val_t largest_conc_val;
        std::vector<val_t> largest_conc_save;
        #endif

        void assert_threads(tid_t thr);
        void do_linearization();
        ConstraintMonitor(MonitorConfig mc);
        ~ConstraintMonitor();

        void print_state() const;

    private:
        void create_storage();
        std::set<val_t> removed;
        void add_concurrency(event_t &, val_t);
};

using IntervalMonitor = ConstraintMonitor<TP, BoundSystem>;
using SegmentMonitor = ConstraintMonitor<SegmentBuilder, SegmentSystem>;

#endif // INTERVALMONITOR_H_
