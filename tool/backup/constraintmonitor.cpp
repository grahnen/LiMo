#include "monitor/constraintmonitor.hpp"
#include <functional>
#include "exception.h"

#define FIXPOINT 0

NODE_TEMPLATE
ConstraintMonitor<T, CS>::~ConstraintMonitor() {
    for(tid_t i = 0; i < thread_count; i++) {
        for(long unsigned int j = 0; j < pushes[i].size(); j++)
            delete pushes[i][j];
    }
}

NODE_TEMPLATE
ConstraintMonitor<T,CS>::ConstraintMonitor(MonitorConfig mc) : Monitor(mc), thread_count(mc.thread_count) {
    /* TODO */
    create_storage();
}

NODE_TEMPLATE
void ConstraintMonitor<T,CS>::create_storage() {
    int tmp = thread_count;
    thread_count = 0;
    assert_threads(tmp);
}

NODE_TEMPLATE
void ConstraintMonitor<T,CS>::assert_threads(tid_t thr) {
    if (thr >= thread_count) {
        if(thread_count != 0) {
            throw Crash("Incorrect thread count supplied in configuration");
        }
        active_push.resize(thr + 1, -1);
        popEmpties.resize(thr + 1, {});
        n_finished.resize(thr + 1, -1);
        pushes.resize(thr + 1);
        conc_map.resize(thr + 1);
        pop_exists.resize(thr + 1, false);
        pops.resize(thr+1);
        heads.resize(thr+1, -1);
        roots.resize(thr+1, MAXIDX);
        for(tid_t i = thread_count; i <= thr; i++) {
            n_finished[i] = -1;
            pushes[i] = std::vector<NP>();
            popEmpties[i] = {};
            pushes[i].reserve(2);
            conc_map[i] = std::vector<std::set<val_t>>();
            conc_map[i].reserve(2);
            pop_exists[i] = 0;
            pops[i] = PopNode();
        }

        thread_count = thr + 1;
    }
}
NODE_TEMPLATE
void ConstraintMonitor<T,CS>::do_linearization() {
    CS::post_process(
        pop_order,
        [&](tid_t t, timestamp_t ts) -> T & { return pushes[t][ts]->constraints; },
        [&](tid_t t, timestamp_t ts) -> std::set<val_t> & {
            return conc_map[t][ts];
        });
}

NODE_TEMPLATE
void ConstraintMonitor<T,CS>::print_state() const {
    std::string dot = Node::render_dot(pushes, roots, heads);

    writefile(dot_dir + "/" + file_name + std::to_string(nOps) + ".dot", dot);
}

NODE_TEMPLATE
void ConstraintMonitor<T,CS>::add_concurrency(event_t &ev, val_t val) {
    for(long unsigned int t = 0; t < pushes.size(); t++) {
        // Pushes
        if(heads[t] > MINIDX) {
            NP c = pushes[t][heads[t]];
            if(c->push == CALLED) {
                c->fst_push[ev.thread] = min_candidate(c->fst_push[ev.thread], candidate {
                    .v = val,
                    .ts = ev.timestamp
                });

                c->lst_push[ev.thread] = max_candidate(c->lst_push[ev.thread], candidate {
                    .v = val,
                    .ts = ev.timestamp
                });
            }

        }
        // Pops
        if(pops[t].active) {
            val_t rmvd = pops[t].rmvd;
            NP c = pushes[rmvd.thread][rmvd.idx];

            c->fst_pop[ev.thread] = min_candidate(c->fst_pop[ev.thread], candidate {
                .v = val,
                .ts = ev.timestamp
            });
            c->lst_pop[ev.thread] = min_candidate(c->lst_pop[ev.thread], candidate {
                .v = val,
                .ts = ev.timestamp
            });
        }
    }
}

