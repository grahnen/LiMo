#include "interval.h"
#include "typedef.h"
#include <cassert>
#include <algorithm>
#include <iostream>
#include <vector>

std::string format_ts(timestamp_t ts) {
  if(ts == NEGINF)
    return "-∞";
  if(ts == POSINF)
    return "∞";
  return std::to_string(ts);
}

std::ostream &operator<<(std::ostream &os, const AtomicInterval &I) {
  if(I.empty())
    return os << "ø";
  return os << (I.l_open ? "(" : "⎣")
	    << format_ts(I.lbound)
	    << ","
	    << format_ts(I.ubound)
	    << (I.u_open ? ")" : "⎤");
}

std::ostream &operator<<(std::ostream &os, const Interval &I) {
  std::vector<AtomicInterval> inter = I.atoms();
  os << *inter.begin();
  for (auto i = inter.begin() + 1; i != inter.end(); i++) {
    os << " | ";
    os << *i;
  }
  return os;
}

bool Interval::atomic() {
  return intervals.size() == 1;
}

bool Interval::left_open() {
  return intervals[0].l_open;
}

bool Interval::right_open() {
  return intervals[intervals.size() - 1].u_open;
}

timestamp_t Interval::lower() {
  return intervals[0].lbound;
}

timestamp_t Interval::upper() {
  return intervals[intervals.size() - 1].ubound;
}

bool Interval::empty() {
  return lower() > upper()
    || (lower() == upper() && (left_open() || right_open()));
}

/* Two intervals overlap if any of its atomic subintervals overlap */
bool Interval::overlaps(const Interval &o) const {
  for (auto i : intervals) {
    for (auto j : o.intervals) {
      if (i.overlaps(j))
	return true;
    }
  }
  return false;
}

Interval::Interval() : Interval(std::vector<AtomicInterval>()) {}

/* Union of all intervals in vector */
Interval::Interval(std::vector<AtomicInterval> atoms, bool sorted) {
  for (auto ai : atoms) {
    if(!ai.empty())
      this->intervals.push_back(ai);
  }
  if (intervals.size() == 0) {
    intervals.push_back(AtomicInterval::nil());
  }
  
  // Sort intervals
  if(!sorted) {
  std::sort(intervals.begin(), intervals.end(),
            [](AtomicInterval &i, AtomicInterval &j) {
              return i.lbound < j.lbound;
	    });
  }

  // Merge intervals if possible
  int i = 0;
  while (i < intervals.size() - 1) {
    AtomicInterval &curr = intervals[i];
    AtomicInterval &succ = intervals[i+1];
    if (curr.overlaps(succ)) {
      // Since curr < succ, we know curr.lbound <= succ.lbound, closed if any of them are
      // Only change ubound
      if (curr.ubound > succ.ubound) {
        // Do nothing, only remove succ
        // succ is entirely inside ubound
      } else if (curr.ubound == succ.ubound) {
        curr.ubound = succ.ubound;
        curr.u_open = curr.u_open && succ.u_open;
      } else { // curr.ubound < succ.ubound
        curr.ubound = succ.ubound;
        curr.u_open = succ.u_open;
      }
      intervals.erase(intervals.begin() + i + 1);
    } else {
      i++;
    }
  }
}

Interval Interval::nil() {
  return Interval::open(POSINF, NEGINF);
}

Interval Interval::open(timestamp_t lbound, timestamp_t ubound) {
  return Interval({AtomicInterval::open(lbound, ubound)});
}

Interval Interval::closed(timestamp_t lbound, timestamp_t ubound) {
  return Interval({
      AtomicInterval::closed(lbound, ubound)
    });
}

Interval Interval::openclosed(timestamp_t lbound, timestamp_t ubound) {
  return Interval({
      AtomicInterval(true, lbound, ubound, false)
    });
}

Interval Interval::closedopen(timestamp_t lbound, timestamp_t ubound) {
  return Interval({
      AtomicInterval(false, lbound, ubound, true)
    });
}

Interval Interval::complement() const {
  std::vector<AtomicInterval> complements;
  timestamp_t lb = NEGINF;
  bool l_open = true;
  for(auto i : atoms()) {
    complements.push_back(AtomicInterval(l_open, lb, i.lbound, !i.l_open));
    lb = i.ubound;
    l_open = !i.u_open;
  }

  complements.push_back(AtomicInterval(l_open, lb, POSINF, true));
  return Interval(complements);
}

