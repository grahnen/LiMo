#ifndef INTERVAL_TREE_H_
#define INTERVAL_TREE_H_


#include "interval.h"

class TreeNode;

extern std::atomic<index_t> id_ctr;

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

inline std::ostream &operator<<(std::ostream &os, Itv i) {
    return os << "(" << i.outer.lbound << i.inner << i.outer.ubound << ")";
}


bool outer_ctn(TreeNode *node, Itv interval);

struct TreeNode {
    Itv interval;
    std::optional<Itv> outer;
    index_t id;
    bool dummy,left;
    TreeNode *l, *r;
    size_t size;

    static TreeNode *mk_dummy(TreeNode *l, TreeNode *r, bool left) {
        return new TreeNode(Itv::nil(), true, l, r, left);
    }

    static TreeNode *mk_real(Itv i) {
        return new TreeNode(i, false);
    }

    TreeNode(Itv i, bool dummy = true, TreeNode *l = nullptr, TreeNode *r = nullptr, bool left = true) : interval(i), outer({}), dummy(dummy), l(l), r(r), size(0),left(left), id(id_ctr++) {
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


    std::pair<TreeNode *, TreeNode *> disj_left(timestamp_t minimal_right_lb);
    std::pair<TreeNode *, TreeNode *> disj_right(timestamp_t maximal_left_ub);

    std::ostream &to_dot(std::ostream &os) const;

    double szbal() const { return ((double) (l == nullptr) ? 0 : l->size) / ((double) size); }

};

inline TreeNode *tree_add(TreeNode *root, TreeNode *i) {
    if( root == nullptr ) {
        return i;
    }
    root = root->add(i);
    //root->balance();
    return root;
}

inline TreeNode *tree_rmv(TreeNode *root, Itv i) {
    if( root == nullptr )
        return nullptr;
    return root->rmv(i);
}

#endif // INTERVAL_TREE_H_