NODE_TEMPLATE
void ConstraintMonitor<T,CS>::handle_push(event_t &ev) {
    assert_threads(ev.thread);
    active_push[ev.thread] = ev.val->idx;
    NP pn = nullptr;
    if(ev.val->idx < (index_t) pushes[ev.thread].size()) {
        // Push exists, since pop was called earlier than push for this or above.
        pn = pushes[ev.thread][ev.val->idx];

    } else {
        pn = Node::create(ev.thread, ev.val->idx, thread_count);
        conc_map[ev.thread].push_back(std::set<val_t>());
        pushes[ev.thread].push_back( pn );
    }
    roots[ev.thread] = std::min(ev.val->idx, roots[ev.thread]);
    pn->push_call(ev.timestamp, pushes, heads, ev.val);
    for(long unsigned int i = 0; i < pops.size(); i++) {
        if(pops[i].active) {
            pn->saved_conc.insert(pops[i].rmvd);
        }
    }
    //Graph.push_call(ev.thread, ev.val->idx);
    add_concurrency(ev, ev.val.value());

}
NODE_TEMPLATE
void ConstraintMonitor<T,CS>::handle_pop(event_t &ev) {
    if(!ev.val.has_value()) {
        handle_popEmpty(ev);
        return;
    }
    cover_depth--;
    if(cover_depth == 0) {
        for(tid_t t = 0; t < thread_count; t++) {
            if(popEmpties[t].has_value())
                popEmpties[t]->ok = true;
        }
    }
    assert_threads(ev.thread);
    assert_threads(ev.val->thread);

    pop_exists[ev.thread] = true;

    if((index_t) pushes[ev.val->thread].size() <= ev.val->idx) {
        // Pop is called before Push, we need to create push node.
        int k = pushes[ev.val->thread].size();
        pushes[ev.val->thread].resize(ev.val->idx + 1);
        conc_map[ev.val->thread].resize(ev.val->idx + 1);
        for(int i = k; i <= ev.val->idx; i++) {
            pushes[ev.val->thread][i] = Node::create(ev.val->thread, i, thread_count);
            conc_map[ev.val->thread].push_back(std::set<val_t>());
        }
    }

    NP pn = pushes[ev.val->thread][ev.val->idx];
    pn->pop_call(ev.timestamp, pushes, roots, heads);
    pop_order.push_back(ev.val.value());
    //Graph.remove(ev.val->thread, ev.val->idx);
    std::vector<index_t> pop_pre(heads.begin(), heads.end());

    for(long unsigned int t = 0; t < pop_pre.size(); t++) {
        if(pop_pre[t] >= 0 && pushes[t][pop_pre[t]]) {
            // We have an actual node.
            if(pushes[t][pop_pre[t]]->in_progress()) {
                pop_pre[t] = pushes[t][pop_pre[t]]->pred[t];
            }
        }
    }

    add_concurrency(ev, ev.val.value());

    pops[ev.thread] = PopNode {
    .rmvd = ev.val.value(),
    .call = ev.timestamp,
    .pre = std::vector<index_t>(pop_pre.begin(), pop_pre.end()),
    .active = true };
}

