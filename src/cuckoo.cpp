#include "cuckoo.h"

CuckooHash::CuckooHash(size_t initial_capacity)
    : capacity_(initial_capacity),
      size_(0),
      table1(initial_capacity),
      table2(initial_capacity) {}

size_t CuckooHash::hash1(const KeyType &key) const {
    return hasher(key) % capacity_;
}

size_t CuckooHash::hash2(const KeyType &key) const {
    size_t h = hasher(key);
    return ((h >> 16) ^ h) % capacity_;
}

void CuckooHash::rehash() {
    capacity_ *= 2;
    size_ = 0;

    std::vector<Entry> old1 = std::move(table1);
    std::vector<Entry> old2 = std::move(table2);

    table1.assign(capacity_, Entry{});
    table2.assign(capacity_, Entry{});

    for (const auto &e : old1)
        if (e.occupied) insert(e.key, e.value);
    for (const auto &e : old2)
        if (e.occupied) insert(e.key, e.value);
}

void CuckooHash::insert(const KeyType &key, const ValueType &value) {
    KeyType cur_key = key;
    ValueType cur_value = value;
    size_t kicks = 0;

    size_t i1 = hash1(cur_key);
    if (table1[i1].occupied && table1[i1].key == cur_key) {
        table1[i1].value = cur_value;
        return;
    }

    size_t i2 = hash2(cur_key);
    if (table2[i2].occupied && table2[i2].key == cur_key) {
        table2[i2].value = cur_value;
        return;
    }

    while (kicks < capacity_) {
        i1 = hash1(cur_key);
        if (!table1[i1].occupied) {
            table1[i1] = {cur_key, cur_value, true};
            ++size_;
            return;
        }
        std::swap(cur_key, table1[i1].key);
        std::swap(cur_value, table1[i1].value);

        i2 = hash2(cur_key);
        if (!table2[i2].occupied) {
            table2[i2] = {cur_key, cur_value, true};
            ++size_;
            return;
        }
        std::swap(cur_key, table2[i2].key);
        std::swap(cur_value, table2[i2].value);

        ++kicks;
    }

    rehash();
    insert(cur_key, cur_value);
}

std::optional<ValueType> CuckooHash::lookup(const KeyType &key) const {
    size_t i1 = hash1(key);
    if (table1[i1].occupied && table1[i1].key == key) return table1[i1].value;
    size_t i2 = hash2(key);
    if (table2[i2].occupied && table2[i2].key == key) return table2[i2].value;
    return std::nullopt;
}

bool CuckooHash::update(const KeyType &key, const ValueType &value) {
    size_t i1 = hash1(key);
    if (table1[i1].occupied && table1[i1].key == key) {
        table1[i1].value = value;
        return true;
    }
    size_t i2 = hash2(key);
    if (table2[i2].occupied && table2[i2].key == key) {
        table2[i2].value = value;
        return true;
    }
    return false;
}

bool CuckooHash::remove(const KeyType &key) {
    size_t i1 = hash1(key);
    if (table1[i1].occupied && table1[i1].key == key) {
        table1[i1].occupied = false;
        --size_;
        return true;
    }
    size_t i2 = hash2(key);
    if (table2[i2].occupied && table2[i2].key == key) {
        table2[i2].occupied = false;
        --size_;
        return true;
    }
    return false;
}

size_t CuckooHash::size() const { return size_; }

void CuckooHash::clear() {
    table1.assign(capacity_, Entry{});
    table2.assign(capacity_, Entry{});
    size_ = 0;
}

double CuckooHash::loadFactor() const {
    return static_cast<double>(size_) / (2.0 * capacity_);
}

size_t CuckooHash::capacity() const { return capacity_; }
