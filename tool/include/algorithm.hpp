
#ifndef ALGORITHM_H_
#define ALGORITHM_H_
#include "monitor.hpp"
#include "monitor/covermonitor.h"
#include "monitor/durable_stack.hpp"
#include "monitor/graphmonitor.hpp"
#include "monitor/naive_durable_stack.hpp"
#include "monitor/queuemonitor.hpp"
#include "typedef.h"

enum Algorithm : int {
undefined,
interval,
segment,
cover,
tree_monitor,
dstack,
naive_dstack,
};

#define DefaultAlgorithm Algorithm::undefined

inline Monitor *make_default(MonitorConfig mc) {
  switch(mc.type) {
    case ADT::queue:
      return (Monitor *) new QueueMonitor(mc);
    case ADT::stack:
      return (Monitor *) new CoverMonitor(mc);
    case ADT::durable_stack:
      return (Monitor *) new DurableStackMonitor(mc);
    case ADT::durable_queue:
      return nullptr;
  }
  return nullptr;
}

inline Monitor *make_monitor(Algorithm alg, MonitorConfig mc) {
  if(alg == undefined) {
    return make_default(mc);
  }
  if(alg == segment)
    return (Monitor *) new GraphMonitor(mc);
  if(alg == cover) {
    if(mc.type == ADT::stack)
      return (Monitor *) new CoverMonitor(mc);
    if(mc.type == ADT::queue)
      return (Monitor *) new QueueMonitor(mc);
  }
  if(alg == dstack)
    return (Monitor *) new DurableStackMonitor(mc);
  if(alg == naive_dstack)
    return (Monitor *) new NaiveDurableStackMonitor(mc);

  throw std::logic_error("Unknown algorithm!");
  return nullptr;
}

std::istream& operator>>(std::istream& in, Algorithm &alg) {
  std::string token;
  in >> token;
  if(token == "cover")
    alg = Algorithm::cover;
  else if (token == "interval")
    alg = interval;
  else if (token == "segment")
    alg = segment;
  else if (token == "tree")
    alg = tree_monitor;
  else if (token == "dstack")
    alg = dstack;
  else if (token == "naive_dstack")
    alg = naive_dstack;
  else
    in.setstate(std::ios_base::failbit);

  return in;
}

std::ostream &operator<<(std::ostream &os, Algorithm &alg) {
  switch(alg) {
    case interval:
      os << "interval";
      break;
    case segment:
      os << "segment";
      break;
    case cover:
      os << "cover";
      break;
    case tree_monitor:
      os << "tree";
      break;
  }
  return os;
}

#endif // ALGORITHM_H_
