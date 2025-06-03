#ifndef DURABLE_STACK_H_
#define DURABLE_STACK_H_

#include "event.h"
#include "monitor.hpp"
#include "coverhistory.h"
#include "typedef.h"


class DurableStackMonitor : public Monitor {

protected:
  index_t height = 0;
  using LinRes = CoverHistory::LinRes;
  CoverHistory history;
  DECLHANDLER(push)
  DECLHANDLER(pop)
  void handle_crash(event_t &e);
public:
  void do_linearization();
  DurableStackMonitor(MonitorConfig mc);
  void print_state() const;
  bool ADT_supported(ADT adt) { return adt == stack; }
};

#endif // DURABLE_STACK_H_
