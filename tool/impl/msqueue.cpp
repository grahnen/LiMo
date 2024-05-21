#include "impl.h"
#include <atomic>
#include <iostream>

struct node {
    val_t data;
    std::atomic<node *> next;
};

struct state_t {
    std::atomic<node *> head, tail;
};

ADTImpl::ADTImpl( tid_t threads ) : adt(queue) {
    node *dummy = new node { .data = val_t(), .next = nullptr };
    state_t *s = new state_t { .head = dummy, .tail = dummy };
    state = s;
}

ADTImpl::~ADTImpl() {
    state_t *S = (state_t *) state;
    node * n = S->head;
    while(n != nullptr) {
        node *np = n->next;
        delete n;
        n = np;
    }
}

void ADTImpl::add(val_t v, tid_t thread) {
    state_t *S = (state_t *) state;
    node *n = new node { .data =v, .next = nullptr };

    node *tail, *next;

    while(true) {
        tail = S->tail;
        next = tail->next;
        if(tail == S->tail) {
            if(next == nullptr) {
                if( tail->next.compare_exchange_weak(next, n) ) {
                    S->tail.compare_exchange_weak(tail, n);
                    return;
                }
            } else {
                S->tail.compare_exchange_weak(tail, next);
            }
        }
    }
}

optval_t ADTImpl::rmv(tid_t thread) {
    state_t *S = (state_t *)state;
    node *head, *tail, *next;

    val_t vl;
    while(true) {
        head = S->head;
        tail = S->tail;
        next = head->next;
        if(head == S->head) {
            if(head == tail) {
                if (next == nullptr) {
                    return {};
                }
                S->tail.compare_exchange_weak(tail, next);
            }  else {
                vl = next->data;
                if(S->head.compare_exchange_weak(head, next)) {
                    delete head;
                    return vl;
                }
            }
        }
    }
}
