#ifndef EXECUTION_H_
#define EXECUTION_H_
#include "io.h"
#include "generator.hpp"
#include "exception.h"
#include "convert.h"


Configuration *hist_from_ints(int count, int *data) {
  std::vector<event_t> history;
  auto h = make_history(count, data);
  tid_t ts = 0;

  tid_t max_t = 0;
  std::transform(h.begin(), h.end(), std::back_inserter(history), [&ts, &max_t](ev_t evt) {
    ts++;
    max_t = std::max(evt.th, max_t);
    return event_t(evt.t, evt.th, val_t(evt.th, 0), ts);
  });

  Configuration *conf = new Configuration(history, ADT::stack, history.size(), true);
  Configuration *simpl = simplify(conf);
  delete conf;

  return simpl;
}


Configuration *hist_from_ints(std::vector<int> &int_h) {
  return hist_from_ints(int_h.size() / 4, int_h.data());
}


bool try_hist(Monitor *m, std::vector<event_t> &hist) {
  try {
    for(auto ev : hist) {
      m->add_event(ev);
    }
    m->do_linearization();
  } catch (Violation &v) {
    return true;
  }
  return false;
}


enum ComparisonResult : char {
  MatchLin,
  MatchViol,
  Mismatch,
  MonitorCrash
};

ComparisonResult try_history(std::vector<event_t> &hist, Monitor *a, Monitor *b = nullptr, bool verbose = false) {
  bool a_res, b_res;
  a->set_verbose(verbose);
  b->set_verbose(verbose);

  try {
    for(auto ev : hist) {
      a->add_event(ev);
    }
    a->do_linearization();
    a_res = true;
  } catch (Violation &v) {

    a_res = false;
  } catch (Crash &c) {
    std::cout << c.what() << std::endl;
    return MonitorCrash;
  }
  if(b == nullptr)
    return a_res ? MatchLin : MatchViol;
  try {
    for(auto ev : hist) {
      b->add_event(ev);
    }
    b->do_linearization();
    b_res = true;
  } catch (Violation &v) {
    b_res = false;
  } catch (Crash &c) {
    std::cout << c.what() << std::endl;
    return MonitorCrash;
  }
  if(a_res != b_res)
    return Mismatch;

  return a_res ? MatchLin : MatchViol;

}


#endif // EXECUTION_H_
