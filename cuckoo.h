#include <iostream>
#include <vector>
#include <optional>
#include <functional>
#include <stdexcept>

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
    const size_t MAX_RELOCATIONS = 32;
    std::hash<KeyType> hasher;

    size_t hash1(const KeyType& key) const {
        return hasher(key) % capacity;
    }

    size_t hash2(const KeyType& key) const {
        return (hasher(key) / capacity) % capacity;
    }

    void rehash() {
        capacity *= 2;
        std::vector<Entry> old1 = std::move(table1);
        std::vector<Entry> old2 = std::move(table2);

        table1 = std::vector<Entry>(capacity);
        table2 = std::vector<Entry>(capacity);

        for (const auto& entry : old1) {
            if (entry.occupied)
                insert(entry.key, entry.value);
        }
        for (const auto& entry : old2) {
            if (entry.occupied)
                insert(entry.key, entry.value);
        }
    }

public:
    CuckooHash(size_t initial_capacity = 16)
        : capacity(initial_capacity), table1(initial_capacity), table2(initial_capacity) {}

    void insert(const KeyType& key, const ValueType& value) {
        KeyType cur_key = key;
        ValueType cur_value = value;

        for (size_t i = 0; i < MAX_RELOCATIONS; ++i) {
            size_t idx1 = hash1(cur_key);
            if (!table1[idx1].occupied) {
                table1[idx1] = {cur_key, cur_value, true};
                return;
            }

            std::swap(cur_key, table1[idx1].key);
            std::swap(cur_value, table1[idx1].value);

            size_t idx2 = hash2(cur_key);
            if (!table2[idx2].occupied) {
                table2[idx2] = {cur_key, cur_value, true};
                return;
            }

            std::swap(cur_key, table2[idx2].key);
            std::swap(cur_value, table2[idx2].value);
        }

        // Rehash and try again
        rehash();
        insert(cur_key, cur_value);
    }

    std::optional<ValueType> lookup(const KeyType& key) const {
        size_t idx1 = hash1(key);
        if (table1[idx1].occupied && table1[idx1].key == key)
            return table1[idx1].value;

        size_t idx2 = hash2(key);
        if (table2[idx2].occupied && table2[idx2].key == key)
            return table2[idx2].value;

        return std::nullopt;
    }

    bool modify(const KeyType& key, const ValueType& new_value) {
        size_t idx1 = hash1(key);
        if (table1[idx1].occupied && table1[idx1].key == key) {
            table1[idx1].value = new_value;
            return true;
        }

        size_t idx2 = hash2(key);
        if (table2[idx2].occupied && table2[idx2].key == key) {
            table2[idx2].value = new_value;
            return true;
        }

        return false;
    }

    bool remove(const KeyType& key) {
        size_t idx1 = hash1(key);
        if (table1[idx1].occupied && table1[idx1].key == key) {
            table1[idx1].occupied = false;
            return true;
        }

        size_t idx2 = hash2(key);
        if (table2[idx2].occupied && table2[idx2].key == key) {
            table2[idx2].occupied = false;
            return true;
        }

        return false;
    }
};
