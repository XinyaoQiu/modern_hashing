#pragma once

#include "hash_base.h"
#include <cmath>
#include <optional>
#include <stdexcept>
#include <functional>

/**
 * @brief A slow partition hash table using raw arrays and compile-time bucket sizing.
 *        Insert: expected O(1), Lookup/Delete: O(log^3 n), No dynamic resizing.
 *
 * @tparam K key type
 * @tparam V value type
 * @tparam N total number of elements the table is expected to store
 */
template <typename K, typename V, int N>
class SlowPartitionHashTable : public HashBase<K, V> {
public:
    SlowPartitionHashTable() : size_(0) {
        for (int i = 0; i < NUM_BUCKETS; ++i) {
            buckets_[i].count = 0;
        }
    }

    void insert(const K& key, const V& value) override {
        size_t b = bucket_index(key);
        Bucket& bucket = buckets_[b];

        if (bucket.count >= BUCKET_CAPACITY) {
            throw std::runtime_error("Bucket overflow - table needs rebuild");
        }

        bucket.slots[bucket.count++] = Entry{key, value};
        ++size_;
    }

    std::optional<V> lookup(const K& key) const override {
        size_t b = bucket_index(key);
        const Bucket& bucket = buckets_[b];

        for (size_t i = 0; i < bucket.count; ++i) {
            if (bucket.slots[i].key == key) {
                return bucket.slots[i].value;
            }
        }
        return std::nullopt;
    }

    bool update(const K& key, const V& value) override {
        size_t b = bucket_index(key);
        Bucket& bucket = buckets_[b];

        for (size_t i = 0; i < bucket.count; ++i) {
            if (bucket.slots[i].key == key) {
                bucket.slots[i].value = value;
                return true;
            }
        }
        return false;
    }

    bool remove(const K& key) override {
        size_t b = bucket_index(key);
        Bucket& bucket = buckets_[b];

        for (size_t i = 0; i < bucket.count; ++i) {
            if (bucket.slots[i].key == key) {
                if (i != bucket.count - 1) {
                    bucket.slots[i] = bucket.slots[bucket.count - 1];
                }
                --bucket.count;
                --size_;
                return true;
            }
        }
        return false;
    }

    size_t size() const override {
        return size_;
    }

    void clear() override {
        for (int i = 0; i < NUM_BUCKETS; ++i) {
            buckets_[i].count = 0;
        }
        size_ = 0;
    }

    double loadFactor() const override {
        return static_cast<double>(size_) / N;
    }

    size_t capacity() const override {
        return N;
    }

private:
    static constexpr double LOGN = std::log(N);
    static constexpr int BUCKET_CAPACITY = static_cast<int>(std::pow(LOGN, 3) + 2.0 * std::pow(LOGN, 2));
    static constexpr int NUM_BUCKETS = static_cast<int>(N / std::pow(LOGN, 3));

    struct Entry {
        K key;
        V value;
    };

    struct Bucket {
        Entry slots[BUCKET_CAPACITY];
        size_t count;
    };

    Bucket buckets_[NUM_BUCKETS];
    size_t size_;
    std::hash<K> hasher_;

    size_t bucket_index(const K& key) const {
        return hasher_(key) % NUM_BUCKETS;
    }
};
