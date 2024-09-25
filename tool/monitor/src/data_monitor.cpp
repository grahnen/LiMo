#include "monitor/data_monitor.hpp"
#include "interval.h"
#include <cassert>
#include <iostream>
#include "exception.h"
#include "typedef.h"

DataMonitor::DataMonitor(MonitorConfig mc) : Monitor(mc) {}

void DataMonitor::ensure_size(tid_t ethread, tid_t vthread, index_t idx) {
  tid_t thread = std::max(ethread, vthread);
  if (thread >= open.size()) {
    open.resize(thread + 1);
  }
  if (thread >= history.size()) {
    history.resize(thread + 1);
  }
  if(idx >= history[vthread].size()) {
    history[vthread].resize(idx + 1);
  }
}

void DataMonitor::handle_push(event_t &e) {
  val_t &v = e.val.value();
  ensure_size(e.thread, v.thread, v.idx);
  state.may_be_present++;
  history[v.thread][v.idx].pushcall = state;
  open[e.thread] = std::pair(v, state);
}

void DataMonitor::handle_ret_push(event_t &e) {
  std::pair<val_t, state_t> vs = open[e.thread];
  state.must_be_present++;
  history[vs.first.thread][vs.first.idx].pushret = state;
}

void DataMonitor::handle_pop(event_t &e) {
    state.must_be_present--;
    val_t v(0,0);
    if(e.val.has_value()) {
      v = e.val.value();
      history[v.thread][v.idx].popcall = state;
    }
    ensure_size(e.thread, v.thread, v.idx);
    open[e.thread] = std::pair(v, state);
}

void DataMonitor::handle_ret_pop(event_t &e) {
  state.may_be_present--;
  if (e.val.has_value()) {
    val_t &v = e.val.value();
    ensure_size(e.thread, v.thread, v.idx);
    state_t pc_s = open[v.thread].second;
    history[v.thread][v.idx].popcall = pc_s;
    history[v.thread][v.idx].popret = state;
  } else {
    auto q = open[e.thread];
    assert(history [q.first.thread][q.first.idx].popcall == q.second);
    history[q.first.thread][q.first.idx].popret = state;
  }
}

void DataMonitor::print_state() const {
  std::cout << state << std::endl;
}

std::vector<AtomicInterval> DataMonitor::collect_bounds() {
  std::vector<AtomicInterval> vec;
  for(tid_t t = 0; t < history.size(); t++) {
    for(index_t i = 0; i < history[t].size(); i++) {
      vec.push_back(history[t][i].bounds());
    }
  }

  std::sort(vec.begin(), vec.end());
  for (auto it : vec) {
    std::cout << it << std::endl;
  }
  return vec;
}

void DataMonitor::do_linearization() {
  std::vector<AtomicInterval> bounds = collect_bounds();
  int i = 0;
  int level = 0;
  while(i < bounds.size()) {
    if(level >= bounds[i].lbound) {
      level++;
      i++;
    }
    else
      throw Violation("Level out of reach!");
  }
}
