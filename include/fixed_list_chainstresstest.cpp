#include "fixed_list_chain.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <numeric>

void test_insert_and_lookup() {
    FixedListChainedHashTable<int, int> table;
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
    FixedListChainedHashTable<int, int> table;
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

void test_update() {
    FixedListChainedHashTable<int, int> table;
    table.insert(5, 50);
    assert(table.update(5, 99));
    auto v = table.lookup(5);
    assert(v.has_value() && v.value() == 99);
    assert(!table.update(12345, 111));

    std::cout << "test_update passed\n";
}

void test_bulk_sequential() {
    FixedListChainedHashTable<int, int> table(4096);  // large enough to reduce chaining
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
    FixedListChainedHashTable<int, int> table;
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
            assert(v.has_value() && v.value() == i);
        }
    }
    for (uint64_t i = 0; i < N; i += 2) {
        assert(!table.remove(i));
    }

    std::cout << "test_remove_evens passed\n";
}

void test_randomized_operations() {
    FixedListChainedHashTable<int, int> table(8192);  // avoid excessive chaining
    const size_t N = 10000;
    std::vector<uint64_t> keys(N);
    std::iota(keys.begin(), keys.end(), 1);

    std::mt19937_64 rng(42);
    std::shuffle(keys.begin(), keys.end(), rng);

    std::vector<uint64_t> values(N);
    for (size_t i = 0; i < N; ++i) {
        values[i] = rng() % 100000;
        table.insert(keys[i], values[i]);
    }

    for (size_t i = 0; i < N; ++i) {
        auto v = table.lookup(keys[i]);
        assert(v.has_value() && v.value() == values[i]);
    }

    std::shuffle(keys.begin(), keys.end(), rng);
    for (size_t i = 0; i < N/2; ++i) {
        bool ok = table.remove(keys[i]);
        if (!ok) {
            std::cerr << "[remove FAILED] i=" << i << " key=" << keys[i] << "\n";
            std::abort();
        }
    }

    for (size_t i = 0; i < N; ++i) {
        auto v = table.lookup(keys[i]);
        if (i < N/2) {
            if (v.has_value()) {
                std::cerr << "[VERIFY FAILED] key not removed: " << keys[i]
                          << " value=" << v.value() << "\n";
                std::abort();
            }
        } else {
            if (!v.has_value()) {
                std::cerr << "[VERIFY FAILED] key missing: " << keys[i] << "\n";
                std::abort();
            }
        }
    }

    for (size_t i = 0; i < N/2; ++i) {
        uint64_t k = keys[i];
        uint64_t newv = k * 3;
        table.insert(k, newv);
        auto v = table.lookup(k);
        if (!v.has_value() || v.value() != newv) {
            std::cerr << "[REINSERT FAILED] key=" << k
                      << " expected=" << newv
                      << " got=" << (v.has_value() ? std::to_string(v.value()) : "none")
                      << "\n";
            std::abort();
        }
    }

    std::cout << "test_randomized_operations passed\n";
}

int main() {
    test_insert_and_lookup();
    test_delete();
    test_update();
    test_bulk_sequential();
    test_remove_evens();
    test_randomized_operations();
    std::cout << "All FixedListChainedHashTable stress tests passed successfully.\n";
    return 0;
}