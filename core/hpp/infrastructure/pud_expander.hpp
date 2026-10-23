#ifndef PUD_EXPANDER_HPP
#define PUD_EXPANDER_HPP

#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_node_delta.hpp"
#include "value_objects/pud_node_id.hpp"
#include "debug_assert.hpp"

// Expands a leaf on a goal: resolves it against the goal's candidates, makes a child per candidate,
// every one of them alive, activates them, then deactivates the leaf. A live leaf always has a
// candidate for the goal, so it never expands into no children. Every child is activated while the
// leaf still holds its queries, so a query the children share is never let go of in between.
template<
    typename IResolve,
    typename IMakeChild,
    typename IStoreLiveChildCount,
    typename IActivateChild,
    typename IRefuteLeaf,
    typename IDeactivateNode,
    typename ICheckLeafRefuted>
struct pud_expander {
    pud_expander(
        IResolve& resolve,
        IMakeChild& make_child,
        IStoreLiveChildCount& store_live_child_count,
        IActivateChild& activate_child,
        IRefuteLeaf& refute_leaf,
        IDeactivateNode& deactivate_node,
        ICheckLeafRefuted& check_leaf_refuted);
    pud_frontier_change expand(pud_node_id leaf, pud_body_goal_id goal);
private:
    IResolve& resolve_;
    IMakeChild& make_child_;
    IStoreLiveChildCount& store_live_child_count_;
    IActivateChild& activate_child_;
    IRefuteLeaf& refute_leaf_;
    IDeactivateNode& deactivate_node_;
    ICheckLeafRefuted& check_leaf_refuted_;
};

#define PUD_EXPANDER_TEMPLATE                                                              \
    template<typename IResolve, typename IMakeChild, typename IStoreLiveChildCount, typename IActivateChild, \
             typename IRefuteLeaf, typename IDeactivateNode, typename ICheckLeafRefuted>
#define PUD_EXPANDER                                                                       \
    pud_expander<IResolve, IMakeChild, IStoreLiveChildCount, IActivateChild, IRefuteLeaf, IDeactivateNode, \
        ICheckLeafRefuted>

PUD_EXPANDER_TEMPLATE
PUD_EXPANDER::pud_expander(
    IResolve& resolve,
    IMakeChild& make_child,
    IStoreLiveChildCount& store_live_child_count,
    IActivateChild& activate_child,
    IRefuteLeaf& refute_leaf,
    IDeactivateNode& deactivate_node,
    ICheckLeafRefuted& check_leaf_refuted)
    : resolve_(resolve)
    , make_child_(make_child)
    , store_live_child_count_(store_live_child_count)
    , activate_child_(activate_child)
    , refute_leaf_(refute_leaf)
    , deactivate_node_(deactivate_node)
    , check_leaf_refuted_(check_leaf_refuted) {}

PUD_EXPANDER_TEMPLATE
pud_frontier_change PUD_EXPANDER::expand(pud_node_id leaf, pud_body_goal_id goal) {
    std::vector<pud_node_id> children;
    for (const pud_node_delta& delta : resolve_.resolve(leaf, goal))
        children.push_back(make_child_.make_child(delta));
    DEBUG_ASSERT(!children.empty());
    store_live_child_count_.store(leaf, children.size());
    pud_frontier_change change{.offspring = {}, .refuted = {}};
    for (pud_node_id child : children) {
        if (!activate_child_.activate_child(leaf, child))
            continue;
        const std::vector<pud_node_id> refuted = refute_leaf_.refute(child);
        change.refuted.insert(change.refuted.end(), refuted.begin(), refuted.end());
    }
    for (pud_node_id stranded : deactivate_node_.deactivate(leaf)) {
        const std::vector<pud_node_id> refuted = refute_leaf_.refute(stranded);
        change.refuted.insert(change.refuted.end(), refuted.begin(), refuted.end());
    }
    for (pud_node_id child : children) {
        if (check_leaf_refuted_.check_refuted(child))
            continue;
        change.offspring.push_back(child);
    }
    return change;
}

#undef PUD_EXPANDER
#undef PUD_EXPANDER_TEMPLATE

#endif
