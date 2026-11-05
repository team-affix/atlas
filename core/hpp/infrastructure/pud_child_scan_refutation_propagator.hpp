#ifndef PUD_CHILD_SCAN_REFUTATION_PROPAGATOR_HPP
#define PUD_CHILD_SCAN_REFUTATION_PROPAGATOR_HPP

#include <optional>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_node_id.hpp"

// Carries a refutation up the stored tree without counting live children: a node is refuted once every one of its
// children is, which its children's refuted flags tell.
template<typename IGetParent, typename IGetChildren, typename IIsNodeRefuted, typename IMarkNodeRefuted>
struct pud_child_scan_refutation_propagator {
    pud_child_scan_refutation_propagator(
        IGetParent& get_parent,
        IGetChildren& get_children,
        IIsNodeRefuted& is_node_refuted,
        IMarkNodeRefuted& mark_node_refuted);
    void refute(pud_global_node_id dead_node);
private:
    IGetParent& get_parent_;
    IGetChildren& get_children_;
    IIsNodeRefuted& is_node_refuted_;
    IMarkNodeRefuted& mark_node_refuted_;
};

#define PUD_CHILD_SCAN_REFUTATION_PROPAGATOR_TEMPLATE                                                    \
    template<typename IGetParent, typename IGetChildren, typename IIsNodeRefuted, typename IMarkNodeRefuted>
#define PUD_CHILD_SCAN_REFUTATION_PROPAGATOR                                                             \
    pud_child_scan_refutation_propagator<IGetParent, IGetChildren, IIsNodeRefuted, IMarkNodeRefuted>

PUD_CHILD_SCAN_REFUTATION_PROPAGATOR_TEMPLATE
PUD_CHILD_SCAN_REFUTATION_PROPAGATOR::pud_child_scan_refutation_propagator(
    IGetParent& get_parent,
    IGetChildren& get_children,
    IIsNodeRefuted& is_node_refuted,
    IMarkNodeRefuted& mark_node_refuted)
    : get_parent_(get_parent)
    , get_children_(get_children)
    , is_node_refuted_(is_node_refuted)
    , mark_node_refuted_(mark_node_refuted) {}

PUD_CHILD_SCAN_REFUTATION_PROPAGATOR_TEMPLATE
void PUD_CHILD_SCAN_REFUTATION_PROPAGATOR::refute(pud_global_node_id dead_node) {
    for (std::optional<pud_global_node_id> parent = get_parent_.get_parent(dead_node); parent.has_value();
         parent = get_parent_.get_parent(parent.value())) {
        bool every_child_refuted = true;
        for (const pud_node_id child : get_children_.get_children(parent.value()))
            every_child_refuted =
                every_child_refuted && is_node_refuted_.is_refuted({.module = parent->module, .node = child});
        if (!every_child_refuted)
            return;
        mark_node_refuted_.mark_refuted(parent.value());
    }
}

#undef PUD_CHILD_SCAN_REFUTATION_PROPAGATOR
#undef PUD_CHILD_SCAN_REFUTATION_PROPAGATOR_TEMPLATE

#endif
