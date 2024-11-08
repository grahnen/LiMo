#ifndef COVER_H
#define COVER_H
#include "util.h"
#include "operation.h"
#include <ostream>
class Interval;

struct AtomicInterval {
  timestamp_t lbound;
  timestamp_t ubound;
  bool l_open;
  bool u_open;
  AtomicInterval() : AtomicInterval(true, POSINF, NEGINF, true) {}
  AtomicInterval(bool left, timestamp_t lower, timestamp_t upper, bool right)
      : l_open(left), lbound(lower), ubound(upper), u_open(right) {}
  
  AtomicInterval intersect(AtomicInterval &other) {
    timestamp_t lb, ub;
    bool lop, uop;
    if (lbound == other.lbound) {
      lb = lbound;
      lop = l_open || other.l_open;
    }
    if (lbound < other.lbound) {
      lb = other.lbound;
      lop = other.l_open;
    }
    if (other.lbound < lbound) {
      lb = lbound;
      lop = l_open;
    }

    if (ubound == other.ubound) {
      uop = u_open || other.u_open;
      ub = ubound;
    }
    if (ubound < other.ubound) {
      ub = ubound;
      uop = u_open;
    }
    if (other.ubound < ubound) {
      ub = other.ubound;
      uop = other.u_open;
    }

    return AtomicInterval(lop, lb, ub, uop);
  }

  bool overlaps(AtomicInterval const &other) const {
    if(ubound < other.lbound || lbound > other.ubound)
      return false;

    if(ubound == other.lbound)
      return !(u_open && other.l_open);
    if(lbound == other.ubound)
      return !(l_open && other.l_open);
    
    return true;
  }

  bool contains(const AtomicInterval &o) const;
  bool contains(const timestamp_t &ts) const;

  Interval complement() const;
  Interval operator+(const AtomicInterval &o) const;

  AtomicInterval operator*(const AtomicInterval &o) const;
  
  inline bool empty() const {
    return lbound > ubound || (lbound >= ubound && (u_open || l_open));
  }

  inline static AtomicInterval nil() {
    return AtomicInterval(false, POSINF, NEGINF, false);
  }

  inline static AtomicInterval complete() {
    return AtomicInterval(false, NEGINF, POSINF, false);
  }

  inline bool is_complete() {
    return lbound == NEGINF && ubound == POSINF;

  }

  inline static AtomicInterval end() {
    return AtomicInterval::closed(POSINF - 2, POSINF);
  }

  bool preceeds(const AtomicInterval &R) const {
    if (ubound == R.lbound) {
      if(u_open)
        return true;
      return R.l_open;
    }
    return ubound < R.lbound;
  }

  friend bool operator<(const AtomicInterval &L, const AtomicInterval &R) {
    if(L.lbound == R.lbound)
      return L.ubound < R.ubound;
    return L.lbound < R.lbound;
  }

  static AtomicInterval open(timestamp_t lbound, timestamp_t ubound);
  static AtomicInterval closed(timestamp_t lbound, timestamp_t ubound);
  static AtomicInterval openclosed(timestamp_t, timestamp_t);
  static AtomicInterval closedopen(timestamp_t, timestamp_t);
};

constexpr bool operator==(const AtomicInterval &L, const AtomicInterval &R) {
  return L.lbound == R.lbound && L.ubound == R.ubound && L.l_open == R.l_open && L.u_open == R.u_open;
}

class Interval {
 public:
  Interval();
  Interval(std::vector<AtomicInterval>, bool sorted = true);
  static Interval nil();
  static Interval open(timestamp_t lbound, timestamp_t ubound);
  static Interval closed(timestamp_t lbound, timestamp_t ubound);
  static Interval openclosed(timestamp_t, timestamp_t);
  static Interval closedopen(timestamp_t, timestamp_t);

  bool left_open();
  bool right_open();
  timestamp_t lower();
  timestamp_t upper();
  bool empty();
  bool atomic();
  bool overlaps(const Interval &o) const;
  
  bool operator==(const Interval &o) const {
    if (o.intervals.size() == intervals.size()) {
      for (int i = 0; i < intervals.size(); i++) {
	if(o.intervals[i] != intervals[i])
	  return false;
      }
      return true;
    }
    return false;
  }

  /* Union of intervals */
  Interval operator+(const Interval &o) const;
  Interval operator+(const AtomicInterval &o) const;
  /* Intersection of intervals */
  Interval operator*(const Interval &o) const;
  Interval operator*(const AtomicInterval &o) const;
  
  Interval complement() const;
  AtomicInterval bounds() const;
  std::vector<AtomicInterval> atoms() const {
    return intervals;
  }

private:
  std::vector<AtomicInterval> intervals; 
};

std::ostream &operator<<(std::ostream &os, const AtomicInterval &I);
std::ostream &operator<<(std::ostream &os, const Interval &I);

AtomicInterval before(AtomicInterval a, AtomicInterval b);
AtomicInterval after(AtomicInterval a, AtomicInterval b);

#endif
