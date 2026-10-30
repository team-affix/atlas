#include "infrastructure/pud_lodestar_focus_set.hpp"

pud_lodestar_focus_set::pud_lodestar_focus_set()
    : nodes_() {}

void pud_lodestar_focus_set::insert(pud_global_node_id node) {
    nodes_.insert(node);
}

bool pud_lodestar_focus_set::contains(pud_global_node_id node) const {
    return nodes_.contains(node);
}
