#pragma once

#include <memory>
#include <optional>
#include <unordered_map>

/**
 * @brief Trie node structure for fixed-width integer keys
 */
template <typename V>
class Trie {
   private:
    static constexpr size_t BITS_PER_LEVEL = 4;  // branching factor = 16
    static constexpr size_t FANOUT = 1 << BITS_PER_LEVEL;
    static constexpr size_t MASK = FANOUT - 1;

    struct Node {
        std::unordered_map<size_t, std::unique_ptr<Node>> children;
        std::optional<V> value;
    };

    std::unique_ptr<Node> root;
    size_t max_bits;

    Node* getOrCreate(Node* node, size_t key, size_t depth) {
        if (depth == 0) return node;
        size_t idx = (key >> ((depth - 1) * BITS_PER_LEVEL)) & MASK;
        if (!node->children[idx]) {
            node->children[idx] = std::make_unique<Node>();
        }
        return getOrCreate(node->children[idx].get(), key, depth - 1);
    }

    Node* find(Node* node, size_t key, size_t depth) const {
        if (!node) return nullptr;
        if (depth == 0) return node;
        size_t idx = (key >> ((depth - 1) * BITS_PER_LEVEL)) & MASK;
        auto it = node->children.find(idx);
        if (it == node->children.end()) return nullptr;
        return find(it->second.get(), key, depth - 1);
    }

    bool erase(Node* node, size_t key, size_t depth) {
        if (!node) return false;
        if (depth == 0) {
            if (!node->value.has_value()) return false;
            node->value.reset();
            return true;
        }
        size_t idx = (key >> ((depth - 1) * BITS_PER_LEVEL)) & MASK;
        auto it = node->children.find(idx);
        if (it == node->children.end()) return false;
        bool removed = erase(it->second.get(), key, depth - 1);
        if (removed && it->second->children.empty() &&
            !it->second->value.has_value()) {
            node->children.erase(it);
        }
        return removed;
    }

   public:
    explicit Trie(size_t key_bits = 16)
        : root(std::make_unique<Node>()), max_bits(key_bits) {}

    void insert(size_t key, const V& value) {
        getOrCreate(root.get(), key,
                    (max_bits + BITS_PER_LEVEL - 1) / BITS_PER_LEVEL)
            ->value = value;
    }

    std::optional<V> lookup(size_t key) const {
        Node* node = find(root.get(), key,
                          (max_bits + BITS_PER_LEVEL - 1) / BITS_PER_LEVEL);
        if (!node || !node->value.has_value()) return std::nullopt;
        return node->value;
    }

    bool remove(size_t key) {
        return erase(root.get(), key,
                     (max_bits + BITS_PER_LEVEL - 1) / BITS_PER_LEVEL);
    }
};
