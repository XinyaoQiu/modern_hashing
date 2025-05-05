#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <optional>
#include <utility>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <iostream>
#include "hash_base.h"

/**
 * @brief Funnel Hashing (greedy, no reordering).
 *
 * Supports insert, lookup, update, remove, clear, and dynamic expansion.
 * Achieves O(log^2(1/δ)) worst-case and O(log(1/δ)) amortized expected probes.
 */
template<typename K, typename V>
class FunnelHash : public HashBase<K, V> {
public:
    using KeyType = K;
    using ValueType = V;

    FunnelHash() : FunnelHash(DEFAULT_CAPACITY, 0.1) {}

    explicit FunnelHash(size_t n, double delta = 0.1)
        : total_size_(n), delta_(delta), inserts_done_(0) {
        // Compute number of levels α and bucket size β (paper Sec.3)
        alpha_ = size_t(std::ceil(4 * std::log2(1.0 / delta_) + 10));
        beta_  = size_t(std::ceil(std::log2(1.0 / delta_)));
        buildLevels(n);
    }

    void insert(const K& key, const V& value) override {
        // Expand if load exceeds (1-δ)
        if (inserts_done_ + 1 > total_size_ * (1 - delta_)) {
            expand();
            insert(key, value);
            return;
        }
        // Greedy tries on levels A1..Aα
        for (size_t lvl = 0; lvl < alpha_; ++lvl) {
            size_t nbuckets = slots_[lvl].size() / beta_;
            size_t b = hashToBucket(lvl, key) % nbuckets;
            size_t start = b * beta_;
            // scan bucket of size β
            for (size_t j = 0; j < beta_; ++j) {
                Entry &e = slots_[lvl][start + j];
                if (e.state == State::Occupied) {
                    if (e.kv->first == key) {
                        e.kv->second = value; // update existing
                        return;
                    }
                    continue;
                }
                // empty or deleted -> place here
                place(lvl, start + j, key, value);
                ++inserts_done_;
                return;
            }
        }
        // Overflow level A_{α+1}
        // First half: uniform probing with up to O(log log n) attempts
        size_t m = slots_[alpha_].size();
        size_t half = m / 2;
        size_t limit = size_t(std::ceil(std::log2(std::log2(total_size_ + 2))));
        for (size_t t = 0; t < limit; ++t) {
            size_t idx = hashPos(alpha_, key, t) % half;
            Entry &e = slots_[alpha_][idx];
            if (e.state == State::Occupied) {
                if (e.kv->first == key) {e.kv->second = value; return;} continue;
            }
            place(alpha_, idx, key, value);
            ++inserts_done_;
            return;
        }
        // Second half: two-choice buckets of size 2·log log n
        size_t bucket_size = limit * 2;
        size_t nb2 = half / bucket_size;
        // hash two buckets
        size_t h1 = hashToBucket(alpha_, key) % nb2;
        size_t h2 = hashToBucket(alpha_, key ^ 0x9e3779b97f4a7c15ULL) % nb2;
        // scan interleaved
        for (size_t j = 0; j < bucket_size; ++j) {
            size_t i1 = half + h1 * bucket_size + j;
            size_t i2 = half + h2 * bucket_size + j;
            for (size_t idx : {i1, i2}) {
                Entry &e = slots_[alpha_][idx];
                if (e.state == State::Occupied) {
                    if (e.kv->first == key) {e.kv->second = value; return;} continue;
                }
                place(alpha_, idx, key, value);
                ++inserts_done_;
                return;
            }
        }
        // if still no place, expand and retry
        expand();
        insert(key, value);
    }

    std::optional<V> lookup(const K& key) const override {
        // search each level greedily (paper Sec.3)
        for (size_t lvl = 0; lvl < alpha_; ++lvl) {
            size_t nbuckets = slots_[lvl].size() / beta_;
            size_t b = hashToBucket(lvl, key) % nbuckets;
            size_t start = b * beta_;
            for (size_t j = 0; j < beta_; ++j) {
                const Entry &e = slots_[lvl][start + j];
                if (e.state == State::Empty) break;
                if (e.state == State::Occupied && e.kv->first == key)
                    return e.kv->second;
            }
        }
        // overflow search
        size_t m = slots_[alpha_].size();
        size_t half = m/2;
        size_t limit = size_t(std::ceil(std::log2(std::log2(total_size_ + 2))));
        // uniform half
        for (size_t t = 0; t < limit; ++t) {
            size_t idx = hashPos(alpha_, key, t) % half;
            const Entry &e = slots_[alpha_][idx];
            if (e.state == State::Empty) break;
            if (e.state == State::Occupied && e.kv->first == key)
                return e.kv->second;
        }
        // two-choice half
        size_t bucket_size = limit*2;
        size_t nb2 = half / bucket_size;
        size_t h1 = hashToBucket(alpha_, key) % nb2;
        size_t h2 = hashToBucket(alpha_, key ^ 0x9e3779b97f4a7c15ULL) % nb2;
        for (size_t j = 0; j < bucket_size; ++j) {
            for (size_t idx : {half + h1*bucket_size + j, half + h2*bucket_size + j}) {
                const Entry &e = slots_[alpha_][idx];
                if (e.state == State::Empty) break;
                if (e.state == State::Occupied && e.kv->first == key)
                    return e.kv->second;
            }
        }
        return std::nullopt;
    }

