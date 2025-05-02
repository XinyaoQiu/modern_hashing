#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "hash_base.h"

/**
 * @brief Elastic Hashing using open addressing and linear probing.
 *
 * Each key hashes to a position and probes linearly until it finds a spot.
 * Deleted slots are reused. Table resizes automatically when load factor
 * exceeds 0.7.
 */
template <typename K, typename V>
class ElasticHash : public HashBase<K, V> {
   public:
    using KeyType = K;
    using ValueType = V;

    /**
     * @brief Constructor with initial capacity.
     * @param initial_capacity Initial number of slots in the table.
     */
    explicit ElasticHash(size_t initial_capacity = 16)
        : capacity_(initial_capacity), size_(0), table(initial_capacity) {}

    /**
     * @brief Inserts or updates a key-value pair.
     * @param key Key to insert.
     * @param value Value to insert.
     */
    void insert(const K& key, const V& value) override {
        size_t idx = probe(key, false);
        if (idx < capacity_ && table[idx].status == Status::Occupied) {
            table[idx].value = value;
            return;
        }

        idx = probe(key, true);
        if (idx == capacity_) {
            rehash();
            insert(key, value);
            return;
        }

        table[idx] = {key, value, Status::Occupied};
        ++size_;

        if (loadFactor() > 0.7) rehash();
    }

    /**
     * @brief Looks up a value by key.
     * @param key Key to look up.
     * @return std::nullopt if not found.
     */
    std::optional<V> lookup(const K& key) const override {
        size_t idx = probe(key, false);
        if (idx < capacity_ && table[idx].status == Status::Occupied)
            return table[idx].value;
        return std::nullopt;
    }

    /**
     * @brief Updates an existing key's value.
     * @param key Key to update.
     * @param value New value.
     * @return True if key exists and was updated.
     */
    bool update(const K& key, const V& value) override {
        size_t idx = probe(key, false);
        if (idx < capacity_ && table[idx].status == Status::Occupied) {
            table[idx].value = value;
            return true;
        }
        return false;
    }

    /**
     * @brief Removes a key-value pair.
     * @param key Key to remove.
     * @return True if removed, false if not found.
     */
    bool remove(const K& key) override {
        size_t idx = probe(key, false);
        if (idx < capacity_ && table[idx].status == Status::Occupied) {
            table[idx].status = Status::Deleted;
            --size_;
            return true;
        }
        return false;
    }

    /**
     * @brief Returns the number of active elements.
     */
    size_t size() const override { return size_; }

    /**
     * @brief Clears the hash table.
     */
    void clear() override {
        table.assign(capacity_, Entry{});
        size_ = 0;
    }

    /**
     * @brief Returns the current load factor.
     */
    double loadFactor() const override {
        return static_cast<double>(size_) / static_cast<double>(capacity_);
    }

    /**
     * @brief Returns the current capacity of the table.
     */
    size_t capacity() const override { return capacity_; }

   private:
    enum class Status { Empty, Occupied, Deleted };

    struct Entry {
        K key;
        V value;
        Status status = Status::Empty;
    };

    size_t capacity_;
    size_t size_;
    std::vector<Entry> table;
    std::hash<K> hasher;

    /**
     * @brief Probes for a key using linear probing.
     * @param key Key to probe for.
     * @param for_insert Whether probing is for insert (true) or lookup/remove
     * (false).
     * @return Index of the key or empty/deleted slot; capacity_ if not found.
     */
    size_t probe(const K& key, bool for_insert) const {
        size_t index = hasher(key) % capacity_;
        size_t original = index;
        size_t i = 0;

        while (true) {
            const Entry& entry = table[index];
            if (entry.status == Status::Empty ||
                entry.status == Status::Deleted) {
                if (for_insert) return index;
                if (entry.status == Status::Empty)
                    return capacity_;  // Not found
            }
            if (entry.status == Status::Occupied && entry.key == key)
                return index;

            ++i;
            index = (original + i) % capacity_;
            if (i == capacity_) return capacity_;  // Full or not found
        }
    }

    /**
     * @brief Doubles the table size and reinserts all active entries.
     */
    void rehash() {
        size_t old_capacity = capacity_;
        capacity_ *= 2;
        std::vector<Entry> old_table = std::move(table);

        table.assign(capacity_, Entry{});
        size_ = 0;

        for (const auto& e : old_table) {
            if (e.status == Status::Occupied) insert(e.key, e.value);
        }
    }
};
