// include/hash_base.h
#pragma once

#include <optional>

template <typename K, typename V>
class HashBase {
   public:
    virtual ~HashBase() = default;

    // Insert a key-value pair. If the key already exists, the subclass may
    // choose to update it.
    virtual void insert(const K& key, const V& value) = 0;

    // Lookup the value associated with a key. Return std::nullopt if the key
    // doesn't exist.
    virtual std::optional<V> lookup(const K& key) const = 0;

    // Remove a key. Return true if the key existed and was removed, false
    // otherwise.
    virtual bool remove(const K& key) = 0;

    // Update the value for a key. Return true if updated, false if the key
    // doesn't exist.
    virtual bool update(const K& key, const V& value) = 0;

    // Return the number of elements currently stored.
    virtual size_t size() const = 0;

    // Clear the hash table.
    virtual void clear() = 0;

    // (Optional) Return the load factor of the hash table.
    virtual double loadFactor() const = 0;

    // (Optional) Return the current capacity of the internal storage.
    virtual size_t capacity() const = 0;
};
