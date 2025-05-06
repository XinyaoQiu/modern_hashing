#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cuckoo.h"
#include "dynamic_resizing_with_linear_probing.h"
#include "elastic.h"
#include "fixed_list_chain.h"
#include "indexed_partition_hash_with_btree.h"
#include "perfect_hashing.h"

using namespace std;
using namespace std::chrono;

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
    for (const auto& [k, v] : dataset) {
        table.insert(k, v);
    }
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

template <typename HashTable>
long long benchmark_update(HashTable& table,
                           const vector<pair<uint64_t, uint64_t>>& dataset) {
    auto start = high_resolution_clock::now();
    for (const auto& [k, v] : dataset) {
        table.update(k, v + 1);
    }
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}

template <typename HashTable>
long long benchmark_delete(HashTable& table,
                           const vector<pair<uint64_t, uint64_t>>& dataset) {
    auto start = high_resolution_clock::now();
    for (const auto& [k, _] : dataset) {
        table.remove(k);
    }
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}

long long baseline_lookup(unordered_map<uint64_t, uint64_t>& table,
                          const vector<pair<uint64_t, uint64_t>>& dataset) {
    auto start = high_resolution_clock::now();
    for (const auto& [k, _] : dataset) {
        auto val = table.find(k);
        assert(val != table.end() && val->second == k * 10);
    }
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}

long long baseline_insert(unordered_map<uint64_t, uint64_t>& table,
                          const vector<pair<uint64_t, uint64_t>>& dataset) {
    auto start = high_resolution_clock::now();
    for (const auto& [k, v] : dataset) table[k] = v;
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}

long long baseline_update(unordered_map<uint64_t, uint64_t>& table,
                          const vector<pair<uint64_t, uint64_t>>& dataset) {
    auto start = high_resolution_clock::now();
    for (const auto& [k, v] : dataset) {
        table[k] = v + 1;
    }
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}

long long baseline_delete(unordered_map<uint64_t, uint64_t>& table,
                          const vector<pair<uint64_t, uint64_t>>& dataset) {
    auto start = high_resolution_clock::now();
    for (const auto& [k, _] : dataset) {
        table.erase(k);
    }
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}

void run_all_benchmarks(size_t num_keys, uint64_t key_range,
                        size_t table_capacity) {
    auto dataset = generate_dataset(num_keys, key_range);
    cout << "===== Benchmark Run: num_keys = " << num_keys
         << ", key_range = " << key_range
         << ", table_capacity = " << table_capacity << " =====" << endl;

    {
        unordered_map<uint64_t, uint64_t> baseline;
        long long insert_time = baseline_insert(baseline, dataset);
        long long lookup_time = baseline_lookup(baseline, dataset);
        long long update_time = baseline_update(baseline, dataset);
        long long delete_time = baseline_delete(baseline, dataset);
        cout << "[unordered_map]\n"
             << "Insert time: " << insert_time << " ms\n"
             << "Lookup time: " << lookup_time << " ms\n"
             << "Update time: " << update_time << " ms\n"
             << "Delete time: " << delete_time << " ms\n";
    }

    auto test_table = [&](auto& table, const string& name) {
        long long insert_time = benchmark_insert(table, dataset);
        long long lookup_time = benchmark_lookup(table, dataset);
        long long update_time = benchmark_update(table, dataset);
        long long delete_time = benchmark_delete(table, dataset);
        cout << "[" << name << "]\n"
             << "Insert time: " << insert_time << " ms\n"
             << "Lookup time: " << lookup_time << " ms\n"
             << "Update time: " << update_time << " ms\n"
             << "Delete time: " << delete_time << " ms\n";
    };

    {
        DynamicResizeWithLinearProb<uint64_t, uint64_t> t(table_capacity);
        test_table(t, "DynamicResizeWithLinearProb");
    }
    {
        FixedListChainedHashTable<uint64_t, uint64_t> t(table_capacity);
        test_table(t, "FixedListChain");
    }
    {
        PerfectHash<uint64_t, uint64_t> t(table_capacity);
        test_table(t, "PerfectHashing");
    }
    {
        IndexedPartitionHashWithBTree<uint64_t, uint64_t> t(table_capacity);
        test_table(t, "IndexedPartitionHashWithBTree");
    }
    {
        CuckooHash<uint64_t, uint64_t> t(table_capacity);
        test_table(t, "CuckooHash");
    }
    {
        ElasticHash<uint64_t, uint64_t> t(table_capacity);
        test_table(t, "ElasticHash");
    }

    cout << "===== End of Run " << " =====\n" << endl;
}

int main() {
    run_all_benchmarks(1e6, 1e8, 1e8);
    return 0;
}
