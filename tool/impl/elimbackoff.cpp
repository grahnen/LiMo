// Modified version of the implementation provided by the Scal Project.

// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

// Implementing the non-blocking lock-free stack from:
//
// D. Hendler, N. Shavit, and L. Yerushalmi. A scalable lock-free stack algorithm.
// In Proc. Symposium on Parallelism in Algorithms and Architectures (SPAA),
// pages 206–215. ACM, 2004.

#include "impl.h"

#include <inttypes.h>
#include <assert.h>
#include <atomic>

#define EMPTY 0

struct TaggedValue {
  uint64_t tag_;
  uint64_t val_;

  val_t val() {
    return *(val_t *)val_;
  }

  uint64_t tag() { return tag_; }
};

enum Opcode {
  OPush = 1,
  OPop  = 2
};

struct Operation {
  Opcode opcode;
  res_t data;
};

struct Node {
  explicit Node(val_t item) : next(NULL), data(item) { }

  Node* next;
  val_t data;
};

class EliminationBackoffStack {
 public:
  EliminationBackoffStack(uint64_t num_threads, uint64_t size_collision,
      uint64_t delay);
  bool push(tid_t thread, res_t item);
  res_t pop(tid_t thread);

 private:
  using AtomicNodePtr = std::atomic<Node *>;

  AtomicNodePtr* top_;
  Operation* *operations_;

  std::atomic<uint64_t> *location_;
  std::atomic<uint64_t> *collision_;
  const uint64_t num_threads_;
  const uint64_t size_collision_;
  const uint64_t delay_;

  bool try_collision(uint64_t thread_id, uint64_t other, res_t *item);
  bool backoff(tid_t thread, Opcode opcode, res_t *item);

};

EliminationBackoffStack::EliminationBackoffStack(uint64_t num_threads, uint64_t size_collision, uint64_t delay)
  : num_threads_(num_threads), size_collision_(size_collision), delay_(delay) {

  top_ = new AtomicNodePtr();

  operations_ = new Operation*[num_threads];
  location_ = new std::atomic<uint64_t>[num_threads];
  collision_ = new std::atomic<uint64_t>[num_threads];

  for(int i = 0; i < num_threads; i++) {
    operations_[i] = new Operation();
    location_[i].store(std::atomic<uint64_t>(0));
    collision_[i].store(std::atomic<uint64_t>(0));
  }
}

inline uint64_t get_pos(uint64_t size) {
  return rand() % size;
}

bool EliminationBackoffStack::try_collision(
    uint64_t thread_id, uint64_t other, res_t *item) {
  TaggedValue old_value(other, 0);
  if (operations_[thread_id]->opcode == Opcode::OPush) {
    TaggedValue new_value(thread_id, 0);
    if (location_[other].compare_exchange_weak(
          other, thread_id)) {
      return true;
    } else {
      return false;
    }
  } else {
    TaggedValue new_value({}, 0);
    if (location_[other].compare_exchange_weak(
          other, EMPTY)) {
      *item = operations_[other]->data;
      return true;
    } else {
      return false;
    }
  }
}

bool EliminationBackoffStack::backoff(tid_t thread_id, Opcode opcode, res_t *item) {
  operations_[thread_id]->opcode = opcode;
  operations_[thread_id]->data = *item;
  location_[thread_id].store(thread_id);
  uint64_t position = get_pos(size_collision_);
  uint64_t him = collision_[position].load();
  while (!collision_[position].compare_exchange_weak(him, thread_id)) {
  }
  if (him != EMPTY) {
    uint64_t other = location_[him].load();
    if (other == him && operations_[other]->opcode != opcode) {
      uint64_t expected = thread_id;
      if (location_[thread_id].compare_exchange_weak(expected, EMPTY)) {
        if (try_collision(thread_id, other, item)) {
         return true;
        } else {
          return false;
        }
      } else {
        if (opcode == Opcode::OPop) {
          *item = operations_[location_[thread_id].load()]->data;
          location_[thread_id].store(0);
        }
        return true;
      }
    }
  }

  // // Wait some time for collisions.
  // uint64_t wait = get_hwtime() + delay_;
  // while (get_hwtime() < wait) {
  //   __asm__("PAUSE");
  // }

  uint64_t expected = thread_id;
  if (!location_[thread_id].compare_exchange_strong(expected, EMPTY)) {
    if (opcode == Opcode::OPop) {
      *item = operations_[location_[thread_id].load()]->data;
      location_[thread_id].store(EMPTY);
    }
    return true;
  }

  return false;
}

bool EliminationBackoffStack::push(tid_t thread, res_t item) {
  if (backoff(thread, Opcode::OPush, &item)) {
    return true;
  }
  Node *n = new Node(item.value());
  Node *top_old;
  Node *top_new;
  while (true) {
    top_old = top_->load();
    n->next = top_old;
    top_new = n;

    if (!top_->compare_exchange_strong(top_old, top_new)) {
      if (backoff(thread, Opcode::OPush, &item)) {
        return true;
      }
    } else {
      break;
    }
  }
  return true;
}

res_t EliminationBackoffStack::pop(tid_t thread) {
  res_t *emp = new res_t({});
  if (backoff(thread, Opcode::OPop, emp)) {
    return true;
  }
  Node *top_old;
  Node *top_new;
  while (true) {
    top_old = top_->load();
    if (top_old == nullptr) {
      return false;
    }
    top_new = top_old->next;

    if (!top_->compare_exchange_strong(top_old, top_new)) {
      if (backoff(thread, Opcode::OPop, emp)) {
        res_t r = *emp;
        delete emp;
        return r;
      }
    } else {
      break;
    }
  }
  *emp = top_old->data;
  res_t r = emp->value();
  delete emp;
  return r;
}


ADTImpl::ADTImpl(tid_t threads) {
  state = (void *) new EliminationBackoffStack(threads, threads / 2, 10);
}

ADTImpl::~ADTImpl(){
  delete (EliminationBackoffStack *)state;
}

void ADTImpl::add(val_t v, tid_t thread) {
  EliminationBackoffStack *s = (EliminationBackoffStack *)state;
  s->push(thread, v);
}

res_t ADTImpl::rmv(tid_t thread) {
  EliminationBackoffStack *s = (EliminationBackoffStack *)state;
  return s->pop(thread);
}
