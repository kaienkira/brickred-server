#ifndef BRICKRED_RANK_SET_H
#define BRICKRED_RANK_SET_H

#include <cstddef>
#include <functional>
#include <utility>

namespace brickred {

template <typename T, typename CompareFunc = std::less<T>>
class RankSet final {
private:
    struct RBTreeNode {
        bool is_red;
        RBTreeNode *parent;
        RBTreeNode *left;
        RBTreeNode *right;
        size_t node_count;
        T value;
    };

    static RBTreeNode *increment(RBTreeNode *node)
    {
        // if tree is empty
        // increment(begin()) will enter a infinite loop
        // increment(end()) will enter a infinite loop

        // has a right hand child
        // go down and then left as far as possible
        if (node->right != nullptr) {
            node = node->right;
            while (node->left != nullptr) {
                node = node->left;
            }
            return node;
        }

        // no right hand children
        // go up and find the first node
        // which is a left hand child of its parent
        RBTreeNode *parent_node = node->parent;
        while (node == parent_node->right) {
            node = parent_node;
            parent_node = parent_node->parent;
        }

        // when node == root and root.right == nullptr
        // then node == header && parent == root after the loop
        // which node->right == parent_node
        if (node->right != parent_node) {
            node = parent_node;
        }

        return node;
    }

    static RBTreeNode *decrement(RBTreeNode *node)
    {
        // if node is header, return max node
        if (node->is_red && node->parent->parent == node) {
            return node->right;
        }

        // has a left hand child
        // go down and then right as far as possible
        if (node->left != nullptr) {
            node = node->left;
            while (node->right != nullptr) {
                node = node->right;
            }
            return node;
        }

        // no left hand children
        // go up and find the first node
        // which is a right hand child of its parent
        RBTreeNode *parent_node = node->parent;
        while (node == parent_node->left) {
            node = parent_node;
            parent_node = parent_node->parent;
        }

        // when node == root and root.left == nullptr
        // then node == header && parent == root after the loop
        // which node->left == parent_node
        if (node->left != parent_node) {
            node = parent_node;
        }

        return node;
    }

    void resetHeaderNode()
    {
        // header.parent -> root
        // header.left -> min node
        // header.right -> max node
        // root.parent -> header
        header_->is_red = true;
        header_->parent = nullptr;
        header_->left = header_;
        header_->right = header_;
        header_->node_count = 0;
    }

    void clearNode(RBTreeNode *node)
    {
        if (node == nullptr) {
            return;
        }

        clearNode(node->left);
        clearNode(node->right);
        delete node;
    }

    RBTreeNode *copyNodes(RBTreeNode *node)
    {
        if (node == nullptr) {
            return nullptr;
        }

        RBTreeNode *new_node = new RBTreeNode();
        new_node->is_red = node->is_red;
        new_node->parent = nullptr;
        new_node->left = nullptr;
        new_node->right = nullptr;
        new_node->value = node->value;
        new_node->node_count = node->node_count;

        try {
            new_node->left = copyNodes(node->left);
            new_node->right = copyNodes(node->right);
        } catch(...) {
            clearNode(new_node);
            throw;
        }

        if (new_node->left != nullptr) {
            new_node->left->parent = new_node;
        }
        if (new_node->right != nullptr) {
            new_node->right->parent = new_node;
        }

        return new_node;
    }

    void updateNodeCount(RBTreeNode *node)
    {
        size_t left_count =
            (node->left == nullptr) ? 0 : node->left->node_count;
        size_t right_count =
            (node->right == nullptr) ? 0 : node->right->node_count;

        node->node_count = left_count + right_count + 1;
    }

    void updateNodeCountToTop(RBTreeNode *node)
    {
        while (node != header_) {
            updateNodeCount(node);
            node = node->parent;
        }
    }

