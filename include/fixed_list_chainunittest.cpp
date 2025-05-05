#include "fixed_list_chain.h"
#include <cassert>
#include <iostream>

void test_insert_and_lookup() {
    FixedListChainedHashTable<int, int> table;

    // Insert and lookup
    table.insert(42, 100);
    {
        auto val = table.lookup(42);
        assert(val.has_value());
        assert(val.value() == 100);
    }

    table.insert(84, 200);
    {
        auto val = table.lookup(84);
        assert(val.has_value());
        assert(val.value() == 200);
    }

    // Update existing key
    table.insert(42, 300);
    {
        auto val = table.lookup(42);
        assert(val.has_value());
        assert(val.value() == 300);
    }

    std::cout << "test_insert_and_lookup passed\n";
}

void test_delete() {
    FixedListChainedHashTable<int, int> table;
    table.insert(1, 10);
    table.insert(2, 20);
    table.insert(3, 30);

    assert(table.remove(2));
    assert(!table.lookup(2).has_value());

    // Removing again should fail
    assert(!table.remove(2));

    {
        auto val1 = table.lookup(1);
        auto val3 = table.lookup(3);
        assert(val1.has_value() && val1.value() == 10);
        assert(val3.has_value() && val3.value() == 30);
    }

    std::cout << "test_delete passed\n";
}

void test_update() {
    FixedListChainedHashTable<int, int> table;
    table.insert(5, 50);
    assert(table.update(5, 99));
    {
        auto val = table.lookup(5);
        assert(val.has_value());
        assert(val.value() == 99);
    }

    // update non-existing key should return false
    assert(!table.update(999, 123));

    std::cout << "test_update passed\n";
}

void test_no_resize() {
    FixedListChainedHashTable<int, int> table(8);  // fixed capacity
    for (int i = 0; i < 100; ++i) {
        table.insert(i, i * 10);
    }
    for (int i = 0; i < 100; ++i) {
        auto val = table.lookup(i);
        assert(val.has_value());
        assert(val.value() == i * 10);
    }

    std::cout << "test_no_resize passed\n";
}

void test_collisions() {
    FixedListChainedHashTable<int, int> table(4);  // force more collisions
    const int base = 0x123456;
    for (int i = 0; i < 50; ++i) {
        int key = base + i * 256;
        table.insert(key, key + 1);
    }

    for (int i = 0; i < 50; ++i) {
        int key = base + i * 256;
        auto val = table.lookup(key);
        assert(val.has_value());
        assert(val.value() == key + 1);
    }

    std::cout << "test_collisions passed\n";
}

int main() {
    test_insert_and_lookup();
    test_delete();
    test_update();
    test_no_resize();
    test_collisions();

    std::cout << "All FixedListChainedHashTable tests passed successfully.\n";
    return 0;
}