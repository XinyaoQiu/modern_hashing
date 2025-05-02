#pragma once

#include "hash_base.h"
#include <vector>
#include <optional>
#include <functional>

using KeyType = int;
using ValueType = int;

/**
 * @brief A secondary hash table used in two-level perfect hashing.
 * 
 * Each SecondaryTable handles collisions within a bucket using 
 * open addressing and quadratic space to guarantee perfect hashing.
 */
class SecondaryTable {
public:
    SecondaryTable() = default;

    /**
     * @brief Builds the table using the provided key-value pairs.
     *        Allocates quadratic space and rehashes to eliminate collisions.
     */
    void build(const std::vector<std::pair<KeyType, ValueType>>& entries);

    /**
     * @brief Looks up a value associated with a key.
     * @return std::nullopt if the key is not found.
     */
    std::optional<ValueType> lookup(KeyType key) const;

    /**
     * @brief Removes a key from the table.
     * @return true if removed successfully, false if not found.
     */
    bool remove(KeyType key);

    /**
     * @brief Inserts a new key or modifies an existing key's value.
     *        Rebuilds the table if the load factor exceeds 0.5.
     */
    bool insert_or_modify(KeyType key, ValueType value);

private:
    std::vector<std::optional<std::pair<KeyType, ValueType>>> table; ///< internal slot array
    size_t size = 0;       ///< current number of elements
    size_t capacity = 0;   ///< table size
    std::hash<KeyType> hasher;

    size_t hash(KeyType key) const;
    void rebuild();
};

/**
 * @brief A two-level perfect hash table.
 * 
 * Uses a vector of secondary hash tables for bucketized perfect hashing.
 * Ensures O(1) lookup in the worst case (with preprocessed input).
 */
class PerfectHash : public HashBase<KeyType, ValueType> {
public:
    /**
     * @brief Constructs a perfect hash table with a given number of buckets.
     */
    explicit PerfectHash(size_t initialBuckets = 16);

    /**
     * @brief Inserts or updates a key-value pair.
     */
    void insert(const KeyType& key, const ValueType& value) override;

    /**
     * @brief Retrieves the value for a given key.
     * @return std::nullopt if not found.
     */
    std::optional<ValueType> lookup(const KeyType& key) const override;

    /**
     * @brief Updates the value of an existing key.
     * @return true if the key was updated, false if not found.
     */
    bool update(const KeyType& key, const ValueType& value) override;

    /**
     * @brief Removes a key-value pair.
     * @return true if successfully removed, false otherwise.
     */
    bool remove(const KeyType& key) override;

    /**
     * @brief Returns the total number of stored elements.
     */
    size_t size() const override;

    /**
     * @brief Clears the entire hash table.
     */
    void clear() override;

    /**
     * @brief Returns the current load factor.
     */
    double loadFactor() const override;

    /**
     * @brief Returns the number of top-level buckets.
     */
    size_t capacity() const override;

private:
    std::vector<SecondaryTable> buckets; ///< top-level bucket array
    size_t bucketCount;                  ///< number of top-level buckets
    size_t size_ = 0;                    ///< total number of stored elements
    std::hash<KeyType> hasher;

    /**
     * @brief Computes the bucket index for a given key.
     */
    size_t getBucketIndex(KeyType key) const;
};
