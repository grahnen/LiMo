#include "monitor/graph.hpp"
#include "exception.h"
#include <cassert>
#include <algorithm>

std::ostream &operator<<(std::ostream &os, const node_t &n) {
    return os << &n;
}

std::ostream &operator<<(std::ostream &os, const node_t *v) {
    os << v->val();
    os << "\tpred: [ " << (v->pred[0] >= 0 ? std::to_string(v->pred[0]) : "ø");
    for(int i = 1; i < v->pred.size(); i++) {
        os << ", " << (v->pred[i] >= 0 ? std::to_string(v->pred[i]) : "ø");
    }
    os << " ]\n\tsucc: [ " << (v->succ[0] < MAXIDX ? std::to_string(v->succ[0]) : "ø");
    for(int i = 1; i < v->succ.size(); i++) {
        os << ", " <<  (v->succ[i] < MAXIDX ? std::to_string(v->succ[i]) : "ø");
    }
    os << " ]\n}\n";
    return os;
}

void ConstraintGraph::ensure_thread(tid_t thr) {
    if(thr > current_max) {
        pemp_ok.resize(thr + 1, true);
        nodes.resize(thr + 1);
        top.resize(thr + 1, MINIDX);
        active.resize(thr + 1, {});

        for(tid_t t = 0; t < thr; t++) {
            for(auto &n : nodes[t]) {
                n.first.resize(thr + 1, {});
                n.last.resize(thr + 1, {});
                n.succ.resize(thr + 1, MAXIDX);
                n.pred.resize(thr + 1, MINIDX);
                //n.traps.resize(thr + 1, MINIDX);
            }
        }
        current_max = thr;
    }
}

void ConstraintGraph::ensure_size(tid_t thr, index_t idx) {
    ensure_thread(thr);
    if(idx >= nodes[thr].size()) {
        index_t start_new = nodes[thr].size();
        nodes[thr].resize(idx + 1);
        for(index_t i = start_new; i < idx + 1; i++) {
            nodes[thr][i].seg = seg_node(thr, i);
            nodes[thr][i].first.resize(current_max + 1, {});
            nodes[thr][i].last.resize(current_max + 1, {});

            //nodes[thr][i].skip = false;
            nodes[thr][i].succ.resize(current_max + 1, MAXIDX);
            nodes[thr][i].pred.resize(current_max + 1, MINIDX);
            //nodes[thr][i].traps.resize(current_max + 1, MINIDX);
        }
    }
}

void ConstraintGraph::add_call(val_t v, timestamp_t ts) {
    ensure_size(v.thread, v.idx);
    node_t &n = nodes[v.thread][v.idx];
    n.seg.add_call = ts;
    n.seg.original_val = v;
    active[v.thread] = v;

    if(n.add != NotStarted)
        throw Crash("Add twice");

    n.add = Called;

    n.seg.add_call = ts;

    // if(n.rmv == Called && adt() == stack) {
    //     n.skip = true;
    //     return;
    // }

    for(tid_t i = 0; i < top.size(); i++) {
        if(top[i] == MINIDX)
            continue;

        node_t *topnode = &nodes[i][top[i]];


        if(topnode->add == Called) {

            if(topnode->pred[i] <= MINIDX) {
                topnode = nullptr;
            } else {
                topnode = &nodes[i][topnode->pred[i]];
            }
        }

        if(topnode != nullptr && topnode->succ[n.seg.thread] > n.seg.index) {
            n.pred[i] = topnode->seg.index;
            topnode->succ[n.seg.thread] = n.seg.index;
        }

    }
    get_first(v);
    add_candidates(v.thread, v);
    top[n.seg.thread] = n.seg.index;
}

void ConstraintGraph::add_ret(val_t v, timestamp_t timestamp) {
    ensure_size(v.thread, v.idx);
    node_t &n = nodes[v.thread][v.idx];
    n.seg.add_ret = timestamp;
    active[v.thread] = {};

    height++;

    if(n.add != Called) {
        throw Crash("Return add before call on " + ext2str(v));
    }
    n.add = Returned;
    add_candidates(v.thread, v);
    n.collect_candidates();

}

void ConstraintGraph::rmvE_call(tid_t thr) {
    pemp_ok[thr] = (height <= 0);
}

