#include "infrastructure/pud_children.hpp"
#include "debug_assert.hpp"

const std::vector<const pud_node*>& pud_children::get(const pud_node* node) const {
    return children_links_.at(node);
}

void pud_children::store(const pud_node* node, const std::vector<const pud_node*>& children) {
    auto [_, success] = children_links_.insert({node, children});
    DEBUG_ASSERT(success);
}
