#ifndef OP_H
#define OP_H
#include "event.h"
#include "typedef.h"
#include <ostream>


class Interval;

// Base class for operations
struct operation {
public:
  EType type;
  tid_t thread;
  timestamp_t inv, ret;
  optval_t value;

  operation(EType t, tid_t thread, timestamp_t inv, timestamp_t res, optval_t val);
  operation() : operation(EType::Enil, 0, POSINF, NEGINF, {}) {}
  std::string name() const;
  std::string label() const;
  Interval interval() const;
  bool operator==(const operation &o) {
    return type == o.type && thread == o.thread && inv == o.inv && ret == o.ret && value == o.value;
  }
};

class op_cmp {
 public:
  bool operator()(const operation *a, const operation *b) const {
    return a->inv < b->inv;
  }
};

#endif
