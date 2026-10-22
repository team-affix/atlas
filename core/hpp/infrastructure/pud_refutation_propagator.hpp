#ifndef PUD_REFUTATION_PROPAGATOR_HPP
#define PUD_REFUTATION_PROPAGATOR_HPP

#include "value_objects/pud_node_id.hpp"

// refute() takes a node that has just died. Each ancestor whose last live child dies dies in turn.
template<
    typename ICheckNodeRoot,
    typename IGetNodeParent,
    typename IDecrementLiveChildCount>
struct pud_refutation_propagator {
    pud_refutation_propagator(
        ICheckNodeRoot& check_node_root,
        IGetNodeParent& get_node_parent,
        IDecrementLiveChildCount& decrement_live_child_count);
    void refute(pud_node_id dead_node);
private:
    ICheckNodeRoot& check_node_root_;
    IGetNodeParent& get_node_parent_;
    IDecrementLiveChildCount& decrement_live_child_count_;
};

#define PUD_REFUTATION_PROPAGATOR_TEMPLATE                                                 \
    template<typename ICheckNodeRoot, typename IGetNodeParent, typename IDecrementLiveChildCount>
#define PUD_REFUTATION_PROPAGATOR                                                          \
    pud_refutation_propagator<ICheckNodeRoot, IGetNodeParent, IDecrementLiveChildCount>

PUD_REFUTATION_PROPAGATOR_TEMPLATE
PUD_REFUTATION_PROPAGATOR::pud_refutation_propagator(
    ICheckNodeRoot& check_node_root,
    IGetNodeParent& get_node_parent,
    IDecrementLiveChildCount& decrement_live_child_count)
    : check_node_root_(check_node_root)
    , get_node_parent_(get_node_parent)
    , decrement_live_child_count_(decrement_live_child_count) {}

PUD_REFUTATION_PROPAGATOR_TEMPLATE
void PUD_REFUTATION_PROPAGATOR::refute(pud_node_id dead_node) {
    pud_node_id dead = dead_node;
    while (!check_node_root_.check_root(dead)) {
        dead = get_node_parent_.get(dead);
        if (decrement_live_child_count_.decrement(dead) > 0)
            return;
    }
}

#undef PUD_REFUTATION_PROPAGATOR
#undef PUD_REFUTATION_PROPAGATOR_TEMPLATE

#endif
