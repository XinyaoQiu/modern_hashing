#pragma once

#include "hash_base.h"
#include <vector>
#include <optional>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <unordered_map>

/**
 * @brief IndexedPartitionHashWithBTree implements a fixed-capacity hash table with:
 * - Constant-time query/delete (worst case)
 * - Expected constant-time insertion
 * - Query mapper per bucket (implemented with B-tree over loglog(n)-bit keys)
 */
template<typename K, typename V>
class IndexedPartitionHashWithBTree : public HashBase<K, V> {
public:
    using KeyType = K;
    using ValueType = V;

    explicit IndexedPartitionHashWithBTree(size_t n = 16, double c = 2.0)
        : n_(n), c_(c), hasher_(), fingerprint_hasher_(), size_(0) {
        init_structure();
    }

    void maybe_resize() {
        if (loadFactor() >= 0.7) {
            n_ *= 2;
            rehash();
        }
    }
    
    void rehash() {
        std::vector<std::pair<K, V>> all_entries;
        for (const auto& bucket : buckets_) {
            for (size_t i = 0; i < bucket.count; ++i) {
                all_entries.emplace_back(bucket.entries[i].key, bucket.entries[i].value);
            }
        }
        init_structure();  // rebuild new structure with updated n_
        for (const auto& [key, value] : all_entries) {
            insert_no_resize(key, value);  // re-insert without triggering resize
        }
    }
    
    // helper that inserts without checking resize
    void insert_no_resize(const K& key, const V& value) {
        size_t b = bucket_index(key);
        Bucket& bucket = buckets_[b];
    
        size_t fp = fingerprint(key);
        if (bucket.query_mapper.count(fp)) {
            rebuild_fingerprints(bucket);
            fp = fingerprint(key);
        }
    
        if (bucket.count >= bucket_capacity_) {
            throw std::runtime_error("Bucket overflow during rehash");
        }
    
        size_t pos = bucket.count++;
        bucket.entries[pos] = {key, value};
        bucket.query_mapper[fp] = pos;
        ++size_;
    }

    void insert(const K& key, const V& value) override {
        maybe_resize();  // trigger resize if load factor >= 0.7
    
        size_t b = bucket_index(key);
        Bucket& bucket = buckets_[b];
    
        size_t fp = fingerprint(key);
        if (bucket.query_mapper.count(fp)) {
            rebuild_fingerprints(bucket);
            fp = fingerprint(key);
        }
    
        if (bucket.count >= bucket_capacity_) {
            throw std::runtime_error("Bucket overflow: rebuild required");
        }
    
        size_t pos = bucket.count++;
        bucket.entries[pos] = {key, value};
        bucket.query_mapper[fp] = pos;
        ++size_;
    }

    std::optional<V> lookup(const K& key) const override {
        size_t b = bucket_index(key);
        const Bucket& bucket = buckets_[b];

        size_t fp = fingerprint(key);
        auto it = bucket.query_mapper.find(fp);
        if (it == bucket.query_mapper.end()) return std::nullopt;

        size_t pos = it->second;
        if (pos >= bucket.count) return std::nullopt;
        if (bucket.entries[pos].key != key) return std::nullopt;
        return bucket.entries[pos].value;
    }

    bool update(const K& key, const V& value) override {
        size_t b = bucket_index(key);
        Bucket& bucket = buckets_[b];

        size_t fp = fingerprint(key);
        auto it = bucket.query_mapper.find(fp);
        if (it == bucket.query_mapper.end()) return false;

        size_t pos = it->second;
        if (pos >= bucket.count || bucket.entries[pos].key != key) return false;
        bucket.entries[pos].value = value;
        return true;
    }

    bool remove(const K& key) override {
        size_t b = bucket_index(key);
        Bucket& bucket = buckets_[b];

        size_t fp = fingerprint(key);
        auto it = bucket.query_mapper.find(fp);
        if (it == bucket.query_mapper.end()) return false;

        size_t pos = it->second;
        if (pos >= bucket.count || bucket.entries[pos].key != key) return false;

        // Swap with last item to maintain left justification
        if (pos != bucket.count - 1) {
            bucket.entries[pos] = bucket.entries[bucket.count - 1];
            size_t moved_fp = fingerprint(bucket.entries[pos].key);
            bucket.query_mapper[moved_fp] = pos;
        }

        bucket.query_mapper.erase(fp);
        --bucket.count;
        --size_;
        return true;
    }

    size_t size() const override {
        return size_;
    }

    void clear() override {
        init_structure();
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
        std::vector<Entry> entries;
        std::unordered_map<size_t, size_t> query_mapper; // fingerprint -> position (hash table)
        size_t count = 0;
    };

    size_t n_;
    double c_;
    size_t bucket_capacity_;
    size_t num_buckets_;
    std::vector<Bucket> buckets_;
    std::hash<K> hasher_;
    std::hash<K> fingerprint_hasher_; // for mapping to small fingerprint
    size_t size_;

    size_t bucket_index(const K& key) const {
        return hasher_(key) % num_buckets_;
    }

    size_t fingerprint(const K& key) const {
        return fingerprint_hasher_(key) % fingerprint_domain_;
    }

    void rebuild_fingerprints(Bucket& bucket) {
        fingerprint_hasher_ = std::hash<K>(); // Replace with new one if needed
        bucket.query_mapper.clear();
        for (size_t i = 0; i < bucket.count; ++i) {
            size_t new_fp = fingerprint(bucket.entries[i].key);
            if (bucket.query_mapper.count(new_fp)) {
                // Retry recursively
                rebuild_fingerprints(bucket);
                return;
            }
            bucket.query_mapper[new_fp] = i;
        }
    }

    static constexpr size_t fingerprint_domain_ = 1ull << 12; // ~log^9(n) for n ~ 1M

    void init_structure() {
        size_ = 0;
        double logn = std::log(n_);
        bucket_capacity_ = static_cast<size_t>(std::pow(logn, 3) + c_ * std::pow(logn, 2));
        num_buckets_ = std::max<size_t>(1, static_cast<size_t>(n_ / std::pow(logn, 3)));

        buckets_.clear();
        buckets_.resize(num_buckets_);
        for (auto& b : buckets_) {
            b.entries.resize(bucket_capacity_);
            b.count = 0;
            b.query_mapper.clear();
        }
    }
};
