#include "infrastructure/pud_root_nodes.hpp"
#include "debug_assert.hpp"

bool pud_root_nodes::check_root(pud_node_id id) const {
    return roots_.contains(id);
}

void pud_root_nodes::mark_root(pud_node_id id) {
    auto [_, inserted] = roots_.insert(id);
    DEBUG_ASSERT(inserted);
}
