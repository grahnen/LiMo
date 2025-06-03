#include "monitor/durable_stack.hpp"
#include <cassert>
#include <iostream>
#include "exception.h"
#include "typedef.h"

DurableStackMonitor::DurableStackMonitor(MonitorConfig mc) : Monitor(mc) {
  if(!(mc.type & (durable_stack | stack))) {
        throw std::logic_error("Unhandled ADT: " + ext2str(mc.type));
    }
}

void DurableStackMonitor::handle_push(event_t &e) {
  //history.add_push_call(e);
}

void DurableStackMonitor::handle_ret_push(event_t &e) {
  //history.add_ret(e);
}

void DurableStackMonitor::handle_pop(event_t &e) {
  //history.add_pop_call(e);
}

void DurableStackMonitor::handle_ret_pop(event_t &e) {
  //history.add_ret(e);
}

void DurableStackMonitor::print_state() const {
  *output << history << std::endl;
}

void DurableStackMonitor::handle_crash(event_t &e) {

}

void DurableStackMonitor::do_linearization() {
  // if (!history.sound()) {
  //   throw Violation("Unmatched Pop");
  // }
  // if (!history.simple()) {
  //   throw std::runtime_error("Simplified history is still not simple.");
  // }
  // if(verbose)
  //   *output << "Linearizing: " << history << std::endl;
  // LinRes lr = history.step();
  // while (!lr.violation() && lr.remaining.size() > 0) {
  //   auto oa = lr.remaining.back();
  //   lr.remaining.pop_back();
  //   if (verbose) {
  //     *output << "Linearizing: " << oa << std::endl;
  //   }
  //   LinRes lr2 = oa.step();
  //   lr = lr + lr2;
  // }
  // if (lr.violation()) {
  //   throw Violation(lr.error());
  // }
}
