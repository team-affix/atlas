#ifndef PUD_REFUTATION_PROPAGATOR_HPP
#define PUD_REFUTATION_PROPAGATOR_HPP

#include <optional>
#include "value_objects/pud_global_node_id.hpp"

// refute() takes a node that has just died. Each ancestor whose last live child dies is marked refuted
// and dies in turn.
template<typename IGetNodeParent, typename IDecrementLiveChildCount, typename IMarkNodeRefuted>
struct pud_refutation_propagator {
    pud_refutation_propagator(
        IGetNodeParent& get_node_parent,
        IDecrementLiveChildCount& decrement_live_child_count,
        IMarkNodeRefuted& mark_node_refuted);
    void refute(pud_global_node_id dead_node);
private:
    IGetNodeParent& get_node_parent_;
    IDecrementLiveChildCount& decrement_live_child_count_;
    IMarkNodeRefuted& mark_node_refuted_;
};

template<typename IGNP, typename IDLCC, typename IMNR>
pud_refutation_propagator<IGNP, IDLCC, IMNR>::pud_refutation_propagator(
    IGNP& get_node_parent, IDLCC& decrement_live_child_count, IMNR& mark_node_refuted)
    : get_node_parent_(get_node_parent)
    , decrement_live_child_count_(decrement_live_child_count)
    , mark_node_refuted_(mark_node_refuted) {}

template<typename IGNP, typename IDLCC, typename IMNR>
void pud_refutation_propagator<IGNP, IDLCC, IMNR>::refute(pud_global_node_id dead_node) {
    pud_global_node_id dead = dead_node;
    while (const std::optional<pud_global_node_id> parent = get_node_parent_.get_parent(dead)) {
        dead = parent.value();
        if (decrement_live_child_count_.decrement(dead) > 0)
            return;
        mark_node_refuted_.mark_refuted(dead);
    }
}

#endif
