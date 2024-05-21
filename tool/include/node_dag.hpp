#ifndef NODE_DAG_H_
#define NODE_DAG_H_
#include "typedef.h"
#include <optional>
#include <vector>
#include <cassert>

enum OpState {
EMPTY,
CALLED,
RETURNED
};

using optidx = std::optional<index_t>;
struct node_t {
    tid_t size;
    OpState push;
    OpState pop;
    std::vector<index_t> pred;
    std::vector<index_t> succ;
    std::vector<index_t> traps;
    node_t(tid_t size) : size(size), push(EMPTY), pop(EMPTY) {
        pred.resize(size, MAXIDX);
        succ.resize(size, MINIDX);
        traps.resize(size, MINIDX);
    }
};


struct graph_t {
    tid_t size;
    std::vector<std::vector<node_t>> nodes;
    std::vector<index_t> top;
    graph_t(tid_t size) : size(size) {
        nodes.resize(size);
        top.resize(size, MAXIDX);
    }

    node_t *get(tid_t t, index_t idx) {
        if(t >= size || idx >= nodes[t].size())
            return nullptr;

        return &nodes[t][idx];
    }

    void push_call(tid_t thr, index_t idx) {

        std::vector<index_t> n_pred(top.begin(), top.end());
        nodes[thr].push_back(node_t(size));
        node_t *nn = get(thr, idx);
        nn->pred = n_pred;
        nn->push = CALLED;
    }

    void push_ret(tid_t thr, index_t idx) {
        node_t *n = get(thr, idx);
        assert(n);
        n->push = RETURNED;

        for(tid_t t = 0; t < size; t++) {
            node_t *top_t = get(t, top[t]);
            if(top_t && top_t->succ[thr] >= idx) {
                top[t] = {};
            }
        }
        top[thr] = idx;
    }

    void pop_call(tid_t thr, index_t idx) {
        node_t *n = get(thr, idx);
        assert(n && n->push == RETURNED);
        n->traps = std::vector(top.begin(), top.end());

        // Remove
        for(tid_t t = 0; t < size; t++) {
            node_t *pre_t = get(t, n->pred[t]);
            node_t *suc_t = get(t, n->succ[t]);

            // All links are double
            assert(!pre_t || pre_t->succ[idx] == thr);
            assert(!suc_t || suc_t->pred[idx] == thr);

            if(pre_t) {
                for(tid_t t2 = 0; t2 < size; t2++) {
                    node_t *pre_t_succ_t2 = get(t2, pre_t->succ[t2]);

                }
            }

            if(suc_t) {
                suc_t->pred[thr] = n->pred[thr];
            }

        }
    }

    void pop_ret(tid_t thr, index_t idx){

    }


};


#endif // NODE_DAG_H_
