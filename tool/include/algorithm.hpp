#ifndef ALGORITHM_H_
#define ALGORITHM_H_
#include "monitor.hpp"
#include "monitor/persistent.hpp"
#include "monitor/graphmonitor.hpp"
#include "monitor/data_monitor.hpp"

enum Algorithm : int {
interval,
segment,
cover,
data_monitor,
};

#define DefaultAlgorithm Algorithm::interval

inline Monitor *make_monitor(Algorithm alg, MonitorConfig mc) {
  if(alg == segment)
    return new GraphMonitor(mc);
  if(alg == cover)
    return new PersistentMonitor(mc);
  if(alg == data_monitor) {
    return new DataMonitor(mc);
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
  else if (token == "data")
    alg = data_monitor;
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
    case data_monitor:
      os << "data";
      break;
  }
  return os;
}


#endif // ALGORITHM_H_
