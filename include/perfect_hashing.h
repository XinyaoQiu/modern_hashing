#pragma once

#include "hash_base.h"
#include <vector>
#include <optional>
#include <functional>

using KeyType = int;
using ValueType = int;

class SecondaryTable {
public:
    SecondaryTable() = default;
    void build(const std::vector<std::pair<KeyType, ValueType>>& entries);
    std::optional<ValueType> lookup(KeyType key) const;
    bool remove(KeyType key);
    bool insert_or_modify(KeyType key, ValueType value);

private:
    std::vector<std::optional<std::pair<KeyType, ValueType>>> table;
    size_t size = 0;
    size_t capacity = 0;
    std::hash<KeyType> hasher;

    size_t hash(KeyType key) const;
    void rebuild();
};

class PerfectHash : public HashBase<KeyType, ValueType> {
public:
    explicit PerfectHash(size_t initialBuckets = 16);

    void insert(const KeyType& key, const ValueType& value) override;
    std::optional<ValueType> lookup(const KeyType& key) const override;
    bool update(const KeyType& key, const ValueType& value) override;
    bool remove(const KeyType& key) override;
    size_t size() const override;
    void clear() override;
    double loadFactor() const override;
    size_t capacity() const override;

private:
    std::vector<SecondaryTable> buckets;
    size_t bucketCount;
    size_t size_ = 0;
    std::hash<KeyType> hasher;

    size_t getBucketIndex(KeyType key) const;
};
