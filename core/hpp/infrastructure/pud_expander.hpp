#ifndef PUD_EXPANDER_HPP
#define PUD_EXPANDER_HPP

#include <utility>
#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_node_delta.hpp"
#include "debug_assert.hpp"

// Expands a leaf on a goal: resolves it against the goal's candidates, stores a child per candidate in
// the leaf's module, every one of them alive, activates them, then deactivates the leaf. A live leaf
// always has a candidate for the goal, so it never expands into no children. Every child is activated
// while the leaf still holds its queries, so a query the children share is never let go of in between.
template<
    typename IResolve,
    typename IStoreInference,
    typename IIndexInference,
    typename IStoreLiveChildCount,
    typename IActivateChild,
    typename IRefuteLeaf,
    typename IDeactivateNode,
    typename ICheckNodeRefuted>
struct pud_expander {
    pud_expander(
        IResolve& resolve,
        IStoreInference& store_inference,
        IIndexInference& index_inference,
        IStoreLiveChildCount& store_live_child_count,
        IActivateChild& activate_child,
        IRefuteLeaf& refute_leaf,
        IDeactivateNode& deactivate_node,
        ICheckNodeRefuted& check_node_refuted);
    pud_frontier_change expand(pud_global_node_id leaf, pud_body_goal_id goal);
private:
    IResolve& resolve_;
    IStoreInference& store_inference_;
    IIndexInference& index_inference_;
    IStoreLiveChildCount& store_live_child_count_;
    IActivateChild& activate_child_;
    IRefuteLeaf& refute_leaf_;
    IDeactivateNode& deactivate_node_;
    ICheckNodeRefuted& check_node_refuted_;
};

#define PUD_EXPANDER_TEMPLATE                                                              \
    template<typename IResolve, typename IStoreInference, typename IIndexInference,       \
             typename IStoreLiveChildCount, typename IActivateChild, typename IRefuteLeaf, \
             typename IDeactivateNode, typename ICheckNodeRefuted>
#define PUD_EXPANDER                                                                       \
    pud_expander<IResolve, IStoreInference, IIndexInference, IStoreLiveChildCount, IActivateChild, \
        IRefuteLeaf, IDeactivateNode, ICheckNodeRefuted>

PUD_EXPANDER_TEMPLATE
PUD_EXPANDER::pud_expander(
    IResolve& resolve,
    IStoreInference& store_inference,
    IIndexInference& index_inference,
    IStoreLiveChildCount& store_live_child_count,
    IActivateChild& activate_child,
    IRefuteLeaf& refute_leaf,
    IDeactivateNode& deactivate_node,
    ICheckNodeRefuted& check_node_refuted)
    : resolve_(resolve)
    , store_inference_(store_inference)
    , index_inference_(index_inference)
    , store_live_child_count_(store_live_child_count)
    , activate_child_(activate_child)
    , refute_leaf_(refute_leaf)
    , deactivate_node_(deactivate_node)
    , check_node_refuted_(check_node_refuted) {}

PUD_EXPANDER_TEMPLATE
pud_frontier_change PUD_EXPANDER::expand(pud_global_node_id leaf, pud_body_goal_id goal) {
    std::vector<pud_global_node_id> children;
    for (pud_node_delta& delta : resolve_.resolve(leaf, goal)) {
        const pud_global_node_id child = store_inference_.store_inference(leaf.module, std::move(delta));
        index_inference_.index_inference(child);
        children.push_back(child);
    }
    DEBUG_ASSERT(!children.empty());
    store_live_child_count_.store(leaf, children.size());
    pud_frontier_change change{.offspring = {}, .refuted = {}};
    for (pud_global_node_id child : children) {
        if (!activate_child_.activate_child(leaf, child))
            continue;
        const std::vector<pud_global_node_id> refuted = refute_leaf_.refute(child);
        change.refuted.insert(change.refuted.end(), refuted.begin(), refuted.end());
    }
    for (pud_global_node_id stranded : deactivate_node_.deactivate(leaf)) {
        const std::vector<pud_global_node_id> refuted = refute_leaf_.refute(stranded);
        change.refuted.insert(change.refuted.end(), refuted.begin(), refuted.end());
    }
    for (pud_global_node_id child : children) {
        if (check_node_refuted_.check_refuted(child))
            continue;
        change.offspring.push_back(child);
    }
    return change;
}

#undef PUD_EXPANDER
#undef PUD_EXPANDER_TEMPLATE

#endif
