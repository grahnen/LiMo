#ifndef EVENT_H
#define EVENT_H
#include "typedef.h"
#include <variant>
#include <ostream>
#include <atomic>
#include "macros.h"

#define PREPEND(s, data, elem) BOOST_PP_CAT(data,elem)

enum EType {
  BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(PREPEND, E, EVT_TYPE_SEQ))
};

const std::string etype_name[] = {
  "return",
  "call push",
  "call pop",
  "call enq",
  "call deq",
  "call add",
  "call rmv",
  "ctn",
  "crash",
  "nil"
};

const std::string method_names[] = {"push", "pop", "enq", "deq", "nil"};

inline std::string event_str(EType &etp, std::optional<std::string> valstr) {
  if(etp == Ereturn) {
    if(valstr.has_value())
      return etype_name[etp] + " " + valstr.value();
    return etype_name[etp];
  }

  if(valstr.has_value())
    return etype_name[etp] + "(" + valstr.value() + ")";
  return etype_name[etp] + "()";
}


struct event_t {
  EType type;
  tid_t thread;
  optval_t val;
  timestamp_t timestamp;
  event_t();
event_t(EType et, tid_t th, optval_t v, timestamp_t ts) : type(et), thread(th), val(v), timestamp(ts) {}
  friend std::ostream &operator<<(std::ostream &os, const event_t &e);
  static event_t create(EType, tid_t, optval_t);

  inline bool operator<(event_t &o) {
    return timestamp < o.timestamp;
  }

  bool operator==(const event_t &o) const;
};

static volatile std::atomic<timestamp_t> EVENT_GLOBAL_TIMESTAMP{0};
#endif