NODE_TEMPLATE
void ConstraintMonitor<T,CS>::handle_ret_push(event_t &ev) {
    NP pn;
    pn = pushes[ev.thread][active_push[ev.thread]];
    pn->push_return(ev.timestamp, pushes, heads);
    cover_depth++;

    add_concurrency(ev, pn->val());

}
NODE_TEMPLATE
void ConstraintMonitor<T,CS>::handle_ret_pop(event_t &ev) {
    if(popEmpties[ev.thread].has_value()) {
        handle_ret_popEmpty(ev);
        return;
    }
    NP pn;
    pop_exists[ev.thread] = false;
    PopNode &pop = pops[ev.thread];
    pop.active = false;
    val_t rm = pop.rmvd;
    pn = pushes[rm.thread][rm.idx];

    add_concurrency(ev, rm);

    pn->pop_return(ev.timestamp, pushes, roots, heads);
    // if(!Graph.ensure_rmv(rm.thread, rm.idx)) {
    //     throw Violation("Violation in graph_node");
    // }
    if(pn->concurrent)
        return;

    if(!CS::sat(pn->constraints))
        throw Violation("Unsat constraints on " + ext2str(pn->val()));

    std::vector<BP> myBounds;
    // this Push concurrent with push
    std::set<val_t> conc_pushes = pn->get_concurrent(pushes, roots, heads);

//            if(conc_pushes.size() > 0) {
    // std::cout << pn->val() << " conc with: " << std::endl;
    // for(auto it : conc_pushes)
    //     std::cout << it << std::endl;
    // std::cout << "----------" << std::endl;
    //}
    // For each thread
    //
    // This pop with other pops (that returned)
    // for(auto it : pn->pops) { TODO FIX
    //     conc_pushes.insert(it);
    // }
    // if(conc_pushes.size() > 0) {
    // std::cout << pn->val() << " (after pops) conc with: " << std::endl;
    // for(auto it : conc_pushes)
    //     std::cout << it << std::endl;
    // std::cout << "----------" << std::endl;
    // }
    for(tid_t i = 0; i < this->thread_count; i++){
        index_t pre, suc;

        // this pop with other pop (that is active)
        if(pop_exists[i]) {
            conc_pushes.insert(pops[i].rmvd);
        }

        // this Pop concurrent with other push
        if(i < pop.pre.size() && pop.pre[i] >= 0) {
            //if(pushes[i][pop.pre[i]]->succ.size() > i)
            pre = pushes[i][pop.pre[i]]->succ[i];
            if(pre == MAXIDX) {
                // If we have lost our predecessor, check the root
                pre = roots[i];
            }

        }
        else {
            pre = roots[i];
        }
        //std::cout << pre << std::endl;


        // heads[i] is either concurrent or before. How do we differentiate?
        suc = heads[i];
        if(suc > MINIDX && suc < MAXIDX) {
            if(pop.pre[i] < suc) {
                conc_pushes.insert(pushes[i][suc]->val());
            }
        }


        if(pre > MINIDX && pre < MAXIDX) {
            // pre is either concurrent or before.
            if(pop.pre[i] < pre) {
                conc_pushes.insert(pushes[i][pre]->val());
            }
        }

    }

    // Filter values that should not be checked.
    std::set<val_t> conc;
    for(auto it : conc_pushes) {
        // if(removed.contains(it))
        //     continue;
        NP n = pushes[it.thread][it.idx];
        if(n->concurrent)
            continue;
        if(pn->thread == n->thread && pn->index == n->index)
            continue;
        if(!CS::sat(n->constraints))
            throw Crash("Constraints on concurrent not sat? Should be detected earlier.");
        if(n->pop == NOT_STARTED) {
            n->constraints = CS::merge(n->constraints, CS::from_intervals(AtomicInterval::complete(), AtomicInterval::closed(ev.timestamp + 1, POSINF)));
        }

        conc.insert(it);
    }

    removed.insert(pn->val());

    #if DEBUGGING
    if(conc.size() > (long unsigned int) largest_conc) {
        largest_conc = conc.size();
        largest_conc_save = std::vector(conc.begin(), conc.end());
        largest_conc_val = pn->val();
    }
    #endif

    conc_map[rm.thread][rm.idx] = std::set<val_t>(conc.begin(), conc.end());
    for(auto ov : conc) {
        NP o = pushes[ov.thread][ov.idx];

        std::pair<T, T> res = CS::compare(pn->constraints, o->constraints);
        // Compare pn and o
        pn->constraints = CS::merge(pn->constraints, res.first);
        o->constraints = CS::merge(o->constraints, res.second);

        if(!CS::sat(pn->constraints))
            throw Violation("Unlinearizable, unsat bounds on " + ext2str(pn->val()));
        if(!CS::sat(o->constraints))
            throw Violation("Unlinearizable, unsat bounds on " + ext2str(ov));
    }

}

NODE_TEMPLATE
void ConstraintMonitor<T,CS>::handle_popEmpty(event_t &ev) {
    popEmpties[ev.thread] = PopEmptyNode(ev.timestamp);
    if(cover_depth == 0)
        popEmpties[ev.thread]->ok = true;
}

NODE_TEMPLATE
void ConstraintMonitor<T,CS>::handle_ret_popEmpty(event_t &ev) {
    assert(popEmpties[ev.thread].has_value());
    PopEmptyNode pn  = popEmpties[ev.thread].value();
    if(!pn.ok) {
        throw Violation("History unseparable around PopEmpty");
    }
    popEmpties[ev.thread] = {};
    //Check that there is a separable point in this popEmpty. I.e. some time point where all pushes to the left have their corresponding pops to the left.
}