void ConstraintGraph::rmvE_ret(tid_t thr) {
    if(!pemp_ok[thr])
        throw Violation("PopEmpty violation, height = " + ext2str(height));
}

void ConstraintGraph::rmv_call(tid_t thr, val_t v, timestamp_t timestamp) {
    ensure_size(v.thread, v.idx);
    node_t &n = nodes[v.thread][v.idx];
    n.seg.rmv_call = timestamp;
    active[thr] = v;
    height--;

    if(height <= 0) {
        for(tid_t t = 0; t < pemp_ok.size(); t++) {
            pemp_ok[t] = true;
        }
    }

    if(n.rmv != NotStarted) {
        throw Violation(ext2str(v) + " is removed twice");
    }

    n.rmv = Called;
    get_first(v);
    add_candidates(thr, v);
    remove(v);
}

void ConstraintGraph::rmv_ret(tid_t thr, val_t v, timestamp_t ts) {
    ensure_size(v.thread, v.idx);
    node_t &n = nodes[v.thread][v.idx];
    n.seg.rmv_ret = ts;
    active[thr] = {};

    if(n.rmv != Called)
        throw Crash("Return before call");

    if(n.add < Called) {
        throw Violation("Remove before add");
    }
    n.rmv = Returned;


    switch(type) {
        case stack:
            check_stack_traps(v);
            break;
        case queue:
            check_queue_traps(v);
            break;
        default:
            break;
    }

    add_candidates(thr, v);
    n.collect_candidates();
}

void ConstraintGraph::add_candidates(tid_t thr, val_t v) {
    node_t &vn = nodes[v.thread][v.idx];
    for(tid_t t = 0; t < current_max; t++) {
        if(!active[t].has_value())
            continue;

        node_t &n = nodes[active[t]->thread][active[t]->idx];
        n.last[thr] = v;
        if(!n.first[thr].has_value())
            n.first[thr] = v;
    }
}

void ConstraintGraph::get_first(val_t v) {
    node_t &n = nodes[v.thread][v.idx];
    for(tid_t t = 0; t < current_max; t++) {
        if(active[t] == v)
            n.first[t] = {};

        n.first[t] = active[t];
    }
}

void ConstraintGraph::add_stack_traps(val_t v) {
    node_t &n = nodes[v.thread][v.idx];

    // for(tid_t t = 0; t < top.size(); t++) {
    //     if(top[t] == MINIDX)
    //         continue;
    //     node_t *topnode = &nodes[t][top[t]];

    //     if(topnode->present())
    //         n.traps[t] = topnode->seg.index;
    //     else if(topnode->pred[t] > MINIDX)
    //         n.traps[t] = topnode->pred[t];
    //     else
    //         continue;

    //     node_t *trap = &nodes[t][n.traps[t]];

    //     if(trap->seg.add_call < n.seg.add_ret) {

    //         n.traps[t] = MINIDX;
    //     }
    // }
    // // Don't trap self
    // if(n.traps[n.seg.thread] == n.seg.index) {

    //     n.traps[n.seg.thread] = MINIDX;
    // }
}

void ConstraintGraph::check_stack_traps(val_t v) {
    node_t &n = nodes[v.thread][v.idx];
    for(tid_t t = 0; t < current_max + 1; t++) {
        if(t >= n.succ.size())
            throw Crash("Succ too small");

        if(n.succ[t] == MAXIDX)
            continue;

        node_t &successor = nodes[t][n.succ[t]];
        // std::cout << v << ".succ = " << successor.val() << std::endl;
        // std::cout << n.seg.add_call << ", " << n.seg.add_ret << ", " << n.seg.rmv_call << ", " << n.seg.rmv_ret << std::endl;
        // std::cout << successor.seg.add_call << ", " << successor.seg.add_ret << ", " << successor.seg.rmv_call << ", " << successor.seg.rmv_ret << std::endl;

        if(n.seg.add_ret < successor.seg.add_call &&
           successor.seg.add_ret < n.seg.rmv_call &&
           successor.seg.rmv_call == POSINF) {
            throw Violation("Trap sprung at " + ext2str(v) + " -> " + ext2str(successor.val()));
        }

        // index_t tr = n.traps[t];
        // if(n.traps[t] < 0)
        //     continue;
        // if(n.succ[t] < MAXIDX) {
        //     node_t &suc = nodes[t][n.succ[t]];
        //     if(suc.present() && suc.seg.add_ret < n.seg.rmv_call)
        //      /* if(nodes[t][n.succ[t]].present() && n.succ[t] <= n.traps[t]) */ {
        //         throw Violation("Trap sprung at value: " + ext2str(v) + " -> " + ext2str(nodes[t][n.succ[t]].val()));
        //     }
        // }
    }
}