    // rotate left on x
    //     x             y
    //   1   y   -->   x   3
    //     2   3     1   2
    // update node count on x
    // update node count on y
    void rotateLeft(RBTreeNode *node_x)
    {
        RBTreeNode *node_y = node_x->right;

        // update x
        node_x->right = node_y->left;

        // update 2
        if (node_y->left != nullptr) {
            node_y->left->parent = node_x;
        }

        // update y
        node_y->parent = node_x->parent;

        if (node_x == header_->parent) {
            // update root
            header_->parent = node_y;
        } else if (node_x == node_x->parent->left) {
            // update x parent
            node_x->parent->left = node_y;
        } else {
            // update x parent
            node_x->parent->right = node_y;
        }

        // update link between x and y
        node_y->left = node_x;
        node_x->parent = node_y;

        updateNodeCount(node_x);
        updateNodeCount(node_x->parent);
    }

    // rotate right on x
    //     x           y
    //   y   3 -->   1   x
    // 1   2           2   3
    // update node count on x
    // update node count on y
    void rotateRight(RBTreeNode *node_x)
    {
        RBTreeNode *node_y = node_x->left;

        // update x
        node_x->left = node_y->right;

        // update 2
        if (node_y->right != nullptr) {
            node_y->right->parent = node_x;
        }

        // update y
        node_y->parent = node_x->parent;

        if (node_x == header_->parent) {
            // update root
            header_->parent = node_y;
        } else if (node_x == node_x->parent->right) {
            // update x parent
            node_x->parent->right = node_y;
        } else {
            // update x parent
            node_x->parent->left = node_y;
        }

        // update link between x and y
        node_y->right = node_x;
        node_x->parent = node_y;

        updateNodeCount(node_x);
        updateNodeCount(node_x->parent);
    }

    std::pair<RBTreeNode *, bool> insertNode(const T &value)
    {
        // insert to empty tree
        if (header_->node_count == 0) {
            RBTreeNode *new_node = new RBTreeNode();
            new_node->is_red = true;
            new_node->parent = header_;
            new_node->left = nullptr;
            new_node->right = nullptr;
            new_node->value = value;

            header_->parent = new_node;
            header_->left = new_node;
            header_->right = new_node;

            updateNodeCountToTop(new_node);
            ++header_->node_count;
            insertNodeFixup(new_node);
            return std::make_pair(new_node, true);
        }

        // find position to insert new node
        RBTreeNode *search_node = header_->parent;
        RBTreeNode *target_node = header_;

        while (search_node != nullptr) {
            if (!compare_func_(search_node->value, value)) {
                target_node = search_node;
                search_node = search_node->left;
            } else {
                search_node = search_node->right;
            }
        }

        // insert at end()
        if (target_node == header_) {
            RBTreeNode *new_node = new RBTreeNode();
            new_node->is_red = true;
            new_node->parent = header_->right;
            new_node->left = nullptr;
            new_node->right = nullptr;
            new_node->value = value;

            header_->right->right = new_node;
            header_->right = new_node;

            updateNodeCountToTop(new_node);
            ++header_->node_count;
            insertNodeFixup(new_node);
            return std::make_pair(new_node, true);
        }

        // insert value alreay exists
        if (!compare_func_(value, target_node->value)) {
            return std::make_pair(target_node, false);
        }

        search_node = target_node->left;
        if (search_node == nullptr) {
            RBTreeNode *new_node = new RBTreeNode();
            new_node->is_red = true;
            new_node->parent = target_node;
            new_node->left = nullptr;
            new_node->right = nullptr;
            new_node->value = value;

            target_node->left = new_node;
            if (header_->left == target_node) {
                header_->left = new_node;
            }

            updateNodeCountToTop(new_node);
            ++header_->node_count;
            insertNodeFixup(new_node);
            return std::make_pair(new_node, true);
        }

        while (search_node->right != nullptr) {
            search_node = search_node->right;
        }
        {
            RBTreeNode *new_node = new RBTreeNode();
            new_node->is_red = true;
            new_node->parent = search_node;
            new_node->left = nullptr;
            new_node->right = nullptr;
            new_node->value = value;

            search_node->right = new_node;
            if (header_->right == search_node) {
                header_->right = new_node;
            }

            updateNodeCountToTop(new_node);
            ++header_->node_count;
            insertNodeFixup(new_node);
            return std::make_pair(new_node, true);
        }
    }

