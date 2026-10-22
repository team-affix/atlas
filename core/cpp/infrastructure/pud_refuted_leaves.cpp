#include "infrastructure/pud_refuted_leaves.hpp"
#include "debug_assert.hpp"

bool pud_refuted_leaves::check_refuted(pud_node_id id) const {
    return refuted_.contains(id);
}

void pud_refuted_leaves::mark_refuted(pud_node_id id) {
    auto [_, inserted] = refuted_.insert(id);
    DEBUG_ASSERT(inserted);
}
