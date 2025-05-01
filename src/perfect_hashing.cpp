#include "perfect_hashing.h"

// --- SecondaryTable ---

size_t SecondaryTable::hash(KeyType key) const {
    return hasher(key) % capacity;
}

void SecondaryTable::rebuild() {
    std::vector<std::pair<KeyType, ValueType>> entries;
    for (const auto& slot : table) {
        if (slot.has_value()) {
            entries.push_back(*slot);
        }
    }
    build(entries);
}

void SecondaryTable::build(const std::vector<std::pair<KeyType, ValueType>>& entries) {
    size = entries.size();
    capacity = std::max(2 * size * size, size_t(4));

    table.clear();
    table.resize(capacity);

    for (const auto& [k, v] : entries) {
        size_t h = hash(k);
        while (table[h].has_value()) {
            h = (h + 1) % capacity;
        }
        table[h] = {k, v};
    }
}

std::optional<ValueType> SecondaryTable::lookup(KeyType key) const {
    if (capacity == 0) return std::nullopt;

    size_t h = hash(key);
    size_t start = h;
    do {
        if (!table[h].has_value()) return std::nullopt;
        if (table[h]->first == key) return table[h]->second;
        h = (h + 1) % capacity;
    } while (h != start);

    return std::nullopt;
}

bool SecondaryTable::remove(KeyType key) {
    if (capacity == 0) return false;

    size_t h = hash(key);
    size_t start = h;
    do {
        if (!table[h].has_value()) return false;
        if (table[h]->first == key) {
            table[h].reset();
            size--;
            return true;
        }
        h = (h + 1) % capacity;
    } while (h != start);

    return false;
}

bool SecondaryTable::insert_or_modify(KeyType key, ValueType value) {
    if (capacity == 0) {
        build({{key, value}});
        return true;
    }

    size_t h = hash(key);
    size_t start = h;
    do {
        if (!table[h].has_value()) {
            table[h] = {key, value};
            size++;
            if (size > capacity / 2) rebuild();
            return true;
        }
        if (table[h]->first == key) {
            table[h]->second = value;
            return true;
        }
        h = (h + 1) % capacity;
    } while (h != start);

    rebuild();
    return insert_or_modify(key, value);
}

// --- PerfectHash ---

PerfectHash::PerfectHash(size_t initialBuckets)
    : bucketCount(initialBuckets) {
    buckets.resize(bucketCount);
}

void PerfectHash::insert(const KeyType& key, const ValueType& value) {
    size_t index = getBucketIndex(key);
    if (buckets[index].insert_or_modify(key, value)) {
        ++size_;
    }
}

std::optional<ValueType> PerfectHash::lookup(const KeyType& key) const {
    size_t index = getBucketIndex(key);
    return buckets[index].lookup(key);
}

bool PerfectHash::update(const KeyType& key, const ValueType& value) {
    size_t index = getBucketIndex(key);
    auto existing = buckets[index].lookup(key);
    if (!existing.has_value()) return false;
    return buckets[index].insert_or_modify(key, value);
}

bool PerfectHash::remove(const KeyType& key) {
    size_t index = getBucketIndex(key);
    if (buckets[index].remove(key)) {
        --size_;
        return true;
    }
    return false;
}

size_t PerfectHash::size() const {
    return size_;
}

void PerfectHash::clear() {
    for (auto& b : buckets) {
        b = SecondaryTable();
    }
    size_ = 0;
}

double PerfectHash::loadFactor() const {
    return static_cast<double>(size_) / static_cast<double>(bucketCount);
}

size_t PerfectHash::capacity() const {
    return bucketCount;
}

size_t PerfectHash::getBucketIndex(KeyType key) const {
    return hasher(key) % bucketCount;
}