void ConstraintGraph::check_queue_traps(val_t v){
    node_t &n = nodes[v.thread][v.idx];
    for(tid_t t = 0; t < current_max + 1; t++) {
        if(t >= n.succ.size())
            throw Crash("Succ too small");

        if(n.pred[t] == MINIDX)
            continue;

        node_t &pred = nodes[t][n.pred[t]];

        if(n.seg.rmv_ret < pred.seg.rmv_call) {
            throw Violation("Trap sprung at " + ext2str(v) + " -> " + ext2str(pred.val()));
        }
    }
}


void ConstraintGraph::remove(val_t v) {
    node_t &n = nodes[v.thread][v.idx];
    tid_t max_t = current_max;
    assert(n.pred.size() == n.succ.size());

    for(tid_t t = 0; t < max_t + 1; t++) {
        if(n.pred[t] > MINIDX) {
            // DISCONNECT SUCCESSOR

            node_t &pre = nodes[t][n.pred[t]];
            pre.succ[v.thread] = MAXIDX; //n.succ[v.thread];
            // if(n.succ[v.thread] < MAXIDX) {
            //     node_t &suc = nodes[v.thread][n.succ[v.thread]];
            //     suc.pred[t] = n.pred[t];
            // }
            //std::cout << addes[t][pred[t]] << std::endl;
        }

        if(n.succ[t] < MAXIDX) {
            // Disconnect predecessor
            node_t &suc = nodes[t][n.succ[t]];

            suc.pred[v.thread] = MINIDX; // n.pred[v.thread];


            // if(n.pred[v.thread] > MINIDX) {
            //     node_t &pre = nodes[v.thread][n.pred[v.thread]];
            //     pre.succ[t] = n.succ[t];
            // }
        }
    }

    for(tid_t tp = 0; tp < max_t; tp++) {
        if(n.pred[tp] != MINIDX && n.succ[v.thread] != MAXIDX) {
            node_t *suc = &nodes[v.thread][n.succ[v.thread]];
            node_t *pre = &nodes[tp][n.pred[tp]];
            //t1 == tp, t2 == v.thread
            if(suc->pred[tp] == MINIDX && pre->succ[v.thread] == MAXIDX) {
                suc->pred[tp] = n.pred[tp];
                pre->succ[v.thread] = n.succ[v.thread];
            }
        }

        if(n.pred[v.thread] != MINIDX && n.succ[tp] != MAXIDX) {
            //t1 == v.thread, t2 == tp
            node_t *suc = &nodes[tp][n.succ[tp]];
            node_t *pre = &nodes[v.thread][n.pred[v.thread]];
            if(suc->pred[v.thread] == MINIDX && pre->succ[tp] == MAXIDX) {
                suc->pred[v.thread] = n.pred[v.thread];
                pre->succ[tp] = n.succ[tp];
            }
        }
    }
    // thread-local program-order connection, to ensure *top* keeps working..
    // if(n.pred[n.seg.thread] > MINIDX && n.succ[n.seg.thread] < MAXIDX) {
    //     //std::cout << "Mod made" << std::endl;
    //     nodes[n.seg.thread][n.pred[n.seg.thread]].succ[n.seg.thread] = n.succ[n.seg.thread];
    //     nodes[n.seg.thread][n.succ[n.seg.thread]].pred[n.seg.thread] = n.pred[n.seg.thread];
    // }

    // Adjust top.
    if(top[n.seg.thread] == n.seg.index) {
        top[n.seg.thread] = n.pred[n.seg.thread];
    }
}

