#ifndef PUD_NODE_AXIOMS_HPP
#define PUD_NODE_AXIOMS_HPP

#include "infrastructure/pud_node_table.hpp"
#include "value_objects/pud_global_node_id.hpp"

// The root axiom of every node, stored once when the node is activated; an axiom is its own root.
struct pud_node_axioms {
    void store(pud_global_node_id node, pud_global_node_id axiom);
    pud_global_node_id get(pud_global_node_id node) const;
private:
    pud_node_table<pud_global_node_id> axioms_;
};

#endif
