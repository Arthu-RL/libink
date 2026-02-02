#include "../include/ink/InkixTree.h"

namespace ink {

void InkixTree::insert(std::string_view key)
{
    // Scenario A: The "New Branch"
    if (_root->label[0] == key[0])
    {

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
