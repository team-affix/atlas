#ifndef PUD_PLAIN_NODE_DEACTIVATOR_HPP
#define PUD_PLAIN_NODE_DEACTIVATOR_HPP

#include <vector>
#include "value_objects/pud_global_node_id.hpp"

// Deactivates a node that holds only bindings and an lvc as its active fields. Nothing else depends on them, so it
// strands no leaf.
template<typename IEraseNodeBindings, typename IEraseNodeLvc>
struct pud_plain_node_deactivator {
    pud_plain_node_deactivator(IEraseNodeBindings& erase_node_bindings, IEraseNodeLvc& erase_node_lvc);
    std::vector<pud_global_node_id> deactivate(pud_global_node_id node);
private:
    IEraseNodeBindings& erase_node_bindings_;
    IEraseNodeLvc& erase_node_lvc_;
};

template<typename IEraseNodeBindings, typename IEraseNodeLvc>
pud_plain_node_deactivator<IEraseNodeBindings, IEraseNodeLvc>::pud_plain_node_deactivator(
    IEraseNodeBindings& erase_node_bindings, IEraseNodeLvc& erase_node_lvc)
    : erase_node_bindings_(erase_node_bindings)
    , erase_node_lvc_(erase_node_lvc) {}

template<typename IEraseNodeBindings, typename IEraseNodeLvc>
std::vector<pud_global_node_id> pud_plain_node_deactivator<IEraseNodeBindings, IEraseNodeLvc>::deactivate(
    pud_global_node_id node) {
    erase_node_bindings_.erase(node);
    erase_node_lvc_.erase(node);
    return {};
}

#endif
