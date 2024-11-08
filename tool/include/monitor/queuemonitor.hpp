#ifndef Stack_MONITOR_H
#define Stack_MONITOR_H
#include "interval.h"
#include "monitor.hpp"
#include "typedef.h"
#include "util.h"
#include "intervaltree.hpp"


class QueueMonitor : public Monitor {
protected:
  using LinRes = LinearizationResult<bool>;
  std::vector<AtomicInterval> outers;

  std::map<val_t, AtomicInterval> inner;
  std::map<val_t, AtomicInterval> outer;

  std::map<tid_t, val_t> active;
  ItvTree queue_tree;

  DECLHANDLER(enq)
  DECLHANDLER(deq)

  void add_val(val_t v);
  void ensure_member(val_t v);
public:
  void do_linearization();
  QueueMonitor(MonitorConfig mc);
  void print_state() const;
};

#endif
