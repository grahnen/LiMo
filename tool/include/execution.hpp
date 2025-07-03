#ifndef EXECUTION_H_
#define EXECUTION_H_
#include "io.h"
#include "generator.hpp"
#include "exception.h"
#include "convert.h"


Configuration *hist_from_ints(int count, int *data, ExhaustiveGenerator::HistoryType type) {
  std::vector<event_t> history;
  auto h = ExhaustiveGenerator::make_history(count, data, type);
  tid_t ts = 0;

  tid_t max_t = 0;

  //Need to change this: make a per-hisory ev_t -> event_t function
  std::transform(h.begin(), h.end(), std::back_inserter(history), [&ts, &max_t, &type](ExhaustiveGenerator::ev_t evt) {
    ts++;
    max_t = std::max(evt.th, max_t);
    // return event_t(evt.t, evt.th, val_t(evt.th, 0), ts);
    return ExhaustiveGenerator::getEventFromEv(evt, ts, type);
  });

  ADT adt = ADT::stack;
  // for(int i = 0; i < count; i++) {
  //   if (data[i] == -1)
  //     adt = ADT::durable_stack;
  // }
  if(type != ExhaustiveGenerator::NORMAL)
    adt = ADT::durable_stack;

  // std::cout<<"ADT is "<<adt<<"\n";
  Configuration *conf = new Configuration(history, adt, history.size(), (adt == ADT::stack));
  if(conf->needs_simpl)
  {
    Configuration *simpl = simplify(conf);
    delete conf;

    return simpl;
  }
  else
  {
    return conf;
  }
}


Configuration *hist_from_ints(int sz, std::vector<int> &int_h, ExhaustiveGenerator::HistoryType type) {
  return hist_from_ints(sz, int_h.data(), type);
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
