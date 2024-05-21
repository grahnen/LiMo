#ifndef IMPL_H
#define IMPL_H
#include <optional>
#include "typedef.h"

class ADTImpl {
 protected:

  void *state;
 public:
  ADT adt = stack;
  ADTImpl(tid_t num_threads);
  ~ADTImpl();
  res_t rmv(tid_t thread);
  void add(val_t v, tid_t thread);
};

#endif
