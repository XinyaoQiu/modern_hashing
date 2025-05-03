#pragma once

#include "hash_base.h"
#include "btree.h"
#include <vector>
#include <cmath>
#include <stdexcept>
#include <optional>
#include <functional>

using namespace std;

/**
 * @brief A partitioned hash table with query mapper using BTree to support
 *        O(1) expected-time insertions and O(1) worst-case query/delete.
 */
template <typename K, typename V>
class IndexedPartitionHashTable : public HashBase<K, V> {
public:
    explicit IndexedPartitionHashTable(size_t n = 16, double c = 2.0)
        : n_(n), hasher_(), fingerprint_hasher_(), size(0) {
        double logn = log(n_);
        size_t bucket_capacity = static_cast<size_t>(pow(logn, 3) + c * pow(logn, 2));
        size_t num_buckets = static_cast<size_t>(n_ / pow(logn, 3));
        btree_order_ = static_cast<size_t>(sqrt(logn));

        buckets_.resize(num_buckets);
        for (auto& b : buckets_) {
            b.slots.resize(bucket_capacity);
            b.count = 0;
            b.query_mapper = BTree<size_t>(btree_order_);
        }
    }

    void insert(const K& key, const V& value) override {
        size_t b = bucket_index(key);
        Bucket& bucket = buckets_[b];

        size_t fp = fingerprint(key);
        if (bucket.query_mapper.search(fp) != nullptr) {
            rebuild_fingerprints(bucket);
            fp = fingerprint(key);
        }

        if (bucket.count >= bucket.slots.size())
            throw runtime_error("Bucket overflow");

        bucket.slots[bucket.count] = {key, value};
        bucket.query_mapper.insert(fp);
        bucket.count++;
        size_++;
    }

    optional<V> lookup(const K& key) const override {
        size_t b = bucket_index(key);
        const Bucket& bucket = buckets_[b];

        size_t fp = fingerprint(key);
        auto node = bucket.query_mapper.search(fp);
        if (!node) return nullopt;

        int pos = node->value_for_key(fp);
        if (pos >= bucket.count || bucket.slots[pos].key != key) return nullopt;
        return bucket.slots[pos].value;
    }

    bool update(const K& key, const V& value) override {
        size_t b = bucket_index(key);
        Bucket& bucket = buckets_[b];

        size_t fp = fingerprint(key);
        auto node = bucket.query_mapper.search(fp);
        if (!node) return false;

        int pos = node->value_for_key(fp);
        if (pos >= bucket.count || bucket.slots[pos].key != key) return false;
        bucket.slots[pos].value = value;
        return true;
    }

    bool remove(const K& key) override {
        size_t b = bucket_index(key);
        Bucket& bucket = buckets_[b];

        size_t fp = fingerprint(key);
        auto node = bucket.query_mapper.search(fp);
        if (!node) return false;

        int pos = node->value_for_key(fp);
        if (pos >= bucket.count || bucket.slots[pos].key != key) return false;

        if (pos != bucket.count - 1) {
            bucket.slots[pos] = bucket.slots[bucket.count - 1];
            size_t moved_fp = fingerprint(bucket.slots[pos].key);
            bucket.query_mapper.insert(moved_fp, pos);
        }

        bucket.query_mapper.remove(fp);
        bucket.count--;
        size_--;
        return true;
    }

    size_t size() const override { return size_; }

    void clear() override {
        for (auto& bucket : buckets_) {
            bucket.count = 0;
            bucket.query_mapper = BTree<size_t>(btree_order_);
        }
        size_ = 0;
    }

    double loadFactor() const override {
        return static_cast<double>(size_) / static_cast<double>(n_);
    }

    size_t capacity() const override {
        return n_;
    }

private:
    struct Entry {
        K key;
        V value;
    };

    struct Bucket {
        vector<Entry> slots;
        size_t count = 0;
        BTree<size_t> query_mapper;  // maps fingerprint -> slot index
    };

    size_t n_;
    size_t btree_order_;
    vector<Bucket> buckets_;
    hash<K> hasher_;
    hash<K> fingerprint_hasher_;
    size_t size_;

    size_t bucket_index(const K& key) const {
        return hasher_(key) % buckets_.size();
    }

    size_t fingerprint(const K& key) const {
        return fingerprint_hasher_(key) % (1ULL << 12);  // log^9(n)
    }

    void rebuild_fingerprints(Bucket& bucket) {
        fingerprint_hasher_ = hash<K>();  // regenerate hash function
        bucket.query_mapper = BTree<size_t>(btree_order_);
        for (size_t i = 0; i < bucket.count; ++i) {
            size_t new_fp = fingerprint(bucket.slots[i].key);
            if (bucket.query_mapper.search(new_fp) != nullptr) {
                rebuild_fingerprints(bucket);
                return;
            }
            bucket.query_mapper.insert(new_fp, i);
        }
    }
};
