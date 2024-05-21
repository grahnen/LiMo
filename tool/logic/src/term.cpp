#include "logic.hpp"
#include <iostream>
#include <functional>
#include <cassert>

#define MERGE_CONJ 1
#define MERGE_DISJ 0

#define DISTR_CONJ 1
#define DISTR_DISJ 0

#define SUBSUME_DISJ 1

TP Conj::simpl() const {
  std::vector<TP> ntp;
  std::vector<BP> bounds;
  std::vector<TP> other;
  int first_disj = -1;
  // Simplify each subterm
  for (auto t : terms) {
    TP t2 = t->simpl();
    if (t2->isConst()) {
      if(t2->value() == false) {
        return std::make_shared<Const>(false);
      }
    }
    else if (t2->isConj()) {
      CP nt2 = std::static_pointer_cast<Conj>(t2);
      for (auto nt2t : nt2->terms) {
        if (t2->isConst()) {
          if(t2->value() == false) {
            return std::make_shared<Const>(false);
          }
        } else {
          ntp.push_back(nt2t->clone());
        }
      }
    }
    else {
      ntp.push_back(t2);
    }
  }

  std::vector<TP> nt;
  for (auto it : ntp) {
    bool in = false;
    for (auto it2 : nt) {
      if (it->equal(it2)) {
        in = true;
        break;
      }
    }
    if (!in)
      nt.push_back(it);
  }
  
  // Exit early if conj is size 0 or 1
  if (nt.size() == 0) {
    return std::make_shared<Const>(true);
  }

  if(nt.size() == 1) {
    return nt[0];
  }

  // Distribute
#if DISTR_CONJ
  first_disj = -1;
  for (int i = 0; i < nt.size(); i++) {
    if (nt[i]->isDisj()) {
      first_disj = i;
      break;
    }
  }
  if (first_disj > -1) {

    DP d = std::static_pointer_cast<Disj>(nt[first_disj]);
    std::vector<TP> conj_term;
    std::vector<TP> dis_term;

    for (auto d_term : d->terms) {
      dis_term.clear();
      dis_term.push_back(d_term);
      for (int i = 0; i < nt.size(); i++) {
        if (i != first_disj) {
          dis_term.push_back(nt[i]);
        }
      }
      TP step = std::make_shared<Conj>(dis_term);
      conj_term.push_back(step->simpl());


    }
    TP simplified = (std::make_shared<Disj>(conj_term))->simpl();
    return simplified;
  }

#endif

#if MERGE_CONJ
  for (auto it : nt) {
    if (it->isBnd()) {
      bounds.push_back(std::static_pointer_cast<Bound>(it));
    } else {
      other.push_back(it);
    }
  }

  if(bounds.size() > 0) {
    BP bp = bounds[0];
    for(int i = 1; i < bounds.size(); i++) {
      bp = bp->join(bounds[i]);
    }
    other.push_back(bp);
  }
#else
  other = nt;  
#endif
  if(other.size() == 0)
    return std::make_shared<Const>(true);
  if(other.size() == 1)
    return other[0]->simpl();

  TP ret = std::make_shared<Conj>(other);
  // if (equal(ret))
  //   return ret;
  return ret;
}

TP Disj::simpl() const {
  std::vector<TP> ntp;
  std::vector<BP> bounds;

  for (auto t : terms) {
    TP t2 = t->simpl();
    if (t2->isConst()) {
      if (t2->value() == true) {
        return std::make_shared<Const>(true);
      }
    } else if (t2->isDisj()) {
      DP nt2 = std::static_pointer_cast<Disj>(t2);
      for (auto nt2t : nt2->terms) {
        ntp.push_back(nt2t);
      }
    } else {
      ntp.push_back(t2);
    }
  }

  std::vector<TP> nt;
  for (auto it : ntp) {
    bool in = false;
    for (auto it2 : nt) {
      if (it->equal(it2)) {
        in = true;
        break;
      }
    }
    if (!in)
      nt.push_back(it);
  }
  
  // Exit early
  if (nt.size() == 0)
    return std::make_shared<Const>(false);

  if(nt.size() == 1)
    return nt[0];

  std::vector<TP> other;
#if SUBSUME_DISJ
  std::vector<TP> newTerms;
  for(auto it : nt) {
    if(it->isBnd()) {
      bounds.push_back(std::static_pointer_cast<Bound>(it));
    } else {
      newTerms.push_back(it);
    }
  }

  /* TODO: analyze the complexity of this. It could be bad! */
  for(int i = 0; i < bounds.size(); i++) {
    BP a = bounds[i];
    bool keep = true;
    for(int j = 0; j < bounds.size(); j++) {
      if (i == j)
        continue;
      BP b = bounds[j];

      if(a->implies(b)) {
        keep = false;
        break;
      }
    }

    if(keep)
      newTerms.push_back(a);
  }

  other = newTerms;
#else
  other = nt;
#endif

  if(other.size() == 0)
    return std::make_shared<Const>(false);
  if (other.size() == 1) {
    return other[0];
  }
  
  DP n = std::make_shared<Disj>(other);
  // if(!equal(n))
  //   return n->simpl();

  return n;
}

TP Const::simpl() const {
  return std::make_shared<Const>(val);
}

bool Comb::equal(TP o) const {
  const std::vector<TP> *oterms;
  if (isConj() && o->isConj()) {
    CP other = std::static_pointer_cast<Conj>(o);
    oterms = &other->terms;
  } else if (isDisj() && o->isDisj()) {
    DP other = std::static_pointer_cast<Disj>(o);
    oterms = &other->terms;
  } else {
    return false;
  }

  if (oterms->size() != terms.size())
    return false;

  for (int i = 0; i < terms.size(); i++) {
    if(!((*oterms)[i]->equal(terms[i])))
      return false;
  }

  return true;
}

bool Const::equal(TP o) const {
  if (o->isConst()) {
    return std::static_pointer_cast<Const>(o)->val == val;
  }
  return false;
}
