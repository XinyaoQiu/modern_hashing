#include "cuckoo.h"
#include <cassert>
#include <iostream>

void test_insert_and_lookup() {
    CuckooHash<int, int> table;

    table.insert(42, 100);
    assert(table.lookup(42).value() == 100);

    table.insert(84, 200);
    assert(table.lookup(84).value() == 200);

    table.insert(42, 300);  // update existing
    assert(table.lookup(42).value() == 300);

    std::cout << "test_insert_and_lookup passed\n";
}

void test_delete() {
    CuckooHash<int, int> table;
    table.insert(1, 10);
    table.insert(2, 20);
    table.insert(3, 30);

    assert(table.remove(2));
    assert(!table.lookup(2).has_value());
    assert(!table.remove(2));  // already removed

    assert(table.lookup(1).value() == 10);
    assert(table.lookup(3).value() == 30);

    std::cout << "test_delete passed\n";
}

void test_update() {
    CuckooHash<int, int> table;
    table.insert(5, 50);
    assert(table.update(5, 99));
    assert(table.lookup(5).value() == 99);

    assert(!table.update(999, 123));  // not found

    std::cout << "test_update passed\n";
}

void test_resize() {
    CuckooHash<int, int> table(2);  // small initial capacity to force resize

    for (int i = 1; i <= 1000; ++i)
        table.insert(i, i * 10);

    for (int i = 1; i <= 1000; ++i)
        assert(table.lookup(i).value() == i * 10);

    std::cout << "test_resize passed\n";
}

void test_collisions() {
    CuckooHash<int, int> table;
    const int base = 0xdeadbeef;

    for (int i = 0; i < 200; ++i) {
        int key = base + i * 1000;
        table.insert(key, key * 2);
    }

    for (int i = 0; i < 200; ++i) {
        int key = base + i * 1000;
        assert(table.lookup(key).value() == key * 2);
    }

    std::cout << "test_collisions passed\n";
}

int main() {
    test_insert_and_lookup();
    test_delete();
    test_update();
    test_resize();
    test_collisions();

    std::cout << "All CuckooHash tests passed successfully.\n";
    return 0;
}
