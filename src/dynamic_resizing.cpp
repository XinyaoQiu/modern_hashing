#include "dynamic_resizing.h"
#include <functional>

DynamicResizing::DynamicResizing(size_t initialCapacity) {
    table.resize(initialCapacity);
}

size_t DynamicResizing::hash(KeyType key) const {
    return std::hash<KeyType>{}(key) % table.size();
}

void DynamicResizing::resize() {
    std::vector<Slot> oldTable = table;
    table.clear();
    table.resize(oldTable.size() * 2);
    count = 0;

    for (const auto& slot : oldTable) {
        if (slot.state == SlotState::OCCUPIED) {
            insert(slot.key, slot.value);
        }
    }
}

void DynamicResizing::insert(const KeyType& key, const ValueType& value) {
    if ((float)(count + 1) / table.size() > loadFactorThreshold) {
        resize();
    }

    size_t idx = hash(key);
    size_t start = idx;

    while (table[idx].state == SlotState::OCCUPIED) {
        if (table[idx].key == key) {
            table[idx].value = value;
            return;
        }
        idx = (idx + 1) % table.size();
        if (idx == start) return;
    }

    table[idx].key = key;
    table[idx].value = value;
    table[idx].state = SlotState::OCCUPIED;
    ++count;
}

std::optional<ValueType> DynamicResizing::lookup(const KeyType& key) const {
    size_t idx = hash(key);
    size_t start = idx;

    while (table[idx].state != SlotState::EMPTY) {
        if (table[idx].state == SlotState::OCCUPIED && table[idx].key == key) {
            return table[idx].value;
        }
        idx = (idx + 1) % table.size();
        if (idx == start) break;
    }

    return std::nullopt;
}

bool DynamicResizing::update(const KeyType& key, const ValueType& value) {
    size_t idx = hash(key);
    size_t start = idx;

    while (table[idx].state != SlotState::EMPTY) {
        if (table[idx].state == SlotState::OCCUPIED && table[idx].key == key) {
            table[idx].value = value;
            return true;
        }
        idx = (idx + 1) % table.size();
        if (idx == start) break;
    }

    return false;
}

bool DynamicResizing::remove(const KeyType& key) {
    size_t idx = hash(key);
    size_t start = idx;

    while (table[idx].state != SlotState::EMPTY) {
        if (table[idx].state == SlotState::OCCUPIED && table[idx].key == key) {
            table[idx].state = SlotState::DELETED;
            --count;
            return true;
        }
        idx = (idx + 1) % table.size();
        if (idx == start) break;
    }

    return false;
}

size_t DynamicResizing::size() const {
    return count;
}

void DynamicResizing::clear() {
    table.assign(table.size(), Slot{});
    count = 0;
}

double DynamicResizing::loadFactor() const {
    return static_cast<double>(count) / static_cast<double>(table.size());
}

size_t DynamicResizing::capacity() const {
    return table.size();
}
