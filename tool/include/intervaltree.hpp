#include "interval.h"
#ifndef __INTERVAL_TREE_H
#include "typedef.h"
#include "exception.h"
using namespace std;

struct ItvNode
{
    AtomicInterval itv;
    timestamp_t k;
    ItvNode(AtomicInterval itv = AtomicInterval::nil(), timestamp_t k = 0) : itv(itv), k(k) {}
    bool operator<(const ItvNode &o) const { return itv < o.itv; }
    bool operator==(const ItvNode &o) const {return itv == o.itv;}
    bool operator<=(const ItvNode &o) const {return itv <= o.itv;}
};

inline ostream& operator<<(std::ostream& out, const ItvNode& i)
{   
    out << "<" << i.itv << "," << i.k << ">" ;

    return out;
}   

// Enumeration for colors of nodes in Red-Black Tree
enum Color
{
    RED,
    BLACK
};

// Class template for Red-Black Tree
template <typename T>
class RedBlackTree
{
public:
    // Structure for a node in Red-Black Tree
    struct Node
    {
        T data;
        Color color;
        Node *parent;
        Node *left;
        Node *right;

        // Constructor to initialize node with data and
        // color
        Node(T value)
            : data(value), color(RED), parent(nullptr), left(nullptr), right(nullptr)
        {
        }
    };

    // Utility function: Left Rotation
    virtual void rotateLeft(Node *&node)
    {
        Node *child = node->right;
        node->right = child->left;
        if (node->right != nullptr)
            node->right->parent = node;
        child->parent = node->parent;
        if (node->parent == nullptr)
            root = child;
        else if (node == node->parent->left)
            node->parent->left = child;
        else
            node->parent->right = child;
        child->left = node;
        node->parent = child;
    }

    // Utility function: Right Rotation
    virtual void rotateRight(Node *&node)
    {
        Node *child = node->left;
        node->left = child->right;
        if (node->left != nullptr)
            node->left->parent = node;
        child->parent = node->parent;
        if (node->parent == nullptr)
            root = child;
        else if (node == node->parent->left)
            node->parent->left = child;
        else
            node->parent->right = child;
        child->right = node;
        node->parent = child;
    }

    // Utility function: Fixing Insertion Violation
    virtual void fixInsert(Node *&node)
    {
        Node *parent = nullptr;
        Node *grandparent = nullptr;
        while (node != root && node->color == RED && node->parent->color == RED)
        {
            parent = node->parent;
            grandparent = parent->parent;
            if (parent == grandparent->left)
            {
                Node *uncle = grandparent->right;
                if (uncle != nullptr && uncle->color == RED)
                {
                    grandparent->color = RED;
                    parent->color = BLACK;
                    uncle->color = BLACK;
                    node = grandparent;
                }
                else
                {
                    if (node == parent->right)
                    {
                        rotateLeft(parent);
                        node = parent;
                        parent = node->parent;
                    }
                    rotateRight(grandparent);
                    swap(parent->color, grandparent->color);
                    node = parent;
                }
            }
            else
            {
                Node *uncle = grandparent->left;
                if (uncle != nullptr && uncle->color == RED)
                {
                    grandparent->color = RED;
                    parent->color = BLACK;
                    uncle->color = BLACK;
                    node = grandparent;
                }
                else
                {
                    if (node == parent->left)
                    {
                        rotateRight(parent);
                        node = parent;
                        parent = node->parent;
                    }
                    rotateLeft(grandparent);
                    swap(parent->color, grandparent->color);
                    node = parent;
                }
            }
        }
        root->color = BLACK;
    }

