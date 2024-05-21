#ifndef Stack_MONITOR_H
#define Stack_MONITOR_H
#include "monitor.h"
#include "stackhistory.h"

class StackMonitor : public Monitor {
protected:
  using LinRes = StackHistory::LinRes;
  StackHistory history;
 public:
  void do_linearization();
  StackMonitor(std::ostream *os);
  void print_state() const;
};

#endif
