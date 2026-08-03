#ifndef INKIXTREE_H
#define INKIXTREE_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ink/InkedList.h"
#include "ink/ink_base.hpp"

namespace ink {

template <typename T>
class InkixTree {
public:
    InkixTree() : _root(std::make_unique<Node>()) {}

    // Returns nullptr if key was never inserted.
    T* get(std::string_view key)
    {
        Node* node = _findNode(key);
        return node ? &node->value : nullptr;
    }

    const T* get(std::string_view key) const
    {
        const Node* node = _findNode(key);
        return node ? &node->value : nullptr;
    }

    std::optional<T> getCopy(std::string_view key) const
    {
        const Node* node = _findNode(key);
        if (!node) return std::nullopt;
        return node->value;
    }

    void insert(std::string_view key, T value)
    {
        Node* current = _root.get();

        while (!key.empty())
        {
            // Find match in first caracter at least
            auto it = std::find_if(current->children.begin(), current->children.end(),
                                   [&](const auto& node) {
                                       return !node->label.empty() && node->label[0] == key[0];
                                   });

            // If no match, so push a new brannch
            if (it == current->children.end())
            {
                current->children.push_back(std::make_unique<Node>(key, std::move(value), true));
                return;
            }

            Node* child = it->get();

            // if there is a match, so, let's see how many char matches we have
            u32 common_len = _get_common_prefix_len(child->label, key);

            // if label of childis a perfect match on key, so
            // advance the key view, marking it as terminal if is empty
            if (common_len == child->label.size())
            {
                key.remove_prefix(common_len);

                current = child;

                if (key.empty())
                {
                    child->is_terminal = true;
                    child->value = std::move(value);
                }

                continue;
            }

            // Get the match node part of string
            std::unique_ptr<Node> splitNode = std::make_unique<Node>(child->label.substr(0, common_len), value, false);

            // let child's label now start from common_len
            child->label = child->label.substr(common_len);

            // move curr child to another pointer
            std::unique_ptr<Node> existingChildPtr = std::move(*it);
            // make now this other pointer parent (which is splitNode) of the child moved
            splitNode->children.push_back(std::move(existingChildPtr));

            if (common_len < key.size())
            {
                // if there is more string to insert, push the new leaf, what lacks from key to be child of splitNode too
                std::unique_ptr<Node> newLeaf = std::make_unique<Node>(key.substr(common_len), std::move(value), true);
                splitNode->children.push_back(std::move(newLeaf));
            }
            else
            {
                splitNode->is_terminal = true;
            }

            // let cuur node now be the new splited node and finish the work
            *it = std::move(splitNode);

            return;
        }
    }

private:
    struct Node {
        Node() = default;

        Node(std::string_view _label, const T& _value, bool _is_terminal) :
            label(_label), value(_value), is_terminal(_is_terminal) {}

        Node(std::string_view _label, T&& _value, bool _is_terminal) :
            label(_label), value(std::move(_value)), is_terminal(_is_terminal) {}

        std::string label;
        bool is_terminal = false;
        T value;
        std::vector<std::unique_ptr<Node>> children;
    };

    u32 _get_common_prefix_len(std::string_view a, std::string_view b) const noexcept
    {
        size_t len = 0;
        while (len < a.size() && len < b.size() && a[len] == b[len]) {
            len++;
        }
        return len;
    }

    // Returns nullptr unless `key` was actually inserted as a complete key
    // (i.e. the matched node is terminal)
    Node* _findNode(std::string_view key)
    {
        Node* current = _root.get();

        while (!key.empty())
        {
            auto it = std::find_if(current->children.begin(), current->children.end(),
                                   [&](const auto& node) {
                                       return !node->label.empty() && node->label[0] == key[0];
                                   });

            // No path exists -> key not found
            if (it == current->children.end())
            {
                return nullptr;
            }

            Node* child = it->get();

            // chack child's label match with key: Key="images/logo", Child="images/"
            if (key.substr(0, child->label.size()) != child->label)
            {
                // Mismatch cmp e.g. Key="apple", Child="appish"
                return nullptr;
            }

            // Continue moving down the tree
            key.remove_prefix(child->label.size());
            current = child;
        }

        return current->is_terminal ? current : nullptr;
    }

    const Node* _findNode(std::string_view key) const
    {
        return const_cast<InkixTree*>(this)->_findNode(key);
    }

private:
    std::unique_ptr<Node> _root;
};

}


#endif // INKIXTREE_H
