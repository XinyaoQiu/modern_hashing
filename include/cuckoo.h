#pragma once

#include "hash_base.h"
#include <vector>
#include <optional>
#include <functional>
#include <cstdint>

using KeyType = uint64_t;
using ValueType = uint64_t;

class CuckooHash : public HashBase<KeyType, ValueType> {
private:
    struct Entry {
        KeyType key;
        ValueType value;
        bool occupied = false;
    };

    size_t capacity_;
    size_t size_;
    std::vector<Entry> table1, table2;
    std::hash<KeyType> hasher;

    size_t hash1(const KeyType& key) const {
        return hasher(key) % capacity_;
    }

    size_t hash2(const KeyType& key) const {
        size_t h = hasher(key);
        return ((h >> 16) ^ h) % capacity_;
    }

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

public:
    CuckooHash(size_t initial_capacity = 16)
        : capacity_(initial_capacity), size_(0),
          table1(initial_capacity), table2(initial_capacity)
    {}

    void insert(const KeyType& key, const ValueType& value) override {
        KeyType cur_key = key;
        ValueType cur_value = value;
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

    std::optional<ValueType> lookup(const KeyType& key) const override {
        size_t i1 = hash1(key);
        if (table1[i1].occupied && table1[i1].key == key)
            return table1[i1].value;
        size_t i2 = hash2(key);
        if (table2[i2].occupied && table2[i2].key == key)
            return table2[i2].value;
        return std::nullopt;
    }

    bool update(const KeyType& key, const ValueType& value) override {
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

    bool remove(const KeyType& key) override {
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

    size_t size() const override {
        return size_;
    }

    void clear() override {
        table1.assign(capacity_, Entry{});
        table2.assign(capacity_, Entry{});
        size_ = 0;
    }

    double loadFactor() const override {
        return static_cast<double>(size_) / (2.0 * capacity_);
    }

    size_t capacity() const override {
        return capacity_;
    }
};
