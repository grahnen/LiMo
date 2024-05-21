#include "impl.h"
#include <atomic>
#include <chrono>

#define CAS(ptr,a,b) ptr.compare_exchange_strong(a, b)

using Timeout = std::runtime_error;

using clk = std::chrono::steady_clock;
using namespace std::chrono_literals;

struct Node {
    val_t value;
    std::atomic<Node *> next;

    Node(val_t v) : value(v), next(nullptr) {}
};

const int EMPTY = 0;
const int WAITING = 1;
const int BUSY = 2;

struct StampedReference {
    res_t *val;
    int stamp;

    static StampedReference Empty() { return StampedReference(nullptr, EMPTY); }
};


struct Exchanger {

    std::atomic<StampedReference> slot;

    res_t exchange(res_t y, int timeout) {
        auto W = clk::now() + (timeout * 1ms);
        while(clk::now() < W) {
            StampedReference x = slot.load();
            switch(x.stamp) {
                case EMPTY: {
                    res_t *yptr = new res_t(y);
                    StampedReference sr(yptr, WAITING);
                    if(addA(x, sr)) {
                        while(clk::now() < W) {
try_match:
                            res_t *rx = new res_t({});
                            if(removeB(rx)) {
                                res_t r = *rx;
                                delete rx;
                                return r;
                            }
                        }
                        // Time out, try reset slot
                        if(CAS(slot, sr, StampedReference::Empty())) {
                            throw Timeout("Timeout");
                        } else {
                            // We got a match before managing to leave!
                            goto try_match;
                        }
                    }
                    break;
                }
                case WAITING: {
                    res_t *yptr = new res_t(y);
                    StampedReference sr(yptr, BUSY);
                    if(addB(x, sr)) {
                        delete yptr;
                        res_t v = *x.val;
                        delete x.val;
                        return v;
                    }
                    break;
                }
                case BUSY:
                default:
                    break;
            }
        }
        throw Timeout("Timeout");
    }

    bool addA(StampedReference x, StampedReference y) {
        return CAS(slot, x, y);
    }

    bool addB(StampedReference x, StampedReference y) {
        return CAS(slot, x, y);
    }


    bool removeB(res_t *r) {
        StampedReference sr = slot.load();
        if(sr.stamp != BUSY) {
            return false;
        }

        slot.store(StampedReference(nullptr, EMPTY));
        *r = *sr.val;
        return true;
    }



};

struct ElimArray {
    Exchanger *exchangers;
    int capacity, timeout;
    ElimArray(int capacity, int timeout) : capacity(capacity), timeout(timeout) {
        exchangers = new Exchanger[capacity];
    }

    ~ElimArray() {
        delete[] exchangers;
    }

    res_t visit(res_t arg) {
        int i = rand() % capacity;
        return exchangers[i].exchange(arg, timeout);
    }
};

struct State {
    std::atomic<Node *> top;
    ElimArray *elim;

    const int capacity = 8;
    const int timeout = 2;

    State() {
        top = nullptr;
        elim = new ElimArray(capacity, timeout);
    }

    ~State() {
        delete elim;
        Node *t = top.load();
        while(t != nullptr) {
            Node *tp = t->next;
            delete t;
            t = tp;
        }
    }

    void push( val_t v ) {
        Node *n = new Node(v);
        while(true) {
            if(try_push(n))
                return;
            try {
                res_t x = elim->visit(v);
                if(!x.has_value()) {
                    delete n;
                    return; // Found matching pop
                }
            } catch (Timeout &t) {
                // Restart attempt
            }
        }
    }

    res_t pop() {
        while(true) {
            Node *n = nullptr;
            if(try_pop(&n)) {
                if(n == nullptr) return {};
                res_t v = n->value;
                delete n;
                return v;
            }
            try {
                res_t attempt = elim->visit({});
                if(attempt.has_value())
                    return attempt; // Found matching push
            } catch( Timeout &t ) {
                // Restart attempt
            }
        }
    }


    bool try_push( Node *input ) {
        Node *m = top.load();
        input->next = m;
        return CAS(top, m, input);

    }

    bool try_pop ( Node **output ) {
        *output = top.load();
        if( *output == nullptr ) {
            return true;
        }
        Node *n = (*output)->next;
        if(CAS(top, *output, n)) {
            return true;
        } else {
            *output = nullptr; //compare_exchange_weak loads on failure...
            return false;
        }
    }
};


ADTImpl::~ADTImpl(){
    State *S = (State *) state;
    delete S;
}

ADTImpl::ADTImpl(tid_t num_threads) {
    state = new State();
}

void ADTImpl::add(val_t v, tid_t thread) {
    State *S = (State *)state;
    S->push(v);
}

res_t ADTImpl::rmv(tid_t thread) {
    State *S = (State *)state;
    return S->pop();
}
