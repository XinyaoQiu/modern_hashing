#pragma once

#include "hash_base.h"
#include <optional>
#include <vector>

using KeyType = int;
using ValueType = int;

/**
 * @brief A simple open-addressing hash table with dynamic resizing.
 *
 * This hash table uses linear probing for collision resolution and
 * automatically doubles in size when the load factor exceeds a threshold.
 * Deleted entries are marked and skipped during probing.
 */
class DynamicResizing : public HashBase<KeyType, ValueType> {
public:
    /**
     * @brief Constructs the hash table with the given initial capacity.
     * @param initialCapacity Initial number of slots in the table.
     */
    explicit DynamicResizing(size_t initialCapacity = 8);

    /**
     * @brief Inserts or updates a key-value pair.
     * @param key The key to insert.
     * @param value The value associated with the key.
     */
    void insert(const KeyType& key, const ValueType& value) override;

    /**
     * @brief Looks up the value associated with a key.
     * @param key The key to find.
     * @return The value if the key is found, otherwise std::nullopt.
     */
    std::optional<ValueType> lookup(const KeyType& key) const override;

    /**
     * @brief Updates an existing key with a new value.
     * @param key The key to update.
     * @param value The new value.
     * @return True if the key existed and was updated, false otherwise.
     */
    bool update(const KeyType& key, const ValueType& value) override;

    /**
     * @brief Removes a key-value pair.
     * @param key The key to remove.
     * @return True if the key was found and removed, false otherwise.
     */
    bool remove(const KeyType& key) override;

    /**
     * @brief Returns the number of active elements in the table.
     */
    size_t size() const override;

    /**
     * @brief Clears the hash table.
     */
    void clear() override;

    /**
     * @brief Returns the current load factor of the table.
     */
    double loadFactor() const override;

    /**
     * @brief Returns the current capacity of the table (number of slots).
     */
    size_t capacity() const override;

private:
    /// State of a single slot: EMPTY, OCCUPIED, or DELETED.
    enum class SlotState { EMPTY, OCCUPIED, DELETED };

    /// A single slot storing a key-value pair and its state.
    struct Slot {
        KeyType key;
        ValueType value;
        SlotState state = SlotState::EMPTY;
    };

    /// The actual hash table as a vector of slots.
    std::vector<Slot> table;

    /// The number of elements currently stored (excluding DELETED).
    size_t count = 0;

    /// Threshold at which resizing is triggered.
    float loadFactorThreshold = 0.6f;

    /// Computes the hash index for a given key.
    size_t hash(KeyType key) const;

    /// Resizes the table to twice its current size and rehashes all elements.
    void resize();
};
