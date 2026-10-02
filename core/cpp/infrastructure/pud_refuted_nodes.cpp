#include "infrastructure/pud_refuted_nodes.hpp"
#include "debug_assert.hpp"

bool pud_refuted_nodes::check_refuted(const pud_node* node) const {
    return refuted_.contains(node);
}

void pud_refuted_nodes::set_refuted(const pud_node* node) {
    auto [_, inserted] = refuted_.insert(node);
    DEBUG_ASSERT(inserted);
}
