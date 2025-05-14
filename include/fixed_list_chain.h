#pragma once

#include <functional>
#include <list>
#include <optional>
#include <vector>

#include "hash_base.h"

/**
 * @brief Fixed-size Hash Table using Separate Chaining (linked lists).
 *
 * No resizing. Each slot holds a std::list of key-value pairs.
 */
template <typename K, typename V>
class FixedListChainedHashTable : public HashBase<K, V> {
   public:
    using KeyType = K;
    using ValueType = V;

    explicit FixedListChainedHashTable(size_t capacity = 17)
        : capacity_(capacity), size_(0), table(capacity) {}

    void insert(const K& key, const V& value) override {
        size_t index = hash(key);
        for (auto& [k, v] : table[index]) {
            if (k == key) {
                v = value;
                return;
            }
        }
        table[index].emplace_back(key, value);
        ++size_;
    }

    std::optional<V> lookup(const K& key) const override {
        size_t index = hash(key);
        for (const auto& [k, v] : table[index]) {
            if (k == key) return v;
        }
        return std::nullopt;
    }

    bool update(const K& key, const V& value) override {
        size_t index = hash(key);
        for (auto& [k, v] : table[index]) {
            if (k == key) {
                v = value;
                return true;
            }
        }
        return false;
    }

    bool remove(const K& key) override {
        size_t index = hash(key);
        auto& chain = table[index];
        for (auto it = chain.begin(); it != chain.end(); ++it) {
            if (it->first == key) {
                chain.erase(it);
                --size_;
                return true;
            }
        }
        return false;
    }

    size_t size() const override { return size_; }

    void clear() override {
        for (auto& chain : table) {
            chain.clear();
        }
        size_ = 0;
    }

    double loadFactor() const override {
        return static_cast<double>(size_) / static_cast<double>(capacity_);
    }

    size_t capacity() const override { return capacity_; }

   private:
    size_t capacity_;
    size_t size_;
    std::vector<std::list<std::pair<K, V>>> table;
    std::hash<K> hasher;

    size_t hash(const K& key) const { return hasher(key) % capacity_; }
};