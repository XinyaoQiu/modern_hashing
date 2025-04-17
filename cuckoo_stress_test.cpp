// cuckoo_stress_test.cpp
#include "cuckoo.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>

void test_insert_and_lookup() {
    CuckooHash table;

    table.insert(42, 100);
    auto v1 = table.lookup(42);
    assert(v1.has_value() && v1.value() == 100);

    table.insert(84, 200);
    auto v2 = table.lookup(84);
    assert(v2.has_value() && v2.value() == 200);

    table.insert(42, 300);
    auto v3 = table.lookup(42);
    assert(v3.has_value() && v3.value() == 300);

    std::cout << "test_insert_and_lookup passed\n";
}

void test_delete() {
    CuckooHash table;
    table.insert(1, 10);
    table.insert(2, 20);
    table.insert(3, 30);

    assert(table.remove(2));
    assert(!table.lookup(2).has_value());
    assert(!table.remove(2));

    assert(table.lookup(1).has_value() && table.lookup(1).value() == 10);
    assert(table.lookup(3).has_value() && table.lookup(3).value() == 30);

    std::cout << "test_delete passed\n";
}

void test_modify() {
    CuckooHash table;
    table.insert(5, 50);
    assert(table.modify(5, 99));
    assert(table.lookup(5).has_value() && table.lookup(5).value() == 99);
    assert(!table.modify(99, 123));

    std::cout << "test_modify passed\n";
}

void test_bulk_sequential() {
    CuckooHash table(4);
    const size_t N = 5000;

    for (uint64_t i = 0; i < N; ++i) {
        table.insert(i, i + 100);
    }
    for (uint64_t i = 0; i < N; ++i) {
        auto v = table.lookup(i);
        assert(v.has_value() && v.value() == i + 100);
    }
    for (uint64_t i = 0; i < N; i += 2) {
        table.insert(i, i * 2);
    }
    for (uint64_t i = 0; i < N; ++i) {
        auto v = table.lookup(i);
        assert(v.has_value());
        if (i % 2 == 0) {
            assert(v.value() == i * 2);
        } else {
            assert(v.value() == i + 100);
        }
    }

    std::cout << "test_bulk_sequential passed\n";
}

void test_remove_evens() {
    CuckooHash table;
    const size_t N = 2000;

    for (uint64_t i = 0; i < N; ++i) {
        table.insert(i, i);
    }
    for (uint64_t i = 0; i < N; i += 2) {
        assert(table.remove(i));
    }
    for (uint64_t i = 0; i < N; ++i) {
        auto v = table.lookup(i);
        if (i % 2 == 0) {
            assert(!v.has_value());
        } else {
            assert(v.value() == i);
        }
    }
    for (uint64_t i = 0; i < N; i += 2) {
        assert(!table.remove(i));
    }

    std::cout << "test_remove_evens passed\n";
}

void test_randomized_operations() {
    CuckooHash table(8);
    const size_t N = 10000;
    std::vector<uint64_t> keys(N);
    std::iota(keys.begin(), keys.end(), 1);

    std::mt19937_64 rng(42);
    std::shuffle(keys.begin(), keys.end(), rng);

    for (auto k : keys) {
        uint64_t val = rng() % 100000;
        table.insert(k, val);
    }
    for (auto k : keys) {
        assert(table.lookup(k).has_value());
    }
    std::shuffle(keys.begin(), keys.end(), rng);
    for (size_t i = 0; i < N / 2; ++i) {
        assert(table.remove(keys[i]));
    }
    for (size_t i = 0; i < N; ++i) {
        auto v = table.lookup(keys[i]);
        if (i < N / 2) {
            assert(!v.has_value());
        } else {
            assert(v.has_value());
        }
    }
    for (size_t i = 0; i < N / 2; ++i) {
        uint64_t k = keys[i];
        table.insert(k, k * 3);
    }
    for (size_t i = 0; i < N / 2; ++i) {
        uint64_t k = keys[i];
        auto v = table.lookup(k);
        assert(v.has_value() && v.value() == k * 3);
    }

    std::cout << "test_randomized_operations passed\n";
}

void test_forced_collisions() {
    CuckooHash table(16);
    for (uint64_t i = 0; i < 1000; ++i) {
        uint64_t key = (i << 32) | 0xDEADBEEF;
        table.insert(key, key ^ 0xFFFFFFFF);
    }
    for (uint64_t i = 0; i < 1000; ++i) {
        uint64_t key = (i << 32) | 0xDEADBEEF;
        auto v = table.lookup(key);
        assert(v.has_value() && v.value() == (key ^ 0xFFFFFFFF));
    }

    std::cout << "test_forced_collisions passed\n";
}

int main() {
    test_insert_and_lookup();
    test_delete();
    test_modify();
    test_bulk_sequential();
    test_remove_evens();
    test_randomized_operations();
    test_forced_collisions();
    std::cout << "All extended tests passed successfully.\n";
    return 0;
}
