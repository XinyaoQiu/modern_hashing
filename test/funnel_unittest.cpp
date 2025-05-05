#include "funnel.h"
#include <cassert>
#include <iostream>

void test_insert_and_lookup() {
    FunnelHash<int, int> table;

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
    FunnelHash<int, int> table;
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
    FunnelHash<int, int> table;
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

void test_resize() {
    // Start with small capacity to force rehash
    FunnelHash<int, int> table(2);
    for (uint64_t i = 1; i <= 1000; ++i) {
        table.insert(i, i * 10);
    }
    for (uint64_t i = 1; i <= 1000; ++i) {
        auto val = table.lookup(i);
        assert(val.has_value());
        assert(val.value() == i * 10);
    }

    std::cout << "test_resize passed\n";
}

void test_collisions() {
    FunnelHash<int, int> table;
    const int base = 0xdeadbeef;
    for (int i = 0; i < 200; ++i) {
        int key = base + static_cast<int>(i) * 1000;
        table.insert(key, key * 2);
    }

    for (int i = 0; i < 200; ++i) {
        int key = base + static_cast<int>(i) * 1000;
        auto val = table.lookup(key);
        assert(val.has_value());
        assert(val.value() == key * 2);
    }

    std::cout << "test_collisions passed\n";
}

int main() {
    test_insert_and_lookup();
    test_delete();
    test_update();
    test_resize();
    test_collisions();

    std::cout << "All FunnelHash tests passed successfully.\n";
    return 0;
}
