#pragma once

#include <cassert>
#include <functional>
#include <optional>
#include <random>
#include <vector>

using KeyType = int;
using ValueType = int;

class SecondaryTable {
   private:
    std::vector<std::optional<std::pair<KeyType, ValueType>>> table;
    size_t size = 0;
    size_t capacity = 0;
    std::hash<KeyType> hasher;

    size_t hash(KeyType key) const { return hasher(key) % capacity; }

    void rebuild() {
        std::vector<std::pair<KeyType, ValueType>> entries;
        for (const auto &slot : table) {
            if (slot.has_value()) {
                entries.push_back(*slot);
            }
        }
        build(entries);
    }

   public:
    SecondaryTable() = default;

    void build(const std::vector<std::pair<KeyType, ValueType>> &entries) {
        size = entries.size();
        capacity = std::max(2 * size * size, size_t(4));  // ensure square size

        table.clear();
        table.resize(capacity);

        for (const auto &[k, v] : entries) {
            size_t h = hash(k);
            while (table[h].has_value()) {
                h = (h + 1) % capacity;
            }
            table[h] = {k, v};
        }
    }

    std::optional<ValueType> lookup(KeyType key) const {
        if (capacity == 0) return std::nullopt;

        size_t h = hash(key);
        size_t start = h;
        do {
            if (!table[h].has_value()) return std::nullopt;
            if (table[h]->first == key) return table[h]->second;
            h = (h + 1) % capacity;
        } while (h != start);

        return std::nullopt;
    }
    bool remove(KeyType key) {
        if (capacity == 0) return false;

        size_t h = hash(key);
        size_t start = h;
        do {
            if (!table[h].has_value()) return false;
            if (table[h]->first == key) {
                table[h].reset();
                size--;
                return true;
            }
            h = (h + 1) % capacity;
        } while (h != start);

        return false;
    }
    bool insert_or_modify(KeyType key, ValueType value) {
        if (capacity == 0) {
            build({{key, value}});
            return true;
        }

        size_t h = hash(key);
        size_t start = h;
        do {
            if (!table[h].has_value()) {
                table[h] = {key, value};
                size++;
                if (size > capacity / 2) rebuild();  // control load
                return true;
            }
            if (table[h]->first == key) {
                table[h]->second = value;
                return true;
            }
            h = (h + 1) % capacity;
        } while (h != start);

        rebuild();
        return insert_or_modify(key, value);
    }
};

class PerfectHash {
   private:
    struct Entry {
        KeyType key;
        ValueType value;
    };

    std::vector<SecondaryTable> buckets;
    size_t bucketCount;
    std::hash<KeyType> hasher;

    size_t getBucketIndex(KeyType key) const {
        return hasher(key) % bucketCount;
    }

   public:
    PerfectHash(size_t initialBuckets = 16) : bucketCount(initialBuckets) {
        buckets.resize(bucketCount);
    }

    bool insert(KeyType key, ValueType value) {
        size_t index = getBucketIndex(key);
        return buckets[index].insert_or_modify(key, value);
    }

    bool modify(KeyType key, ValueType value) {
        size_t index = getBucketIndex(key);
        auto existing = buckets[index].lookup(key);
        if (!existing.has_value()) return false;
        return buckets[index].insert_or_modify(key, value);
    }

    std::optional<ValueType> lookup(KeyType key) const {
        size_t index = getBucketIndex(key);
        return buckets[index].lookup(key);
    }

    bool remove(KeyType key) {
        size_t index = getBucketIndex(key);
        return buckets[index].remove(key);
    }
};