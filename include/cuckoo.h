#pragma once

#include <functional>
#include <optional>
#include <utility>
#include <vector>

#include "hash_base.h"

/**
 * @brief Cuckoo hashing with two hash functions and two tables.
 *
 * Uses displacement-based collision resolution.
 * Each key can reside in one of two possible positions (two tables).
 * If collision chain exceeds capacity, the table will resize and rehash.
 */
template <typename K, typename V>
class CuckooHash : public HashBase<K, V> {
   public:

    /**
     * @brief Constructs the hash table with the given initial capacity.
     * @param initial_capacity Initial number of slots in the table.
     */
    explicit CuckooHash(size_t initial_capacity = 16)
        : capacity_(initial_capacity),
          size_(0),
          table1(initial_capacity),
          table2(initial_capacity) {}

    /**
     * @brief Inserts or updates a key-value pair using cuckoo displacement.
     * @param key Key to insert.
     * @param value Value to insert.
     */
    void insert(const K& key, const V& value) override {
        K cur_key = key;
        V cur_value = value;
        size_t kicks = 0;

        size_t i1 = hash1(cur_key);
        if (table1[i1].occupied && table1[i1].key == cur_key) {
            table1[i1].value = cur_value;
            return;
        }

        size_t i2 = hash2(cur_key);
        if (table2[i2].occupied && table2[i2].key == cur_key) {
            table2[i2].value = cur_value;
            return;
        }

        while (kicks < capacity_) {
            i1 = hash1(cur_key);
            if (!table1[i1].occupied) {
                table1[i1] = {cur_key, cur_value, true};
                ++size_;
                return;
            }
            std::swap(cur_key, table1[i1].key);
            std::swap(cur_value, table1[i1].value);

            i2 = hash2(cur_key);
            if (!table2[i2].occupied) {
                table2[i2] = {cur_key, cur_value, true};
                ++size_;
                return;
            }
            std::swap(cur_key, table2[i2].key);
            std::swap(cur_value, table2[i2].value);

            ++kicks;
        }

        rehash();
        insert(cur_key, cur_value);
    }

    /**
     * @brief Retrieves a value by key.
     * @param key Key to look up.
     * @return std::nullopt if not found.
     */
    std::optional<V> lookup(const K& key) const override {
        size_t i1 = hash1(key);
        if (table1[i1].occupied && table1[i1].key == key)
            return table1[i1].value;

        size_t i2 = hash2(key);
        if (table2[i2].occupied && table2[i2].key == key)
            return table2[i2].value;

        return std::nullopt;
    }

    /**
     * @brief Updates the value of an existing key.
     * @param key Key to update.
     * @param value New value.
     * @return True if updated, false if key not found.
     */
    bool update(const K& key, const V& value) override {
        size_t i1 = hash1(key);
        if (table1[i1].occupied && table1[i1].key == key) {
            table1[i1].value = value;
            return true;
        }

        size_t i2 = hash2(key);
        if (table2[i2].occupied && table2[i2].key == key) {
            table2[i2].value = value;
            return true;
        }

        return false;
    }

    /**
     * @brief Removes a key from the table.
     * @param key Key to remove.
     * @return True if removed, false if key not found.
     */
    bool remove(const K& key) override {
        size_t i1 = hash1(key);
        if (table1[i1].occupied && table1[i1].key == key) {
            table1[i1].occupied = false;
            --size_;
            return true;
        }

        size_t i2 = hash2(key);
        if (table2[i2].occupied && table2[i2].key == key) {
            table2[i2].occupied = false;
            --size_;
            return true;
        }

        return false;
    }

    /**
     * @brief Returns the number of key-value pairs stored.
     */
    size_t size() const override { return size_; }

    /**
     * @brief Clears all entries from the table.
     */
    void clear() override {
        table1.assign(capacity_, Entry{});
        table2.assign(capacity_, Entry{});
        size_ = 0;
    }

    /**
     * @brief Returns the current load factor.
     */
    double loadFactor() const override {
        return static_cast<double>(size_) / (2.0 * capacity_);
    }

    /**
     * @brief Returns the total capacity (per table).
     */
    size_t capacity() const override { return capacity_; }

   private:
    struct Entry {
        K key;
        V value;
        bool occupied = false;
    };

    size_t capacity_;
    size_t size_;
    std::vector<Entry> table1;
    std::vector<Entry> table2;
    std::hash<K> hasher;

    /**
     * @brief Primary hash function.
     * @param key Key to hash.
     */
    size_t hash1(const K& key) const { return hasher(key) % capacity_; }

    /**
     * @brief Secondary hash function using xor-shift variation.
     * @param key Key to hash.
     */
    size_t hash2(const K& key) const {
        size_t h = hasher(key);
        return ((h >> 16) ^ h) % capacity_;
    }

    /**
     * @brief Resizes the table and re-inserts all entries.
     */
    void rehash() {
        capacity_ *= 2;
        size_ = 0;

        std::vector<Entry> old1 = std::move(table1);
        std::vector<Entry> old2 = std::move(table2);

        table1.assign(capacity_, Entry{});
        table2.assign(capacity_, Entry{});

        for (const auto& e : old1)
            if (e.occupied) insert(e.key, e.value);
        for (const auto& e : old2)
            if (e.occupied) insert(e.key, e.value);
    }
};
