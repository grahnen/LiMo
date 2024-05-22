#include "coverhistory.h"
#include <ostream>
#include <iostream>
#include "exception.h"
#include <algorithm>

void CoverHistory::add_push_call(event_t &call) {
  open[call.thread] = call;
}
void CoverHistory::add_pop_call(event_t &call) {
  open[call.thread] = call;
}

void CoverHistory::add_crash(event_t &crash){
  std::map<tid_t, event_t> m_op(open.begin(), open.end());
  std::set<CoverVal *> returned;
  for(auto &i : m_op) {
     event_t &call = i.second;
     event_t ev = event_t::create(Ereturn, i.first, {});
     add_ret(ev, true);
  }


  AtomicInterval ai = cover_bounds();
  std::set<CoverVal> tmp(values.begin(), values.end());
  for(auto &it : values) {
    if(it.pop_crash) {
      if (ai.ubound <= it.add.lbound) {
        // Maximal push
        std::cout << it.val << "Is maximal" << std::endl;
        it.pop_crash = false; // We keep it
      } else {
        std::cout << "Removing Pop " << it.val << std::endl;
        it.rmv = AtomicInterval::end();
        it.pop_crash = false;
      }
    }
  }
  era++;
}

// CoverHistory CoverHistory::get_of_era(int e) {
//   CoverHistory ch;
//   for(CoverVal cv : values) {
//     if(cv.push_era <= e) {
//       ch.add_value(cv);
//     }
//   }
//   return ch;
// }

CoverHistory::LinRes CoverHistory::add_ret(event_t &ret, bool crash){
  if (!open.contains(ret.thread)) {
    return LinRes("Return before call");
  }
  
  event_t &call = open[ret.thread];
  AtomicInterval op = AtomicInterval::closed(call.timestamp, ret.timestamp);
  auto eqval = [](val_t v) { return [v](CoverVal cv) { return v == cv.val; }; };
  
  if (call.type == Epush) {
    AtomicInterval rm = AtomicInterval::end();
    if (unmatched_removes.contains(call.val.value())) {
      rm = unmatched_removes.at(call.val.value());
      unmatched_removes.erase(call.val.value());
    }
    if (!rm.overlaps(op)) {
      CoverVal vl(call.val.value(), op);
      vl.push_era = era;
      vl.push_crash = crash;
      add_value(CoverVal(call.val.value(), op));
    }


  } else if (call.type == Epop) {
    optval_t v = call.val;
    if(ret.val.has_value())
      v = ret.val;

    if (v.has_value()) {
      auto val =
        std::find_if(values.begin(), values.end(), eqval(v.value()));

      if (val == values.end()) {
        unmatched_removes[ret.val.value()] = op;
      } else {
        if (val->rmv != AtomicInterval::end()) {
          std::cout << val->rmv << std::endl;
          return LinRes("Multiple pops on value ");
        }
        val->rmv = op;
        val->pop_era = era;
        val->pop_crash = crash;
        if (val->rmv.overlaps(val->add)) {
          values.erase(val);
        }
      }
    } else {
      add_empty(op);
    }
  }

  if(open.contains(ret.thread)) {
    open.erase(ret.thread);
  }
  return LinRes();
}

bool CoverHistory::complete() const {
  return open.size() == 0;
}

void CoverHistory::add_value(CoverVal op) {
  values.insert(op);
}

void CoverHistory::add_empty(AtomicInterval op) {
  empties.push_back(op);
}

Interval CoverHistory::cover() const {
  std::vector<AtomicInterval> atoms;
  std::transform(values.begin(), values.end(), std::back_inserter(atoms),
		 [](CoverVal v) { return v.cover(); });

  return Interval(atoms);
}

AtomicInterval CoverHistory::cover_bounds() const {
  timestamp_t lb = POSINF;
  timestamp_t ub = NEGINF;

  for (auto v : values) {
    if(v.cover().lbound < lb)
      lb = v.cover().lbound;
    if(v.cover().ubound > ub)
      ub = v.cover().ubound;
  }
  
  return AtomicInterval(false, lb, ub, false);
}

std::set<CoverVal> *CoverHistory::minmax() const {
  AtomicInterval ai = cover_bounds();
    
  std::set<CoverVal> *minmax = new std::set<CoverVal>();

  for (auto it : values) {
    if (ai.lbound >= it.add.lbound) {
      if (ai.ubound <= it.rmv.ubound) {
	minmax->insert(it);
      }
    }
  }
  
  return minmax;
}

std::vector<CoverHistory> CoverHistory::split(AtomicInterval i) {
  std::set<CoverVal> L, R;
  std::partition_copy(values.begin(), values.end(), std::inserter(L, L.end()), std::inserter(R, R.end()), [&](CoverVal v) {
    return v.add.ubound < i.lbound || v.rmv.lbound <= i.lbound;
  });

  return std::vector {
    CoverHistory(L),
    CoverHistory(R),
  };
}

std::vector<CoverHistory> CoverHistory::segmentize(IIter begin, IIter end) {
  if(begin == end)
    return {*this};

  AtomicInterval curr = *begin;
  std::vector<CoverHistory> spl = split(curr);
  CoverHistory L = spl[0];
  CoverHistory R = spl[1];

  std::vector<CoverHistory> res { L };
  std::vector<CoverHistory> next = R.segmentize(++begin, end);
  res.insert(res.end(), next.begin(), next.end());
  return res;
}

Last CoverHistory::last() const {
  if(empties.size() > 0)
    return Last::PopEmpty;

  if(values.size() > 0)
    return Last::PushPop;

  return Last::Epsilon;
}

bool CoverHistory::prune_crash() {
  bool ret = false;
  for(CoverVal cv : values) {
    if(cv.push_crash) {
      if(cv.pop_crash || cv.rmv.empty())
        values.erase(cv);

      cv.push_crash = false;
    }

    if(cv.pop_crash && cv.add.empty())
      values.erase(cv);
  }
  return ret;
}

bool CoverHistory::minmax_prune() {
  bool ret = false;
  std::set<CoverVal> *mm = minmax();
  while (mm->size() > 0) {
    ret = true;
    /* Remove values in mm */
    auto v_it = values.begin();
    auto m_it = mm->begin();
    while (v_it != values.end() && m_it != mm->end()) {
      if (v_it->val == m_it->val) {
	v_it = values.erase(v_it);
      } else if (*v_it < *m_it) {
	v_it++;
      } else {
	m_it++;
      }
    }
    delete mm;
    mm = minmax();
  }
  delete mm;
  return ret;
}

void CoverHistory::simplify() {
  std::set<CoverVal> nVals;
  
  auto sim = [&](CoverVal v) {
    return !v.add.overlaps(v.rmv);
  };
  
  std::copy_if(values.begin(), values.end(), std::inserter(nVals, nVals.end()), sim);
  
  values = nVals;
}

std::ostream &operator<<(std::ostream &os, const CoverHistory &h) {
  os << "(" << h.values.size() << " values, " << h.empties.size() << " PopEmpty) ";
  switch (h.last()) {
  case Push:
    os << "Push";
    break;
  case PushPop:
    os << "PushPop";
    break;
  case PopEmpty:
    os << "PopEmpty";
    break;
  case Epsilon:
    os << "Epsilon";
    break;
  }
  return os;
}
