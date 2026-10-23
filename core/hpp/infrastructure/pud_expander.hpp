#ifndef PUD_EXPANDER_HPP
#define PUD_EXPANDER_HPP

#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_node_id.hpp"

// Expands a leaf on a goal: resolves it against the goal's candidates, unfolds it into the children,
// activates them, then retires the leaf. Children are activated while the leaf still holds its
// queries, so a query a child shares is never let go of in between.
template<typename IResolve, typename IUnfold, typename IActivateChild, typename IRetire, typename ICollectChange>
struct pud_expander {
    pud_expander(
        IResolve& resolve,
        IUnfold& unfold,
        IActivateChild& activate_child,
        IRetire& retire,
        ICollectChange& collect_change);
    pud_frontier_change expand(pud_node_id leaf, pud_body_goal_id goal);
private:
    IResolve& resolve_;
    IUnfold& unfold_;
    IActivateChild& activate_child_;
    IRetire& retire_;
    ICollectChange& collect_change_;
};

template<typename IR, typename IU, typename IAC, typename IRet, typename ICC>
pud_expander<IR, IU, IAC, IRet, ICC>::pud_expander(
    IR& resolve,
    IU& unfold,
    IAC& activate_child,
    IRet& retire,
    ICC& collect_change)
    : resolve_(resolve)
    , unfold_(unfold)
    , activate_child_(activate_child)
    , retire_(retire)
    , collect_change_(collect_change) {}

template<typename IR, typename IU, typename IAC, typename IRet, typename ICC>
pud_frontier_change pud_expander<IR, IU, IAC, IRet, ICC>::expand(pud_node_id leaf, pud_body_goal_id goal) {
    const std::vector<pud_node_id> children = unfold_.unfold(leaf, goal, resolve_.resolve(leaf, goal));
    std::vector<pud_node_id> refuted_children;
    for (pud_node_id child : children) {
        if (!activate_child_.activate_child(leaf, child))
            continue;
        refuted_children.push_back(child);
    }
    return collect_change_.collect(children, retire_.retire({leaf}, refuted_children));
}

#endif
