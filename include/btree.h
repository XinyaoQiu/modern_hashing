// C++ Program to Implement B-Tree
#include <iostream>
#include <vector>
using namespace std;

// class for the node present in a B-Tree
template <typename K>
class BTreeNode {
   public:
    // order_
    int order_;
    // Current number of keys
    int n;
    // True if leaf node, false otherwise
    bool leaf;
    // Array of keys
    vector<K> keys;
    // Array of child pointers
    vector<BTreeNode<K>*> children;

    BTreeNode(int order, bool isLeaf)
        : order_(order),
          n(0),
          leaf(isLeaf),
          keys(order),
          children(order, nullptr) {}
};

// class for B-Tree
template <typename K>
class BTree {
   private:
    int order_;
    BTreeNode<K>* root;  // Pointer to root node

    // Function to split a full child node
    void splitChild(BTreeNode<K>* x, int i) {
        BTreeNode<K>* y = x->children[i];
        BTreeNode<K>* z = new BTreeNode<K>(order_, y->leaf);
        z->n = order_ / 2 - 1;

        for (int j = 0; j < order_ / 2 - 1; j++)
            z->keys[j] = y->keys[j + order_ / 2];

        if (!y->leaf) {
            for (int j = 0; j < order_ / 2; j++)
                z->children[j] = y->children[j + order_ / 2];
        }

        y->n = order_ / 2 - 1;

        for (int j = x->n; j >= i + 1; j--) x->children[j + 1] = x->children[j];

        x->children[i + 1] = z;

        for (int j = x->n - 1; j >= i; j--) x->keys[j + 1] = x->keys[j];

        x->keys[i] = y->keys[order_ / 2 - 1];
        x->n = x->n + 1;
    }

    // Function to insert a key in a non-full node
    void insertNonFull(BTreeNode<K>* x, K k) {
        int i = x->n - 1;

        if (x->leaf) {
            while (i >= 0 && k < x->keys[i]) {
                x->keys[i + 1] = x->keys[i];
                i--;
            }

            x->keys[i + 1] = k;
            x->n = x->n + 1;
        } else {
            while (i >= 0 && k < x->keys[i]) i--;

            i++;
            if (x->children[i]->n == order_ - 1) {
                splitChild(x, i);

                if (k > x->keys[i]) i++;
            }
            insertNonFull(x->children[i], k);
        }
    }

    // Function to traverse the tree
    void traverse(BTreeNode<K>* x) {
        int i;
        for (i = 0; i < x->n; i++) {
            if (!x->leaf) traverse(x->children[i]);
            cout << " " << x->keys[i];
        }

        if (!x->leaf) traverse(x->children[i]);
    }

    // Function to search a key in the tree
    BTreeNode<K>* search(BTreeNode<K>* x, K k) {
        int i = 0;
        while (i < x->n && k > x->keys[i]) i++;

        if (i < x->n && k == x->keys[i]) return x;

        if (x->leaf) return nullptr;

        return search(x->children[i], k);
    }

    // Function to find the predecessor
    K getPredecessor(BTreeNode<K>* node, int idx) {
        BTreeNode<K>* current = node->children[idx];
        while (!current->leaf) current = current->children[current->n];
        return current->keys[current->n - 1];
    }

    // Function to find the successor
    K getSuccessor(BTreeNode<K>* node, int idx) {
        BTreeNode<K>* current = node->children[idx + 1];
        while (!current->leaf) current = current->children[0];
        return current->keys[0];
    }

    // Function to fill child node
    void fill(BTreeNode<K>* node, int idx) {
        if (idx != 0 && node->children[idx - 1]->n >= order_ / 2)
            borrowFromPrev(node, idx);
        else if (idx != node->n && node->children[idx + 1]->n >= order_ / 2)
            borrowFromNext(node, idx);
        else {
            if (idx != node->n)
                merge(node, idx);
            else
                merge(node, idx - 1);
        }
    }

    // Function to borrow from previous sibling
    void borrowFromPrev(BTreeNode<K>* node, int idx) {
        BTreeNode<K>* child = node->children[idx];
        BTreeNode<K>* sibling = node->children[idx - 1];

        for (int i = child->n - 1; i >= 0; --i)
            child->keys[i + 1] = child->keys[i];

        if (!child->leaf) {
            for (int i = child->n; i >= 0; --i)
                child->children[i + 1] = child->children[i];
        }

        child->keys[0] = node->keys[idx - 1];

        if (!child->leaf) child->children[0] = sibling->children[sibling->n];

        node->keys[idx - 1] = sibling->keys[sibling->n - 1];

        child->n += 1;
        sibling->n -= 1;
    }

