#ifndef POP_NODE_H
#define POP_NODE_H
#include "typedef.h"

struct PopNode {
    val_t rmvd;
    timestamp_t call;
    //std::vector<index_t> pre;
    bool active;
};

struct PopEmptyNode {
    timestamp_t call;
    bool ok = false;
    PopEmptyNode(timestamp_t call) : call(call) {}
};

#endif
