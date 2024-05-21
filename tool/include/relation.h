#ifndef RELATION_H
#define RELATION_H
#include "typedef.h"
#include "interval.h"
#include <iostream>

enum RelVal : uint32_t {
  Before = 1 << 0,
  After = 1 << 1,
  Above = 1 << 2,
  Below = 1 << 3,
};

std::ostream &operator<<(std::ostream &os, const class Relation r);

const std::vector<RelVal> components{RelVal::Before, RelVal::After,
                                     RelVal::Above, RelVal::Below};

constexpr RelVal operator|(const RelVal a, const RelVal b) {
  return (RelVal)(uint32_t(a) | uint32_t(b));
}
constexpr RelVal operator&(const RelVal a, const RelVal b) {
  return (RelVal)(uint32_t(a) & uint32_t(b));
}
constexpr RelVal operator^(const RelVal a, const RelVal b) {
  return (RelVal)(uint32_t(a) ^ uint32_t(b));
}

constexpr RelVal reverse_rel(RelVal a) {
  if(a == Before)
    return After;
  if(a == After)
    return Before;
  if(a == Above)
    return Below;

  return Above;
}


constexpr RelVal all_vals() {
	return RelVal::Before | RelVal::After | RelVal::Above | RelVal::Below;
};


// Double checked, see notebook if in doubt.
const std::map<RelVal, std::map<RelVal, RelVal>> MulTbl{
    {RelVal::Before,
     {{RelVal::Before, RelVal::Before},
      {RelVal::After, all_vals()},
      {RelVal::Above, RelVal::Above | RelVal::Before},
      {RelVal::Below, RelVal::Before}}},
    {RelVal::After,
     {{RelVal::Before, all_vals()},
      {RelVal::After, RelVal::After},
      {RelVal::Above, RelVal::After | RelVal::Above},
      {RelVal::Below, RelVal::After}}},
    {RelVal::Above,
     {{RelVal::Before, RelVal::Before},
      {RelVal::After, RelVal::After},
      {RelVal::Above, RelVal::Above},
      {RelVal::Below, all_vals()}}},
    {RelVal::Below,
     {{RelVal::Before, RelVal::Below | RelVal::Before},
      {RelVal::After, RelVal::Below | RelVal::After},
      {RelVal::Above, RelVal::Above | RelVal::Below},
      {RelVal::Below, RelVal::Below}}}};

class Relation {
public:
  RelVal value;
  Relation(RelVal val) : value(val) {}
  Relation() : Relation((RelVal)0) {}
  bool isAll() const { return *this == All(); }
  bool empty() const { return *this == None(); }
  int size() const {
    return __builtin_popcount(value);
  };
  Relation clone() const { return Relation(value); }
  
  Relation operator|(const Relation o) const {
    return Relation(value | o.value);
  }
  Relation operator&(const Relation o) const {
    return Relation(value & o.value);
  }
  Relation operator^(const Relation o) const {
    return Relation(value ^ o.value);
  }

  std::vector<RelVal> branches() const {
    std::vector<RelVal> v;
    for (auto c : components) {
      if(value & c) {
        v.push_back(c);
      }
    }
    return v;
  }

  Relation operator=(const Relation o) {
    value = o.value;
    return *this;
  }
  Relation operator|=(const Relation o) { value = value | o.value; return *this; }
  Relation operator&=(const Relation o) { value = value & o.value; return *this; }
  Relation operator^=(const Relation o) { value = value ^ o.value; return *this; }

  bool operator==(const Relation o) const { return value == o.value; }

  static Relation All() {
    return Relation(RelVal::Before | RelVal::After | RelVal::Above |
                    RelVal::Below);
  }
  static Relation None() { return Relation((RelVal)0); }
  Relation negated() const { return *this ^ All(); }
  Relation reversed() const {
    Relation res = Relation::None();
    if(value & RelVal::Above)
      res |= RelVal::Below;
    if(value & RelVal::Below)
      res |= RelVal::Above;

    if(value & RelVal::Before)
      res |= RelVal::After;
    if(value & RelVal::After)
      res |= RelVal::Before;

    if (res.value == 0 && value != 0) {
      std::cout << "Incorrect reverse of: " << this << std::endl;
    }
    return res;
  }

  Relation meet(Relation o) { return Relation(value | o.value); }
  Relation join(Relation o) { return Relation(value & o.value); }

  Relation operator*(const Relation o) const {
    Relation res = None();
    for (auto r : components) {
      if ((r & value) == r) {
        for (auto k : components) {
          if ((k & o.value) == k) {
            // r * k
	    res |= MulTbl.at(r).at(k);
          }
        }
      }
    }
    return res;
  }
};
inline std::ostream &operator<<(std::ostream &os, const Relation r) {
  os << "{";
  if (r.value & RelVal::Before) {
    os << "◀";
  }
  if (r.value & RelVal::After) {
    os << "▶";
  }
  if (r.value & RelVal::Above) {
    os << "▲";
  }
  if (r.value & RelVal::Below) {
    os << "▼";
  }
    return os << "}";
}

Relation compare(const struct Value &v1, const struct Value &v2);

struct Value {
  val_t val;
  AtomicInterval add;
  std::optional<AtomicInterval> rmv;

  std::set<Value> must_be_above;
  int concurrent_pops = 0;
  
  Value(val_t v, AtomicInterval a, std::optional<AtomicInterval> r) : val(v), add(a), rmv(r) {}
  Value(const Value &) = default;
  Value() : val(val_t(-1, -1)), add(AtomicInterval::nil()), rmv({}) {}
  Value(val_t v, AtomicInterval add) : Value(v, add, {}) {}
  std::ostream &operator<<(std::ostream &os) const {
    return os << val;
  }
  bool operator<(const Value other) const {
    if(rmv < other.rmv)
      return true;
    if(other.rmv < rmv)
      return false;

    if(add < other.add)
      return true;
    if(other.add < add)
      return false;

    return val < other.val;
  }
  bool operator==(const Value other) const {
    return val == other.val && add == other.add && rmv == other.rmv;
  }
  
  bool concurrent(const Value other) const {
    return other.add.overlaps(add)
      || (rmv.has_value() && other.add.overlaps(rmv.value()))
      || (other.rmv.has_value() && other.rmv.value().overlaps(add))
      || (other.rmv.has_value() && rmv.has_value() && other.rmv.value().overlaps(rmv.value()));
    return compare(*this, other).size() > 1;
  }
};

inline std::ostream &operator<<(std::ostream &os, const Value v) {
  return os << v.val << v.add << v.rmv;
}

#endif
