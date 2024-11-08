#ifndef TREEMONITOR_H_
#define TREEMONITOR_H_


#include "interval_tree.hpp"
#include "monitor.hpp"
#include <map>


class TreeMonitor : public Monitor {
        std::map<tid_t, val_t> active;
        std::map<val_t, Itv> values;

        TreeNode *tree = nullptr;

        DECLHANDLER(push)
        DECLHANDLER(pop)

        bool ADT_supported(ADT adt) { return adt == stack; }

    public:
        TreeMonitor(MonitorConfig mc);
        ~TreeMonitor();

        void print_state() const;
        void do_linearization();
};


#endif // TREEMONITOR_H_
