
#ifndef ALGORITHM_H_
#define ALGORITHM_H_
#include "monitor.hpp"
#include "monitor/covermonitor.h"
#include "monitor/graphmonitor.hpp"
#include "monitor/queuemonitor.hpp"

enum Algorithm : int {
undefined,
interval,
segment,
cover,
tree_monitor,
};

#define DefaultAlgorithm Algorithm::undefined

inline Monitor *make_default(MonitorConfig mc) {
  switch(mc.type) {
    case ADT::queue:
      return (Monitor *) new QueueMonitor(mc);
    case ADT::stack:
      return (Monitor *) new CoverMonitor(mc);
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
