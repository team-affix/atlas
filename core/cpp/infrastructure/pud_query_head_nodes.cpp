#include "infrastructure/pud_query_head_nodes.hpp"
#include "debug_assert.hpp"

pud_node_id pud_query_head_nodes::get(pud_mhq_head_id head_id) const {
    return nodes_.at(head_id);
}

void pud_query_head_nodes::store(pud_mhq_head_id head_id, pud_node_id node) {
    auto [_, inserted] = nodes_.insert({head_id, node});
    DEBUG_ASSERT(inserted);
}
