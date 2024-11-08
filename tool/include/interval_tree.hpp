#ifndef INTERVAL_TREE_H_
#define INTERVAL_TREE_H_


#include "interval.h"

class TreeNode;

class Itv {
    public:
        AtomicInterval inner, outer;
        Itv(AtomicInterval inner, AtomicInterval outer) : inner(inner), outer(outer) {}
        Itv(AtomicInterval itv) : inner(itv), outer(itv) {}
        Itv() : Itv(AtomicInterval::nil()) {}

        static Itv nil() { return Itv(AtomicInterval::nil(), AtomicInterval::nil()); }
        bool operator==(const Itv &o) const {
            return inner == o.inner && outer == o.outer;
        }
};


bool outer_ctn(TreeNode *node, Itv interval);

struct TreeNode {
    Itv interval;
    std::optional<Itv> outer;

    bool dummy,left;
    TreeNode *l, *r;
    size_t size;

    static TreeNode *mk_dummy(TreeNode *l, TreeNode *r, bool left) {
        return new TreeNode(Itv::nil(), true, l, r, left);
    }

    static TreeNode *mk_real(Itv i) {
        return new TreeNode(i, false);
    }



    TreeNode(Itv i, bool dummy = true, TreeNode *l = nullptr, TreeNode *r = nullptr, bool left = true) : interval(i), outer({}), dummy(dummy), l(l), r(r), size(0),left(left) {
        update();
    }

    void update();
    bool disjoint() const;

    TreeNode *do_add(TreeNode *i);
    TreeNode *add(TreeNode *i);
    TreeNode *rmv(Itv i);

    TreeNode *balance();
    TreeNode *rot_left();
    TreeNode *rot_right();


    std::pair<TreeNode *, TreeNode *> disj_left();
    std::pair<TreeNode *, TreeNode *> disj_right();

    double szbal() const { return ((double) (l == nullptr) ? 0 : l->size) / ((double) size); }

};

inline TreeNode *tree_add(TreeNode *root, TreeNode *i) {
    if( root == nullptr ) {
        return i;
    }
    return root->add(i);
}

inline TreeNode *tree_rmv(TreeNode *root, Itv i) {
    if( root == nullptr )
        return nullptr;
    return root->rmv(i);
}

#endif // INTERVAL_TREE_H_