    void insertNodeFixup(RBTreeNode *node_x)
    {
        // x is red, if parent of x is also red
        // rebalancing is needed
        while (node_x != header_->parent && node_x->parent->is_red) {
            //      g(b)
            // p(r)      u
            if (node_x->parent == node_x->parent->parent->left) {
                RBTreeNode *node_u = node_x->parent->parent->right;

                // case1: uncle is red
                //           g(b)                    g(r)
                //      p(r)      u(r) -->      p(b)      u(b)
                // x(r)                    x(r)
                // p -> black
                // u -> black
                // g -> red
                // then let x = g and loop again
                if (node_u != nullptr && node_u->is_red) {
                    node_x->parent->is_red = false;
                    node_u->is_red = false;
                    node_x->parent->parent->is_red = true;
                    node_x = node_x->parent->parent;
                } else {
                    // case2: uncle is black, x is right child
                    //       g(b)                    g(b)
                    //  p(r)      u(b)  -->     x(r)      u(b)
                    //       x(r)           p(r)
                    // left rotate on p, change case2 to case3
                    if (node_x == node_x->parent->right) {
                        node_x = node_x->parent;
                        rotateLeft(node_x);
                    }
                    // case3: uncle is black, x is left child
                    //            g(b)                    g(r)
                    //       p(r)      u(b) -->      p(b)      u(b)
                    //  x(r)                    x(r)
                    // p -> black
                    // g -> red
                    //
                    //            g(r)               p(b)
                    //       p(b)      u(b) --> x(r)      g(r)
                    //  x(r)                                   u(b)
                    // right rotate on g
                    node_x->parent->is_red = false;
                    node_x->parent->parent->is_red = true;
                    rotateRight(node_x->parent->parent);
                }
            } else {
                //   g(b)
                // u      p(r)
                RBTreeNode *node_u = node_x->parent->parent->left;

                // case1: uncle is red
                //      g(b)                    g(r)
                // u(r)      p(r)      --> u(b)      p(b)
                //                x(r)                    x(r)
                // p -> black
                // u -> black
                // g -> red
                // then let x = g and loop again
                if (node_u != nullptr && node_u->is_red) {
                    node_x->parent->is_red = false;
                    node_u->is_red = false;
                    node_x->parent->parent->is_red = true;
                    node_x = node_x->parent->parent;
                } else {
                    // case2: uncle is black, x is left child
                    //      g(b)                g(b)
                    // u(b)      p(r) -->  u(b)      x(r)
                    //      x(r)                          p(r)
                    // right rotate on p, change case2 to case3
                    if (node_x == node_x->parent->left) {
                        node_x = node_x->parent;
                        rotateRight(node_x);
                    }
                    // case3: uncle is black, x is right child
                    //      g(b)                    g(r)
                    // u(b)      p(r)      --> u(b)      p(b)
                    //                x(r)                    x(r)
                    // p -> black
                    // g -> red
                    //      g(r)                         p(b)
                    // u(b)      p(b)      -->      g(r)      x(r)
                    //                x(r)     u(b)
                    // left rotate on g
                    node_x->parent->is_red = false;
                    node_x->parent->parent->is_red = true;
                    rotateLeft(node_x->parent->parent);
                }
            }
        }

        updateNodeCountToTop(node_x);
        header_->parent->is_red = false;
    }

