#include "exception.h"
#include "monitor/graphmonitor.hpp"
#include "segment.hpp"

void GraphMonitor::print_state() const {

}

GraphMonitor::GraphMonitor(MonitorConfig mc) : Monitor(mc), max_threads(mc.thread_count), graph(mc.type, mc.thread_count), cover_depth(0) {
    if(!(mc.type & (stack | queue))) {
        throw std::logic_error("Unhandled ADT: " + ext2str(mc.type));
    }
    active.resize(mc.thread_count, {});
    rmvEmpties.resize(mc.thread_count, {});

}

GraphMonitor::~GraphMonitor() {}

void GraphMonitor::handle_push(event_t &ev) {
    active[ev.thread] = ev.val;
    graph.add_call(ev.val.value(), ev.timestamp);
}

void GraphMonitor::handle_enq(event_t &ev) {
    handle_push(ev); // Identical behaviour!
}

void GraphMonitor::handle_pop(event_t &ev) {
    if(!ev.val.has_value()) {
        handle_rmvEmpty(ev);
        return;
    }
    val_t v = ev.val.value();
    active[ev.thread] = v;
    graph.rmv_call(ev.thread, ev.val.value(), ev.timestamp);
    rmv_order.push_back(v);
    // if(graph[v.thread][v.idx].add == Returned) {
    //     cover_depth--;
    //     if(cover_depth == 0) {
    //         for(tid_t t = 0; t < rmvEmpties.size(); t++) {
    //             if(rmvEmpties[t].has_value())
    //                 rmvEmpties[t] = true;
    //         }
    //     }
    // }

    // active[ev.thread] = ev.val.value();
}

void GraphMonitor::handle_deq(event_t &ev) {
    handle_pop(ev); // Identical behaviour!
}


void GraphMonitor::handle_ret_push(event_t &ev) {
    graph.add_ret(active[ev.thread].value(), ev.timestamp);
    active[ev.thread] = {};
    // val_t v = active[ev.thread].value();
    // if(graph[v.thread][v.idx].rmv == NotStarted) {
    //     cover_depth++;
    // }
    // active[ev.thread] = {};
    // add_concurrency(ev, v);

}

void GraphMonitor::handle_ret_enq(event_t &ev) {
    handle_ret_push(ev);
}

void GraphMonitor::handle_ret_pop(event_t &ev) {
    if(active[ev.thread].has_value())
        graph.rmv_ret(ev.thread, active[ev.thread].value(), ev.timestamp);
    else
        graph.rmvE_ret(ev.thread);

    active[ev.thread] = {};
    // if(!graph[v.thread][v.idx].skip) {
    //     add_concurrency(ev, v);
    //     rmv_order.push_back(v);
    // }
}

void GraphMonitor::handle_ret_deq(event_t &ev) {
    handle_ret_pop(ev);
}

void GraphMonitor::handle_rmvEmpty(event_t &ev) {
    graph.rmvE_call(ev.thread);
    //rmvEmpties[ev.thread] = (cover_depth == 0);
}

void GraphMonitor::handle_ret_rmvEmpty(event_t &ev) {
    graph.rmvE_ret(ev.thread);
    // if(!rmvEmpties[ev.thread].value())
    //     throw Violation("History unseparable around rmv empty");
    // rmvEmpties[ev.thread] = {};
}

void GraphMonitor::do_linearization() {
    graph.close_open(rmv_order);
    Accessor a = graph.segment_nodes();
    SegmentAccessor sa = segmentize(rmv_order, a);
    check_segments(graph.adt(), rmv_order, a, sa);
}
