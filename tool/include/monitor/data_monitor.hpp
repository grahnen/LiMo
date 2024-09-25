#ifndef DATA_MONITOR_H_
#define DATA_MONITOR_H_

#include "interval.h"
#include "monitor.hpp"
#include "interval.h"
#include <algorithm>

struct state_t {
  int must_be_present = 0;
  int may_be_present = 0;

  bool operator==(state_t &o) {
    return must_be_present == o.must_be_present && may_be_present == o.may_be_present;
  }
};

inline std::ostream &operator<<(std::ostream &os, state_t s) {
  return os << "[" << s.must_be_present << s.may_be_present << "]";
}



struct opdata_t {
  state_t pushcall, pushret, popcall, popret;

  AtomicInterval bounds() {
    AtomicInterval push = AtomicInterval::closed(std::min(pushcall.must_be_present, pushret.must_be_present), std::max(pushcall.may_be_present, pushret.may_be_present));
    AtomicInterval pop = AtomicInterval::closed( std::min(popcall.must_be_present, popret.must_be_present), std::max(popcall.may_be_present, popret.may_be_present));

    return push.intersect(pop);

  }
};

inline std::ostream &operator<<(std::ostream &os, opdata_t s) {
  return os << s.pushcall << s.pushret << s.popcall << s.popret;
}

using opvec = std::vector<std::vector<opdata_t>>;

class DataMonitor : public Monitor {
protected:
    std::vector<std::pair<val_t, state_t>> open;
    state_t state;
    opvec history;
  DECLHANDLER(push)
  DECLHANDLER(pop)

  void ensure_size(tid_t ethread, tid_t vthread, index_t idx);
  std::vector<AtomicInterval> collect_bounds();
public:
  void do_linearization();
  DataMonitor(MonitorConfig mc);
  void print_state() const;
};



#endif // DATA_MONITOR_H_