Interval Interval::operator+(const AtomicInterval &o) const {
  Interval I({o});
  return I + (*this);
}

Interval Interval::operator*(const AtomicInterval &o) const {
  Interval I({o});
  return (*this) * I;
}

Interval Interval::operator+(const Interval &o) const {
  std::vector<AtomicInterval> inter;
  inter.insert(inter.end(), intervals.begin(), intervals.end());
  inter.insert(inter.end(), o.intervals.begin(), o.intervals.end());
  return Interval(inter);
}

Interval Interval::operator*(const Interval &o) const {
  std::vector<AtomicInterval> svec(intervals.begin(), intervals.end());
  std::vector<AtomicInterval> ovec(o.intervals.begin(), o.intervals.end());

  std::vector<AtomicInterval> inters;
  
  auto self = svec.begin();
  auto other = ovec.begin();

  while (self != svec.end() && other != ovec.end()) {
    if (self->overlaps(*other)) {
      inters.push_back(self->intersect(*other));

      if(self->ubound >= other->ubound)
	other++;
      else if(other->ubound >= self->ubound)
	self++;
    } else {
      // No overlap, jump ahead one interval.
      if(self->ubound < other->lbound)
	self++;
      else if(other->ubound < self->lbound)
	other++;
      else {
	// This should never happen
	assert(false);
      }
    }
  }
  return Interval(inters);
}


Interval AtomicInterval::operator+(const AtomicInterval &o) const {
  Interval I({ *this });
  return I + o;
}

AtomicInterval AtomicInterval::operator*(const AtomicInterval &o) const {
  timestamp_t lbnd = lbound;
  timestamp_t ubnd = ubound;
  bool lopen = l_open;
  bool ropen = u_open;
  if (lbnd < o.lbound) {
    lbnd = o.lbound;
    lopen = o.l_open;
  }
  if (lbnd == o.lbound) {
    lopen |= o.l_open;
  }

  if (ubnd > o.ubound) {
    ubnd = o.ubound;
    ropen = o.u_open;
  }
  if (ubnd == o.ubound) {
    ropen |= o.u_open;
  }

  return AtomicInterval(lopen, lbnd, ubnd, ropen);
}

Interval AtomicInterval::complement() const {
  AtomicInterval
    l(false, NEGINF, lbound, !l_open),
    r(!u_open, ubound, POSINF, false);
  return l + r;
}

AtomicInterval Interval::bounds() const {
  return AtomicInterval(intervals[0].l_open, intervals[0].lbound, intervals[intervals.size() - 1].ubound, intervals[intervals.size() - 1].u_open);
}

AtomicInterval AtomicInterval::open(timestamp_t lbound, timestamp_t ubound) {
  return AtomicInterval(true, lbound, ubound, true);
}

AtomicInterval AtomicInterval::closed(timestamp_t lbound, timestamp_t ubound) {
  return AtomicInterval(false, lbound, ubound, false);
}
AtomicInterval AtomicInterval::openclosed(timestamp_t lbound, timestamp_t ubound) {
  return AtomicInterval(true, lbound, ubound, false);
}
AtomicInterval AtomicInterval::closedopen(timestamp_t lbound, timestamp_t ubound) {
  return AtomicInterval(false, lbound, ubound, true);
}

bool AtomicInterval::contains(const AtomicInterval &o) const {
  return ((lbound < o.lbound) || ((o.l_open || !l_open) && lbound <= o.lbound))
    && ((ubound > o.ubound) || ((o.u_open || !u_open) && ubound >= o.ubound));
}

bool AtomicInterval::contains(const timestamp_t &ts) const {
  return ((lbound < ts) || (!l_open && lbound <= ts))
    && ((ts < ubound) || (!u_open && ts <= ubound));
}


/* Returns a restriction on b given that b < a */
AtomicInterval before(AtomicInterval a, AtomicInterval b) {
  return AtomicInterval::closed(std::max(a.lbound, b.lbound), b.ubound);
}


/* Returns a restriction on b given that b > a */
AtomicInterval after(AtomicInterval a, AtomicInterval b) {
  return AtomicInterval::closed(b.lbound, std::min(a.ubound, b.ubound));
}