    void eraseNode(RBTreeNode *node_z)
    {
        // update min max value
        if (header_->node_count == 1) {
            header_->left = header_;
            header_->right = header_;
        } else if (header_->left == node_z) {
            header_->left = increment(node_z);
        } else if (header_->right == node_z) {
            header_->right = decrement(node_z);
        }

        RBTreeNode *node_y = node_z;
        RBTreeNode *node_x = nullptr;
        RBTreeNode *node_x_new_parent = nullptr;

        if (node_y->left == nullptr) {
            // node_z has at most one non-null child
            // node_x might be null
            node_x = node_y->right;
        } else if (node_y->right == nullptr) {
            // node_z has exactly one non-null child
            node_x = node_y->left;
        } else {
            // node_z has two non-null children
            // set node_y to node_z's successor
            // node_x might be null
            node_y = node_y->right;
            while (node_y->left != nullptr) {
                node_y = node_y->left;
            }
            node_x = node_y->right;
        }

        if (node_y == node_z) {
            // link node_x to node_z's parent
            //          p       p
            //     y(z)   --> x
            //   x
            node_x_new_parent = node_y->parent;
            if (node_x != nullptr) {
                node_x->parent = node_x_new_parent;
            }
            // fix node_z's parent
            if (header_->parent == node_z) {
                header_->parent = node_x;
            } else if (node_z->parent->left == node_z) {
                node_z->parent->left = node_x;
            } else {
                node_z->parent->right = node_x;
            }
        } else {
            // relink node_y in place of node_z
            // node_y is node_z's successor
            node_z->left->parent = node_y;
            node_y->left = node_z->left;
            if (node_y != node_z->right) {
                //         z           y
                //      1     2      1   2
                //          3   -->    3
                //        y          x
                //          x
                node_x_new_parent = node_y->parent;
                if (node_x != nullptr) {
                    node_x->parent = node_x_new_parent;
                }
                node_y->parent->left = node_x;
                node_y->right = node_z->right;
                node_z->right->parent = node_y;
            } else {
                //      z           y
                //    1   y   --> 1   x
                //          x
                node_x_new_parent = node_y;
            }

            // fix node_z's parent
            if (header_->parent == node_z) {
                header_->parent = node_y;
            } else if (node_z->parent->left == node_z) {
                node_z->parent->left = node_y;
            } else {
                node_z->parent->right = node_y;
            }

            // fix node y
            node_y->parent = node_z->parent;
            std::swap(node_y->is_red, node_z->is_red);
            node_y = node_z;
        }

        updateNodeCountToTop(node_x_new_parent);

        // node_y now points to node to be actually deleted
        if (node_y->is_red == false) {
            eraseNodeFixup(node_x, node_x_new_parent);
        }

        // delete node
        --header_->node_count;
        delete node_y;
    }

