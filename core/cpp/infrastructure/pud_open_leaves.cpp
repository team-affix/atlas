#include "infrastructure/pud_open_leaves.hpp"
#include "debug_assert.hpp"

bool pud_open_leaves::check_leaf_open(pud_node_id id) const {
    return open_leaves_.contains(id);
}

void pud_open_leaves::mark_leaf_open(pud_node_id id) {
    auto [_, inserted] = open_leaves_.insert(id);
    DEBUG_ASSERT(inserted);
}

void pud_open_leaves::mark_leaf_closed(pud_node_id id) {
    const size_t erased = open_leaves_.erase(id);
    DEBUG_ASSERT(erased == 1);
}

const std::set<pud_node_id>& pud_open_leaves::get() const {
    return open_leaves_;
}
