#include "impl.h"
#include <atomic>


class HWQueue {
    int size;  // A lot of pointers..
    std::atomic<val_t *> *items;
    std::atomic<int> loc;
public:
    HWQueue(int size) : size(size), loc(0) {
        items = new std::atomic<val_t *>[size];
        for(int i = 0; i < size; i++) {
            items[i] = nullptr;
        }
    }

    ~HWQueue() {
        delete[] items;
    }

    void enq(val_t itm) {
        int idx = loc.fetch_add(1);
        val_t *v = new val_t(itm);
        items[idx].store(v);
    }

    optval_t deq() {
        while(true) {
            for(int i = 0; i < size; i++) {
                val_t *v_i = items[i].exchange(nullptr);
                if(v_i != nullptr) {
                    val_t v = *v_i;
                    delete v_i;
                    return v;
                }
            }
        }
    }
};

ADTImpl::ADTImpl(tid_t num_threads) : adt(queue) {
    state = new HWQueue(30000);
}

ADTImpl::~ADTImpl() {
    delete (HWQueue*) state;
}

void ADTImpl::add(val_t v, tid_t thread) {
    HWQueue *q = (HWQueue *) state;
    q->enq(v);
}

res_t ADTImpl::rmv(tid_t thread) {
    HWQueue *q = (HWQueue *) state;
    return q->deq();
}