    void eraseNodeFixup(RBTreeNode *node_x, RBTreeNode *node_x_new_parent)
    {
        while (node_x != header_->parent &&
               (node_x == nullptr || node_x->is_red == false)) {
            //       p(b)
            //  x(b)      w
            if (node_x == node_x_new_parent->left) {
                RBTreeNode *node_w = node_x_new_parent->right;

                // case1: w is red
                //      p(b)                         w(b)
                // x(b)      w(r)      -->      p(r)      2(b)
                //      1(b)      2(b)     x(b)      1(b)
                // w -> black
                // p -> red
                // left rotate on p, change case1 to case2,3,4
                if (node_w->is_red) {
                    node_w->is_red = false;
                    node_x_new_parent->is_red = true;
                    rotateLeft(node_x_new_parent);
                    node_w = node_x_new_parent->right;
                }

                // case2: w is black and both children of w are black
                //      p                       p
                // x(b)      w(b)      --> x(b)      w(r)
                //      1(b)      2(b)          1(b)      2(b)
                // w -> red
                // then let x = p and loop again
                if ((node_w->left == nullptr ||
                     node_w->left->is_red == false) &&
                    (node_w->right == nullptr ||
                     node_w->right->is_red == false)) {
                    node_w->is_red = true;
                    node_x = node_x_new_parent;
                    node_x_new_parent = node_x_new_parent->parent;

                } else {
                    // case3: w is black and
                    // left child of w is red and
                    // right child of w is black
                    //      p                        p
                    // x(b)      w(b)      -->  x(b)     1(b)
                    //      1(r)      2(b)                    w(r)
                    //                                             2(b)
                    // w -> red
                    // 1 -> black
                    // right rotate on w, change case3 to case4
                    if (node_w->right == nullptr ||
                        node_w->right->is_red == false) {
                        if (node_w->left != nullptr) {
                            node_w->left->is_red = false;
                        }
                        node_w->is_red = true;
                        rotateRight(node_w);
                        node_w = node_x_new_parent->right;
                    }

                    // case4: w is black and right child of w is red
                    //      p(c1)                         w(c1)
                    // x(b)       w(b)      -->      p(b)       2(b)
                    //      1(c2)      2(r)     x(b)      1(c2)
                    // w -> c1
                    // p -> black
                    // 2 -> black
                    // left rotate on p, break the loop
                    node_w->is_red = node_x_new_parent->is_red;
                    node_x_new_parent->is_red = false;
                    if (node_w->right != nullptr) {
                        node_w->right->is_red = false;
                    }
                    rotateLeft(node_x_new_parent);
                    updateNodeCountToTop(node_x_new_parent);
                    break;
                }
            } else {
                //   p(b)
                // w      x(b)
                RBTreeNode *node_w = node_x_new_parent->left;

                // case1: w is red
                //           p(b)                w(b)
                //      w(r)      x(b) -->  1(b)      p(r)
                // 1(b)      2(b)                2(b)      x(b)
                // w -> black
                // p -> red
                // right rotate on p, change case1 to case2,3,4
                if (node_w->is_red) {
                    node_w->is_red = false;
                    node_x_new_parent->is_red = true;
                    rotateRight(node_x_new_parent);
                    node_w = node_x_new_parent->left;
                }

                // case2: w is black and both children of w are black
                //           p
                //      w(b)      x(b)
                // 1(b)      2(b)
                // w -> red
                // then let x = p and loop again
                if ((node_w->right== nullptr ||
                     node_w->right->is_red == false) &&
                    (node_w->left == nullptr ||
                     node_w->left->is_red == false)) {
                    node_w->is_red = true;
                    node_x = node_x_new_parent;
                    node_x_new_parent = node_x_new_parent->parent;

                } else {
                    // case3: w is black
                    // left child of w is black and
                    // right child of w is red
                    //           p                            p
                    //      w(b)      x(b) -->           2(b)   x(b)
                    // 1(b)      2(r)               w(r)
                    //                         1(b)
                    // w -> red
                    // 2 -> black
                    // left rotate on w, change case3 to case4
                    if (node_w->left == nullptr ||
                        node_w->left->is_red == false) {
                        if (node_w->right != nullptr) {
                            node_w->right->is_red = false;
                        }
                        node_w->is_red = true;
                        rotateLeft(node_w);
                        node_w = node_x_new_parent->left;
                    }

                    // case4: w is black and left child of w is red
                    //           p(c1)                  w(c1)
                    //      w(b)        x(b) -->   1(b)       p(b)
                    // 1(r)      2(c2)                  2(c2)      x(b)
                    // w -> c1
                    // p -> black
                    // 1 -> black
                    // right rotate on p, break the loop
                    node_w->is_red = node_x_new_parent->is_red;
                    node_x_new_parent->is_red = false;
                    if (node_w->left != nullptr) {
                        node_w->left->is_red = false;
                    }
                    rotateRight(node_x_new_parent);
                    updateNodeCountToTop(node_x_new_parent);
                    break;
                }
            }
        }

        if (node_x != nullptr) {
            node_x->is_red = false;
        }
    }

    RBTreeNode *findNode(const T &value)
    {
        RBTreeNode *search_node = header_->parent;
        RBTreeNode *target_node = header_;

        while (search_node != nullptr) {
            if (!compare_func_(search_node->value, value)) {
                target_node = search_node;
                search_node = search_node->left;
            } else {
                search_node = search_node->right;
            }
        }

        if (target_node != header_) {
            if (compare_func_(value, target_node->value)) {
                target_node = header_;
            }
        }

        return target_node;
    }

    RBTreeNode *lowerBoundNode(const T &value)
    {
        RBTreeNode *search_node = header_->parent;
        RBTreeNode *target_node = header_;

        while (search_node != nullptr) {
            if (compare_func_(search_node->value, value)) {
                search_node = search_node->right;
            } else {
                target_node = search_node;
                search_node = search_node->left;
            }
        }

        return target_node;
    }