    // Utility function: Fixing Deletion Violation
    virtual void fixDelete(Node *&node)
    {
        while (node != root && node->color == BLACK)
        {
            if (node == node->parent->left)
            {
                Node *sibling = node->parent->right;
                if (sibling->color == RED)
                {
                    sibling->color = BLACK;
                    node->parent->color = RED;
                    rotateLeft(node->parent);
                    sibling = node->parent->right;
                }
                if ((sibling->left == nullptr || sibling->left->color == BLACK) && (sibling->right == nullptr || sibling->right->color == BLACK))
                {
                    sibling->color = RED;
                    node = node->parent;
                }
                else
                {
                    if (sibling->right == nullptr || sibling->right->color == BLACK)
                    {
                        if (sibling->left != nullptr)
                            sibling->left->color = BLACK;
                        sibling->color = RED;
                        rotateRight(sibling);
                        sibling = node->parent->right;
                    }
                    sibling->color = node->parent->color;
                    node->parent->color = BLACK;
                    if (sibling->right != nullptr)
                        sibling->right->color = BLACK;
                    rotateLeft(node->parent);
                    node = root;
                }
            }
            else
            {
                Node *sibling = node->parent->left;
                if (sibling->color == RED)
                {
                    sibling->color = BLACK;
                    node->parent->color = RED;
                    rotateRight(node->parent);
                    sibling = node->parent->left;
                }
                if ((sibling->left == nullptr || sibling->left->color == BLACK) && (sibling->right == nullptr || sibling->right->color == BLACK))
                {
                    sibling->color = RED;
                    node = node->parent;
                }
                else
                {
                    if (sibling->left == nullptr || sibling->left->color == BLACK)
                    {
                        if (sibling->right != nullptr)
                            sibling->right->color = BLACK;
                        sibling->color = RED;
                        rotateLeft(sibling);
                        sibling = node->parent->left;
                    }
                    sibling->color = node->parent->color;
                    node->parent->color = BLACK;
                    if (sibling->left != nullptr)
                        sibling->left->color = BLACK;
                    rotateRight(node->parent);
                    node = root;
                }
            }
        }
        node->color = BLACK;
    }

    // Utility function: Find Node with Minimum Value
    Node *minValueNode(Node *&node)
    {
        Node *current = node;
        while (current->left != nullptr)
            current = current->left;
        return current;
    }

    // Utility function: Transplant nodes in Red-Black Tree
    virtual void transplant(Node *&root, Node *&u, Node *&v)
    {
        if (u->parent == nullptr)
            root = v;
        else if (u == u->parent->left)
            u->parent->left = v;
        else
            u->parent->right = v;
        if (v != nullptr)
            v->parent = u->parent;
    }

    // Utility function: Helper to print Red-Black Tree
    void printHelper(Node *root, string indent, bool last)
    {
        if (root != nullptr)
        {
            cout << indent;
            if (last)
            {
                cout << "R----";
                indent += "   ";
            }
            else
            {
                cout << "L----";
                indent += "|  ";
            }
            string sColor = (root->color == RED) ? "RED" : "BLACK";
            cout << root->data << "(" << sColor << ")"
                 << endl;
            printHelper(root->left, indent, false);
            printHelper(root->right, indent, true);
        }
    }

    // Utility function: Delete all nodes in the Red-Black
    // Tree
    void deleteTree(Node *node)
    {
        if (node != nullptr)
        {
            deleteTree(node->left);
            deleteTree(node->right);
            delete node;
        }
    }
    Node *root; // Root of the Red-Black Tree
    // Constructor: Initialize Red-Black Tree
    RedBlackTree()
        : root(nullptr)
    {
    }

    // Destructor: Delete Red-Black Tree
    ~RedBlackTree() { deleteTree(root); }

    // Public function: Insert a value into Red-Black Tree
    virtual void insert(T key)
    {
        Node *node = new Node(key);
        Node *parent = nullptr;
        Node *current = root;
        while (current != nullptr)
        {
            parent = current;
            if (node->data < current->data)
                current = current->left;
            else
                current = current->right;
        }
        node->parent = parent;
        if (parent == nullptr)
            root = node;
        else if (node->data < parent->data)
            parent->left = node;
        else
            parent->right = node;
        fixInsert(node);
    }

    // Public function: Remove a value from Red-Black Tree
    virtual void remove(T key)
    {
        Node *node = root;
        Node *z = nullptr;
        Node *x = nullptr;
        Node *y = nullptr;
        while (node != nullptr)
        {
            if (node->data == key)
            {
                z = node;
            }

            if (node->data <= key)
            {
                node = node->right;
            }
            else
            {
                node = node->left;
            }
        }

        if (z == nullptr)
        {
            cout << "Key not found in the tree" << endl;
            return;
        }

        y = z;
        Color yOriginalColor = y->color;
        if (z->left == nullptr)
        {
            x = z->right;
            transplant(root, z, z->right);
        }
        else if (z->right == nullptr)
        {
            x = z->left;
            transplant(root, z, z->left);
        }
        else
        {
            y = minValueNode(z->right);
            yOriginalColor = y->color;
            x = y->right;
            if (y->parent == z)
            {
                if (x != nullptr)
                    x->parent = y;
            }
            else
            {
                transplant(root, y, y->right);
                y->right = z->right;
                y->right->parent = y;
            }
            transplant(root, z, y);
            y->left = z->left;
            y->left->parent = y;
            y->color = z->color;
        }
        delete z;
        if (yOriginalColor == BLACK)
        {
            fixDelete(x);
        }
    }

