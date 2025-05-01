#include "dynamic_resizing.h"
#include <functional>

// Constructor: initializes the table with the given capacity
DynamicResizing::DynamicResizing(size_t initialCapacity) {
    table.resize(initialCapacity);
}

// Hash function: computes index for a given key
size_t DynamicResizing::hash(KeyType key) const {
    return std::hash<KeyType>{}(key) % table.size();
}

// Doubles the table size and re-inserts all occupied entries
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

// Inserts a key-value pair or updates if key already exists
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

// Looks up a value by key
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

// Updates the value of an existing key
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

// Removes a key-value pair if found
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

// Returns the number of elements stored
size_t DynamicResizing::size() const {
    return count;
}

// Clears the hash table
void DynamicResizing::clear() {
    table.assign(table.size(), Slot{});
    count = 0;
}

// Returns the current load factor
double DynamicResizing::loadFactor() const {
    return static_cast<double>(count) / static_cast<double>(table.size());
}

// Returns the current capacity (number of slots)
size_t DynamicResizing::capacity() const {
    return table.size();
}