void ConstraintGraph::join_path(val_t cause, tid_t t1, index_t i1, tid_t t2, index_t i2) {
    node_t *pre = nullptr;
    node_t *suc = nullptr;
    if(cause.thread == t1 || cause.thread == t2)
                return;
    if(i1 < MAXIDX && i1 > MINIDX) {
        pre = &nodes[t1][i1];
    }

    if(i2 < MAXIDX && i2 > MINIDX) {
        suc = &nodes[t2][i2];
    }

    if(pre == nullptr && suc == nullptr) {
        // Neither exists
        return;
    }
    if(pre != nullptr && suc != nullptr) {
        //std::cout << pre << " -> " << suc << ": ";
        // Scenario one:
        // if(pre->succ[t2] == i2 && suc->pred[t1] == i1) {
        //     // Do nothing with the links

        //     //std::cout << "scenario 1" << std::endl;
        // }
        // if(pre->succ[t2] < i2 && suc->pred[t1] > i1) {
        //     // Do nothing
        //     //std::cout << "scenario 2" << std::endl;
        // }
        if(pre->succ[t2] == i2 && suc->pred[t1] > i1) {
            //std::cout << "scenario 3" << std::endl;
            pre->succ[t2] = MAXIDX;
        }

        if(pre->succ[t2] < i2 && suc->pred[t1] == i1) {
            //std::cout << "scenario 4" << std::endl;
            suc->pred[t1] = MINIDX;
        }

        // // Missing successor
        // if(pre->succ[t2] == MAXIDX && suc->pred[t1] > MINIDX) {
        //     // Do nothing
        //     //std::cout << "scenario 5" << std::endl;
        // }
        // // Missing predecessor
        // if(suc->pred[t1] == MINIDX && pre->succ[t2] < MAXIDX) {
        //     // Do nothing
        //     //std::cout << "scenario 6" << std::endl;
        // }

        // Both missing
        if(suc->pred[t1] == MINIDX && pre->succ[t2] == MAXIDX) {
            return;
            // Connect
            //std::cout << "scenario 7" << std::endl;
            // Connect only if at least one is disconnected
            if((std::all_of(suc->pred.begin(), suc->pred.end(), [](index_t i) { return i == MINIDX; }) ||
                std::all_of(pre->succ.begin(), pre->succ.end(), [](index_t i) { return i == MAXIDX; }))) {
                std::cout << "HAPPENS" << std::endl;
                suc->pred[t1] = i1;
                pre->succ[t2] = i2;
            }

            //std::cout << pre << suc;
        }
    }

    else if(pre != nullptr && suc == nullptr) {
        //std::cout << "scenario 8" << std::endl;
        //pre->succ[t2] = std::min(pre->succ[t2], MAXIDX);
    }

    else if(suc != nullptr && pre == nullptr) {
        //std::cout << "scenario 9" << std::endl;
        //suc->pred[t1] = std::max(suc->pred[t1], MINIDX);
    }
}

void node_t::add_candidate(tid_t thread, val_t v) {
    if(!first[thread].has_value())
        first[thread] = v;
    last[thread] = v;
}

void node_t::collect_candidates() {
    for(tid_t t = 0; t < first.size(); t++) {
        if(first[t].has_value())
            seg.conc.insert(first[t].value());
        if(last[t].has_value())
            seg.conc.insert(last[t].value());

        first[t] = {};
        last[t] = {};
    }
}

Accessor ConstraintGraph::segment_nodes() {
    Accessor a;
    for(auto it : nodes) {
        std::vector<seg_node> v;
        for(node_t &n : it) {
            v.push_back(n.seg);
        }
        a.push_back(v);
    }
    return a;
}

void ConstraintGraph::close_open(std::vector<val_t> &order) {
    for(tid_t t = 0; t < current_max + 1; t++) {
        for(index_t i = 0; i < nodes[t].size(); i++) {
            node_t &n = nodes[t][i];

            if(n.add == Called) {
                n.seg.add_ret = POSINF - 3;
                n.add = Returned;
            }
            if(n.rmv == NotStarted) {
                order.push_back(n.val());
                n.seg.rmv_call = POSINF - 2;
                n.rmv = Called;
            }
            if(n.rmv == Called) {
                n.rmv = Returned;
                n.seg.rmv_ret = POSINF - 1;
            }
        }
    }
}