    // Public function: Print the Red-Black Tree
    void printTree()
    {
        if (root == nullptr)
            cout << "Tree is empty." << endl;
        else
        {
            cout << "Red-Black Tree:" << endl;
            printHelper(root, "", true);
        }
    }
};
using ItvTree = RedBlackTree<ItvNode>;

inline timestamp_t compute_k(ItvTree::Node *t)
{
    if (t == nullptr)
        return 0;

    timestamp_t l = compute_k(t->left);
    timestamp_t r = compute_k(t->right);
    // std::cout << "Got sub" << std::endl;
    t->data.k = std::max(l, std::max(r, t->data.itv.ubound));
    return t->data.k;
}

inline bool contains(ItvTree::Node *t, AtomicInterval i)
{
    if (t == nullptr)
        return false;
    timestamp_t L = i.lbound;
    timestamp_t R = i.ubound;

    timestamp_t ii = t->data.itv.lbound;
    timestamp_t jj = t->data.itv.ubound;
    timestamp_t kk = t->data.k;

    if (ii <= L && R <= jj)
        return true;
    if (t->left != nullptr && ii <= L && R <= t->left->data.k)
        return true;
    if (t->left != nullptr && ii <= L && R > t->left->data.k)
        return contains(t->right, i);
    if (L <= ii)
        return contains(t->left, i);
    return false;
}

inline bool overlap(ItvTree::Node *t,const AtomicInterval& i, AtomicInterval &itv)
{
    if (t == nullptr)
        return false;
    timestamp_t L = i.lbound;
    timestamp_t R = i.ubound;

    timestamp_t ii = t->data.itv.lbound;
    timestamp_t jj = t->data.itv.ubound;
    timestamp_t kk = t->data.k;

    if (L <= jj && ii <= R)
    {
        itv = t->data.itv;
        return true;
    }
    else if (kk < L)
        return false;
    else if (R < ii)
        return overlap(t->left, i, itv);
    else if (jj < L && L <= kk)
    {
        if (t->left != nullptr && t->left->data.k >= L)
        {
            return overlap(t->left, i, itv);
        }
        else
        {
            return overlap(t->right, i, itv);
        }
    }

    throw Exception("Unhandled case!");
    return false;
}

/**
 * This class implements an interval tree as a red-black tree
 * It inherits from the red-black tree class
 * It fixes the high key of all node eagerly, i.e. after every insert/delete
 * operation
 * compute_k need not be called on this
 */
class EagerItvTree : public ItvTree
{

public:
    static inline std::ostream* output = &std::cout;
    bool verbose;

    void set_verbose(bool v)
    {
        verbose = v;
    }

    void set_output(std::ostream* out)
    {
        output = out;
    }

    void fixAncestors(Node *node)
    {
        while (node != nullptr)
        {
            timestamp_t l = (node->left == nullptr) ? NEGINF : node->left->data.k;
            timestamp_t r = (node->right == nullptr) ? NEGINF : node->right->data.k;
            node->data.k = std::max(std::max(l, r), node->data.itv.ubound);
            node = node->parent;
        }
    }

    void rotateLeft(Node *&node)
    {
        ItvTree::rotateLeft(node);
        fixAncestors(node);
    }

    void rotateRight(Node *&node)
    {
        ItvTree::rotateRight(node);
        fixAncestors(node);
    }

    void fixInsert(Node *&node)
    {
        fixAncestors(node);
        ItvTree::fixInsert(node);
    }

    void fixDelete(Node *& node)
    {
        fixAncestors(node);
        ItvTree::fixDelete(node);
    }

    void swap(Node*& root, Node*& u, Node*& v)
    {
        if(u == nullptr)
            return;
        if(v == nullptr)
            return;
        std::swap(u->data, v->data);
        std::swap(u,v);
    }

