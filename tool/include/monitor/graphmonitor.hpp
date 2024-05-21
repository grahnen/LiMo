#ifndef GRAPHMONITOR_H_
#define GRAPHMONITOR_H_

#include "monitor.hpp"
#include "monitor/graph.hpp"

class GraphMonitor : public Monitor {
    private:
        tid_t max_threads;
        std::vector<val_t> rmv_order;
        int cover_depth;
        std::vector<std::optional<bool>> rmvEmpties;
        ConstraintGraph graph;
        std::vector<optval_t> active;

        DECLHANDLER(rmvEmpty)
        DECLHANDLER(push)
        DECLHANDLER(pop)
        DECLHANDLER(enq)
        DECLHANDLER(deq)


    public:
        GraphMonitor(MonitorConfig mc);
        ~GraphMonitor();

        void print_state() const;
        void do_linearization();
};

#endif // GRAPHMONITOR_H_
