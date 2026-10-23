#ifndef PUD_EXPANDER_HPP
#define PUD_EXPANDER_HPP

#include <set>
#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_child_delta.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"

// Expands a leaf on a goal: resolves it against the goal's candidates, unfolds it into the children,
// activates them, then withdraws the leaf. Children are activated while the leaf still holds its
// queries, so a query a child shares is never let go of in between.
template<
    typename IResolve,
    typename IUnfold,
    typename IActivateChild,
    typename IRefuteLeaf,
    typename IDeactivateNode,
    typename IGetQueryHolders,
    typename ICollectChange>
struct pud_expander {
    pud_expander(
        IResolve& resolve,
        IUnfold& unfold,
        IActivateChild& activate_child,
        IRefuteLeaf& refute_leaf,
        IDeactivateNode& deactivate_node,
        IGetQueryHolders& get_query_holders,
        ICollectChange& collect_change);
    pud_frontier_change expand(pud_node_id leaf, pud_body_goal_id goal);
private:
    IResolve& resolve_;
    IUnfold& unfold_;
    IActivateChild& activate_child_;
    IRefuteLeaf& refute_leaf_;
    IDeactivateNode& deactivate_node_;
    IGetQueryHolders& get_query_holders_;
    ICollectChange& collect_change_;
};

template<typename IR, typename IU, typename IAC, typename IRL, typename IDN, typename IGQH, typename ICC>
pud_expander<IR, IU, IAC, IRL, IDN, IGQH, ICC>::pud_expander(
    IR& resolve,
    IU& unfold,
    IAC& activate_child,
    IRL& refute_leaf,
    IDN& deactivate_node,
    IGQH& get_query_holders,
    ICC& collect_change)
    : resolve_(resolve)
    , unfold_(unfold)
    , activate_child_(activate_child)
    , refute_leaf_(refute_leaf)
    , deactivate_node_(deactivate_node)
    , get_query_holders_(get_query_holders)
    , collect_change_(collect_change) {}

template<typename IR, typename IU, typename IAC, typename IRL, typename IDN, typename IGQH, typename ICC>
pud_frontier_change pud_expander<IR, IU, IAC, IRL, IDN, IGQH, ICC>::expand(pud_node_id leaf, pud_body_goal_id goal) {
    unfold_.begin_unfold(leaf, goal);
    std::vector<pud_node_id> children;
    for (const pud_child_delta& delta : resolve_.resolve(leaf, goal))
        children.push_back(unfold_.unfold_child(leaf, goal, delta));
    unfold_.finish_unfold(leaf, children);
    std::vector<pud_node_id> refuted_children;
    for (pud_node_id child : children) {
        if (!activate_child_.activate_child(leaf, child))
            continue;
        refuted_children.push_back(child);
    }
    std::vector<pud_node_id> all_refuted;
    std::vector<pud_node_id> releasing;
    std::vector<pud_node_id> refuting = refuted_children;
    releasing.push_back(leaf);
    while (!releasing.empty() || !refuting.empty()) {
        for (pud_node_id node : refuting) {
            refute_leaf_.refute(node);
            all_refuted.push_back(node);
            releasing.push_back(node);
        }
        refuting.clear();
        const std::vector<pud_node_id> releasable = std::move(releasing);
        releasing.clear();
        std::set<pud_node_id> next_refuting;
        for (pud_node_id node : releasable) {
            for (pud_mhq_head_id head_id : deactivate_node_.deactivate(node)) {
                const std::vector<pud_node_id> held_by = get_query_holders_.get_holders(head_id);
                next_refuting.insert(held_by.begin(), held_by.end());
            }
        }
        refuting.assign(next_refuting.begin(), next_refuting.end());
    }
    for (pud_node_id child : children)
        collect_change_.add_produced(child);
    for (pud_node_id refuted : all_refuted)
        collect_change_.add_refuted(refuted);
    return collect_change_.finish();
}

#endif
