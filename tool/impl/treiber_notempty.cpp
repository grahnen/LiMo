#include "impl.h"
#include <atomic>
#include <iostream>

struct node_t {
  std::atomic<node_t *> next;
  val_t val;
  node_t(val_t v) : val(v), next(nullptr) {}
};

struct state_t {
  std::atomic<node_t *> head;
};

res_t ADTImpl::rmv(tid_t t) {
  state_t *d = (state_t *) this->state;

  node_t *oh;
  node_t *nh;
  do {
    do {
      oh = d->head;
    } while (oh == nullptr);

    nh = oh->next;
  } while(!d->head.compare_exchange_weak(oh,nh));

  return oh->val;
}

void ADTImpl::add(val_t v, tid_t t) {
  state_t *d = (state_t *) this->state;
  node_t *oh;
  node_t *nh = new node_t(v);
  do {
    oh = d->head;
    nh->next = oh;
  } while(!d->head.compare_exchange_weak(oh, nh));
}

ADTImpl::ADTImpl(tid_t threads) {
  state = new state_t{
    .head = nullptr
  };
}

ADTImpl::~ADTImpl() {
  state_t *s = (state_t *)state;
  node_t *n = (node_t *) s->head;
  while (n != nullptr) {
    node_t *tmp = n;
    n = n->next;
    delete tmp;
  }
}
