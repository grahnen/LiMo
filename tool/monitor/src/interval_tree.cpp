#include "interval_tree.hpp"
#include <stdexcept>

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
            l = i;
        }
        return this;
    }

    if( i->interval.inner.lbound < l->interval.inner.lbound ) {
        i->left = true;
        l = l->add(i);
    } else {
        i->left = false;
        r = r->add(i);
    }

    // Rebalance overlapping
    if( l->interval.inner.contains(r->interval.inner) ) {
        r->left = true;
        return l->add(r);
    }
    if( r->interval.inner.contains(l->interval.inner) ) {
        l->left = false;
        return r->add(l);
    }

    return this;
}

TreeNode *TreeNode::add(TreeNode *i) {
    if( dummy || interval.inner.contains(i->interval.inner) ) {
        TreeNode *n = do_add(i);
        n->update();
        return n;
    } else {
        if( interval.inner.lbound < i->interval.inner.lbound ) {
            left = true;
            i->left = false;
            return TreeNode::mk_dummy(this, i, true);
        } else {
            left = false;
            i->left = true;
            return TreeNode::mk_dummy(i, this, true);
        }
    }
}

TreeNode *TreeNode::rmv(Itv i) {
    if (interval == i) {
        if (dummy) {
            throw std::logic_error("Removing dummy??");
        } else {
            if ( l != nullptr || r != nullptr ) {
                dummy = true;
                update();
                return this;
            }
        }
    }

    if( l != nullptr && l->interval.inner.contains(i.inner) ) {
        l = l->rmv(i);
        update();
        if (dummy && l == nullptr)
            return r;
    } else if( r != nullptr && r->interval.inner.contains(i.inner) ) {
        r = r->rmv(i);
        update();
        if (dummy && r == nullptr)
            return l;
    } else {
        throw std::logic_error("Removing nothing?");
        return this;
    }

    if (l != nullptr && l->disjoint()) {
        TreeNode *nl = l->add(r);
        nl->update();
        return nl;
    }
    if (r != nullptr && r->disjoint()) {
        TreeNode *nr = r->add(l);
        nr->update();
        return nr;
    }

    update();
    return this;
}


bool TreeNode::disjoint() const {
    return l != nullptr && r != nullptr && ! l->interval.inner.overlaps(r->interval.inner);
}


TreeNode *TreeNode::rot_left() {
    TreeNode *root = r;
    this->r = root->l;
    root->l = this;
    root->size = size;
    this->update();
    root->update();
    return root;
}

TreeNode *TreeNode::rot_right() {
    TreeNode *root = l;
    this->l = root->r;
    root->r = this;
    root->size = this->size;
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
        this->interval = AtomicInterval::closed(l->interval.inner.lbound, r->interval.inner.ubound);
        // Find outer
        if( dummy ) {
            if ( outer_ctn(l, interval) ) {
                if( outer_ctn(r, interval) ) {
                    if (left) {
                        outer = (r->outer->outer.ubound < l->outer->outer.ubound) ? l->outer : r->outer;
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
        }
    }
    size = 1;
    if (l != nullptr) {
        size += l->size;
    }
    if (r != nullptr) {
        size += r->size;
    }
}


std::pair<TreeNode *, TreeNode *> TreeNode::disj_left() {
    if(l == nullptr)
        return std::pair(nullptr, this);

    if(disjoint()) {
        TreeNode *l = l;
        this->l = nullptr;
        update();
        return std::pair(l, this);
    }

    auto p = l->disj_left();
    if(p.first != nullptr) {
        l = p.second;
        update();
        return std::pair(p.first, this);
    }

    return std::pair(nullptr, this);
}

std::pair<TreeNode *, TreeNode *> TreeNode::disj_right() {
    if(r == nullptr)
        return std::pair(nullptr, this);

    if(disjoint()) {
        TreeNode *r = r;
        this->r = nullptr;
        update();
        return std::pair(r, this);
    }

    auto p = l->disj_right();
    if(p.first != nullptr) {
        r = p.second;
        update();
        return std::pair(p.first, this);
    }

    return std::pair(nullptr, this);
}
