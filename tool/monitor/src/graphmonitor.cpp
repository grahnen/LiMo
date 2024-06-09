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

void GraphMonitor::add_cmps(std::vector<std::pair<val_t, val_t>> &cmps, Accessor &a, val_t v) {
    auto &vv = a[v.thread][v.idx];
    std::set<val_t> nconc;
    for(auto cvl : vv.conc) { // O(k)
        auto &cc = a[cvl.thread][cvl.idx];
        int n = vv.overlaps(cc); // O(1)
        if(n == 1) {
            cmps.push_back(VP(v, cvl));
            // We enqueue their comparisons, and we remove their concurrency
            cc.conc.erase(v); // O(log k)...
        } else {
            nconc.insert(cvl);
        }
    }
    vv.conc = nconc;
}


void GraphMonitor::make_cmp(std::vector<VP> &cmps, VP pair, Accessor &a) {
    auto
        &v = a[pair.first.thread][pair.first.idx],
        &c = a[pair.second.thread][pair.second.idx];



    AtomicInterval
        av = v.addI(),
        rv = v.rmvI(),
        ac = c.addI(),
        rc = c.rmvI();

    if(v.overlaps(c) != 1) {
        //std::cout << "Number of overlaps reduced!" << std::endl;
        if(!valid(graph.adt(), av, rv, ac, rc))
            throw Violation("Violation on " + ext2str(pair.first) + ", " + ext2str(pair.second));
    }


    if(av.overlaps(ac)) {
        if(rv.preceeds(rc)) {
            v.add_call = std::max(v.add_call, c.add_call);
            c.add_ret = std::min(v.add_ret, c.add_ret);
        } else {
            c.add_call = std::max(v.add_call, c.add_call);
            v.add_ret = std::min(v.add_ret, c.add_ret);
        }
    } else if(av.overlaps(rc)) {
        v.add_call = std::max(v.add_call, c.rmv_call);
        c.rmv_ret = std::min(c.rmv_ret, v.add_ret);
    } else if (ac.overlaps(rv)) {
        c.add_call = std::max(c.add_call, v.rmv_call);
        v.rmv_ret = std::min(v.rmv_ret, c.add_ret);
    } else if (rv.overlaps(rc)) {
        if(av.preceeds(ac)) {
            c.rmv_ret = std::min(c.rmv_ret, v.rmv_ret);
            v.rmv_call = std::max(v.rmv_call, c.rmv_call);
        } else {
            v.rmv_ret = std::min(v.rmv_ret, c.rmv_ret);
            c.rmv_call = std::max(c.rmv_call, v.rmv_call);
        }
    }

    if(av != v.addI() || rv != v.rmvI()) {
        add_cmps(cmps, a, pair.first);
    }

    if(ac != c.addI() || rc != c.rmvI())
        add_cmps(cmps, a, pair.second);
}

void GraphMonitor::do_linearization() {
    graph.close_open(rmv_order); // O(k)
    Accessor a = graph.segment_nodes(); // O(n)

    std::vector<VP> cmps;
    for(auto v : rmv_order) { // O(n)
        add_cmps(cmps, a, v); // O(k log k)
    }
    // Number of values that *can* be added to cmps is O(n * k)..
    while(!cmps.empty()) { // O(n * k)
        VP el = *cmps.rbegin();
        cmps.pop_back();
        // O(1)
        make_cmp(cmps, el, a);
    }

    for(auto &vvl : rmv_order) {
        auto &v = a[vvl.thread][vvl.idx];
        for(auto &cvl : v.conc) {
            auto &c = a[cvl.thread][cvl.idx];
            AtomicInterval
                ai = v.addI(),
                ri = v.rmvI(),
                av = c.addI(),
                rv = c.rmvI();

            if(!valid(graph.adt(), ai, ri, av, rv)) {
                throw Violation("Unlin at end");
            } else {
                //std::cout << ai << ri << av << rv << " ok" << std::endl;
            }
        }
    }

    // SegmentAccessor sa = segmentize(rmv_order, a);
    // check_segments(graph.adt(), rmv_order, a, sa);
}
