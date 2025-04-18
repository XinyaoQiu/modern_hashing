#pragma once

#include <optional>
#include <vector>

using KeyType = int;
using ValueType = int;

class DynamicResizing {
   private:
    enum class SlotState { EMPTY, OCCUPIED, DELETED };

    struct Slot {
        KeyType key;
        ValueType value;
        SlotState state = SlotState::EMPTY;
    };

    std::vector<Slot> table;
    size_t count = 0;
    float loadFactorThreshold = 0.6f;

    void resize() {
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
    size_t hash(KeyType key) const {
        return std::hash<KeyType>{}(key) % table.size();
    }

   public:
   DynamicResizing(size_t initialCapacity = 8) {
        table.resize(initialCapacity);
    }

    bool insert(KeyType key, ValueType value) {
        if ((float)(count + 1) / table.size() > loadFactorThreshold) {
            resize();
        }

        size_t idx = hash(key);
        size_t start = idx;

        while (table[idx].state == SlotState::OCCUPIED) {
            if (table[idx].key == key) {
                table[idx].value = value;
                return true;  // modifyd
            }
            idx = (idx + 1) % table.size();
            if (idx == start) return false;  // full
        }

        table[idx].key = key;
        table[idx].value = value;
        table[idx].state = SlotState::OCCUPIED;
        ++count;
        return true;
    }
    bool modify(KeyType key, ValueType value) {
        size_t idx = hash(key);
        size_t start = idx;

        while (table[idx].state != SlotState::EMPTY) {
            if (table[idx].state == SlotState::OCCUPIED &&
                table[idx].key == key) {
                table[idx].value = value;
                return true;
            }
            idx = (idx + 1) % table.size();
            if (idx == start) break;
        }

        return false;
    }
    std::optional<ValueType> lookup(KeyType key) const {
        size_t idx = hash(key);
        size_t start = idx;

        while (table[idx].state != SlotState::EMPTY) {
            if (table[idx].state == SlotState::OCCUPIED &&
                table[idx].key == key) {
                return table[idx].value;
            }
            idx = (idx + 1) % table.size();
            if (idx == start) break;
        }

        return std::nullopt;
    }
    bool remove(KeyType key) {
        size_t idx = hash(key);
        size_t start = idx;

        while (table[idx].state != SlotState::EMPTY) {
            if (table[idx].state == SlotState::OCCUPIED &&
                table[idx].key == key) {
                table[idx].state = SlotState::DELETED;
                --count;
                return true;
            }
            idx = (idx + 1) % table.size();
            if (idx == start) break;
        }

        return false;
    }
};