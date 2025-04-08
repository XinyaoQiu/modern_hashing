#include "iceberg.h"
#include <cassert>
#include <iostream>

void test_insert_and_lookup() {
    IcebergHash table;

    // Insert and lookup
    assert(table.insert(42, 100));
    assert(table.lookup(42).value() == 100);
    assert(table.insert(84, 200));
    assert(table.lookup(84).value() == 200);

    // Update value for existing key
    assert(table.insert(42, 300));
    assert(table.lookup(42).value() == 300);

    std::cout << "test_insert_and_lookup passed\n";
}

void test_delete() {
    IcebergHash table;
    table.insert(1, 10);
    table.insert(2, 20);
    table.insert(3, 30);

    assert(table.remove(2));
    assert(!table.lookup(2).has_value());

    // Deleting again should return false
    assert(!table.remove(2));

    assert(table.lookup(1).value() == 10);
    assert(table.lookup(3).value() == 30);

    std::cout << "test_delete passed\n";
}

void test_modify() {
    IcebergHash table;
    table.insert(5, 50);
    assert(table.modify(5, 99));
    assert(table.lookup(5).value() == 99);

    // Modifying non-existing key should return false
    assert(!table.modify(99, 123));

    std::cout << "test_modify passed\n";
}

void test_resize() {
    IcebergHash table(2); // force early resize
    for (uint64_t i = 1; i <= 1000; ++i) {
        assert(table.insert(i, i * 10));
    }
    for (uint64_t i = 1; i <= 1000; ++i) {
        auto val = table.lookup(i);
        assert(val.has_value());
        assert(val.value() == i * 10);
    }

    std::cout << "test_resize passed\n";
}

void test_collisions() {
    IcebergHash table;
    const KeyType base = 0xdeadbeef;
    for (int i = 0; i < 200; ++i) {
        KeyType key = base + i * 1000;
        table.insert(key, key * 2);
    }

    for (int i = 0; i < 200; ++i) {
        KeyType key = base + i * 1000;
        auto val = table.lookup(key);
        assert(val.has_value());
        assert(val.value() == key * 2);
    }

    std::cout << "test_collisions passed\n";
}

int main() {
    test_insert_and_lookup();
    test_delete();
    test_modify();
    test_resize();
    test_collisions();

    std::cout << "All tests passed successfully.\n";
    return 0;
}