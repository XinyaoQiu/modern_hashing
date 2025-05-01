#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <unordered_set>
#include <vector>

#include "cuckoo.h"
#include "elastic.h"

using namespace std;
using namespace std::chrono;

const size_t NUM_KEYS = 1e7;
const uint64_t KEY_RANGE = 1e8;

vector<pair<uint64_t, uint64_t>> generate_dataset(size_t count,
                                                  uint64_t range) {
    vector<pair<uint64_t, uint64_t>> dataset;
    unordered_set<uint64_t> used;
    mt19937_64 rng(42);
    uniform_int_distribution<uint64_t> dist(1, range);

    while (dataset.size() < count) {
        uint64_t key = dist(rng);
        if (used.insert(key).second) {
            dataset.emplace_back(key, key * 10);
        }
    }
    return dataset;
}

template <typename HashTable>
long long benchmark_insert(HashTable& table,
                           const vector<pair<uint64_t, uint64_t>>& dataset) {
    auto start = high_resolution_clock::now();
    for (const auto& [k, v] : dataset) table.insert(k, v);
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}

template <typename HashTable>
long long benchmark_lookup(HashTable& table,
                           const vector<pair<uint64_t, uint64_t>>& dataset) {
    auto start = high_resolution_clock::now();
    for (const auto& [k, _] : dataset) {
        auto val = table.lookup(k);
        assert(val.has_value() && val.value() == k * 10);
    }
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}

int main() {
    auto dataset = generate_dataset(NUM_KEYS, KEY_RANGE);

    {
        CuckooHash cuckoo;
        long long insert_time = benchmark_insert(cuckoo, dataset);
        long long lookup_time = benchmark_lookup(cuckoo, dataset);
        cout << "[CuckooHash]\n"
             << "Insert time: " << insert_time << " ms\n"
             << "Lookup time: " << lookup_time << " ms\n";
    }

    {
        ElasticHash elastic;
        long long insert_time = benchmark_insert(elastic, dataset);
        long long lookup_time = benchmark_lookup(elastic, dataset);
        cout << "[ElasticHash]\n"
             << "Insert time: " << insert_time << " ms\n"
             << "Lookup time: " << lookup_time << " ms\n";
    }

    return 0;
}
