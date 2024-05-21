#ifndef PARSER_H
#define PARSER_H
#include "event.h"
#include <cstring>
#include <map>

const std::map<std::string, EType> str2evt = {
  {"return", EType::Ereturn},
  {"push", EType::Epush},
  {"Push", EType::Epush},
  {"pop", EType::Epop},
  {"Pop", EType::Epop},
  {"enq", EType::Eenq},
  {"Enq", EType::Eenq},
  {"deq", EType::Edeq},
  {"Deq", EType::Edeq},
  {"crash", EType::Ecrash},
  // Compatibility with logs from Violin
  {"add", EType::Eenq},
  {"remove", EType::Edeq},

};

event_t parse_event(const tid_t t, const std::string name, const optval_t arg);

#endif
