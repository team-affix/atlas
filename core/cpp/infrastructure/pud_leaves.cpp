#include "infrastructure/pud_leaves.hpp"
#include "debug_assert.hpp"

bool pud_leaves::check_leaf(const pud_node* node) const {
    return leaves_.contains(node);
}

void pud_leaves::set_leaf(const pud_node* node) {
    auto [_, inserted] = leaves_.insert(node);
    DEBUG_ASSERT(inserted);
}

void pud_leaves::unset_leaf(const pud_node* node) {
    const size_t erased = leaves_.erase(node);
    DEBUG_ASSERT(erased == 1);
}