    RBTreeNode *upperBoundNode(const T &value)
    {
        RBTreeNode *search_node = header_->parent;
        RBTreeNode *target_node = header_;

        while (search_node != nullptr) {
            if (compare_func_(value, search_node->value)) {
                target_node = search_node;
                search_node = search_node->left;
            } else {
                search_node = search_node->right;
            }
        }

        return target_node;
    }

    RBTreeNode *selectNode(size_t rank)
    {
        RBTreeNode *search_node = header_->parent;

        while (search_node != nullptr) {
            RBTreeNode *left_child = search_node->left;
            size_t left_count =
                (left_child == nullptr) ? 0 : left_child->node_count;
            if (rank == left_count) {
                return search_node;
            } else if (rank < left_count) {
                search_node = left_child;
            } else {
                rank -= left_count + 1;
                search_node = search_node->right;
            }
        }

        return header_;
    }

    size_t rankNode(const T &value)
    {
        RBTreeNode *search_node = header_->parent;
        size_t rank = 0;

        while (search_node != nullptr) {
            RBTreeNode *left_child = search_node->left;
            if (compare_func_(value, search_node->value)) {
                search_node = left_child;
            } else if (compare_func_(search_node->value, value)) {
                rank += (left_child == nullptr)
                    ? 1 : 1 + left_child->node_count;
                search_node = search_node->right;
            } else {
                rank += (left_child == nullptr)
                    ? 0 : left_child->node_count;
                break;
            }
        }

        return rank;
    }

public:
    class Iterator final {
    public:
        Iterator() : node_(nullptr)
        {
        }

        Iterator(RBTreeNode *node) : node_(node)
        {
        }

        Iterator(const Iterator &other) : node_(other.node_)
        {
        }

        Iterator &operator=(const Iterator &other)
        {
            node_ = other.node_;
            return *this;
        }

        T &operator*() const
        {
            return node_->value;
        }

        T *operator->() const
        {
            return &node_->value;
        }

        friend bool operator==(const Iterator &lhs, const Iterator &rhs)
        {
            return lhs.node_ == rhs.node_;
        }

        friend bool operator!=(const Iterator &lhs, const Iterator &rhs)
        {
            return lhs.node_ != rhs.node_;
        }

        Iterator &operator++()
        {
            node_ = RankSet::increment(node_);
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator tmp = *this;
            node_ = RankSet::increment(node_);
            return tmp;
        }

        Iterator &operator--()
        {
            node_ = RankSet::decrement(node_);
            return *this;
        }

        Iterator operator--(int)
        {
            Iterator tmp = *this;
            node_ = RankSet::decrement(node_);
            return tmp;
        }

    private:
        friend class RankSet;
        RBTreeNode *node_;
    };

    class ConstIterator {
    public:
        ConstIterator() : node_(nullptr)
        {
        }

        ConstIterator(const RBTreeNode *node) :
            node_(const_cast<RBTreeNode *>(node))
        {
        }

        ConstIterator(const ConstIterator &other) :
            node_(other.node_)
        {
        }

        ConstIterator &operator=(const ConstIterator &other)
        {
            node_ = other.node_;
            return *this;
        }

        const T &operator*() const
        {
            return node_->value;
        }

        const T *operator->() const
        {
            return &node_->value;
        }

        friend bool operator==(
            const ConstIterator &lhs, const ConstIterator &rhs)
        {
            return lhs.node_ == rhs.node_;
        }

        friend bool operator!=(
            const ConstIterator &lhs, const ConstIterator &rhs)
        {
            return lhs.node_ != rhs.node_;
        }

        ConstIterator &operator++()
        {
            node_ = RankSet::increment(node_);
            return *this;
        }

        ConstIterator operator++(int)
        {
            ConstIterator tmp = *this;
            node_ = RankSet::increment(node_);
            return tmp;
        }

        ConstIterator &operator--()
        {
            node_ = RankSet::decrement(node_);
            return *this;
        }

