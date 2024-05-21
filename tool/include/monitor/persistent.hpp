#ifndef Pers_MONITOR_H
#define Pers_MONITOR_H
#include "monitor.hpp"
#include "coverhistory.h"

class PersistentMonitor : public Monitor {
protected:
  using LinRes = CoverHistory::LinRes;
  CoverHistory history;
  DECLHANDLER(push)
  DECLHANDLER(pop)
  void handle_crash(event_t &ev);
public:
  void do_linearization();
  PersistentMonitor(MonitorConfig mc);
  void print_state() const;
};

#endif
