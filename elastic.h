#ifndef ELASTIC_HASH_H
#define ELASTIC_HASH_H

#include <vector>
#include <optional>
#include <functional>
#include <cstdint>
#include <cmath>
#include <algorithm>

using KeyType = uint64_t;
using ValueType = uint64_t;

class ElasticHash {
private:
    struct Entry {
        KeyType key;
        ValueType value;
        bool occupied = false;
    };

    size_t capacity;                        // total table capacity
    double delta;                           // global free-fraction threshold
    size_t inserted;                        // number of stored entries
    size_t max_probes;                      // maximum probes to scan on lookup/remove/modify
    std::vector<size_t> sizes;              // sizes of each level
    std::vector<std::vector<Entry>> tables; // per-level storage
    std::vector<size_t> occupied_counts;    // per-level occupancy
    std::hash<KeyType> hasher;

    static constexpr double C = 4.0;        // tuning constant for probe counts

    // Initialize levels based on capacity
    void init_tables(size_t cap) {
        sizes.clear();
        tables.clear();
        occupied_counts.clear();

        size_t sum = 0;
        size_t lev = 0;
        while ((cap >> (lev + 1)) > 0) {
            size_t s = cap >> (lev + 1);
            sizes.push_back(s);
            sum += s;
            ++lev;
        }
        sizes.push_back(cap - sum);

        size_t L = sizes.size();
        tables.resize(L);
        occupied_counts.assign(L, 0);
        for (size_t i = 0; i < L; ++i) {
            tables[i].assign(sizes[i], Entry{});
        }
    }

    // Double capacity and reinsert all entries
    void rehash() {
        size_t new_cap = capacity * 2;
        std::vector<std::pair<KeyType, ValueType>> all;
        all.reserve(inserted);

        for (auto &level : tables) {
            for (auto &e : level) {
                if (e.occupied) all.emplace_back(e.key, e.value);
            }
        }

        capacity = new_cap;
        inserted = 0;
        init_tables(capacity);

        for (auto &kv : all) {
            insert(kv.first, kv.second);
        }
    }

    // Compute the j-th probe position at level i
    size_t probe_pos(const KeyType &key, size_t level, size_t j) const {
        uint64_t h = hasher(key);
        uint64_t mix = h ^ (h >> (level + 1)) ^ (j * 0x9e3779b97f4a7c15ULL);
        return mix % sizes[level];
    }

public:
    // Constructor
    ElasticHash(size_t initial_capacity = 16, double delta_thresh = 0.1)
        : capacity(initial_capacity),
          delta(delta_thresh),
          inserted(0)
    {
        // Precompute the maximum possible probes (ceil(C * log2(1/delta)))
        max_probes = static_cast<size_t>(
            std::ceil(C * std::log2(1.0 / delta))
        );
        init_tables(capacity);
    }

    // Insert or update
    void insert(const KeyType &key, const ValueType &value) {
        // if present, just modify
        if (modify(key, value)) return;

        double free_glob = capacity > inserted
            ? double(capacity - inserted) / capacity
            : 0.0;
        double max_glob = free_glob > 0
            ? std::log2(1.0 / free_glob)
            : std::log2(1.0 / delta);

        for (size_t i = 0; i < tables.size(); ++i) {
            double free_i = sizes[i] > occupied_counts[i]
                ? double(sizes[i] - occupied_counts[i]) / sizes[i]
                : 0.0;
            double max_i = free_i > 0
                ? std::log2(1.0 / free_i)
                : std::log2(1.0 / delta);

            size_t probes = std::max<size_t>(
                1,
                static_cast<size_t>(std::ceil(C * std::min(max_i, max_glob)))
            );

            for (size_t j = 0; j < probes; ++j) {
                size_t pos = probe_pos(key, i, j);
                if (!tables[i][pos].occupied) {
                    tables[i][pos] = {key, value, true};
                    occupied_counts[i]++;
                    inserted++;
                    return;
                }
            }
        }

        // grow and retry
        rehash();
        insert(key, value);
    }

    // Lookup
    std::optional<ValueType> lookup(const KeyType &key) const {
        for (size_t i = 0; i < tables.size(); ++i) {
            for (size_t j = 0; j < max_probes; ++j) {
                size_t pos = probe_pos(key, i, j);
                const auto &e = tables[i][pos];
                if (e.occupied && e.key == key)
                    return e.value;
            }
        }
        return std::nullopt;
    }

    // Modify existing entry
    bool modify(const KeyType &key, const ValueType &new_value) {
        for (size_t i = 0; i < tables.size(); ++i) {
            for (size_t j = 0; j < max_probes; ++j) {
                size_t pos = probe_pos(key, i, j);
                if (tables[i][pos].occupied && tables[i][pos].key == key) {
                    tables[i][pos].value = new_value;
                    return true;
                }
            }
        }
        return false;
    }

    // Remove entry
    bool remove(const KeyType &key) {
        for (size_t i = 0; i < tables.size(); ++i) {
            for (size_t j = 0; j < max_probes; ++j) {
                size_t pos = probe_pos(key, i, j);
                if (tables[i][pos].occupied && tables[i][pos].key == key) {
                    tables[i][pos].occupied = false;
                    occupied_counts[i]--;
                    inserted--;
                    return true;
                }
            }
        }
        return false;
    }
};

#endif // ELASTIC_HASH_H
