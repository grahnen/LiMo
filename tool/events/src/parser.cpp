#include "parser.h"
event_t parse_event(const tid_t t, const std::string name, const optval_t arg) {
  for (std::pair<std::string, EType> it : str2evt) {
    if (strcmp(name.c_str(), it.first.c_str()) == 0) {
      return event_t::create(it.second, t, arg);
    }
  }
  // No match, return error event
  std::cout << "No match event" << std::endl;
  return event_t::create(Enil, t, arg);
}
