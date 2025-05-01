#pragma once

#include "hash_base.h"
#include <vector>
#include <optional>
#include <functional>
#include <cstdint>

using KeyType = uint64_t;
using ValueType = uint64_t;

class ElasticHash : public HashBase<KeyType, ValueType> {
public:
    explicit ElasticHash(size_t initial_capacity = 16);

    void insert(const KeyType& key, const ValueType& value) override;
    std::optional<ValueType> lookup(const KeyType& key) const override;
    bool update(const KeyType& key, const ValueType& value) override;
    bool remove(const KeyType& key) override;
    size_t size() const override;
    void clear() override;
    double loadFactor() const override;
    size_t capacity() const override;

private:
    enum class Status { Empty, Occupied, Deleted };

    struct Entry {
        KeyType key;
        ValueType value;
        Status status = Status::Empty;
    };

    size_t capacity_;
    size_t size_;
    std::vector<Entry> table;
    std::hash<KeyType> hasher;

    size_t probe(const KeyType& key, bool for_insert) const;
    void rehash();
};
