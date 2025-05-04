#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <optional>
#include <utility>
#include <stdexcept>
#include <cmath>
#include <functional>

#include <iostream>

#include "hash_base.h"

/**
 * @brief Multi-level Elastic Hashing without reordering.
 *
 * Supports insert, lookup, update, remove, clear, and dynamic expansion.
 * Achieves O(1) amortized and O(log(1/δ)) worst-case expected probe complexity.
 */

template <typename K, typename V>
class ElasticHash : public HashBase<K, V> {
public:
    using KeyType = K;
    using ValueType = V;

    ElasticHash()
        : ElasticHash(DEFAULT_CAPACITY, 0.1)
    {}

    explicit ElasticHash(size_t n, double delta = 0.1)
        : total_size_(n), delta_(delta), inserts_done_(0)
    {
        buildLevels(n);
        computeTargets();
    }

    void insert(const K& key, const V& value) override {
        if (inserts_done_ + 1 > total_size_ * (1 - delta_)) {
            expand();
            insert(key, value);
            return;
        }

        size_t lvl = currentBatch();

        double eps1 = double(slots_[lvl].size() - occupied_[lvl]) / slots_[lvl].size();
        double eps2 = double(slots_[lvl+1].size() - occupied_[lvl+1]) / slots_[lvl+1].size();
        size_t tries = probes_(std::min(eps1, eps2));

        bool placed = false;

        if (lvl == 0) {
            double eps0 = double(slots_[0].size() - occupied_[0]) / slots_[0].size();
            size_t tries0 = probes_(eps0);
            bool placed0 = false;

            for (size_t j = 0; j < tries0; ++j) {
                size_t idx = hashPos(0, key, j);
                std::cout << "insert lvl: " << 0 << ", idx: " << idx << "\n";
                auto &e = slots_[0][idx];
                if (e.state == State::Occupied) {
                    if (e.kv->first == key) { e.kv->second = value; return; }
                    continue;
                }
                place(0, idx, key, value);
                placed0 = true;
                break;
            }

            if (!placed0) {
                uint64_t j = 0;
                for (;; ++j) {
                    size_t idx = hashPos(0, key, j);
                    auto &e = slots_[0][idx];
                    if (e.state == State::Occupied) {
                        if (e.kv->first == key) { e.kv->second = value; return; }
                        continue;
                    }
                    place(0, idx, key, value);
                    placed0 = true;
                    break;
                }

                if (!placed0) {
                    expand(); insert(key, value); 
                    return;
                }
            }
            ++inserts_done_;
            return;
        }

        if (eps1 > delta_/2 && eps2 > 0.25) {
            for (size_t j = 0; j < tries; ++j) {
                size_t idx = hashPos(lvl, key, j);
                auto &e = slots_[lvl][idx];

                if (e.state == State::Occupied) {
                    if (e.kv->first == key) {
                        e.kv->second = value;
                        return;
                    }
                    continue;
                }

                place(lvl, idx, key, value);
                placed = true;
                break;
            }

            if (!placed) {
                uint64_t j = 0;
                for (;; ++j) {
                    size_t idx = hashPos(lvl+1, key, j);
                    auto &e = slots_[lvl+1][idx];
                    if (e.state == State::Occupied) {
                        if (e.kv->first == key) {
                            e.kv->second = value;
                            return;
                        }
                        continue;
                    }
                    place(lvl+1, idx, key, value);
                    placed = true;
                    break;
                }
                if (!placed) {
                    expand();
                    insert(key, value);
                    return;
                }
            }
        }

        else if (eps1 <= delta_/2) {
            uint64_t j = 0;
            for (;; ++j) {
                size_t idx = hashPos(lvl+1, key, j);
                auto &e = slots_[lvl+1][idx];
                if (e.state == State::Occupied) {
                    if (e.kv->first == key) {
                        e.kv->second = value;
                        return;
                    }
                    continue;
                }
                place(lvl+1, idx, key, value);
                placed = true;
                break;
            }
            if (!placed) {
                expand();
                insert(key, value);
                return;
            }
        }

        else {
            uint64_t j = 0;
            for (;; ++j) {
                size_t idx = hashPos(lvl, key, j);
                auto &e = slots_[lvl][idx];
                if (e.state == State::Occupied) {
                    if (e.kv->first == key) {
                        e.kv->second = value;
                        return;
                    }
                    continue;
                }
                place(lvl, idx, key, value);
                placed = true;
                break;
            }
            if (!placed) {
                expand();
                insert(key, value);
                return;
            }
        }

        ++inserts_done_;
    }

    std::optional<V> lookup(const K& key) const override {
        for (size_t lvl = 0; lvl + 1 < slots_.size(); ++lvl) {
            double eps = double(slots_[lvl].size() - occupied_[lvl])
                       / double(slots_[lvl].size());
            size_t limit = probes_(eps);
            std::cout << "limit: " << limit << "\n";
            std::cout << "key: " << key << "\n";
            for (size_t j = 0; j < limit; ++j) {
                size_t idx = hashPos(lvl, key, j);
                std::cout << "lookup lvl: " << lvl << ", idx: " << idx << "\n";
                const auto &e = slots_[lvl][idx];

                if (e.state == State::Empty)
                    break;

                if (e.state == State::Occupied && e.kv->first == key)
                    return e.kv->second;
            }
        }
        return std::nullopt;
    }

