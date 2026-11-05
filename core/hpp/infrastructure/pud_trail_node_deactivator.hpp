#ifndef PUD_TRAIL_NODE_DEACTIVATOR_HPP
#define PUD_TRAIL_NODE_DEACTIVATOR_HPP

#include <vector>
#include "value_objects/pud_global_node_id.hpp"

// Deactivates a node whose active fields are its trails, structure watch, bindings and lvc. Nothing else depends on
// them, so it strands no leaf.
template<typename IEraseNodeTrails, typename IEraseNodeWatch, typename IEraseNodeBindings, typename IEraseNodeLvc>
struct pud_trail_node_deactivator {
    pud_trail_node_deactivator(
        IEraseNodeTrails& erase_node_trails,
        IEraseNodeWatch& erase_node_watch,
        IEraseNodeBindings& erase_node_bindings,
        IEraseNodeLvc& erase_node_lvc);
    std::vector<pud_global_node_id> deactivate(pud_global_node_id node);
private:
    IEraseNodeTrails& erase_node_trails_;
    IEraseNodeWatch& erase_node_watch_;
    IEraseNodeBindings& erase_node_bindings_;
    IEraseNodeLvc& erase_node_lvc_;
};

#define PUD_TRAIL_NODE_DEACTIVATOR_TEMPLATE                                                              \
    template<typename IEraseNodeTrails, typename IEraseNodeWatch, typename IEraseNodeBindings,           \
             typename IEraseNodeLvc>
#define PUD_TRAIL_NODE_DEACTIVATOR                                                                       \
    pud_trail_node_deactivator<IEraseNodeTrails, IEraseNodeWatch, IEraseNodeBindings, IEraseNodeLvc>

PUD_TRAIL_NODE_DEACTIVATOR_TEMPLATE
PUD_TRAIL_NODE_DEACTIVATOR::pud_trail_node_deactivator(
    IEraseNodeTrails& erase_node_trails,
    IEraseNodeWatch& erase_node_watch,
    IEraseNodeBindings& erase_node_bindings,
    IEraseNodeLvc& erase_node_lvc)
    : erase_node_trails_(erase_node_trails)
    , erase_node_watch_(erase_node_watch)
    , erase_node_bindings_(erase_node_bindings)
    , erase_node_lvc_(erase_node_lvc) {}

PUD_TRAIL_NODE_DEACTIVATOR_TEMPLATE
std::vector<pud_global_node_id> PUD_TRAIL_NODE_DEACTIVATOR::deactivate(pud_global_node_id node) {
    erase_node_trails_.erase(node);
    erase_node_watch_.erase(node);
    erase_node_bindings_.erase(node);
    erase_node_lvc_.erase(node);
    return {};
}

#undef PUD_TRAIL_NODE_DEACTIVATOR
#undef PUD_TRAIL_NODE_DEACTIVATOR_TEMPLATE

#endif
