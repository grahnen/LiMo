#include "event.h"
#include <ostream>

std::ostream &operator<<(std::ostream &os, const event_t &ev) {
  os << "[" << ev.thread << "] " << etype_name[ev.type];
  if (ev.val.has_value()) {
    if(ev.type == Ereturn)
      os << " " << ev.val.value();
    else
      os << "(" << ev.val.value() << ")";
  }

  return os;
}

event_t event_t::create(const EType type, const tid_t thread, const optval_t val) {
  event_t e;
  e.type = type;
  e.thread = thread;
  e.val = val;
  e.timestamp = ++EVENT_GLOBAL_TIMESTAMP;
  return e;
}
event_t::event_t() : type(Enil), thread(0), val(val_t(-1, -1)), timestamp(0) {}

bool event_t::operator==(const event_t &o) const {
  return type == o.type && thread == o.thread && val == o.val && timestamp == o.timestamp;
}
