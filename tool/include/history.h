#ifndef HISTORY_H
#define HISTORY_H
#include "util.h"
#include "operation.h"
#include <functional>

template <typename T>
class History {
public:
  using LinRes = LinearizationResult<T>;
  virtual LinRes step() = 0;
  virtual void add_push_call(event_t &call) = 0;
  virtual void add_pop_call(event_t &call) = 0;
  virtual LinRes add_ret(event_t &ret, bool crash = false) = 0;
  virtual bool complete() const = 0;
};

#endif
