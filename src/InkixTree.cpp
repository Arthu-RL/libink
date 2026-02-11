#include "../include/ink/InkixTree.h"

namespace ink {

InkixTree::InkixTree() : _count(0) {
    _root = std::make_unique<Node>("", false);
}

std::string_view InkixTree::get(std::string_view key)
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
            return "";
        }

        Node* child = it->get();

        // chack child's label match with key: Key="images/logo", Child="images/"
        if (key.substr(0, child->label.size()) != child->label)
        {
            // Mismatch cmp e.g. Key="apple", Child="appish"
            return "";
        }

        // Continue moving down the tree
        key.remove_prefix(child->label.size());
        current = child;
    }

    return current->label;
}

void InkixTree::insert(std::string_view key)
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
            current->children.push_back(std::make_unique<Node>(key, true));
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
            }

            continue;
        }

        // Get the match node part of string
        std::unique_ptr<Node> splitNode = std::make_unique<Node>(child->label.substr(0, common_len), false);

        // let child's label now start from common_len
        child->label = child->label.substr(common_len);

        // move curr child to another pointer
        std::unique_ptr<Node> existingChildPtr = std::move(*it);
        // make now this other pointer parent (which is splitNode) of the child moved
        splitNode->children.push_back(std::move(existingChildPtr));

        if (common_len < key.size())
        {
            // if there is more string to insert, push the new leaf, what lacks from key to be child of splitNode too
            std::unique_ptr<Node> newLeaf = std::make_unique<Node>(key.substr(common_len), true);
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

u32 InkixTree::_get_common_prefix_len(std::string_view a, std::string_view b) const noexcept
{
    size_t len = 0;
    while (len < a.size() && len < b.size() && a[len] == b[len]) {
        len++;
    }
    return len;
}

}
