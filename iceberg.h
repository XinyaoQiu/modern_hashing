#include <iostream>
#include <vector>
#include <list>
#include <cmath>
#include <cstring>
#include <optional>
#include <cassert>

using KeyType = uint64_t;
using ValueType = uint64_t;

const double RESIZE_THRESHOLD = 0.85;
const size_t SLOT_BITS = 6; // 64 slots
const size_t LV2_SLOTS = 8;

struct Entry {
    KeyType key = 0;
    ValueType value = 0;
};

class IcebergHash {
private:
    struct Block {
        std::vector<Entry> slots;
        Block(size_t n) : slots(n) {}
    };

    size_t capacity_blocks;
    size_t size = 0;

    std::vector<Block> level1;
    std::vector<Block> level2;
    std::vector<std::list<Entry>> level3;

    size_t hash1(KeyType key) const { return std::hash<KeyType>{}(key) % capacity_blocks; }
    size_t hash2(KeyType key) const { return (std::hash<KeyType>{}(key) / 37) % capacity_blocks; }

    void resize() {
        size_t old_blocks = capacity_blocks;
        capacity_blocks *= 2;
        auto old_level1 = level1;
        auto old_level2 = level2;
        auto old_level3 = level3;

        level1 = std::vector<Block>(capacity_blocks, Block(1ULL << SLOT_BITS));
        level2 = std::vector<Block>(capacity_blocks, Block(LV2_SLOTS));
        level3 = std::vector<std::list<Entry>>(capacity_blocks);

        size = 0;
        for (size_t i = 0; i < old_blocks; ++i) {
            for (const auto& e : old_level1[i].slots) {
                if (e.key != 0) insert(e.key, e.value);
            }
            for (const auto& e : old_level2[i].slots) {
                if (e.key != 0) insert(e.key, e.value);
            }
            for (const auto& e : old_level3[i]) {
                insert(e.key, e.value);
            }
        }
    }

public:
    IcebergHash(size_t init_blocks = 64)
        : capacity_blocks(init_blocks),
          level1(init_blocks, Block(1ULL << SLOT_BITS)),
          level2(init_blocks, Block(LV2_SLOTS)),
          level3(init_blocks) {}

    bool insert(KeyType key, ValueType value) {
        if ((double)size / (capacity_blocks * ((1ULL << SLOT_BITS) + LV2_SLOTS)) >= RESIZE_THRESHOLD)
            resize();

        size_t idx1 = hash1(key);
        for (auto& e : level1[idx1].slots) {
            if (e.key == 0 || e.key == key) {
                if (e.key == 0) ++size;
                e = {key, value};
                return true;
            }
        }

        size_t idx2 = hash2(key);
        for (auto& e : level2[idx2].slots) {
            if (e.key == 0 || e.key == key) {
                if (e.key == 0) ++size;
                e = {key, value};
                return true;
            }
        }

        level3[idx1].push_front({key, value});
        ++size;
        return true;
    }

    std::optional<ValueType> lookup(KeyType key) const {
        size_t idx1 = hash1(key);
        for (const auto& e : level1[idx1].slots) {
            if (e.key == key) return e.value;
        }

        size_t idx2 = hash2(key);
        for (const auto& e : level2[idx2].slots) {
            if (e.key == key) return e.value;
        }

        for (const auto& e : level3[idx1]) {
            if (e.key == key) return e.value;
        }

        return std::nullopt;
    }

    bool remove(KeyType key) {
        size_t idx1 = hash1(key);
        for (auto& e : level1[idx1].slots) {
            if (e.key == key) {
                e.key = 0;
                --size;
                return true;
            }
        }

        size_t idx2 = hash2(key);
        for (auto& e : level2[idx2].slots) {
            if (e.key == key) {
                e.key = 0;
                --size;
                return true;
            }
        }

        auto& lst = level3[idx1];
        for (auto it = lst.begin(); it != lst.end(); ++it) {
            if (it->key == key) {
                lst.erase(it);
                --size;
                return true;
            }
        }

        return false;
    }

    bool modify(KeyType key, ValueType new_value) {
        if (auto val = lookup(key)) {
            remove(key);
            insert(key, new_value);
            return true;
        }
        return false;
    }
};