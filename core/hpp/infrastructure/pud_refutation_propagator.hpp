#ifndef PUD_REFUTATION_PROPAGATOR_HPP
#define PUD_REFUTATION_PROPAGATOR_HPP

#include <optional>
#include "value_objects/pud_node_id.hpp"

// refute() takes a node that has just died. Each ancestor whose last live child dies dies in turn.
template<typename IGetNodeParent, typename IDecrementLiveChildCount>
struct pud_refutation_propagator {
    pud_refutation_propagator(IGetNodeParent& get_node_parent, IDecrementLiveChildCount& decrement_live_child_count);
    void refute(pud_node_id dead_node);
private:
    IGetNodeParent& get_node_parent_;
    IDecrementLiveChildCount& decrement_live_child_count_;
};

template<typename IGNP, typename IDLCC>
pud_refutation_propagator<IGNP, IDLCC>::pud_refutation_propagator(
    IGNP& get_node_parent, IDLCC& decrement_live_child_count)
    : get_node_parent_(get_node_parent)
    , decrement_live_child_count_(decrement_live_child_count) {}

template<typename IGNP, typename IDLCC>
void pud_refutation_propagator<IGNP, IDLCC>::refute(pud_node_id dead_node) {
    pud_node_id dead = dead_node;
    while (const std::optional<pud_node_id> parent = get_node_parent_.get_parent(dead)) {
        dead = parent.value();
        if (decrement_live_child_count_.decrement(dead) > 0)
            return;
    }
}

#endif
