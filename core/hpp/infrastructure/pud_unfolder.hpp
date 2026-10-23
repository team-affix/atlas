#ifndef PUD_UNFOLDER_HPP
#define PUD_UNFOLDER_HPP

#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "debug_assert.hpp"

// Makes a leaf interior: it records the goal the leaf was unfolded on and the children that came of
// it, every one of them alive. A live leaf always has a candidate for the goal, so it never unfolds
// into no children.
template<typename IStoreCallSite, typename IStoreChildren, typename IUnsetLeaf, typename IStoreLiveChildCount>
struct pud_unfolder {
    pud_unfolder(
        IStoreCallSite& store_call_site,
        IStoreChildren& store_children,
        IUnsetLeaf& unset_leaf,
        IStoreLiveChildCount& store_live_child_count);
    void unfold(pud_node_id leaf, pud_body_goal_id goal, const std::vector<pud_node_id>& children);
private:
    IStoreCallSite& store_call_site_;
    IStoreChildren& store_children_;
    IUnsetLeaf& unset_leaf_;
    IStoreLiveChildCount& store_live_child_count_;
};

#define PUD_UNFOLDER_TEMPLATE                                                              \
    template<typename IStoreCallSite, typename IStoreChildren, typename IUnsetLeaf, typename IStoreLiveChildCount>
#define PUD_UNFOLDER pud_unfolder<IStoreCallSite, IStoreChildren, IUnsetLeaf, IStoreLiveChildCount>

PUD_UNFOLDER_TEMPLATE
PUD_UNFOLDER::pud_unfolder(
    IStoreCallSite& store_call_site,
    IStoreChildren& store_children,
    IUnsetLeaf& unset_leaf,
    IStoreLiveChildCount& store_live_child_count)
    : store_call_site_(store_call_site)
    , store_children_(store_children)
    , unset_leaf_(unset_leaf)
    , store_live_child_count_(store_live_child_count) {}

PUD_UNFOLDER_TEMPLATE
void PUD_UNFOLDER::unfold(pud_node_id leaf, pud_body_goal_id goal, const std::vector<pud_node_id>& children) {
    DEBUG_ASSERT(!children.empty());
    store_call_site_.store(leaf, goal);
    store_children_.store(leaf, children);
    unset_leaf_.unset_leaf(leaf);
    store_live_child_count_.store(leaf, children.size());
}

#undef PUD_UNFOLDER
#undef PUD_UNFOLDER_TEMPLATE

#endif
