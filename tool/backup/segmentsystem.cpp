#include "monitor/constraintmonitor.hpp"

SegmentBuilder SegmentSystem::init() {
    return SegmentBuilder();
}

SegmentBuilder SegmentSystem::from_intervals(AtomicInterval push, AtomicInterval pop) {
    return SegmentBuilder(push, pop);
}

SegmentBuilder SegmentSystem::merge(SegmentBuilder a, SegmentBuilder b) {
    // TODO
    AtomicInterval push = AtomicInterval::closedopen(std::max(a.push_call, b.push_call), std::min(a.push_ret, b.push_ret));
    AtomicInterval pop = AtomicInterval::closedopen(std::max(a.pop_call, b.pop_call), std::min(a.pop_ret, b.pop_ret));
    return SegmentBuilder(push, pop);
}

bool SegmentSystem::sat(SegmentBuilder &a) {
    return true; // Always satisfiable before we process.
}

std::pair<SegmentBuilder, SegmentBuilder> SegmentSystem::compare(SegmentBuilder &pn, SegmentBuilder &o) {
    // We don't do the logic here. We do it in post-process.
    return std::pair(pn, o);
}

SegmentBuilder SegmentSystem::push_call(SegmentBuilder &s, timestamp_t ts) { s.push_call = ts; return s; }
SegmentBuilder SegmentSystem::push_ret(SegmentBuilder &s, timestamp_t ts) { s.push_ret = ts; return s; }
SegmentBuilder SegmentSystem::pop_call(SegmentBuilder &s, timestamp_t ts) { s.pop_call = ts; return s; }
SegmentBuilder SegmentSystem::pop_ret(SegmentBuilder &s, timestamp_t ts) { s.pop_ret = ts; return s; }


void SegmentSystem::post_process(std::vector<val_t> pop_order, std::function<SegmentBuilder &(tid_t, timestamp_t)> pushes, std::function<std::set<val_t> &(tid_t, timestamp_t)> conc_map) {
    std::set<val_t> seen; // Seen is only for checking that the input is of the correct format.

    // Construct segmentations
    std::vector<std::vector<Segmentation *>> segmentations;

    for(auto it : pop_order) {
        seen.insert(it);
        if(segmentations.size() < it.thread + 1)
            segmentations.resize(it.thread + 1);

        if(segmentations[it.thread].size() < (long unsigned int) it.idx + 1)
            segmentations[it.thread].resize(it.idx + 1, nullptr);

        std::vector<timestamp_t> conc;
        for(auto it2 : conc_map(it.thread, it.idx)) {
            seen.insert(it2);

            std::vector<timestamp_t> it_ts = pushes(it2.thread, it2.idx).timestamps();
            conc.insert(conc.end(), it_ts.begin(), it_ts.end());

        }
        std::sort(conc.begin(), conc.end());

        segmentations[it.thread][it.idx] = pushes(it.thread, it.idx).Segmentize(conc);
    }

    for(auto it : seen) {
        if(!(it.thread < segmentations.size()))
            throw std::logic_error("thread " + ext2str(it.thread) + " missing");
        if(!((long unsigned int) it.idx < segmentations[it.thread].size())) {
            throw std::logic_error("idx" + ext2str(it.idx) + " too large: " + ext2str(segmentations[it.thread].size()) + " on thr: " + ext2str(it.thread));
        }
        if(segmentations[it.thread][it.idx] == nullptr) {
            throw std::logic_error("Uninitialized value " + ext2str(it));
        }
    }

    // Start doing comparisons

    for(auto it : pop_order) {
        tid_t thr = it.thread;
        index_t idx = it.idx;
        Segmentation *curr = segmentations[thr][idx];
        assert(curr != nullptr);

        for(auto v : conc_map(thr, idx)) {
            Segmentation *other = segmentations[v.thread][v.idx];

            assert(other != nullptr);
            Matrix n_c(curr->valid.rows, curr->valid.cols, false);
            Matrix o_c(other->valid.rows, other->valid.cols, false);

            for(long unsigned int i = 0; i < curr->push_ts.size() - 1; i++) {
                for(long unsigned int j = 0; j < curr->pop_ts.size() - 1; j++) {

                    if(!curr->valid.at(i,j))
                        continue;

                    for(long unsigned int  i2 = 0; i2 < other->push_ts.size() - 1; i2++) {
                        for(long unsigned int j2 = 0; j2 < other->pop_ts.size() - 1; j2++) {

                            if(!other->valid.at(i2,j2))
                                continue;

                            //std::cout << "Checking <" << curr->push(i) << ", " << curr->pop(j) << "> ~ <" << other->push(i2) << ", " << other->pop(i2) << ">" << std::endl;

                            bool vl = valid(curr->push(i), curr->pop(j), other->push(i2), other->pop(j2));

                            n_c.set_or(i,j, vl);
                            o_c.set_or(i2,j2,vl);
                        }
                    }
                }
            }
            curr->valid = curr->valid * n_c;
            other->valid = other->valid * o_c;
            if(curr->valid.all_false())
                throw Violation("No lin for " + ext2str(it) + " found when checking " + ext2str(v));
            if(other->valid.all_false())
                throw Violation("No lin for " + ext2str(v) + " found on " + ext2str(it));
        }
    }


    for(long unsigned int i = 0; i < segmentations.size(); i++) {
        for(long unsigned int j = 0; j < segmentations[i].size(); j++) {
            if(segmentations[i][j]->valid.all_false())
                throw Violation("No linearization for " + ext2str(val_t(i,j)) + " with segmentation " + ext2str(*segmentations[i][j]));
            delete segmentations[i][j];
        }
    }
}
