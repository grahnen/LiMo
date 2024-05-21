#include "monitor/persistent.hpp"

#include "monitor/covermonitor.h"
#include "interval.h"
#include <cassert>
#include <iostream>
#include "exception.h"

PersistentMonitor::PersistentMonitor(MonitorConfig mc) : Monitor(mc) {}

void PersistentMonitor::handle_crash(event_t &e) {
    history.add_crash(e);
}

void PersistentMonitor::handle_push(event_t &e) {
  history.add_push_call(e);
}

void PersistentMonitor::handle_ret_push(event_t &e) {
  history.add_ret(e);
}

void PersistentMonitor::handle_pop(event_t &e) {
  history.add_pop_call(e);
}

void PersistentMonitor::handle_ret_pop(event_t &e) {
  history.add_ret(e);
}

void PersistentMonitor::print_state() const {
  *output << history << std::endl;
}

void PersistentMonitor::do_linearization() {
  if (!history.sound()) {
    throw Violation("Unmatched Pop");
  }
  if (!history.simple()) {
    throw std::runtime_error("Simplified history is still not simple.");
  }


  if(verbose)
    *output << "Linearizing: " << history << std::endl;

  history.prune_crash();

  LinRes lr = history.check_durable();

  if(lr.violation())
      throw Violation("Unlinearizable" + ext2str(lr.error()));

}
