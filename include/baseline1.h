#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "hash_base.h"

/**
 * @brief A simple open-addressing hash table with dynamic resizing.
 *
 * This hash table uses linear probing for collision resolution and
 * automatically doubles in size when the load factor exceeds a threshold.
 * Deleted entries are marked and skipped during probing.
 */
template <typename K, typename V>
class Baseline1 : public HashBase<K, V> {
   public:
    /**
     * @brief Constructs the hash table with the given initial capacity.
     * @param initialCapacity Initial number of slots in the table.
     */
    explicit Baseline1(size_t initialCapacity = 8) {
        table.resize(initialCapacity);
    }

    /**
     * @brief Inserts or updates a key-value pair.
     * @param key The key to insert.
     * @param value The value associated with the key.
     */
    void insert(const K& key, const V& value) override {
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

    /**
     * @brief Looks up the value associated with a key.
     * @param key The key to find.
     * @return The value if the key is found, otherwise std::nullopt.
     */
    std::optional<V> lookup(const K& key) const override {
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

    /**
     * @brief Updates an existing key with a new value.
     * @param key The key to update.
     * @param value The new value.
     * @return True if the key existed and was updated, false otherwise.
     */
    bool update(const K& key, const V& value) override {
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

    /**
     * @brief Removes a key-value pair.
     * @param key The key to remove.
     * @return True if the key was found and removed, false otherwise.
     */
    bool remove(const K& key) override {
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

    /**
     * @brief Returns the number of active elements in the table.
     */
    size_t size() const override { return count; }

    /**
     * @brief Clears the hash table.
     */
    void clear() override {
        table.assign(table.size(), Slot{});
        count = 0;
    }

    /**
     * @brief Returns the current load factor of the table.
     */
    double loadFactor() const override {
        return static_cast<double>(count) / static_cast<double>(table.size());
    }

    /**
     * @brief Returns the current capacity of the table (number of slots).
     */
    size_t capacity() const override { return table.size(); }

   private:
    // State of a single slot: EMPTY, OCCUPIED, or DELETED.
    enum class SlotState { EMPTY, OCCUPIED, DELETED };

    // A single slot storing a key-value pair and its state.
    struct Slot {
        K key;
        V value;
        SlotState state = SlotState::EMPTY;
    };

    // The actual hash table as a vector of slots.
    std::vector<Slot> table;

    // The number of elements currently stored (excluding DELETED).
    size_t count = 0;

    // Threshold at which resizing is triggered.
    float loadFactorThreshold = 0.6f;

    // Computes the hash index for a given key.
    size_t hash(const K& key) const {
        return std::hash<K>{}(key) % table.size();
    }

    // Resizes the table to twice its current size and rehashes all elements.
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
};
