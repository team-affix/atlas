#include "infrastructure/pud_leaf_nodes.hpp"
#include "debug_assert.hpp"

bool pud_leaf_nodes::check_leaf(pud_node_id id) const {
    return leaves_.contains(id);
}

void pud_leaf_nodes::set_leaf(pud_node_id id) {
    auto [_, inserted] = leaves_.insert(id);
    DEBUG_ASSERT(inserted);
}

void pud_leaf_nodes::unset_leaf(pud_node_id id) {
    const size_t erased = leaves_.erase(id);
    DEBUG_ASSERT(erased == 1);
}
