#include "infrastructure/pud_node_axioms.hpp"

void pud_node_axioms::store(pud_global_node_id node, pud_global_node_id axiom) {
    axioms_.store(node, axiom);
}

pud_global_node_id pud_node_axioms::get(pud_global_node_id node) const {
    return axioms_.get(node);
}
