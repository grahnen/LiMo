#include "monitor/queuemonitor.hpp"
#include "interval.h"
#include <boost/intrusive/detail/tree_node.hpp>
#include <boost/intrusive/rbtree.hpp>
#include <cassert>
#include <iostream>
#include <utility>
#include "exception.h"
#include "typedef.h"

QueueMonitor::QueueMonitor(MonitorConfig mc) : Monitor(mc) {}

void QueueMonitor::ensure_member(val_t v) {
    if(!inner.contains(v)) {
        inner.insert_or_assign(
            v,AtomicInterval::closed(NEGINF, POSINF));
    }
    if(!outer.contains(v)) {
        outer.insert_or_assign(
            v,AtomicInterval::closed(NEGINF, POSINF));
    }
}

void QueueMonitor::handle_enq(event_t &e) {
    active.insert_or_assign(e.thread, e.val.value());
    ensure_member(e.val.value());
    outer.at(e.val.value()).lbound = e.timestamp;
}

void QueueMonitor::handle_ret_enq(event_t &e) {
    val_t v = active[e.thread];
    ensure_member(v);
    inner.at(v).lbound = e.timestamp;
    if(inner.at(v).ubound < POSINF)
        add_val(v);
}

void QueueMonitor::handle_deq(event_t &e) {
    active.insert_or_assign(e.thread, e.val.value());
    ensure_member(e.val.value());
    inner.at(e.val.value()).ubound = e.timestamp;
    if(inner.at(e.val.value()).lbound > NEGINF)
        add_val(e.val.value());
}

void QueueMonitor::handle_ret_deq(event_t &e) {
    val_t v = active[e.thread];
    outer.at(v).ubound = e.timestamp;
}

void QueueMonitor::add_val(val_t v) {
    auto &vv = inner.at(v);
    ItvNode n = ItvNode(vv);
    queue_tree.insert(n);
}

void QueueMonitor::print_state() const {

}

void QueueMonitor::do_linearization() {

    ItvTree::Node *n = queue_tree.root;
    compute_k(n);

    for(auto vl : outer) {
        std::cout << "Outer: " << vl.first << ": " << vl.second << std::endl;
        if(contains(n, vl.second)) {
            throw Violation("Unlinearizable: outer in cover!");
        }
    }
}
