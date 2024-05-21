#ifndef Stack_MONITOR_H
#define Stack_MONITOR_H
#include "monitor.hpp"
#include "coverhistory.h"

class CoverMonitor : public Monitor {
protected:
  using LinRes = CoverHistory::LinRes;
  CoverHistory history;
  DECLHANDLER(push)
  DECLHANDLER(pop)
public:
  void do_linearization();
  CoverMonitor(MonitorConfig mc);
  void print_state() const;
};

#endif
