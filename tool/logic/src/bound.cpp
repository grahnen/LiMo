#include "logic.hpp"
#include "relation.h"
#include <algorithm>

TP Bound::simpl() const {
    if(add.empty() || rmv.empty())
        return std::make_shared<Const>(false);
    return clone();
}

BP Bound::join(BP o) const {
    return std::make_shared<Bound>(add * o->add, rmv * o->rmv);
}

BP Bound::extend(BP bnd) const {
    return std::make_shared<Bound>((add + bnd->add).bounds(), (rmv + bnd->rmv).bounds());
}

bool Bound::equal(const TP o) const{
    if(!o->isBnd())
        return false;
    BP ob = std::static_pointer_cast<Bound>(o);
    return add == ob->add && rmv == ob->rmv;
}

TP Bound::clone() const {
    return std::make_shared<Bound>(add, rmv);
}

void Bound::out(std::ostream &os) const {
    os << "❬" << add << "," << rmv << "❭";
}

bool Bound::implies(BP other) const {
    // Returns true if other is always sat if this is.
    return other->add.contains(add) && other->rmv.contains(rmv);
}

Relation compare_bounds(const BP b1, const BP b2) {
    // Value v1('a', b1->add, b1->rmv);
    // Value v2('b', b2->add, b2->rmv);
    // return compare(v1, v2);
    return Relation::All();
}

BP Const::bounds() const {
    if(val)
        return std::make_shared<Bound>(AtomicInterval::complete(), AtomicInterval::complete());

    return std::make_shared<Bound>();

}

BP Term::bounds() const {
    return std::make_shared<Bound>();
}

BP Bound::bounds() const {
    return std::static_pointer_cast<Bound>(clone());
}

BP Disj::bounds() const {
    AtomicInterval ab = AtomicInterval::nil();
    AtomicInterval rb = AtomicInterval::nil();

    for(auto it : terms) {
        ab = (ab + it->bounds()->add).bounds();
        rb = (rb + it->bounds()->rmv).bounds();
    }

    return std::make_shared<Bound>(ab, rb);
}

BP Conj::bounds() const {
    AtomicInterval ab = AtomicInterval::complete();
    AtomicInterval rb = AtomicInterval::complete();

    for(auto it : terms) {
        ab = (ab * it->bounds()->add);
        rb = (rb * it->bounds()->rmv);
    }

    return std::make_shared<Bound>(ab, rb);
}

BP restr_bounds(BP a, RelVal r, BP b) {
  // Add a after/before b
  AtomicInterval aaBab = before(a->add, b->add);
  AtomicInterval aaAab = after(a->add, b->add);

  // Add a after/before rmv b
  AtomicInterval aaBrb = before(a->add, b->rmv);
  AtomicInterval aaArb = after(a->add, b->rmv);

  // Rmv a after/before add b
  AtomicInterval raBab = before(a->rmv, b->add);
  AtomicInterval raAab = after(a->rmv, b->add);

  // Rmv a after/before rmv b
  AtomicInterval raBrb = before(a->rmv, b->rmv);
  AtomicInterval raArb = after(a->rmv, b->rmv);

  switch (r) {
  case Above:
    // a above b
    return std::make_shared<Bound>(aaAab * raAab, raBrb * aaBrb);
  case Below:
    return std::make_shared<Bound>(aaBab * raAab, raArb * aaBrb);
  case Before:
    // a < b
    return std::make_shared<Bound>(aaBab * raBab, aaBrb * raBrb);
  case After:
    // b < a
    return std::make_shared<Bound>(aaAab * raAab, aaArb * raArb);
  }

  std::cout << "Error" << std::endl;
  return std::make_shared<Bound>(b->add, b->rmv);
}


std::vector<BP> Term::bound_disj() const {
  return std::vector { std::make_shared<Bound>() };
}

std::vector<BP> Bound::bound_disj() const {
  return std::vector { std::static_pointer_cast<Bound>(clone()) };

}


std::vector<BP> Conj::bound_disj() const {
  throw std::logic_error("Unimplemented: disj for conj. Simplify first");
}

std::vector<BP> Disj::bound_disj() const {
  std::vector<BP> out;
  std::transform(terms.begin(), terms.end(), std::back_inserter(out), [](TP t) {
    return std::static_pointer_cast<Bound>(t);
  });
  return out;
}
