#include "segmentizer.hpp"
#include "exception.h"

std::vector<timestamp_t> seg_node::timestamps() const {
    return std::vector { add_call, add_ret, rmv_call, rmv_ret };
}


SegmentAccessor segmentize(std::vector<val_t> order, Accessor acc) {
    // std::vector<std::vector<SegmentBuilder *>> builders;
    // std::transform(acc.begin(), acc.end(), std::back_inserter(builders), [](std::vector<seg_node> nodes) {
    //     std::vector<SegmentBuilder *> sb;
    //     std::transform(nodes.begin(), nodes.end(), std::back_inserter(sb), [](seg_node n) {
    //         return SegmentBuilder(n.add_call, n.add_ret, n.rmv_call, n.rmv_ret);
    //     });
    //     return sb;
    // });

    SegmentAccessor segmentations;

    for(auto it : order) {
        if(segmentations.size() < it.thread + 1)
            segmentations.resize(it.thread + 1);

        if(segmentations[it.thread].size() < it.idx + 1)
            segmentations[it.thread].resize(it.idx + 1, nullptr);

        std::vector<timestamp_t> conc;

        for(auto it2 : acc[it.thread][it.idx].conc) {
            // std::cout << it << " with " << it2 << std::endl;
            std::vector<timestamp_t> it_ts = acc[it2.thread][it2.idx].timestamps();
            conc.insert(conc.end(), it_ts.begin(), it_ts.end());
        }

        std::sort(conc.begin(), conc.end());

        seg_node &n = acc[it.thread][it.idx];
        SegmentBuilder sb(n.add_call, n.add_ret, n.rmv_call, n.rmv_ret);

        segmentations[it.thread][it.idx] = sb.Segmentize(conc);
    }
    return segmentations;
}

void check_segments(ADT adt, const std::vector<val_t> &order, const Accessor &acc, const SegmentAccessor &segmentations){
    size_t max_conc = 0;
    for(auto it : order) {
        tid_t thr = it.thread;
        index_t idx = it.idx;
        if(thr >= segmentations.size() || idx >= segmentations[thr].size())
            continue;
        Segmentation *curr = segmentations[thr][idx];
        assert(curr != nullptr);
        //std::cout << it << " has " << acc[thr][idx].conc.size() << " conc" << std::endl;
        max_conc = std::max(max_conc, acc[thr][idx].conc.size());
        for(auto v : acc[thr][idx].conc) {
            if(v.thread >= segmentations.size() || v.idx >= segmentations[v.thread].size())
                continue;
            Segmentation *other = segmentations[v.thread][v.idx];

            assert(other != nullptr);
            Matrix n_c(curr->valid.rows, curr->valid.cols, false);
            Matrix o_c(other->valid.rows, other->valid.cols, false);

            for(long unsigned int i = 0; i < curr->add_ts.size() - 1; i++) {
                for(long unsigned int j = 0; j < curr->rmv_ts.size() - 1; j++) {

                    if(!curr->valid.at(i,j))
                        continue;

                    for(long unsigned int  i2 = 0; i2 < other->add_ts.size() - 1; i2++) {
                        for(long unsigned int j2 = 0; j2 < other->rmv_ts.size() - 1; j2++) {

                            if(!other->valid.at(i2,j2))
                                continue;

                            //std::cout << "Checking <" << curr->add(i) << ", " << curr->rmv(j) << "> ~ <" << other->add(i2) << ", " << other->rmv(j2) << ">" << std::endl;
                            bool vl = valid(adt, curr->add(i), curr->rmv(j), other->add(i2), other->rmv(j2));

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

    //std::cout << "Max conc: " << max_conc << std::endl;
    for(long unsigned int i = 0; i < segmentations.size(); i++) {
        for(long unsigned int j = 0; j < segmentations[i].size(); j++) {
            if(segmentations[i][j] == nullptr)
                continue;
            if(segmentations[i][j]->valid.all_false())
                throw Violation("No linearization for " + ext2str(val_t(i,j)) + " with segmentation " + ext2str(*segmentations[i][j]));
            delete segmentations[i][j];
        }
    }
}
