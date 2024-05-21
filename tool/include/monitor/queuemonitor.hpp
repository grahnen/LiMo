#ifndef QUEUE_MONITOR_H
#define QUEUE_MONITOR_H
#include "monitor.hpp"
#include <vector>
#include <ostream>

using Graph = std::map<val_t, std::set<val_t>>;


struct op_node {
  optval_t v;
  bool add;
  bool done;
  std::map<tid_t, int> succ;
  op_node(val_t v, bool add) : v(v), add(add), done(false), succ() {}
  friend std::ostream &operator<<(std::ostream &o, const op_node &op);
};

class QueueMonitor : public Monitor {
protected:
  std::map<tid_t, int> last_finished_enq;
  std::map<tid_t, int> last_finished_deq;
  
  std::map<tid_t, std::vector<op_node>> ops;
  std::map<tid_t, event_t> callee;

  void assume_has_vec(tid_t thr);
 public:
  QueueMonitor(MonitorConfig mc);
  
  DECLHANDLER(enq)
  DECLHANDLER(deq)

  void do_linearization();
  void print_state() const;
};

bool is_cyclic(Graph g);

#endif
