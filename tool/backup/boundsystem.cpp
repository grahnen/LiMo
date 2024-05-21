#include "monitor/constraintmonitor.hpp"

TP BoundSystem::push_call(TP &s, timestamp_t ts) { return BoundSystem::merge(s, std::make_shared<Bound>(AtomicInterval::closed(ts, POSINF), AtomicInterval::complete())); }
TP BoundSystem::push_ret(TP &s, timestamp_t ts) { return BoundSystem::merge(s, std::make_shared<Bound>(AtomicInterval::closed(NEGINF, ts), AtomicInterval::complete())); }
TP BoundSystem::pop_call(TP &s, timestamp_t ts) { return BoundSystem::merge(s, std::make_shared<Bound>(AtomicInterval::complete(), AtomicInterval::closed(ts, POSINF))); }
TP BoundSystem::pop_ret(TP &s, timestamp_t ts) { return BoundSystem::merge(s, std::make_shared<Bound>(AtomicInterval::complete(), AtomicInterval::closed(NEGINF, ts))); }


// BoundSystem does not have post-processing
void BoundSystem::post_process(std::vector<val_t> pop_order, std::function<TP &(tid_t, timestamp_t)> pushes, std::function<std::set<val_t> &(tid_t, timestamp_t)> conc_map) {}

std::pair<TP, TP> BoundSystem::compare(TP &pn, TP &o) {
    std::vector<TP> terms, self_terms;
    std::vector<BP> myBounds = pn->bound_disj();
    std::vector<BP> obnds = o->bound_disj();

    for(auto ob : obnds) {
        for (auto mb : myBounds) {
            Relation a = compare_bounds(mb, ob);

            for(RelVal branch : a.branches()) {
                TP r_it = restr_bounds(mb, branch, ob);
                TP r_self = restr_bounds(ob, reverse_rel(branch), mb);
                terms.push_back(r_it);
                self_terms.push_back(r_self);
            }
        }
    }
    if(terms.empty())
        throw Violation("Unlinearizable: Empty bounds");

    if(self_terms.empty())
        throw Violation("Unlinearizable: Empty bounds on self");

    TP ntp = std::make_shared<Disj>(terms)->simpl();
    TP ntp_self = std::make_shared<Disj>(self_terms)->simpl();

    if(!BoundSystem::sat(ntp) || !BoundSystem::sat(ntp_self)) {
        throw Violation("Unlinearizable: Bounds are false");
    }

    return std::pair(ntp_self, ntp);
}
