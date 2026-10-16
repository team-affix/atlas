#ifndef PUD_REFUTATION_PROPAGATOR_HPP
#define PUD_REFUTATION_PROPAGATOR_HPP

#include "value_objects/pud_node_id.hpp"

// refute() takes a live leaf. Each ancestor whose last live child dies is refuted in turn.
template<
    typename IDecrementLiveChildCount,
    typename ICheckNodeRoot,
    typename IGetNodeParent>
struct pud_refutation_propagator {
    pud_refutation_propagator(
        IDecrementLiveChildCount& decrement_live_child_count,
        ICheckNodeRoot& check_node_root,
        IGetNodeParent& get_node_parent);
    void refute(pud_node_id leaf);
private:
    IDecrementLiveChildCount& decrement_live_child_count_;
    ICheckNodeRoot& check_node_root_;
    IGetNodeParent& get_node_parent_;
};

#define PUD_REFUTATION_PROPAGATOR_TEMPLATE                                                 \
    template<typename IDecrementLiveChildCount, typename ICheckNodeRoot, typename IGetNodeParent>
#define PUD_REFUTATION_PROPAGATOR                                                          \
    pud_refutation_propagator<IDecrementLiveChildCount, ICheckNodeRoot, IGetNodeParent>

PUD_REFUTATION_PROPAGATOR_TEMPLATE
PUD_REFUTATION_PROPAGATOR::pud_refutation_propagator(
    IDecrementLiveChildCount& decrement_live_child_count,
    ICheckNodeRoot& check_node_root,
    IGetNodeParent& get_node_parent)
    : decrement_live_child_count_(decrement_live_child_count)
    , check_node_root_(check_node_root)
    , get_node_parent_(get_node_parent) {}

PUD_REFUTATION_PROPAGATOR_TEMPLATE
void PUD_REFUTATION_PROPAGATOR::refute(pud_node_id leaf) {
    pud_node_id refuted = leaf;
    while (decrement_live_child_count_.decrement(refuted) == 0 && !check_node_root_.check_root(refuted))
        refuted = get_node_parent_.get(refuted);
}

#undef PUD_REFUTATION_PROPAGATOR
#undef PUD_REFUTATION_PROPAGATOR_TEMPLATE

#endif
