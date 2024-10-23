
#ifndef ALGORITHM_H_
#define ALGORITHM_H_
#include "monitor.hpp"
#include "monitor/covermonitor.h"
#include "monitor/graphmonitor.hpp"
#include "monitor/treemonitor.hpp"

enum Algorithm : int {
interval,
segment,
cover,
tree_monitor,
};

#define DefaultAlgorithm Algorithm::interval

inline Monitor *make_monitor(Algorithm alg, MonitorConfig mc) {
  if(alg == segment)
    return new GraphMonitor(mc);
  if(alg == cover)
    return new CoverMonitor(mc);
  if(alg == tree_monitor) {
    return new TreeMonitor(mc);
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
