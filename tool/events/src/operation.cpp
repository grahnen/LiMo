#include "operation.h"
#include "interval.h"
operation::operation(EType t, tid_t th, timestamp_t inv, timestamp_t ret, optval_t val)
  : type(t), thread(th), inv(inv), ret(ret), value(val) {}



std::string operation::name() const {
  if(type == Epush)
    return "push(" + ext2str(value.value()) + ")";
  if(type == Epop) {
    if(value == std::nullopt)
      return "popEmpty";
    return "pop(" + ext2str(value.value()) + ")";
  }
  return "ERRNAME";
}

std::string operation::label() const {
  if(type == Epush)
    return "!" + ext2str(value.value());
  if(type == Epop) {
    if(value == std::nullopt)
      return "XX";
    return "?" + ext2str(value.value());
  }
  return "ERRLABEL";
}

Interval operation::interval() const {
  return Interval::closed(inv, ret);
}
