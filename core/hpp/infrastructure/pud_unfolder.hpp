#ifndef PUD_UNFOLDER_HPP
#define PUD_UNFOLDER_HPP

#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_child_delta.hpp"
#include "value_objects/pud_node_id.hpp"

// Unfolds a leaf on a goal into one child per delta. The leaf becomes interior; with no children it
// is dead, and so is every ancestor it leaves without a live child.
template<
    typename IStoreCallSite,
    typename IMakeChild,
    typename IStoreChildren,
    typename IUnsetLeaf,
    typename IStoreLiveChildCount,
    typename IPropagateRefutation>
struct pud_unfolder {
    pud_unfolder(
        IStoreCallSite& store_call_site,
        IMakeChild& make_child,
        IStoreChildren& store_children,
        IUnsetLeaf& unset_leaf,
        IStoreLiveChildCount& store_live_child_count,
        IPropagateRefutation& propagate_refutation);
    std::vector<pud_node_id> unfold(pud_node_id leaf, pud_body_goal_id goal, const std::vector<pud_child_delta>& deltas);
private:
    IStoreCallSite& store_call_site_;
    IMakeChild& make_child_;
    IStoreChildren& store_children_;
    IUnsetLeaf& unset_leaf_;
    IStoreLiveChildCount& store_live_child_count_;
    IPropagateRefutation& propagate_refutation_;
};

#define PUD_UNFOLDER_TEMPLATE                                                              \
    template<typename IStoreCallSite, typename IMakeChild, typename IStoreChildren,        \
             typename IUnsetLeaf, typename IStoreLiveChildCount, typename IPropagateRefutation>
#define PUD_UNFOLDER                                                                       \
    pud_unfolder<IStoreCallSite, IMakeChild, IStoreChildren, IUnsetLeaf,               \
        IStoreLiveChildCount, IPropagateRefutation>

PUD_UNFOLDER_TEMPLATE
PUD_UNFOLDER::pud_unfolder(
    IStoreCallSite& store_call_site,
    IMakeChild& make_child,
    IStoreChildren& store_children,
    IUnsetLeaf& unset_leaf,
    IStoreLiveChildCount& store_live_child_count,
    IPropagateRefutation& propagate_refutation)
    : store_call_site_(store_call_site)
    , make_child_(make_child)
    , store_children_(store_children)
    , unset_leaf_(unset_leaf)
    , store_live_child_count_(store_live_child_count)
    , propagate_refutation_(propagate_refutation) {}

PUD_UNFOLDER_TEMPLATE
std::vector<pud_node_id> PUD_UNFOLDER::unfold(
    pud_node_id leaf, pud_body_goal_id goal, const std::vector<pud_child_delta>& deltas) {
    store_call_site_.store(leaf, goal);
    std::vector<pud_node_id> children;
    for (const pud_child_delta& delta : deltas)
        children.push_back(make_child_.make_child(leaf, goal, delta));
    store_children_.store(leaf, children);
    unset_leaf_.unset_leaf(leaf);
    store_live_child_count_.store(leaf, children.size());
    if (children.empty())
        propagate_refutation_.refute(leaf);
    return children;
}

#undef PUD_UNFOLDER
#undef PUD_UNFOLDER_TEMPLATE

#endif