    // Function to borrow from next sibling
    void borrowFromNext(BTreeNode<K>* node, int idx) {
        BTreeNode<K>* child = node->children[idx];
        BTreeNode<K>* sibling = node->children[idx + 1];

        child->keys[child->n] = node->keys[idx];

        if (!child->leaf) child->children[child->n + 1] = sibling->children[0];

        node->keys[idx] = sibling->keys[0];

        for (int i = 1; i < sibling->n; ++i)
            sibling->keys[i - 1] = sibling->keys[i];

        if (!sibling->leaf) {
            for (int i = 1; i <= sibling->n; ++i)
                sibling->children[i - 1] = sibling->children[i];
        }

        child->n += 1;
        sibling->n -= 1;
    }

    // Function to merge two nodes
    void merge(BTreeNode<K>* node, int idx) {
        BTreeNode<K>* child = node->children[idx];
        BTreeNode<K>* sibling = node->children[idx + 1];

        child->keys[order_ / 2 - 1] = node->keys[idx];

        for (int i = 0; i < sibling->n; ++i)
            child->keys[i + order_ / 2] = sibling->keys[i];

        if (!child->leaf) {
            for (int i = 0; i <= sibling->n; ++i)
                child->children[i + order_ / 2] = sibling->children[i];
        }

        for (int i = idx + 1; i < node->n; ++i)
            node->keys[i - 1] = node->keys[i];

        for (int i = idx + 2; i <= node->n; ++i)
            node->children[i - 1] = node->children[i];

        child->n += sibling->n + 1;
        node->n--;

        delete sibling;
    }

    // Function to remove a key from a non-leaf node
    void removeFromNonLeaf(BTreeNode<K>* node, int idx) {
        K k = node->keys[idx];

        if (node->children[idx]->n >= order_ / 2) {
            K pred = getPredecessor(node, idx);
            node->keys[idx] = pred;
            remove(node->children[idx], pred);
        } else if (node->children[idx + 1]->n >= order_ / 2) {
            K succ = getSuccessor(node, idx);
            node->keys[idx] = succ;
            remove(node->children[idx + 1], succ);
        } else {
            merge(node, idx);
            remove(node->children[idx], k);
        }
    }

    // Function to remove a key from a leaf node
    void removeFromLeaf(BTreeNode<K>* node, int idx) {
        for (int i = idx + 1; i < node->n; ++i)
            node->keys[i - 1] = node->keys[i];

        node->n--;
    }

    // Function to remove a key from the tree
    void remove(BTreeNode<K>* node, K k) {
        int idx = 0;
        while (idx < node->n && node->keys[idx] < k) ++idx;

        if (idx < node->n && node->keys[idx] == k) {
            if (node->leaf)
                removeFromLeaf(node, idx);
            else
                removeFromNonLeaf(node, idx);
        } else {
            if (node->leaf) {
                cout << "The key " << k << " is not present in the tree\n";
                return;
            }

            bool flag = ((idx == node->n) ? true : false);

            if (node->children[idx]->n < order_ / 2) fill(node, idx);

            if (flag && idx > node->n)
                remove(node->children[idx - 1], k);
            else
                remove(node->children[idx], k);
        }
    }

   public:
    BTree(int order) : order_(order) { root = new BTreeNode<K>(order, true); }

    // Function to insert a key in the tree
    void insert(K k) {
        if (root->n == order_ - 1) {
            BTreeNode<K>* s = new BTreeNode<K>(order_, false);
            s->children[0] = root;
            root = s;
            splitChild(s, 0);
            insertNonFull(s, k);
        } else
            insertNonFull(root, k);
    }

    // Function to traverse the tree
    void traverse() {
        if (root != nullptr) traverse(root);
    }

    // Function to search a key in the tree
    BTreeNode<K>* search(K k) {
        return (root == nullptr) ? nullptr : search(root, k);
    }

    // Function to remove a key from the tree
    void remove(K k) {
        if (!root) {
            cout << "The tree is empty\n";
            return;
        }

        remove(root, k);

        if (root->n == 0) {
            BTreeNode<K>* tmp = root;
            if (root->leaf)
                root = nullptr;
            else
                root = root->children[0];

            delete tmp;
        }
    }
};