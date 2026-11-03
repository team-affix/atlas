#include "infrastructure/pud_node_depth_store.hpp"

pud_node_depth_store::pud_node_depth_store()
    : depths_() {}

void pud_node_depth_store::insert(pud_global_node_id node, size_t depth) {
    depths_.insert_or_assign(node, depth);
}

size_t pud_node_depth_store::get_depth(pud_global_node_id node) const {
    return depths_.at(node);
}
