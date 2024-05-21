#ifndef MONITOR_H
#define MONITOR_H
#include "event.h"
#include <functional>
#include <map>
#include "util.h"

#define HANDLER_DEF_HELPER(data,elem) void handle_##data##elem(event_t &)

#define DECLHANDLER(elem) \
  HANDLER_DEF_HELPER(,elem); \
  HANDLER_DEF_HELPER(ret_,elem);

#define DEFHANDLER(r, data, elem) \
  virtual HANDLER_DEF_HELPER(data,elem) { throw std::logic_error("Unimplemented"); }


struct MonitorConfig {
  std::ostream *os;
  ADT type;
  std::string dot_dir;
  std::string file_name;
  tid_t thread_count;

  MonitorConfig() {
    os = &std::cout;
    dot_dir = "dots";
    thread_count = 32;
  }
};

class Monitor {
 public:
  Monitor(MonitorConfig mc) : output(mc.os), verbose(false), nConc(0), maxConc(0), totConc(0), nOps(0), dot_dir(mc.dot_dir), file_name(mc.file_name) {}
  virtual ~Monitor() = default;
  void add_event(event_t &);
  void set_verbose(bool v);

  BOOST_PP_SEQ_FOR_EACH(DEFHANDLER,, OP_TYPE_SEQ)

  BOOST_PP_SEQ_FOR_EACH(DEFHANDLER,ret_,OP_TYPE_SEQ)

  virtual void handle_crash(event_t &ev) { throw std::logic_error("Unimplemented"); }

  int max_conc();
  float avg_conc();

  virtual void print_state() const = 0;
  virtual void do_linearization() {};

protected:
  bool verbose;
  std::ostream *output;
  std::string dot_dir;
  std::string file_name;
  Array<EType> lastEvt;
  int nOps;

private:
  int nConc, maxConc, totConc;

  void add_conc();
  void rem_conc();

};

using MonitorCreate = std::function<Monitor *(std::ostream*)>;

using ParseMap = std::unordered_map<std::string, MonitorCreate>;

#endif
