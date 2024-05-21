#include "monitor/covermonitor.h"
#include "interval.h"
#include <cassert>
#include <iostream>
#include "exception.h"

CoverMonitor::CoverMonitor(MonitorConfig mc) : Monitor(mc) {}

void CoverMonitor::handle_push(event_t &e) {
  history.add_push_call(e);
}

void CoverMonitor::handle_ret_push(event_t &e) {
  history.add_ret(e);
}

void CoverMonitor::handle_pop(event_t &e) {
  history.add_pop_call(e);
}

void CoverMonitor::handle_ret_pop(event_t &e) {
  history.add_ret(e);
}

void CoverMonitor::print_state() const {
  *output << history << std::endl;
}

void CoverMonitor::do_linearization() {
  if (!history.sound()) {
    throw Violation("Unmatched Pop");
  }
  if (!history.simple()) {
    throw std::runtime_error("Simplified history is still not simple.");
  }
  if(verbose)
    *output << "Linearizing: " << history << std::endl;
  LinRes lr = history.step();
  while (!lr.violation() && lr.remaining.size() > 0) {
    auto oa = lr.remaining.back();
    lr.remaining.pop_back();
    if (verbose) {
      *output << "Linearizing: " << oa << std::endl;
    }
    LinRes lr2 = oa.step();
    lr = lr + lr2;
  }
  if (lr.violation()) {
    throw Violation(lr.error());
  }
}
