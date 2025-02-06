#include "interval_tree.hpp"
#include "exception.h"
#include <stdexcept>

std::atomic<index_t> id_ctr = 0;

bool outer_ctn(TreeNode *node, Itv interval) {
    if( node == nullptr || !node->outer.has_value() ) {
        return false;
    }
    return node->outer->outer.contains(interval.inner);
}

TreeNode *TreeNode::do_add(TreeNode *i) {
    if (l == nullptr) {
       l = i;
       return this;
    }

    if ( r == nullptr ) {
        if(i->interval.inner.lbound < l->interval.inner.lbound) {
            r = i;
        } else {
            r = l;
            r->left = false;
            l = i;
            i->left = true;
        }
        return this;
    }

    if(i->interval.inner.ubound < interval.inner.lbound) {
        i->left = true;
        this->left = false;
        return mk_dummy(i, this, true);
    }

    if(interval.inner.ubound < i->interval.inner.lbound) {
        i->left = false;
        this->left = true;
        return mk_dummy(this,i, true);
    }

    if (i->interval.inner.lbound > r->interval.inner.lbound) {
        i->left = false;
        r = r->add(i);
    }
    else if( i->interval.inner.lbound < l->interval.inner.ubound) {
        i->left = true;
        l = l->add(i);
    } else {
        i->left = false;
        r = r->add(i);
    }

    // Rebalance overlapping..
    // TODO Gives a stack overflow because this can be an idempotent operation
    // When do we actually need it?
    //
    // if( l->dummy && l->interval.inner.contains(r->interval.inner) ) {
    //     r->left = true;
    //     TreeNode *res = l->add(r);
    //     delete this;
    //     return res;
    // }

    // if( r->dummy && r->interval.inner.contains(l->interval.inner) ) {
    //     l->left = false;
    //     TreeNode *res = r->add(l);
    //     delete this;
    //     return res;
    // }

    update();
    return this;
}

TreeNode *TreeNode::add(TreeNode *i) {
    if( dummy ) {
        TreeNode *n = do_add(i);
        n->update();
        return n;
    } else {
        if( interval.inner.lbound < i->interval.inner.lbound ) {
            bool dir = left;
            left = true;
            i->left = false;
            return TreeNode::mk_dummy(this, i, dir);
        } else {
            bool dir = left;
            left = false;
            i->left = true;
            return TreeNode::mk_dummy(i, this, dir);
        }
    }
}

TreeNode *TreeNode::rmv(Itv i) {
    if (interval == i) {
        if (dummy) {
            if(l == nullptr && r == nullptr) {
                throw std::logic_error("Dummy without children?");
            } else if (l == nullptr) {
                throw std::logic_error("Dummy without l?");
            } else if (r == nullptr) {
                throw std::logic_error("Dummy without r?");
            }
        } else {
            if (l != nullptr && r != nullptr) {
                dummy = true;
                update();
                return this;
            }
            else if ( l != nullptr || r != nullptr ) {
                if(l != nullptr)
                    return l;
                return r;
            } else {
                delete this;
                return nullptr;
            }
        }
    }

    if( r != nullptr && r->outer.has_value() && r->outer.value() == i ) {
        r = r->rmv(i);

        if(r != nullptr) {
            r->update();
        }
        update();
        if (dummy && r == nullptr) {
            l->left = left;
            TreeNode *res = l;
            delete this;
            return l;
        }
    } else if( l != nullptr && l->outer.has_value() && l->outer.value() == i ) {
        l = l->rmv(i);

        if(l != nullptr) {
            l->update();
        }
        update();
        if (dummy && l == nullptr) {
            r->left = left;
            TreeNode *res = r;
            delete this;
            return r;
        }
    } else {
        throw Violation("Removing nothing???");
        throw std::logic_error("Removing nothing?");
        return this;
    }

    // if (l != nullptr && l->disjoint()) {
    //     TreeNode *nl = l->add(r);
    //     nl->update();
    //     return nl;
    // }
    // if (r != nullptr && r->disjoint()) {
    //     TreeNode *nr = r->add(l);
    //     nr->update();
    //     return nr;
    // }
    return this;
}


