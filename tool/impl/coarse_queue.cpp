#include "impl.h"
#include <mutex>
#include <vector>
#include <cassert>

struct node { val_t v; node *next; };

struct state_t {
    node *head = nullptr;
    node *tail = nullptr;
    std::mutex mut;

    state_t() : head(nullptr), tail(nullptr), mut() {
        node *dummy = new node(0, nullptr);
        head = dummy;
        tail = dummy;
    }
};

res_t ADTImpl::rmv(tid_t t) {
  state_t *s = (state_t *) this->state;
  s->mut.lock();
  node *head = s->head;
  node *tail = s->tail;
  node *next = head->next;

  if(head == tail) {
      s->mut.unlock();
      return {};
  }

  val_t vl = next->v;
  s->head = next;
  delete head;

  s->mut.unlock();
  return vl;
}

void ADTImpl::add(val_t v, tid_t t) {
  state_t *s = (state_t *) this->state;
  s->mut.lock();

  node *n = new node(v, nullptr);

  node *tail = s->tail;
  node *next = tail->next;
  assert(next == nullptr);
  tail->next = n;
  s->tail = n;
  s->mut.unlock();
}

ADTImpl::ADTImpl(tid_t threads) {
    adt = queue;
    state = new state_t;
}

ADTImpl::~ADTImpl() {
    state_t *S = (state_t *)state;
    node *n = S->head;
    while(n != nullptr) {
        node *next = n->next;
        delete n;
        n = next;
    }
}
