#include "logic.hpp"

std::ostream &operator<<(std::ostream &os, const Term *t) {
  t->out(os);
  return os;
}

void Const::out(std::ostream &os) const {
  os << value();
}

void Comb::out(std::ostream &os) const {
  if (terms.size() == 0) {
    os << "[" << sep << "]";
    return;
  }
  os << "(";
  terms[0]->out(os);
  for (int i = 1; i < terms.size(); i++) {
    os << sep;
    terms[i]->out(os);
  }
  os << ")";
}
