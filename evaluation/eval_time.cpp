#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cuckoo.h"
#include "dynamic_resizing_with_linear_probing.h"
#include "elastic.h"
#include "fixed_list_chain.h"
#include "funnel.h"
#include "indexed_partition_hash_with_btree.h"
#include "perfect_hashing.h"

using namespace std;
using namespace std::chrono;

vector<pair<uint64_t, uint64_t>> generate_number_dataset(size_t count,
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

vector<pair<string, string>> generate_string_dataset(size_t count,
                                                     uint64_t range) {
    vector<pair<string, string>> dataset;
    unordered_set<string> used;
    mt19937_64 rng(42);
    uniform_int_distribution<uint64_t> dist(1, range);
    while (dataset.size() < count) {
        string key = "key" + to_string(dist(rng));
        if (used.insert(key).second) {
            string val = "val" + to_string(dist(rng));
            dataset.emplace_back(key, val);
        }
    }
    return dataset;
}

template <typename HashTable, typename DataSet>
long long benchmark_insert(HashTable& table, DataSet& dataset) {
    auto start = high_resolution_clock::now();
    for (const auto& [k, v] : dataset) {
        table.insert(k, v);
    }
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}

template <typename HashTable, typename DataSet>
long long benchmark_lookup(HashTable& table, DataSet& dataset) {
    auto start = high_resolution_clock::now();
    for (const auto& [k, v] : dataset) {
        auto val = table.lookup(k);
        assert(val.has_value() && val.value() == v);
    }
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}

template <typename HashTable, typename DataSet>
long long benchmark_update(HashTable& table, DataSet& dataset) {
    auto start = high_resolution_clock::now();
    for (const auto& [k, v] : dataset) {
        table.update(k, k + v);
    }
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}

template <typename HashTable, typename DataSet>
long long benchmark_delete(HashTable& table, DataSet& dataset) {
    auto start = high_resolution_clock::now();
    for (const auto& [k, _] : dataset) {
        table.remove(k);
    }
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}

template <typename HashTable, typename DataSet>
long long baseline_lookup(HashTable& table, DataSet& dataset) {
    auto start = high_resolution_clock::now();
    for (const auto& [k, v] : dataset) {
        auto val = table.find(k);
        assert(val != table.end() && val->second == v);
    }
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}

template <typename HashTable, typename DataSet>
long long baseline_insert(HashTable& table, DataSet& dataset) {
    auto start = high_resolution_clock::now();
    for (const auto& [k, v] : dataset) table[k] = v;
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}

template <typename HashTable, typename DataSet>
long long baseline_update(HashTable& table, DataSet& dataset) {
    auto start = high_resolution_clock::now();
    for (const auto& [k, v] : dataset) {
        table[k] = k + v;
    }
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}

template <typename HashTable, typename DataSet>
long long baseline_delete(HashTable& table, DataSet& dataset) {
    auto start = high_resolution_clock::now();
    for (const auto& [k, _] : dataset) {
        table.erase(k);
    }
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}

template <typename HashTable, typename DataSet>
void run_baseline(HashTable& table, DataSet& dataset, ofstream& of) {
    long long insert_time = baseline_insert(table, dataset);
    long long lookup_time = baseline_lookup(table, dataset);
    long long update_time = baseline_update(table, dataset);
    long long delete_time = baseline_delete(table, dataset);
    cout << "[unordered_map]\n"
         << "Insert time: " << insert_time << " ms\n"
         << "Lookup time: " << lookup_time << " ms\n"
         << "Update time: " << update_time << " ms\n"
         << "Delete time: " << delete_time << " ms\n";
    of << "[unordered_map]\n"
       << "Insert time: " << insert_time << " ms\n"
       << "Lookup time: " << lookup_time << " ms\n"
       << "Update time: " << update_time << " ms\n"
       << "Delete time: " << delete_time << " ms\n";
}

template <typename HashTable, typename DataSet>
void run_benchmark(HashTable& table, DataSet& dataset, ofstream& of,
                   string name) {
    long long insert_time = benchmark_insert(table, dataset);
    long long lookup_time = benchmark_lookup(table, dataset);
    long long update_time = benchmark_update(table, dataset);
    long long delete_time = benchmark_delete(table, dataset);
    cout << "[" << name << "]\n"
         << "Insert time: " << insert_time << " ms\n"
         << "Lookup time: " << lookup_time << " ms\n"
         << "Update time: " << update_time << " ms\n"
         << "Delete time: " << delete_time << " ms\n";
    of << "[" << name << "]\n"
       << "Insert time: " << insert_time << " ms\n"
       << "Lookup time: " << lookup_time << " ms\n"
       << "Update time: " << update_time << " ms\n"
       << "Delete time: " << delete_time << " ms\n";
}

void print_help() {
    cout << "Usage: ./bin/eval_time [--numKeys <int>] [--load <float>] "
            "[--hashtable <string>]\n"
         << "Options:\n"
         << "  --numKeys <int>         Number of keys (default: 1e5)\n"
         << "  --load <float>          Load factor in (0, 100] (default: 1.0)\n"
         << "  --type <string>         number, string (default: number)\n"
         << "  --hashtable <string>    Hash table to test. Options:\n"
         << "                          unordered_map, dynamic, fixed,\n"
         << "                          perfect, partition, cuckoo, elastic, "
            "funnel\n"
         << "  --help                  Show this help message\n";
}

int main(int argc, char* argv[]) {
    size_t num_keys = 1e5;
    double load_factor = 1.0;
    string type = "number";
    string hashtable = "unordered_map";

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        } else if (strcmp(argv[i], "--numKeys") == 0 && i + 1 < argc) {
            num_keys = static_cast<size_t>(stod(argv[++i]));
        } else if (strcmp(argv[i], "--load") == 0 && i + 1 < argc) {
            load_factor = stod(argv[++i]);
            if (load_factor <= 0.0 || load_factor > 100.0) {
                cerr << "Error: load factor must be in (0, 1].\n";
                return 1;
            }
        } else if (strcmp(argv[i], "--type") == 0 && i + 1 < argc) {
            type = argv[++i];
        } else if (strcmp(argv[i], "--hashtable") == 0 && i + 1 < argc) {
            hashtable = argv[++i];
        } else {
            cerr << "Unknown or incomplete argument: " << argv[i] << endl;
            return 1;
        }
    }

    std::ostringstream filename;
    filename << "./output/time_" << hashtable << "_" << type << "_" << num_keys
             << "_" << load_factor << ".txt";
    std::ofstream of(filename.str());

    size_t table_capacity = static_cast<size_t>(num_keys / load_factor);
    size_t key_range = 1e8;

    cout << "=== Benchmark Configuration: hashtable=" << hashtable
         << ", type=" << type << ", capacity=" << table_capacity
         << ", load_factor=" << load_factor << ", num_keys=" << num_keys
         << " ===\n\n";

    of << "=== Benchmark Configuration: hashtable=" << hashtable
       << ", type=" << type << ", capacity=" << table_capacity
       << ", load_factor=" << load_factor << ", num_keys=" << num_keys
       << " ===\n\n";

    if (type == "number") {
        vector<pair<uint64_t, uint64_t>> dataset =
            generate_number_dataset(num_keys, key_range);
        if (hashtable == "unordered_map") {
            unordered_map<uint64_t, uint64_t> table;
            run_baseline(table, dataset, of);
        } else if (hashtable == "dynamic") {
            DynamicResizeWithLinearProb<uint64_t, uint64_t> table(
                table_capacity);
            run_benchmark(table, dataset, of, "DynamicResizeWithLinearProb");
        } else if (hashtable == "fixed") {
            FixedListChainedHashTable<uint64_t, uint64_t> table(table_capacity);
            run_benchmark(table, dataset, of, "FixedListChainedHashTable");
        } else if (hashtable == "perfect") {
            PerfectHash<uint64_t, uint64_t> table(table_capacity);
            run_benchmark(table, dataset, of, "PerfectHash");
        } else if (hashtable == "partition") {
            IndexedPartitionHashWithBTree<uint64_t, uint64_t> table(
                table_capacity);
            run_benchmark(table, dataset, of, "IndexedPartitionHashWithBTree");
        } else if (hashtable == "cuckoo") {
            CuckooHash<uint64_t, uint64_t> table(table_capacity);
            run_benchmark(table, dataset, of, "CuckooHash");
        } else if (hashtable == "elastic") {
            ElasticHash<uint64_t, uint64_t> table(table_capacity);
            run_benchmark(table, dataset, of, "ElasticHash");
        } else if (hashtable == "funnel") {
            FunnelHash<uint64_t, uint64_t> table(table_capacity);
            run_benchmark(table, dataset, of, "FunnelHash");
        } else {
            cerr << "Error: unknown hashtable: " << hashtable << endl;
            return 1;
        }
    } else if (type == "string") {
        vector<pair<string, string>> dataset =
            generate_string_dataset(num_keys, key_range);
        if (hashtable == "unordered_map") {
            unordered_map<string, string> table;
            run_baseline(table, dataset, of);
        } else if (hashtable == "dynamic") {
            DynamicResizeWithLinearProb<string, string> table(table_capacity);
            run_benchmark(table, dataset, of, "DynamicResizeWithLinearProb");
        } else if (hashtable == "fixed") {
            FixedListChainedHashTable<string, string> table(table_capacity);
            run_benchmark(table, dataset, of, "FixedListChainedHashTable");
        } else if (hashtable == "perfect") {
            PerfectHash<string, string> table(table_capacity);
            run_benchmark(table, dataset, of, "PerfectHash");
        } else if (hashtable == "partition") {
            IndexedPartitionHashWithBTree<string, string> table(table_capacity);
            run_benchmark(table, dataset, of, "IndexedPartitionHashWithBTree");
        } else if (hashtable == "cuckoo") {
            CuckooHash<string, string> table(table_capacity);
            run_benchmark(table, dataset, of, "CuckooHash");
        } else if (hashtable == "elastic") {
            ElasticHash<string, string> table(table_capacity);
            run_benchmark(table, dataset, of, "ElasticHash");
        } else if (hashtable == "funnel") {
            FunnelHash<string, string> table(table_capacity);
            run_benchmark(table, dataset, of, "FunnelHash");
        } else {
            cerr << "Error: unknown hashtable: " << hashtable << endl;
            return 1;
        }
    } else {
        cerr << "Error: unknown type: " << type << endl;
        return 1;
    }

    return 0;
}