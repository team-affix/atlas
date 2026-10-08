#include "infrastructure/pud_refuted_nodes.hpp"
#include "debug_assert.hpp"

bool pud_refuted_nodes::check_refuted(pud_node_id id) const {
    return refuted_.contains(id);
}

void pud_refuted_nodes::set_refuted(pud_node_id id) {
    auto [_, inserted] = refuted_.insert(id);
    DEBUG_ASSERT(inserted);
}
