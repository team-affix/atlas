#include "infrastructure/pud_leaves.hpp"
#include "debug_assert.hpp"

bool pud_leaves::check_leaf(pud_node_id id) const {
    return leaves_.contains(id);
}

void pud_leaves::set_leaf(pud_node_id id) {
    auto [_, inserted] = leaves_.insert(id);
    DEBUG_ASSERT(inserted);
}

void pud_leaves::unset_leaf(pud_node_id id) {
    const size_t erased = leaves_.erase(id);
    DEBUG_ASSERT(erased == 1);
}
