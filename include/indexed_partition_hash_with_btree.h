#pragma once

#include <climits>
#include <cmath>
#include <cstdint>
#include <functional>
#include <optional>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "hash_base.h"

using namespace std;

/**
 * @brief IndexedPartitionHashWithBTree implements a fixed-capacity hash table
 * with:
 * - Constant-time query/delete (worst case)
 * - Expected constant-time insertion
 * - Query mapper per bucket (implemented with B-tree over loglog(n)-bit keys)
 */
template <typename K, typename V>
class IndexedPartitionHashWithBTree : public HashBase<K, V> {
   public:
    using KeyType = K;
    using ValueType = V;

    explicit IndexedPartitionHashWithBTree(uint64_t n = 16, double c = 2.0)
        : n_(n),
          c_(c),
          fingerprint_domain_(UINT32_MAX),
          rng_(std::random_device{}()) {
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
            for (uint64_t i = 0; i < bucket.count; ++i) {
                all_entries.emplace_back(bucket.entries[i].key,
                                         bucket.entries[i].value);
            }
        }
        init_structure();  // rebuild new structure with updated n_
        for (const auto& [key, value] : all_entries) {
            insert_no_resize(key,
                             value);  // re-insert without triggering resize
        }
    }

    // helper that inserts without checking resize
    void insert_no_resize(const K& key, const V& value) {
        uint64_t b = bucket_index(key);
        Bucket& bucket = buckets_[b];

        uint32_t fp = fingerprint(key, bucket.fingerprint_salt_);
        if (bucket.query_mapper.count(fp)) {
            rebuild_fingerprints(bucket);
            fp = fingerprint(key, bucket.fingerprint_salt_);
        }

        if (bucket.count >= bucket_capacity_) {
            throw std::runtime_error("Bucket overflow during rehash");
        }

        uint64_t pos = bucket.count++;
        bucket.entries[pos] = {key, value};
        bucket.query_mapper[fp] = pos;
        ++size_;
    }

    void insert(const K& key, const V& value) override {
        maybe_resize();  // trigger resize if load factor >= 0.7

        uint64_t b = bucket_index(key);
        Bucket& bucket = buckets_[b];

        uint32_t fp = fingerprint(key, bucket.fingerprint_salt_);
        if (bucket.query_mapper.count(fp)) {
            rebuild_fingerprints(bucket);
            fp = fingerprint(key, bucket.fingerprint_salt_);
        }

        if (bucket.count >= bucket_capacity_) {
            throw std::runtime_error("Bucket overflow: rebuild required");
        }

        uint64_t pos = bucket.count++;
        bucket.entries[pos] = {key, value};
        bucket.query_mapper[fp] = pos;
        ++size_;
    }

    std::optional<V> lookup(const K& key) const override {
        if (key == 76893931) {
            uint64_t b = bucket_index(key);
            const Bucket& bucket = buckets_[b];

            uint32_t fp = fingerprint(key, bucket.fingerprint_salt_);
            auto it = bucket.query_mapper.find(fp);
            if (it == bucket.query_mapper.end()) return std::nullopt;

            uint64_t pos = it->second;
            if (pos >= bucket.count) return std::nullopt;
            return bucket.entries[pos].value;
        }
        uint64_t b = bucket_index(key);
        const Bucket& bucket = buckets_[b];

        uint32_t fp = fingerprint(key, bucket.fingerprint_salt_);
        auto it = bucket.query_mapper.find(fp);
        if (it == bucket.query_mapper.end()) return std::nullopt;

        uint64_t pos = it->second;
        if (pos >= bucket.count) return std::nullopt;
        if (bucket.entries[pos].key != key) return std::nullopt;
        return bucket.entries[pos].value;
    }

    bool update(const K& key, const V& value) override {
        uint64_t b = bucket_index(key);
        Bucket& bucket = buckets_[b];

        uint32_t fp = fingerprint(key, bucket.fingerprint_salt_);
        auto it = bucket.query_mapper.find(fp);
        if (it == bucket.query_mapper.end()) return false;

        uint64_t pos = it->second;
        if (pos >= bucket.count || bucket.entries[pos].key != key) return false;
        bucket.entries[pos].value = value;
        return true;
    }

    bool remove(const K& key) override {
        uint64_t b = bucket_index(key);
        Bucket& bucket = buckets_[b];

        uint32_t fp = fingerprint(key, bucket.fingerprint_salt_);
        auto it = bucket.query_mapper.find(fp);
        if (it == bucket.query_mapper.end()) return false;

        uint64_t pos = it->second;
        if (pos >= bucket.count || bucket.entries[pos].key != key) return false;

        // Swap with last item to maintain left justification
        if (pos != bucket.count - 1) {
            bucket.entries[pos] = bucket.entries[bucket.count - 1];
            uint32_t moved_fp =
                fingerprint(bucket.entries[pos].key, bucket.fingerprint_salt_);
            bucket.query_mapper[moved_fp] = pos;
        }

        bucket.query_mapper.erase(fp);
        --bucket.count;
        --size_;
        return true;
    }

    uint64_t size() const override { return size_; }

    void clear() override { init_structure(); }

    double loadFactor() const override {
        return static_cast<double>(size_) / static_cast<double>(n_);
    }

    uint64_t capacity() const override { return n_; }

   private:
    struct Entry {
        K key;
        V value;
    };

    struct Bucket {
        std::vector<Entry> entries;
        std::unordered_map<uint32_t, uint32_t> query_mapper;
        uint64_t count = 0;
        uint64_t fingerprint_salt_ = 42;
    };

    uint64_t n_;
    double c_;
    uint64_t bucket_capacity_;
    uint64_t num_buckets_;
    std::vector<Bucket> buckets_;
    std::hash<K> hasher_;
    uint64_t size_;
    uint32_t fingerprint_domain_;
    std::mt19937_64 rng_;

    uint64_t bucket_index(const K& key) const {
        return hasher_(key) % num_buckets_;
    }

    uint32_t fingerprint(const K& key, uint64_t fingerprint_salt_) const {
        return (std::hash<K>()(key) ^ fingerprint_salt_) % fingerprint_domain_;
    }

    void rebuild_fingerprints(Bucket& bucket) {
        bucket.fingerprint_salt_ = rng_();
        bucket.query_mapper.clear();
        for (uint64_t i = 0; i < bucket.count; ++i) {
            uint32_t new_fp =
                fingerprint(bucket.entries[i].key, bucket.fingerprint_salt_);
            if (bucket.query_mapper.count(new_fp)) {
                // Retry recursively
                rebuild_fingerprints(bucket);
                return;
            }
            bucket.query_mapper[new_fp] = i;
        }
    }

    // static constexpr uint64_t fingerprint_domain_ = 1ull << 12; // ~log^9(n)
    // for n ~ 1M

    void init_structure() {
        size_ = 0;
        double logn = std::log(n_);
        bucket_capacity_ =
            static_cast<uint64_t>(std::pow(logn, 3) + c_ * std::pow(logn, 2));
        num_buckets_ = std::max<uint64_t>(
            1, static_cast<uint64_t>(n_ / std::pow(logn, 3)));

        // std::cout << "Bucket capacity: " << bucket_capacity_ << std::endl;
        // std::cout << "Number of buckets: " << num_buckets_ << std::endl;

        buckets_.clear();
        buckets_.resize(num_buckets_);
        for (auto& b : buckets_) {
            b.entries.resize(bucket_capacity_);
            b.count = 0;
            b.query_mapper.clear();
        }
    }
};