bool TreeNode::disjoint() const {
    return l != nullptr &&
        r != nullptr &&
        ! l->interval.inner.overlaps(r->interval.inner);
}


TreeNode *TreeNode::rot_left() {
    TreeNode *root = r;
    this->r = root->l;
    r->left = false;
    root->l = this;
    root->size = size;
    left = true;
    this->update();
    root->update();
    return root;
}

TreeNode *TreeNode::rot_right() {
    TreeNode *root = l;
    this->l = root->r;
    l->left = true;
    root->r = this;
    root->size = this->size;
    left = false;
    update();
    root->update();
    return root;
}

TreeNode *TreeNode::balance() {
    if(size == 1 || l == nullptr) {
        return this;
    }

    double wbal = szbal();

    if(wbal > 0.70711) {
        if (l->szbal() > 0.414213) {
            return rot_right();
        } else {
            l = l->rot_left();
            return rot_right();
        }
    } else if (wbal < 0.29289 && r != nullptr ) {
        if( r->szbal() < 0.585786 ) {
            return rot_left();
        } else {
            r = r->rot_right();
            return rot_left();
        }
    }

    return this;
}


void TreeNode::update() {
    if(dummy){
        if (l == nullptr)
            this->interval = r->interval;
        else if (r == nullptr)
            this->interval = l->interval;
        else {
            this->interval =
                AtomicInterval::closed(
                    l->interval.inner.lbound,
                    std::max(l->interval.inner.ubound, r->interval.inner.ubound));
        }
        // Find outer
        if ( outer_ctn(l, interval) ) {
            if( outer_ctn(r, interval) ) {
                if (left) {
                    outer = (l->outer->outer.ubound < r->outer->outer.ubound) ? r->outer : l->outer;
                } else {
                    outer = (l->outer->outer.lbound < r->outer->outer.lbound) ? l->outer : r->outer;
                }
            } else {
                outer = l->outer;
            }
        } else if (outer_ctn(r, interval)) {
            outer = r->outer;
        } else {
            outer = {};
        }
    } else {
        outer = this->interval;
    }


    size = 1;
    if (l != nullptr) {
        size += l->size;
    }
    if (r != nullptr) {
        size += r->size;
    }
}

std::pair<TreeNode *, TreeNode *> TreeNode::disj_left(timestamp_t minimal_right_lb) {
    if(l == nullptr)
        return std::pair(nullptr, this);

    if(disjoint() && l->interval.inner.ubound < minimal_right_lb) {
        TreeNode *nl = l;
        this->l = nullptr;
        update();

        if(dummy) {
            TreeNode *res = this->r;
            res->left = left;
            delete this;
            return std::pair(nl, res);
        }
        return std::pair(nl, this);
    }

    auto p = l->disj_left(std::min(minimal_right_lb, r->interval.inner.lbound));
    if(p.first != nullptr) {
        this->l = p.second;
        this->l->update();
        update();
        return std::pair(p.first, this);
    }

    return std::pair(nullptr, this);
}

std::pair<TreeNode *, TreeNode *> TreeNode::disj_right(timestamp_t maximal_left_ub) {
    if(r == nullptr) {
        return std::pair(nullptr, this);
    }
    if(disjoint() && r->interval.inner.lbound > maximal_left_ub) {
        TreeNode *nr = r;
        this->r = nullptr;
        update();
        if(dummy) {
            TreeNode *res = this->l;
            res->left = left;
            delete this;
            return std::pair(nr, res);
        }
        return std::pair(nr, this);
    }

    auto p = r->disj_right(std::max(maximal_left_ub, l->interval.inner.ubound));
    if(p.first != nullptr) {
        this->r = p.second;
        this->r->update();
        update();
        return std::pair(p.first, this);
    }

    return std::pair(nullptr, this);
}

std::ostream &TreeNode::to_dot(std::ostream &os) const {
    os << id
       << "[label=\""
       << interval
       << (dummy ? "d" : "")
       << (left ? "L" : "R")
       << outer
       << "\"]\n";

    if( l != nullptr ) {
        os << id << " -> " << l->id << ";\n";
        l->to_dot(os);
    }

    if( r != nullptr ) {
        os << id << " -> " << r->id << ";\n";
        r->to_dot(os);
    }

    return os;
}
