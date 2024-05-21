#ifndef STACKHISTORY_H
#define STACKHISTORY_H

#include "history.h"
#include "util.h"
#include "interval.h"
#include <map>

struct CoverVal {
  val_t val;
  AtomicInterval add;
  mutable AtomicInterval rmv;
  mutable int push_era = 0;
  mutable int pop_era = 0;
  mutable bool push_crash = false;
  mutable bool pop_crash = false;

  CoverVal(val_t val, AtomicInterval add) : CoverVal(val, add, AtomicInterval::end()) {}
  CoverVal(val_t val, AtomicInterval add, AtomicInterval rmv)
      : val(val), add(add), rmv(rmv) {}
  AtomicInterval cover() const {
    return AtomicInterval::closed(add.ubound, rmv.lbound);
  }
  
  AtomicInterval bound() const {
    return AtomicInterval::closed(add.lbound, rmv.ubound);
  }

  bool operator<(const CoverVal &o) const {
    return add.ubound < o.add.ubound;
  }
};

class CoverHistory : public History<CoverHistory> {
public:
  std::set<CoverVal> values;
  std::vector<AtomicInterval> empties;

    int era;
  /* Used during initial read */
  std::map<tid_t, event_t> open;
  std::map<val_t, AtomicInterval> unmatched_removes;

  CoverHistory(std::set<CoverVal> values) : values(values) {}
  CoverHistory() {}

  void add_value(CoverVal op);
  std::vector<CoverHistory> add_branch(CoverVal op);
  void add_empty(AtomicInterval op);
  std::vector<CoverHistory> split(AtomicInterval i);

  using IIter = std::vector<AtomicInterval>::iterator;
  std::vector<CoverHistory> segmentize(IIter begin, IIter end);
  Last last() const;
  std::set<CoverVal> *minmax() const;
  bool minmax_prune();
  bool prune_crash();
  Interval cover() const;
  AtomicInterval cover_bounds() const;

  std::vector<CoverHistory> guesses();
  
  bool sound() const { return unmatched_removes.size() == 0; }
  bool simple() const {
    for (auto v : values) {
      if (v.add.overlaps(v.rmv))
	return false;
    }
    return true;
  }

  void simplify();


  /* Implementations of superclass History */
  void add_push_call(event_t &call);
  void add_pop_call(event_t &call);
  void add_crash(event_t &crash);
  LinRes add_ret(event_t &ret, bool crash = false);
  LinRes step();
  LinRes check_durable();
  bool complete() const;
  
  friend std::ostream &operator<<(std::ostream &os, const CoverHistory &h);
};

#endif
