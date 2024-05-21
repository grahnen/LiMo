#ifndef SET_MONITOR_H
#define SET_MONITOR_H
#include "interval.h"
#include "monitor.hpp"
#include <ostream>


class ProjectionMonitor {
  
 public:
  // Sorted by increasing return time
  std::set<AtomicInterval> adds;
  // Sorted by increasing return time
  std::set<AtomicInterval> rmvs;


  Interval may, in, out;

  std::set<AtomicInterval> require_present, require_absent;

  void add_add(AtomicInterval i, bool ret);
  void add_rmv(AtomicInterval i, bool ret);
  void add_ctn(AtomicInterval i, bool ret);

  void do_linearization();
};

std::ostream &operator<<(std::ostream &os, const ProjectionMonitor &m);

class SetMonitor : public Monitor {
  std::map<val_t, ProjectionMonitor> monitors;
  std::map<tid_t, event_t> active;
 public:
  SetMonitor(std::ostream *os);

  DECLHANDLER(add)
  DECLHANDLER(rmv)
  DECLHANDLER(ctn)


  void do_linearization();
  void print_state() const;
  
};

#endif
