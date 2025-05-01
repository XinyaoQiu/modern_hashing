#include "dynamic_resizing.h"
#include <cassert>
#include <iostream>

void test_insert_and_lookup() {
    DynamicResizing table;

    table.insert(10, 100);
    table.insert(20, 200);
    table.insert(30, 300);

    assert(table.lookup(10).value() == 100);
    assert(table.lookup(20).value() == 200);
    assert(table.lookup(30).value() == 300);
    assert(!table.lookup(99).has_value());

    std::cout << "test_insert_and_lookup passed\n";
}

void test_update() {
    DynamicResizing table;

    table.insert(42, 10);
    assert(table.update(42, 999));
    assert(table.lookup(42).value() == 999);

    assert(!table.update(1234, 888)); // not present

    std::cout << "test_update passed\n";
}

void test_remove() {
    DynamicResizing table;

    table.insert(5, 55);
    table.insert(6, 66);
    table.remove(5);

    assert(!table.lookup(5).has_value());
    assert(table.lookup(6).value() == 66);

    assert(!table.remove(100)); // already gone

    std::cout << "test_remove passed\n";
}

void test_heavy_insertions() {
    DynamicResizing table;

    for (int i = 0; i < 10000; ++i) {
        table.insert(i, i * 10);
    }

    for (int i = 0; i < 10000; ++i) {
        assert(table.lookup(i).value() == i * 10);
    }

    std::cout << "test_heavy_insertions passed\n";
}

int main() {
    test_insert_and_lookup();
    test_update();
    test_remove();
    test_heavy_insertions();

    std::cout << "All tests passed for Dynamic Resizing.\n";
    return 0;
}