    bool update(const K& key, const V& value) override {
        for (size_t lvl = 0; lvl + 1 < slots_.size(); ++lvl) {
            double eps = double(slots_[lvl].size() - occupied_[lvl])
                       / double(slots_[lvl].size());
            size_t limit = probes_(eps);
    
            for (size_t j = 0; j < limit; ++j) {
                size_t idx = hashPos(lvl, key, j);
                auto &e = slots_[lvl][idx];

                if (e.state == State::Empty)
                    break;
    
                if (e.state == State::Occupied && e.kv->first == key) {
                    e.kv->second = value;
                    return true;
                }
            }
        }
        return false;
    }

    bool remove(const K& key) override {
        for (size_t lvl = 0; lvl + 1 < slots_.size(); ++lvl) {
            double eps = double(slots_[lvl].size() - occupied_[lvl])
                       / double(slots_[lvl].size());
            size_t limit = probes_(eps);
    
            for (size_t j = 0; j < limit; ++j) {
                size_t idx = hashPos(lvl, key, j);
                auto &e = slots_[lvl][idx];
    
                if (e.state == State::Empty)
                    break;
    
                if (e.state == State::Occupied && e.kv->first == key) {
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
        for (auto &lvl : slots_) {
            for (auto &e : lvl) {
                e.state = State::Empty;
                e.kv.reset();
            }
        }
        occupied_.assign(occupied_.size(), 0);
        inserts_done_ = 0;
    }

    double loadFactor() const override {
        return double(inserts_done_) / double(total_size_);
    }

    size_t capacity() const override { return total_size_; }

    void debugPrint() const {
        for (size_t i = 0; i < slots_.size(); ++i) {
            std::cout << "Level " << i << ": ";
            for (const auto &e : slots_[i]) {
                if (e.state == State::Occupied) {
                    std::cout << "(" << e.kv->first << ") ";
                } else if (e.state == State::Deleted) {
                    std::cout << "[D] ";
                } else {
                    std::cout << "[E] ";
                }
            }
            std::cout << "\n";
        }
    }

private:
    static constexpr size_t DEFAULT_CAPACITY = 1024;

    enum class State { Empty, Occupied, Deleted };
    struct Entry { State state=State::Empty; std::optional<std::pair<K,V>> kv; };

    std::vector<std::vector<Entry>> slots_;
    std::vector<size_t> occupied_, fullTarget_, partialTarget_;
    size_t total_size_;
    double delta_;
    size_t inserts_done_;

    void buildLevels(size_t n) {
        slots_.clear();
        occupied_.clear();

        size_t remaining = n;
        while (remaining > 0) {
            size_t levelSize = (remaining + 1) / 2;
            slots_.emplace_back(levelSize);
            occupied_.push_back(0);
            remaining -= levelSize;
        }
    }

    void computeTargets() {
        size_t L=slots_.size();
        fullTarget_.resize(L);
        partialTarget_.resize(L);
        for (size_t i=0;i<L;++i) {
            size_t sz=slots_[i].size();
            fullTarget_[i]=sz - size_t(std::floor(delta_*sz/2));
            partialTarget_[i]=size_t(std::ceil(0.75*sz));
        }
    }

    size_t currentBatch() const {
        size_t L=slots_.size();
        for (size_t i=0;i+1<L;++i) {
            if (occupied_[i]<fullTarget_[i] || occupied_[i+1]<partialTarget_[i+1])
                return i;
        }
        return L-2;
    }

    size_t probes_(double eps) const{
        return size_t(std::ceil(std::min(std::log2(1/eps),std::log2(1/delta_))));
    }
    size_t maxProbes() const { return probes_(delta_); }

    static uint64_t splitmix64(uint64_t x) {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    }

    size_t hashPos(size_t lvl, const K& key, size_t j) const {
        uint64_t h = std::hash<K>{}(key);
        uint64_t a = splitmix64(h ^ (uint64_t)lvl);
        uint64_t b = splitmix64(h ^ (uint64_t)j);
        uint64_t idx = splitmix64(a ^ b);
        return idx % slots_[lvl].size();
    }

    void place(size_t lvl,size_t idx,const K& key,const V& val){
        slots_[lvl][idx].state=State::Occupied;
        slots_[lvl][idx].kv={key,val};
        ++occupied_[lvl];
    }

    void expand(){
        total_size_*=2;
        std::vector<std::pair<K,V>> items;
        items.reserve(inserts_done_);
        for(auto &lvl:slots_)
            for(auto &e:lvl)
                if(e.state==State::Occupied)
                    items.push_back(*e.kv);
        buildLevels(total_size_);
        computeTargets();inserts_done_=0;
        for(auto &kv:items) insert(kv.first,kv.second);
    }

    static size_t nextPow2(size_t n) {
        if(n<2)return 1;
        --n;for(size_t i=1;i<sizeof(n)*8;i<<=1)n|=n>>i;
        return ++n;
    }
};
