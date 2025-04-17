#ifndef CUCKOO_H
#define CUCKOO_H

#include <vector>
#include <optional>
#include <functional>
#include <cstdint>

using KeyType = uint64_t;
using ValueType = uint64_t;

class CuckooHash {
private:
    struct Entry {
        KeyType key;
        ValueType value;
        bool occupied = false;
    };

    size_t capacity;
    std::vector<Entry> table1, table2;
    std::hash<KeyType> hasher;

    size_t hash1(const KeyType& key) const {
        return hasher(key) % capacity;
    }

    size_t hash2(const KeyType& key) const {
        size_t h = hasher(key);
        return ((h >> 16) ^ h) % capacity;
    }

    void rehash() {
        capacity *= 2;
        std::vector<Entry> old1 = std::move(table1);
        std::vector<Entry> old2 = std::move(table2);

        table1.assign(capacity, Entry{});
        table2.assign(capacity, Entry{});

        for (const auto& e : old1) {
            if (e.occupied) insert(e.key, e.value);
        }
        for (const auto& e : old2) {
            if (e.occupied) insert(e.key, e.value);
        }
    }

public:
    CuckooHash(size_t initial_capacity = 16)
        : capacity(initial_capacity),
          table1(initial_capacity),
          table2(initial_capacity)
    {}

    void insert(const KeyType& key, const ValueType& value) {
        KeyType cur_key   = key;
        ValueType cur_value = value;
        size_t kicks = 0;

        // update in-place if already present
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

        // cuckoo kicks up to capacity times
        while (kicks < capacity) {
            // try table1
            i1 = hash1(cur_key);
            if (!table1[i1].occupied) {
                table1[i1] = {cur_key, cur_value, true};
                return;
            }
            std::swap(cur_key,   table1[i1].key);
            std::swap(cur_value, table1[i1].value);

            // try table2
            i2 = hash2(cur_key);
            if (!table2[i2].occupied) {
                table2[i2] = {cur_key, cur_value, true};
                return;
            }
            std::swap(cur_key,   table2[i2].key);
            std::swap(cur_value, table2[i2].value);

            ++kicks;
        }

        // if too many kicks, grow and reinsert
        rehash();
        insert(cur_key, cur_value);
    }

    std::optional<ValueType> lookup(const KeyType& key) const {
        size_t i1 = hash1(key);
        if (table1[i1].occupied && table1[i1].key == key)
            return table1[i1].value;
        size_t i2 = hash2(key);
        if (table2[i2].occupied && table2[i2].key == key)
            return table2[i2].value;
        return std::nullopt;
    }

    bool modify(const KeyType& key, const ValueType& new_value) {
        size_t i1 = hash1(key);
        if (table1[i1].occupied && table1[i1].key == key) {
            table1[i1].value = new_value;
            return true;
        }
        size_t i2 = hash2(key);
        if (table2[i2].occupied && table2[i2].key == key) {
            table2[i2].value = new_value;
            return true;
        }
        return false;
    }

    bool remove(const KeyType& key) {
        size_t i1 = hash1(key);
        if (table1[i1].occupied && table1[i1].key == key) {
            table1[i1].occupied = false;
            return true;
        }
        size_t i2 = hash2(key);
        if (table2[i2].occupied && table2[i2].key == key) {
            table2[i2].occupied = false;
            return true;
        }
        return false;
    }
};

#endif // CUCKOO_H