    //This function fixes a seg-fault in RedBlackTree fixDelete function
    void fixDeleteLeaf(Node *& node)
    {
        if(node == nullptr)
            return;
        if(node->left == nullptr && node->right == nullptr)
            return;
        if(node->left == nullptr)
        {
            Node* sibling = node->right;
            if(sibling->color == RED)
            {
                std::swap(node->color, sibling->color);
                rotateLeft(node);
                sibling = node->right;
            }

            if ((sibling->left == nullptr || sibling->left->color == BLACK) && (sibling->right == nullptr || sibling->right->color == BLACK))
            {
                sibling->color = RED;
                fixDelete(node);
            }
            else
            {
                if(sibling->right == nullptr || sibling->right->color == BLACK)
                {
                    if (sibling->left != nullptr)
                        sibling->left->color = BLACK;
                    sibling->color = RED;
                    rotateRight(sibling);
                    sibling = node->right;
                }
                sibling->color = node->color;
                node->color = BLACK;
                if (sibling->right != nullptr)
                    sibling->right->color = BLACK;
                rotateLeft(node);
            }
           
        }
        else
        {
            Node* sibling = node->left;
            if(sibling->color == RED)
            {
                std::swap(node->color, sibling->color);
                rotateRight(node);
                sibling = node->left;
            }

            if ((sibling->right == nullptr || sibling->right->color == BLACK) && (sibling->left == nullptr || sibling->left->color == BLACK))
            {
                sibling->color = RED;
                fixDelete(node);
            }
            else
            {
                if(sibling->left == nullptr || sibling->left->color == BLACK)
                {
                    if (sibling->right != nullptr)
                        sibling->right->color = BLACK;
                    sibling->color = RED;
                    rotateLeft(sibling);
                    sibling = node->left;
                }
                sibling->color = node->color;
                node->color = BLACK;
                if (sibling->left != nullptr)
                    sibling->left->color = BLACK;
                rotateRight(node);
            }
        }
    }
    // Public function: Remove a value from Red-Black Tree
    void remove(ItvNode key)
    {
        if(verbose)
        {
            *output << "Removing: " << key << std::endl;
        }

        Node *node = root;
        Node *z = nullptr;
        Node *x = nullptr;
        Node *y = nullptr;
        Node *zParent = nullptr;
        while (node != nullptr)
        {
            if (node->data == key)
            {
                z = node;
            }

            if (node->data <= key)
            {
                node = node->right;
            }
            else
            {
                node = node->left;
            }
        }

        if (z == nullptr)
        {
            throw Exception("key not found");
            // cout << "Key not found in the tree" << endl;
            return;
        }

        y = z;
        Color yOriginalColor = y->color;
        if (z->left == nullptr)
        {
            zParent = z->parent;
            x = z->right;
            transplant(root, z, z->right);
        }
        else if (z->right == nullptr)
        {
            zParent = z->parent; 
            x = z->left;
            transplant(root, z, z->left);
        }
        else
        {
            // printTree();
            y = minValueNode(z->right);
            swap(root,z,y);
            // printTree();
            removeNode(z);
            return;
        }
        delete z;
        if(x != nullptr)
            fixAncestors(x);
        else
            fixAncestors(zParent);
        fixAncestors(y);
        if (yOriginalColor == BLACK)
        {
            if(x == nullptr)
            {
                if(zParent != nullptr)
                    fixDeleteLeaf(zParent);
            }
            else
                fixDelete(x);
        }

        if(verbose) printTree();
    }

    void removeNode(Node* z)
    {
        Node *node = root;
        Node *x = nullptr;
        Node *y = nullptr;
        Node *zParent = nullptr;

        if (z == nullptr)
        {
            throw Exception("key not found");
            // cout << "Key not found in the tree" << endl;
            return;
        }
        y = z;
        Color yOriginalColor = y->color;
        if (z->left == nullptr)
        {
            zParent = z->parent;
            x = z->right;
            transplant(root, z, z->right);
        }
        else if (z->right == nullptr)
        {
            zParent = z->parent; 
            x = z->left;
            transplant(root, z, z->left);
        }
        else
        {
            // printTree();
            y = minValueNode(z->right);
            swap(root,z,y);
            if(verbose) printTree();
            removeNode(z);
            return;
        }
        delete z;
        if(x != nullptr)
            fixAncestors(x);
        else
            fixAncestors(zParent);
        fixAncestors(y);
        if (yOriginalColor == BLACK)
        {
            if(x == nullptr)
            {
                if(zParent != nullptr)
                    fixDeleteLeaf(zParent);
            }
            else
                fixDelete(x);
        }

        if(verbose) printTree();
    }

    void insert(ItvNode key)
    {
        if(verbose)
        {
            *output << "Inserting: " << key << std::endl;
        }

        ItvTree::insert(key);
        if(verbose) printTree();
    }


};

#endif
