#include "impl.h"
#include <mutex>
#include <vector>
#include <cassert>

struct state_t {
  std::vector<val_t> vec;
  std::mutex mut;
};

res_t ADTImpl::rmv(tid_t t) {
  state_t *s = (state_t *) this->state;
  s->mut.lock();
  if(s->vec.empty()) {
    s->mut.unlock();
    return {};
  }
  res_t r(s->vec.back());
  s->vec.pop_back();
  s->mut.unlock();
  return r;
}

void ADTImpl::add(val_t v, tid_t t) {
  state_t *s = (state_t *) this->state;
  s->mut.lock();
  s->vec.push_back(v);
  s->mut.unlock();
}

ADTImpl::ADTImpl(tid_t threads) {
  state = new state_t;
}

ADTImpl::~ADTImpl() {}
