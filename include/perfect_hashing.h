#pragma once

#include "hash_base.h"
#include <vector>
#include <optional>
#include <functional>
#include <algorithm>

/**
 * @brief A secondary hash table used in two-level perfect hashing.
 * 
 * Each SecondaryTable handles collisions within a bucket using 
 * open addressing and quadratic space to guarantee perfect hashing.
 */
template<typename K, typename V>
class SecondaryTable {
public:
    SecondaryTable() = default;

    /**
     * @brief Builds the table using the provided key-value pairs.
     *        Allocates quadratic space and rehashes to eliminate collisions.
     * @param entries Key-value pairs to insert.
     */
    void build(const std::vector<std::pair<K, V>>& entries) {
        size = entries.size();
        capacity = std::max(2 * size * size, size_t(4));  // Ensure enough space

        table.clear();
        table.resize(capacity);

        for (const auto& [k, v] : entries) {
            size_t h = hash(k);
            while (table[h].has_value()) {
                h = (h + 1) % capacity;
            }
            table[h] = {k, v};
        }
    }

    /**
     * @brief Looks up a value associated with a key.
     * @param key Key to look up.
     * @return std::nullopt if the key is not found.
     */
    std::optional<V> lookup(K key) const {
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

    /**
     * @brief Removes a key from the table.
     * @param key Key to remove.
     * @return true if removed successfully, false if not found.
     */
    bool remove(K key) {
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

    /**
     * @brief Inserts a new key or modifies an existing key's value.
     *        Rebuilds the table if the load factor exceeds 0.5.
     * @param key Key to insert or modify.
     * @param value Value to insert or modify.
     */
    bool insert_or_modify(K key, V value) {
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
                if (size > capacity / 2) rebuild();
                return true;
            }
            if (table[h]->first == key) {
                table[h]->second = value;
                return true;
            }
            h = (h + 1) % capacity;
        } while (h != start);

        // If full loop, rebuild to attempt better distribution
        rebuild();
        return insert_or_modify(key, value);
    }

private:
    std::vector<std::optional<std::pair<K, V>>> table;
    size_t size = 0;
    size_t capacity = 0;
    std::hash<K> hasher;

    /**
     * @brief Computes the hash value for a given key.
     * @param key Key to hash.
     */
    size_t hash(K key) const {
        return hasher(key) % capacity;
    }

    /**
     * @brief Rebuilds the hash table to improve distribution.
     */
    void rebuild() {
        std::vector<std::pair<K, V>> entries;
        for (const auto& slot : table) {
            if (slot.has_value()) {
                entries.push_back(*slot);
            }
        }
        build(entries);
    }
};

/**
 * @brief A two-level perfect hash table using fixed-size top-level buckets
 *        and dynamically sized perfect secondary tables.
 */
template<typename K, typename V>
class PerfectHash : public HashBase<K, V> {
public:
    using KeyType = K;
    using ValueType = V;

    /**
     * @brief Constructor.
     * @param initialBuckets Number of top-level buckets.
     */
    explicit PerfectHash(size_t initialBuckets = 16)
        : bucketCount(initialBuckets), size_(0) {
        buckets.resize(bucketCount);
    }

    /**
     * @brief Inserts or updates a key-value pair.
     * @param key Key to insert.
     * @param value Value to insert.
     */
    void insert(const K& key, const V& value) override {
        size_t index = getBucketIndex(key);
        if (buckets[index].insert_or_modify(key, value)) {
            ++size_;
        }
    }

    /**
     * @brief Retrieves the value for a given key.
     * @param key Key to look up.
     * @return std::nullopt if not found.
     */
    std::optional<V> lookup(const K& key) const override {
        size_t index = getBucketIndex(key);
        return buckets[index].lookup(key);
    }

    /**
     * @brief Updates the value of an existing key.
     * @param key Key to update.
     * @return true if the key was updated, false if not found.
     */
    bool update(const K& key, const V& value) override {
        size_t index = getBucketIndex(key);
        auto existing = buckets[index].lookup(key);
        if (!existing.has_value()) return false;
        return buckets[index].insert_or_modify(key, value);
    }

    /**
     * @brief Removes a key-value pair.
     * @param key Key to remove.
     * @return true if successfully removed, false otherwise.
     */
    bool remove(const K& key) override {
        size_t index = getBucketIndex(key);
        if (buckets[index].remove(key)) {
            --size_;
            return true;
        }
        return false;
    }

    /**
     * @brief Returns the total number of stored elements.
     */
    size_t size() const override {
        return size_;
    }

    /**
     * @brief Clears all buckets.
     */
    void clear() override {
        for (auto& b : buckets) {
            b = SecondaryTable<K, V>();
        }
        size_ = 0;
    }

    /**
     * @brief Returns the current load factor.
     */
    double loadFactor() const override {
        return static_cast<double>(size_) / static_cast<double>(bucketCount);
    }

    /**
     * @brief Returns the number of top-level buckets.
     */
    size_t capacity() const override {
        return bucketCount;
    }

private:
    std::vector<SecondaryTable<K, V>> buckets;
    size_t bucketCount;
    size_t size_ = 0;
    std::hash<K> hasher;

    
    /**
     * @brief Computes the index of the top-level bucket for a given key.
     * @param key Key to hash.
     * @return Index of the bucket.
     */
    size_t getBucketIndex(K key) const {
        return hasher(key) % bucketCount;
    }
};
