#include "convert.h"
#include "io.h"


Configuration *preprocess(Configuration *c) {
    // Remove overlaps !a >< ?a
    if(c->type != stack)
        return c;
    std::vector<event_t> evts;
    std::vector<std::vector<bool>> skip_val;
    skip_val.resize(c->num_threads);
    std::vector<optval_t> active;
    active.resize(c->num_threads, {});
    for(event_t &ev : c->history) {
        if(ev.type == Ereturn) {
            active[ev.thread] = {};
        }
        else {
            for(tid_t t = 0; t < c->num_threads; t++) {

                if(active[t].has_value() && active[t] == ev.val) {

                    if(skip_val[ev.val->thread].size() <= ev.val->idx) {

                        skip_val[ev.val->thread].resize(ev.val->idx + 1, false);
                    }

                    skip_val[ev.val->thread][ev.val->idx] = true;
                }
            }
            active[ev.thread] = ev.val;
        }
    }


    std::vector<bool> skip_active;
    skip_active.resize(c->num_threads, false);
    for(event_t &ev : c->history) {
        if(ev.val.has_value()) {
            if(skip_val[ev.val->thread].size() <= ev.val->idx || skip_val[ev.val->thread][ev.val->idx] == false) {
                evts.push_back(ev);
            } else {
                skip_active[ev.thread] = true;
            }

        } else if (skip_active[ev.thread] == false){
            evts.push_back(ev);
        } else {
            skip_active[ev.thread] = false;
        }
    }
    delete c;
    return new Configuration(evts, c->type, c->num_threads, false);
}
