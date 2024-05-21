#ifndef LOGIC_H
#define LOGIC_H
#include "interval.h"
#include "relation.h"
#include <variant>
#include <vector>
#include <iostream>
#include <memory>

// Forward declarations;
class Term;
class Conj;
class Disj;
class Bound;
class Const;

using TP = std::shared_ptr<Term>;
using BP = std::shared_ptr<Bound>;
using CP = std::shared_ptr<Conj>;
using DP = std::shared_ptr<Disj>;
using COP = std::shared_ptr<Const>;


// Forward declaration;
class Bound;

struct Term {
  Term() = default;
  virtual ~Term() = default;
  virtual TP simpl() const = 0;
  virtual bool isConst() const { return false; }
  virtual bool isConj() const { return false; }
  virtual bool isDisj() const { return false; }
  virtual bool isBnd() const { return false; }
  virtual bool value() const { return false; }

  virtual std::vector<BP> bound_disj() const;
  virtual BP bounds() const;
  virtual void out(std::ostream &os) const { os << "Term"; }
  virtual int size() const { return 1; }
  virtual bool equal(TP o) const = 0;
  virtual TP clone() const = 0;
};

std::ostream &operator<<(std::ostream &os, const Term *t);


struct Comb : public Term {
public:
  std::vector<TP> terms;
  std::string sep;
  Comb(std::vector<TP> terms, std::string sep) : sep(sep) {
    for (auto t : terms) {
      this->terms.push_back(t->clone());
    }
  }
  void out(std::ostream &os) const;
  int size() const {
    int i = 0;
    for (auto t : terms) {
      i += t->size();
    }
    return i;
  }
  ~Comb() { }
  bool equal(TP o) const;
};

struct Conj : public Comb {
public:
  Conj(std::vector<TP> terms) : Comb(terms, " ∧ ") {}
  TP simpl() const;
  bool isConj() const { return true; }
  TP clone() const { return std::make_shared<Conj>(terms); }
  BP bounds() const;
  std::vector<BP> bound_disj() const;
};

struct Disj : public Comb {
public:
  Disj(const std::vector<TP> terms) : Comb(terms, " ∨ ") {}
  TP simpl() const;
  bool isDisj() const { return true; }
  TP clone() const { return std::make_shared<Disj>(terms); }
  BP bounds() const;
  std::vector<BP> bound_disj() const;
};

struct Const : public Term {
public:
  bool val;
  Const(const bool v) : val(v) {}
  TP simpl() const;
  bool isConst() const { return true; }
  bool value() const { return val; };
  void out(std::ostream &os) const;
  bool equal(TP o) const;
  TP clone() const { return std::make_shared<Const>(val); };
  BP bounds() const;
};

struct Bound : public Term {
    AtomicInterval add, rmv;
    Bound() : Bound(AtomicInterval::nil(), AtomicInterval::nil()) {}
    Bound(AtomicInterval a, AtomicInterval r) : add(a), rmv(r) {}
    ~Bound() = default;


    TP simpl() const;


    bool implies(BP other) const;

    BP join(BP) const;
    BP extend(BP) const;
    BP bounds() const;

    bool isBnd() const {
      return true;
    }
    virtual std::vector<BP> bound_disj() const;
    bool equal(TP o) const;
    TP clone() const;

    void out(std::ostream &os) const;
};

Relation compare_bounds(const BP, const BP);

template<typename T>
inline std::ostream &operator<<(std::ostream &os, const std::vector<T> o) {
  os << "[";
  bool first = true;
  for(auto it : o) {
    if(!first)
      os << ", ";
    first = false;
    os << it;
  }
  return os << "]";
}


BP restr_bounds(BP a, RelVal r, BP b);



#endif
