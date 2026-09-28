#include "infrastructure/pud_parents.hpp"
#include "debug_assert.hpp"

const pud_node* pud_parents::get(const pud_node* node) const {
    return parent_links_.at(node);
}

void pud_parents::store(const pud_node* node, const pud_node* parent) {
    auto [_, success] = parent_links_.insert({node, parent});
    DEBUG_ASSERT(success);
}
