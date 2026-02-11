#ifndef INKIXTREE_H
#define INKIXTREE_H

#include <vector>
#include <string>
#include <memory>

#include "ink/InkedList.h"
#include "ink/ink_base.hpp"

namespace ink {

class InkixTree {
public:
    InkixTree();

    std::string_view get(std::string_view key);
    void insert(std::string_view key);
    void remove();

private:
    struct Node {
        Node(std::string_view _label, bool _is_terminal) :
            label(_label), is_terminal(_is_terminal) {}

        std::string_view label;
        bool is_terminal = false;
        std::vector<std::unique_ptr<Node>> children;
    };

    u32 _get_common_prefix_len(std::string_view a, std::string_view b) const noexcept;

private:
    std::unique_ptr<Node> _root;
    u32 _count;
};

}


#endif // INKIXTREE_H
