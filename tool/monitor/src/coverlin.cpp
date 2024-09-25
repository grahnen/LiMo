#include "monitor/covermonitor.h"
#include <iostream>
#include <cassert>
#include <algorithm>
#include <optional>
#include "exception.h"

std::vector<CoverHistory> CoverHistory::guesses() {
  std::set<CoverVal> crashed;
  std::set<CoverVal> safe;
  for(auto it : values) {
    if(it.push_crash || it.pop_crash)
      crashed.insert(it);
    else
      safe.insert(it);
  }

  // std::cout << crashed.size() << " crashed values" << std::endl;
  // std::cout << safe.size() << " safe values" << std::endl;

  CoverHistory base;
  for(auto it : safe)
    base.add_value(it);

  std::vector<CoverHistory> hists = std::vector<CoverHistory> { base };
  for(auto it : crashed) {
    std::vector<CoverHistory> nres;
    for(auto h : hists) {
      std::vector<CoverHistory> r = h.add_branch(it);
      nres.insert(nres.end(), r.begin(), r.end());
    }
    hists = nres;
  }

  for(auto &h : hists) {
    h.empties = std::vector(empties.begin(), empties.end());
  }

  if(hists.size() > 1) {
    std::cout << "There are " << hists.size() << " guesses for durable" << std::endl;
  }

  return hists;
}


std::vector<CoverHistory> CoverHistory::add_branch(CoverVal it) {
  CoverHistory a = *this, b = *this;
  if(it.push_crash) {
    CoverVal av = it;
    av.push_crash = false;

    CoverVal bv = it;
    av.push_crash = false;
    av.add = AtomicInterval::nil();

    if(it.pop_crash) {
      throw std::logic_error("Should have removed the double crash case already");
    }

    a.add_value(av);
    b.add_value(bv);



  } else if (it.pop_crash) {
    CoverVal av = it;
    av.pop_crash = false;
    CoverVal bv = it;
    av.pop_crash = false;
    av.rmv = AtomicInterval::nil();

    a.add_value(av);
    b.add_value(bv);


  }
  return std::vector<CoverHistory> {a, b};
}


CoverHistory::LinRes CoverHistory::check_durable() {
  std::vector<CoverHistory> hists = guesses();
  bool durable = false;
  if (hists.size() > 1)
    durable = true;
  int i = 0;
  LinRes lr;
  for(auto h : hists) {
    i++;
    lr = h.step();
    std::cout << lr.violation() << std::endl;
    while (!lr.violation() && lr.remaining.size() > 0) {
      auto oa = lr.remaining.back();
      lr.remaining.pop_back();
      LinRes lr2 = oa.step();
      lr = lr + lr2;
    }
    if(!lr.violation())
      return lr;
  }

  if(durable)
    return LinRes("no guess works");
  else
    return lr;

}


CoverHistory::LinRes CoverHistory::step() {
    switch (last()) {
      case Epsilon:
        return LinRes();
      case PopEmpty: {
        std::cout << "Doing PopEmpty" << std::endl;
        Interval opens = cover().complement();
        std::cout << cover() << std::endl;
    
        auto atoms = opens.atoms();

        std::cout << opens << std::endl;

        auto first_match = [&](AtomicInterval empty) -> AtomicInterval {
          auto i = std::find_if(atoms.begin(), atoms.end(), [&](AtomicInterval opening) {
            return opening.overlaps(empty);
          });
      
          if (i == atoms.end()) {
            return AtomicInterval::nil();
          } else {
            std::cout << empty << " overlaps " << *i << std::endl;
            return *i;
          }
        };
    
        std::vector<AtomicInterval> openings;
        std::cout << empties << std::endl;
        std::transform(empties.begin(), empties.end(), std::back_inserter(openings), first_match);
        for (auto inter : openings) {
          if (inter.empty())
            return LinRes("does not intersect an opening");
        }
    
        Interval I(openings);
        std::vector<AtomicInterval> i_atoms = I.atoms();
        std::vector<CoverHistory> segments = segmentize(i_atoms.begin(), i_atoms.end());
        return LinRes(segments);
      }
      case Push:
      case PushPop: {
        if (minmax_prune()) {
          // some elements were both minimal and maximal
          // We need to linearize the remainder.
          return LinRes({ *this });
        }
        Interval cov = cover();
        if (cov.atomic()) {
          return LinRes("Cover is atomic but no element in max & min");
        }
        Interval opens = cov.complement();
        std::vector<CoverHistory> spl = split(opens.atoms()[1]);
        return LinRes(spl);
      }
    }
  return LinRes();
}