        ConstIterator operator--(int)
        {
            ConstIterator tmp = *this;
            node_ = RankSet::decrement(node_);
            return tmp;
        }

    private:
        friend class RankSet;
        RBTreeNode *node_;
    };

    class ReverseIterator {
    public:
        ReverseIterator() : node_(nullptr)
        {
        }

        ReverseIterator(RBTreeNode *node) : node_(node)
        {
        }

        ReverseIterator(const ReverseIterator &other) : node_(other.node_)
        {
        }

        ReverseIterator &operator=(const ReverseIterator &other)
        {
            node_ = other.node_;
            return *this;
        }

        T &operator*() const
        {
            return node_->value;
        }

        T *operator->() const
        {
            return &node_->value;
        }

        friend bool operator==(
            const ReverseIterator &lhs, const ReverseIterator &rhs)
        {
            return lhs.node_ == rhs.node_;
        }

        friend bool operator!=(
            const ReverseIterator &lhs, const ReverseIterator &rhs)
        {
            return lhs.node_ != rhs.node_;
        }

        ReverseIterator &operator++()
        {
            node_ = RankSet::decrement(node_);
            return *this;
        }

        ReverseIterator operator++(int)
        {
            ReverseIterator tmp = *this;
            node_ = RankSet::decrement(node_);
            return tmp;
        }

        ReverseIterator &operator--()
        {
            node_ = RankSet::increment(node_);
            return *this;
        }

        ReverseIterator operator--(int)
        {
            ReverseIterator tmp = *this;
            node_ = RankSet::increment(node_);
            return tmp;
        }

    private:
        friend class RankSet;
        RBTreeNode *node_;
    };

    class ConstReverseIterator {
    public:
        ConstReverseIterator() : node_(nullptr)
        {
        }

        ConstReverseIterator(const RBTreeNode *node) :
            node_(const_cast<RBTreeNode *>(node))
        {
        }

        ConstReverseIterator(const ConstReverseIterator &other) :
            node_(other.node_)
        {
        }

        ConstReverseIterator &operator=(const ConstReverseIterator &other)
        {
            node_ = other.node_;
            return *this;
        }

        const T &operator*() const
        {
            return node_->value;
        }

        const T *operator->() const
        {
            return &node_->value;
        }

        friend bool operator==(
            const ConstReverseIterator &lhs, const ConstReverseIterator &rhs)
        {
            return lhs.node_ == rhs.node_;
        }

        friend bool operator!=(
            const ConstReverseIterator &lhs, const ConstReverseIterator &rhs)
        {
            return lhs.node_ != rhs.node_;
        }

        ConstReverseIterator &operator++()
        {
            node_ = RankSet::decrement(node_);
            return *this;
        }

        ConstReverseIterator operator++(int)
        {
            ConstReverseIterator tmp = *this;
            node_ = RankSet::decrement(node_);
            return tmp;
        }

        ConstReverseIterator &operator--()
        {
            node_ = RankSet::increment(node_);
            return *this;
        }

        ConstReverseIterator operator--(int)
        {
            ConstReverseIterator tmp = *this;
            node_ = RankSet::increment(node_);
            return tmp;
        }

    private:
        friend class RankSet;
        RBTreeNode *node_;
    };

    ///////////////////////////////////////////////////////////////////////////
    RankSet() : header_(new RBTreeNode())
    {
        resetHeaderNode();
    }

    RankSet(const RankSet &other) : header_(new RBTreeNode())
    {
        resetHeaderNode();
        // copy nodes
        header_->parent = copyNodes(other.header_->parent);
        if (header_->parent != nullptr) {
            header_->parent->parent = header_;
        }
        header_->node_count = other.header_->node_count;
        // update min max
        if (header_->parent == nullptr) {
            header_->left = header_;
            header_->right = header_;
        } else {
            RBTreeNode *node_min = header_->parent;
            while (node_min->left != nullptr) {
                node_min = node_min->left;
            }
            header_->left = node_min;

            RBTreeNode *node_max = header_->parent;
            while (node_max->right != nullptr) {
                node_max = node_max->right;
            }
            header_->right = node_max;
        }

        compare_func_ = other.compare_func_;
    }

