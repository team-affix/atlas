#include "infrastructure/pud_root_nodes.hpp"
#include "debug_assert.hpp"

bool pud_root_nodes::check_root(pud_node_id id) const {
    return root_set_.contains(id);
}

void pud_root_nodes::mark_root(pud_node_id id) {
    auto [_, inserted] = root_set_.insert(id);
    DEBUG_ASSERT(inserted);
    roots_.push_back(id);
}

const std::vector<pud_node_id>& pud_root_nodes::get() const {
    return roots_;
}