    bool update(const K& key, const V& value) override {
        auto opt = lookup(key);
        if (!opt) return false;
        insert(key, value);
        return true;
    }

    bool remove(const K& key) override {
        for (size_t lvl = 0; lvl < slots_.size(); ++lvl) {
            size_t start = 0, sz = slots_[lvl].size();
            if (lvl < alpha_) {
                size_t nbuckets = sz/beta_;
                size_t b = hashToBucket(lvl, key) % nbuckets;
                start = b*beta_; sz = beta_;
            } else {
                // search all overflow
                sz = slots_[lvl].size();
            }
            for (size_t j = 0; j < sz; ++j) {
                Entry &e = slots_[lvl][start+j];
                if (e.state==State::Empty) break;
                if (e.state==State::Occupied && e.kv->first==key) {
                    e.state = State::Deleted;
                    e.kv.reset();
                    --occupied_[lvl];
                    return true;
                }
            }
        }
        return false;
    }

    size_t size() const override { return inserts_done_; }

    void clear() override {
        for (auto &lvl : slots_)
            for (auto &e : lvl) { e.state = State::Empty; e.kv.reset(); }
        occupied_.assign(occupied_.size(), 0);
        inserts_done_ = 0;
    }

    double loadFactor() const override { return double(inserts_done_) / total_size_; }
    size_t capacity() const override { return total_size_; }

    void debugPrint() const {
        for (size_t i = 0; i < slots_.size(); ++i) {
            std::cout << "Level " << i << ": ";
            for (const auto &e : slots_[i]) {
                char c = e.state==State::Occupied ? 'O' : (e.state==State::Deleted?'D':'E');
                std::cout << c;
            }
            std::cout << "\n";
        }
    }

private:
    static constexpr size_t DEFAULT_CAPACITY = 1024;
    enum class State { Empty, Occupied, Deleted };
    struct Entry { State state = State::Empty; std::optional<std::pair<K,V>> kv; };

    std::vector<std::vector<Entry>> slots_;
    std::vector<size_t> occupied_;
    size_t total_size_{}, inserts_done_{};
    double delta_{};
    size_t alpha_{}, beta_{};

    // build levels A1..Aα and overflow A_{α+1} (paper Sec.3)
    void buildLevels(size_t n) {
        slots_.clear(); occupied_.clear();
        size_t overflow_min = size_t(std::ceil(delta_ * n / 2));
        size_t remaining = n - overflow_min;
        std::vector<double> geom(alpha_);
        double sum = 0;
        for (size_t i = 0; i < alpha_; ++i) {
            geom[i] = std::pow(0.75, double(i));
            sum += geom[i];
        }
        size_t assigned = 0;
        for (size_t i = 0; i < alpha_; ++i) {
            size_t sz = size_t(std::floor(remaining * geom[i] / sum));
            slots_.emplace_back(sz);
            occupied_.push_back(0);
            assigned += sz;
        }
        size_t rem = n - assigned;
        if (rem < overflow_min) {
            rem = overflow_min;
        }
        slots_.emplace_back(rem);
        occupied_.push_back(0);
    }

    static uint64_t splitmix64(uint64_t x) {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    }

    size_t hashPos(size_t lvl, const K& key, size_t probe) const {
        uint64_t h = std::hash<K>{}(key);
        uint64_t a = splitmix64(h ^ lvl);
        uint64_t b = splitmix64(h ^ probe);
        return size_t(splitmix64(a ^ b));
    }

    size_t hashToBucket(size_t lvl, const K& key) const {
        return hashPos(lvl, key, 0);
    }

    void place(size_t lvl, size_t idx, const K& key, const V& val) {
        slots_[lvl][idx].state = State::Occupied;
        slots_[lvl][idx].kv = std::pair<K,V>(key,val);
        ++occupied_[lvl];
    }

    void expand() {
        total_size_ *= 2;
        std::vector<std::pair<K,V>> items;
        items.reserve(inserts_done_);
        for (auto &lvl : slots_)
            for (auto &e : lvl)
                if (e.state == State::Occupied)
                    items.push_back(*e.kv);
        buildLevels(total_size_);
        inserts_done_ = 0;
        for (auto &kv : items)
            insert(kv.first, kv.second);
    }
};
