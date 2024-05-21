#include "convert.h"
#include <cassert>
#include "exception.h"
#include "io.h"

Configuration *simplify(Configuration *c, bool compactify) {
    if(!(c->type & (stack | queue))) {
        throw std::logic_error("Simplifying non-implemented ADT history: " + ext2str(c->type));
    }

    std::vector<event_t> output;
    tid_t THREADS = c->num_threads;
    Sequence input = c->history;
    std::vector<bool> active(THREADS, false);
    std::vector<std::optional<tid_t>> remap(THREADS, std::nullopt);
    std::vector<index_t> popmap(THREADS, -1);
    std::vector<index_t> maxi(THREADS, -1);
    std::map<val_t, val_t> valmap; // Can we get rid of this?

    tid_t max_t = 0;

    index_t popEmpties = 0;

    for(auto it : input) {
        if(it.type == Ecrash) {
            output.push_back(it);
            continue;
        }

        tid_t thr = 0;
        if(compactify) {
            if(it.type == Ereturn) {
                assert(remap[it.thread].has_value());
                thr = remap[it.thread].value();
                remap[it.thread] = std::nullopt;
            } else {
                while(active[thr]) {
                    thr++;
                }
                remap[it.thread] = thr;
            }
        } else {
            thr = it.thread;
        }
        if(thr > max_t) {
            max_t = thr;

        }

        if(it.type == Eenq || it.type == Epush) {
            assert(it.val.has_value());
            if(!it.val.has_value()) {
                throw std::logic_error("Push without value");
            }
            val_t &v = it.val.value();
            active[thr] = true;
            if(!(maxi[thr] >= 0))
                maxi[thr] = 0;
            index_t idx = maxi[thr];
            maxi[thr]++;
            val_t mv = val_t(v.input_val, thr, idx);

            valmap[v] = mv;
            output.push_back(event_t(it.type, thr, mv, it.timestamp));
        } else if (it.type == Epop || it.type == Edeq) {
            active[thr] = true;
            popmap[thr] = output.size();
            output.push_back(event_t(it.type, thr, it.val, it.timestamp));
        } else if (it.type == Ereturn) {
            assert(active[thr]);
            active[thr] = false;
            if(popmap[thr] >= 0) {
                event_t ev = output[popmap[thr]];
                if (ev.val.has_value()) {
                    if(!valmap.contains(ev.val.value())) {
                        throw Violation("Rmv before Add: " + ext2str(ev.val.value()));
                    }
                    output[popmap[thr]].val = valmap[ev.val.value()];
                    popmap[thr] = -1;
                }
                else if(it.val.has_value()) {
                    if(!valmap.contains(it.val.value()))
                    {
                        throw Violation("Rmv before Add: " + ext2str(it.val.value()));
                    }
                    // std::cout << it << std::endl;
                    // std::cout << output[popmap[thr]].val << " <- " << valmap[it.val.value()] << " <- " << it.val.value() << std::endl;
                    output[popmap[thr]].val = valmap[it.val.value()];

                    popmap[thr] = -1;
                }
                else {
                    output[popmap[thr]].val = {};
                    popEmpties++;
                }
            } else {
                // Push. Nothing needs changing.
            }

            output.push_back(event_t(Ereturn, thr, {}, it.timestamp));
        }

    }

    Configuration *out = new Configuration(output, c->type, max_t + 1, false);

    return preprocess(out);
}
