#include "infrastructure/pud_children.hpp"
#include "debug_assert.hpp"

pud_children::children_map pud_children::get(const pud_node* node) const {
    return children_links_.at(node);
}

void pud_children::store(const pud_node* node, children_map children) {
    auto [_, success] = children_links_.insert({node, children});
    DEBUG_ASSERT(success);
}
