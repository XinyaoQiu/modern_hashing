#include "elastic.h"

ElasticHash::ElasticHash(size_t initial_capacity)
    : capacity_(initial_capacity), size_(0), table(initial_capacity) {}

size_t ElasticHash::probe(const KeyType& key, bool for_insert) const {
    size_t index = hasher(key) % capacity_;
    size_t original = index;
    size_t i = 0;

    while (true) {
        const Entry& entry = table[index];
        if (entry.status == Status::Empty || entry.status == Status::Deleted) {
            if (for_insert) return index;
            if (entry.status == Status::Empty) return capacity_;  // Not found
        }
        if (entry.status == Status::Occupied && entry.key == key) return index;

        ++i;
        index = (original + i) % capacity_;
        if (i == capacity_) return capacity_;  // Full or not found
    }
}

void ElasticHash::insert(const KeyType& key, const ValueType& value) {
    size_t idx = probe(key, false);
    if (idx < capacity_ && table[idx].status == Status::Occupied) {
        table[idx].value = value;
        return;
    }

    idx = probe(key, true);
    if (idx == capacity_) {
        rehash();
        insert(key, value);
        return;
    }

    table[idx] = {key, value, Status::Occupied};
    ++size_;

    if (loadFactor() > 0.7) rehash();
}

std::optional<ValueType> ElasticHash::lookup(const KeyType& key) const {
    size_t idx = probe(key, false);
    if (idx < capacity_ && table[idx].status == Status::Occupied)
        return table[idx].value;
    return std::nullopt;
}

bool ElasticHash::update(const KeyType& key, const ValueType& value) {
    size_t idx = probe(key, false);
    if (idx < capacity_ && table[idx].status == Status::Occupied) {
        table[idx].value = value;
        return true;
    }
    return false;
}

bool ElasticHash::remove(const KeyType& key) {
    size_t idx = probe(key, false);
    if (idx < capacity_ && table[idx].status == Status::Occupied) {
        table[idx].status = Status::Deleted;
        --size_;
        return true;
    }
    return false;
}

size_t ElasticHash::size() const { return size_; }

void ElasticHash::clear() {
    table.assign(capacity_, Entry{});
    size_ = 0;
}

double ElasticHash::loadFactor() const {
    return static_cast<double>(size_) / static_cast<double>(capacity_);
}

size_t ElasticHash::capacity() const { return capacity_; }

void ElasticHash::rehash() {
    size_t old_capacity = capacity_;
    capacity_ *= 2;
    std::vector<Entry> old_table = std::move(table);

    table.assign(capacity_, Entry{});
    size_ = 0;

    for (const auto& e : old_table) {
        if (e.status == Status::Occupied) insert(e.key, e.value);
    }
}
