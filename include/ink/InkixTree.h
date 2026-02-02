#ifndef INKIXTREE_H
#define INKIXTREE_H

#include <vector>
#include <string>

#include "ink/InkedList.h"
#include "ink/ink_base.hpp"

namespace ink {

static int inkixNodeSize = 50;

struct InkixNode {
    std::string_view label;
    bool is_terminal = false;
    std::vector<InkixNode> children;
};

class InkixTree {
public:
    void insert(std::string_view key);
    void remove();

private:
    u32 _get_common_prefix_len(std::string_view a, std::string_view b) const noexcept;

private:
    InkixNode* _root;
    u32 _count;
};

}


#endif // INKIXTREE_H
