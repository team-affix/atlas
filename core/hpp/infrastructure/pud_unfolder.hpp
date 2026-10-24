#ifndef PUD_UNFOLDER_HPP
#define PUD_UNFOLDER_HPP

#include <utility>
#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_inference.hpp"
#include "debug_assert.hpp"

// Unfolds a leaf on a goal: resolves it against the goal's candidates and stores a child per candidate
// in the leaf's module, then hands the leaf's place on the frontier to them. Every child is activated
// while the leaf still holds its queries, so a query the children share is never let go of in between,
// and none is refuted before all are activated, so the leaf never runs out of live children early. The
// leaf is deactivated last. A live leaf always has a candidate for the goal, so it never unfolds into
// no children.
template<
    typename IResolve,
    typename IStoreInference,
    typename IActivateChild,
    typename IRefuteLeaf,
    typename IDeactivateNode,
    typename ICheckNodeRefuted>
struct pud_unfolder {
    pud_unfolder(
        IResolve& resolve,
        IStoreInference& store_inference,
        IActivateChild& activate_child,
        IRefuteLeaf& refute_leaf,
        IDeactivateNode& deactivate_node,
        ICheckNodeRefuted& check_node_refuted);
    pud_frontier_change unfold(pud_global_node_id leaf, pud_body_goal_id goal);
private:
    IResolve& resolve_;
    IStoreInference& store_inference_;
    IActivateChild& activate_child_;
    IRefuteLeaf& refute_leaf_;
    IDeactivateNode& deactivate_node_;
    ICheckNodeRefuted& check_node_refuted_;
};

#define PUD_UNFOLDER_TEMPLATE                                                              \
    template<typename IResolve, typename IStoreInference, typename IActivateChild,        \
             typename IRefuteLeaf, typename IDeactivateNode, typename ICheckNodeRefuted>
#define PUD_UNFOLDER \
    pud_unfolder<IResolve, IStoreInference, IActivateChild, IRefuteLeaf, IDeactivateNode, ICheckNodeRefuted>

PUD_UNFOLDER_TEMPLATE
PUD_UNFOLDER::pud_unfolder(
    IResolve& resolve,
    IStoreInference& store_inference,
    IActivateChild& activate_child,
    IRefuteLeaf& refute_leaf,
    IDeactivateNode& deactivate_node,
    ICheckNodeRefuted& check_node_refuted)
    : resolve_(resolve)
    , store_inference_(store_inference)
    , activate_child_(activate_child)
    , refute_leaf_(refute_leaf)
    , deactivate_node_(deactivate_node)
    , check_node_refuted_(check_node_refuted) {}

PUD_UNFOLDER_TEMPLATE
pud_frontier_change PUD_UNFOLDER::unfold(pud_global_node_id leaf, pud_body_goal_id goal) {
    std::vector<pud_global_node_id> children;
    for (pud_inference& inference : resolve_.resolve(leaf, goal))
        children.push_back(store_inference_.store_inference(leaf.module, std::move(inference)));
    DEBUG_ASSERT(!children.empty());
    std::vector<pud_global_node_id> arrived_refuted;
    for (pud_global_node_id child : children) {
        if (!activate_child_.activate_child(leaf, child))
            continue;
        arrived_refuted.push_back(child);
    }
    pud_frontier_change change{.offspring = {}, .refuted = {}};
    for (pud_global_node_id child : arrived_refuted) {
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

#undef PUD_UNFOLDER
#undef PUD_UNFOLDER_TEMPLATE

#endif
