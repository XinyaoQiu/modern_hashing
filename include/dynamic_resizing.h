#pragma once

#include "hash_base.h"
#include <optional>
#include <vector>

using KeyType = int;
using ValueType = int;

class DynamicResizing : public HashBase<KeyType, ValueType> {
public:
    explicit DynamicResizing(size_t initialCapacity = 8);

    void insert(const KeyType& key, const ValueType& value) override;
    std::optional<ValueType> lookup(const KeyType& key) const override;
    bool update(const KeyType& key, const ValueType& value) override;
    bool remove(const KeyType& key) override;
    size_t size() const override;
    void clear() override;
    double loadFactor() const override;
    size_t capacity() const override;

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

    size_t hash(KeyType key) const;
    void resize();
};
