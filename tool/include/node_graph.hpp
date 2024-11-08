#ifndef NODE_GRAPH_H_
#define NODE_GRAPH_H_
#include <vector>
#include "typedef.h"
#include <cassert>
#include <optional>
#include "exception.h"

using optidx = std::optional<index_t>;

enum State : uint32_t {
Called = 1 << 1,
Returned = 1 << 2,
Popped = 1 << 3,
Done = 1 << 4,
};

struct node_t {
    bool concurrent = false;
    tid_t thread;
    index_t index;
    State state;
    node_t(tid_t thr, index_t v) : thread(thr), index(v), state(Called) {}
    node_t &operator=(const node_t &o) = default;
    std::vector<optidx> pred;
    std::vector<optidx> succ;
    std::vector<optidx> traps;
};

struct graph_t {
    std::set<val_t> concurrent;
    tid_t SIZE;
    graph_t(tid_t sz) : SIZE(sz) {
        nodes.resize(SIZE);
        heads.resize(SIZE, {});
    }
    std::vector<std::vector<node_t>> nodes;
    std::vector<optidx> heads;
    std::vector<optidx> roots;

    void push_call(tid_t thr, index_t idx) {
        if(concurrent.contains(val_t(thr, idx))) {
            return;
        }
        nodes[thr].push_back(node_t(thr, idx));
        nodes[thr][idx].succ.resize(SIZE, {});
        nodes[thr][idx].pred.resize(SIZE, {});

        for(tid_t t = 0; t < SIZE; t++) {

            if(!heads[t].has_value())
                continue;
            if(nodes[t][heads[t].value()].state == Returned) {
                nodes[thr][idx].pred[t] = heads[t];
                nodes[t][heads[t].value()].succ[thr] = idx;
                heads[t] = {};
            }
        }
        heads[thr] = idx;
    }

    void push_return(tid_t thr, index_t idx) {
        nodes[thr][idx].state = Returned;
    }

    void remove(tid_t thr, index_t idx) {
        if(nodes[thr].size() <= idx) {
            concurrent.insert(val_t(thr, idx));
            // Pop first
            return;
        }
        if (nodes[thr][idx].state == Called) {
            concurrent.insert(val_t(thr, idx));
        }
        node_t &n = nodes[thr][idx];
        n.traps.resize(SIZE, {});
        for(tid_t t = 0; t < SIZE; t++) {
            if(!heads[thr].has_value())
                continue;
            if(nodes[thr][heads[thr].value()].state == Called)
                continue;

            n.traps[thr] = heads[thr];
        }

        for(tid_t t = 0; t < SIZE; t++) {
            if(n.pred[t].has_value()) {
                node_t &pre = nodes[t][n.pred[t].value()];
                std::cout << pre.succ[thr] << std::endl;
                assert(pre.succ[thr] == idx);
                if(n.succ[thr].has_value()) {
                    node_t &suc = nodes[thr][n.succ[thr].value()];
                    if(suc.pred[t] > n.pred[t]) {
                        pre.succ[thr] = {};
                    } else {
                        pre.succ[thr] = n.succ[thr];
                        suc.pred[t] = n.pred[t];
                    }
                } else {
                    pre.succ[thr] = {};
                }
            }
            if(n.succ[t].has_value()) {
                node_t &suc = nodes[t][n.succ[t].value()];
                assert(suc.pred[thr] == idx);
                if(n.pred[thr].has_value()) {
                    node_t &pre = nodes[thr][n.pred[thr].value()];
                    if(pre.succ[t] < n.succ[t]) {
                        suc.pred[thr] = {};
                    } else {
                        suc.pred[thr] = n.pred[thr];
                        pre.succ[t] = n.succ[t];
                    }
                } else {
                    suc.pred[thr] = {};
                }
            }
        }
        if(heads[thr] == idx)
            heads[thr] = {};

        if(std::all_of(heads.begin(), heads.end(), [](auto v) { return !v.has_value(); })) {
            // Heads is empty:
            heads = std::vector(n.pred.begin(), n.pred.end());
        }


    }

    bool ensure_rmv(tid_t thr, index_t idx) {
        if(concurrent.contains(val_t(thr, idx))) {
            concurrent.erase(val_t(thr, idx));
            std::cout << "It's concurrent!" << std::endl;
            return true;
        }
        node_t &n = nodes[thr][idx];
        for(tid_t j = 0; j < SIZE; j++) {
            if(!n.succ[j].has_value())
                continue;
            if(!n.traps[j].has_value())
                continue;
            if(n.succ[j].value() < n.traps[j].value()) {
                std::cout << nodes[thr][idx].succ[j] << " < " << nodes[thr][idx].traps[j] << std::endl;
                return false;
            }
        }

        return true;
    }
};


#endif // NODE_GRAPH_H_