    RankSet(RankSet &&other)
    {
        header_ = other.header_;
        compare_func_ = other.compare_func_;

        other.header_ = new RBTreeNode();
        other.resetHeaderNode();
    }

    RankSet &operator=(const RankSet &other)
    {
        if (this != &other) {
            RankSet tmp(other);
            swap(tmp);
        }

        return *this;
    }

    RankSet &operator=(RankSet &&other)
    {
        RankSet tmp(std::move(other));
        swap(tmp);

        return *this;
    }

    ~RankSet()
    {
        clear();
        delete header_;
    }

    void clear()
    {
        clearNode(header_->parent);
        resetHeaderNode();
    }

    void swap(RankSet &other)
    {
        std::swap(header_, other.header_);
        std::swap(compare_func_, other.compare_func_);
    }

    Iterator begin()
    {
        return Iterator(header_->left);
    }

    ConstIterator begin() const
    {
        return ConstIterator(header_->left);
    }

    ConstIterator cbegin() const
    {
        return ConstIterator(header_->left);
    }

    Iterator end()
    {
        return Iterator(header_);
    }

    ConstIterator end() const
    {
        return ConstIterator(header_);
    }

    ConstIterator cend() const
    {
        return ConstIterator(header_);
    }

    ReverseIterator rbegin()
    {
        return ReverseIterator(header_->right);
    }

    ConstReverseIterator rbegin() const
    {
        return ConstReverseIterator(header_->right);
    }

    ConstReverseIterator crbegin() const
    {
        return ConstReverseIterator(header_->right);
    }

    ReverseIterator rend()
    {
        return ReverseIterator(header_);
    }

    ConstReverseIterator rend() const
    {
        return ConstReverseIterator(header_);
    }

    ConstReverseIterator crend() const
    {
        return ConstReverseIterator(header_);
    }

    bool empty() const
    {
        return header_->node_count == 0;
    }

    size_t size() const
    {
        return header_->node_count;
    }

    std::pair<Iterator, bool> insert(const T &value)
    {
        std::pair<RBTreeNode *, bool> ret = insertNode(value);
        return std::pair<Iterator, bool>(Iterator(ret.first), ret.second);
    }

    Iterator erase(Iterator iter)
    {
        if (iter == end()) {
            return iter;
        }

        Iterator ret_iter = iter;
        ++ret_iter;
        eraseNode(iter.node_);
        return ret_iter;
    }

    size_t erase(const T &value)
    {
        Iterator iter = find(value);
        if (iter == end()) {
            return 0;
        }
        erase(iter);
        return 1;
    }

    Iterator find(const T &value)
    {
        return Iterator(findNode(value));
    }

    ConstIterator find(const T &value) const
    {
        return ConstIterator(
            const_cast<RankSet *>(this)->findNode(value));
    }

    Iterator lowerBound(const T &value)
    {
        return Iterator(lowerBoundNode(value));
    }

    ConstIterator lowerBound(const T &value) const
    {
        return ConstIterator(
            const_cast<RankSet *>(this)->lowerBoundNode(value));
    }

    Iterator upperBound(const T &value)
    {
        return Iterator(upperBoundNode(value));
    }

    ConstIterator upperBound(const T &value) const
    {
        return ConstIterator(
            const_cast<RankSet *>(this)->upperBoundNode(value));
    }

    // rank is [0, size)
    Iterator select(size_t rank)
    {
        return Iterator(selectNode(rank));
    }

    // rank is [0, size)
    ConstIterator select(size_t rank) const
    {
        return ConstIterator(const_cast<RankSet *>(this)->selectNode(rank));
    }

    // rank is [0, size)
    size_t rank(const T &value) const
    {
        return const_cast<RankSet *>(this)->rankNode(value);
    }

private:
    RBTreeNode *header_;
    CompareFunc compare_func_;
};

} // namespace brickred

#endif
