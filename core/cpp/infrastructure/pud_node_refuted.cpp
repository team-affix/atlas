#include "infrastructure/pud_node_refuted.hpp"
#include "debug_assert.hpp"

void pud_node_refuted::store(const pud_node* node, bool refuted) {
    auto [_, success] = node_refuted_.insert({node, refuted});
    DEBUG_ASSERT(success);
}

bool pud_node_refuted::get(const pud_node* node) const {
    return node_refuted_.at(node);
}
