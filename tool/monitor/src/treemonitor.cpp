#include "monitor/treemonitor.hpp"
#include "exception.h"
#include "interval_tree.hpp"
#include "monitor.hpp"
#include "util.h"
#include <cstdio>
#include <vector>

void write_dot(std::string filename, TreeNode *n) {
    if(n == nullptr)
        return;
    std::fstream file;
    file.open(filename, std::ios::out);

    file << "digraph G {";
    n->to_dot(file);
    file << "}";

    file.close();
}


TreeMonitor::TreeMonitor(MonitorConfig mc) : Monitor(mc), tree(nullptr) {}
TreeMonitor::~TreeMonitor() {}

Itv &get_itv(std::map<val_t, Itv> &vls, val_t v) {
    if(!vls.contains(v))
        vls[v] = Itv::nil();

    return vls.at(v);
}

void TreeMonitor::handle_push(event_t &ev) {
    active[ev.thread] = ev.val.value();
    Itv &i = get_itv(values, ev.val.value());
    i.outer.lbound = ev.timestamp;
}
void TreeMonitor::handle_ret_push(event_t &ev) {
    val_t v = active[ev.thread];
    Itv &i = get_itv(values, v);
    i.inner.lbound = ev.timestamp;
}

void TreeMonitor::handle_pop(event_t &ev) {
    active[ev.thread] = ev.val.value();
    Itv &i = get_itv(values, ev.val.value());
    i.inner.ubound = ev.timestamp;
}
void TreeMonitor::handle_ret_pop(event_t &ev) {
    val_t v = active[ev.thread];
    Itv &i = get_itv(values, v);
    i.outer.ubound = ev.timestamp;

    TreeNode *n = TreeNode::mk_real(i);
    tree = tree_add(tree, n);
    if(verbose) {
            write_dot(dot_dir + "/dot" + ext2str(dot_ctr) + ".dot", tree);
            dot_ctr++;
    }
}


void TreeMonitor::print_state() const {}
void TreeMonitor::do_linearization() {
    std::vector<TreeNode *> trees;
    trees.push_back(tree);
    TreeNode *t = nullptr;
    while(!trees.empty()) {
        t = trees.back();
        trees.pop_back();
        if(verbose) {
            write_dot(dot_dir + "/dot" + ext2str(dot_ctr) + ".dot", t);
            dot_ctr++;
        }

        if( t == nullptr || t->size == 1) {
            continue;
        }

        if(t->l == nullptr || t->r == nullptr) {
            if( t->l != nullptr )
                trees.push_back(t->l);
            if( t->r != nullptr )
                trees.push_back(t->r);
        } else if( t->disjoint() ) {
            // Separation!
            trees.push_back(t->l);
            trees.push_back(t->r);
        } else {
            if(t->dummy) {
                if(t->outer.has_value()) {
                    TreeNode *tp = t->rmv(t->outer.value());
                    tp = tp->balance();
                    trees.push_back(tp);
                } else {
                    auto p = t->disj_left(t->r->interval.inner.lbound);
                    if(p.first == nullptr) {
                        auto q = t->disj_right(t->l->interval.inner.ubound);
                        if( q.first == nullptr ) {
                            throw Violation("Inseparable and no minmax!");
                        } else {

                            trees.push_back(q.first);
                            trees.push_back(q.second);
                        }
                    } else {
                        // there was a sep left
                        trees.push_back(p.first);
                        trees.push_back(p.second);
                    }
                }
            } else {
                // Minmax
                TreeNode *tp = t->rmv(t->interval);
                trees.push_back(tp);
            }
        }

    }
}
