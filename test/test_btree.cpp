#include <iostream>

#include "btree.h"

int main() {
    BTree<int> t(3);

    t.insert(10);
    t.insert(20);
    t.insert(5);
    t.insert(6);
    t.insert(12);
    t.insert(30);
    t.insert(7);
    t.insert(17);

    cout << "Traversal of the constructed tree is: ";
    t.traverse();
    cout << endl;

    int k = 6;
    (t.search(k) != nullptr) ? cout << k << " is found" << endl
                             : cout << k << " is not found" << endl;

    k = 15;
    (t.search(k) != nullptr) ? cout << k << " is found" << endl
                             : cout << k << " is not found" << endl;

    t.remove(6);
    cout << "Traversal of the tree after removing 6: ";
    t.traverse();
    cout << endl;

    t.remove(13);
    cout << "Traversal of the tree after removing 13: ";
    t.traverse();
    cout << endl;

   

    return 0;
}