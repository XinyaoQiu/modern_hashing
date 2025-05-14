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

size_t get_memory_usage_kb() {
    std::ifstream statm("/proc/self/status");
    std::string line;
    while (std::getline(statm, line)) {
        if (line.find("VmRSS:") != std::string::npos) {
            std::istringstream iss(line);
            std::string label;
            size_t memory_kb;
            iss >> label >> memory_kb;
            return memory_kb;
        }
    }
    return 0;
}

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
size_t benchmark_space(HashTable& table, DataSet& dataset) {
    size_t mem_before = get_memory_usage_kb();
    for (const auto& [k, v] : dataset) table.insert(k, v);
    size_t mem_after = get_memory_usage_kb();
    return mem_after - mem_before;
}

template <typename HashTable, typename DataSet>
size_t baseline_space(HashTable& table, DataSet& dataset) {
    size_t mem_before = get_memory_usage_kb();
    for (const auto& [k, v] : dataset) table[k] = v;
    size_t mem_after = get_memory_usage_kb();
    return mem_after - mem_before;
}

void print_help() {
    cout << "Usage: ./bin/eval_space [--numKeys <int>] [--load <float>] "
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

    ostringstream filename;
    filename << "./output/space_" << hashtable << "_" << type << "_" << num_keys
             << ".txt";
    ofstream of(filename.str());

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

        size_t mem_used_kb = 0;
        if (hashtable == "unordered_map") {
            unordered_map<uint64_t, uint64_t> table;
            mem_used_kb = baseline_space(table, dataset);
        } else if (hashtable == "dynamic") {
            DynamicResizeWithLinearProb<uint64_t, uint64_t> table(
                table_capacity);
            mem_used_kb = benchmark_space(table, dataset);
        } else if (hashtable == "fixed") {
            FixedListChainedHashTable<uint64_t, uint64_t> table(table_capacity);
            mem_used_kb = benchmark_space(table, dataset);
        } else if (hashtable == "perfect") {
            PerfectHash<uint64_t, uint64_t> table(table_capacity);
            mem_used_kb = benchmark_space(table, dataset);
        } else if (hashtable == "partition") {
            IndexedPartitionHashWithBTree<uint64_t, uint64_t> table(
                table_capacity);
            mem_used_kb = benchmark_space(table, dataset);
        } else if (hashtable == "cuckoo") {
            CuckooHash<uint64_t, uint64_t> table(table_capacity);
            mem_used_kb = benchmark_space(table, dataset);
        } else if (hashtable == "elastic") {
            ElasticHash<uint64_t, uint64_t> table(table_capacity);
            mem_used_kb = benchmark_space(table, dataset);
        } else if (hashtable == "funnel") {
            FunnelHash<uint64_t, uint64_t> table(table_capacity);
            mem_used_kb = benchmark_space(table, dataset);
        } else {
            cerr << "Error: unknown hashtable: " << hashtable << endl;
            return 1;
        }

        cout << "[" << hashtable << "] Memory usage: " << mem_used_kb
             << " KB\n";
        of << "[" << hashtable << "] Memory usage: " << mem_used_kb << " KB\n";
    } else {
        cerr << "Error: only number dataset is supported in this space test.\n";
        return 1;
    }

    return 0;
}
