#ifndef PUD_LEAF_ACTIVATOR_HPP
#define PUD_LEAF_ACTIVATOR_HPP

#include <utility>
#include "value_objects/pud_node_id.hpp"

// Gives a leaf its fresh descent: a leaf that arrives with a refuted goal is refuted, and one that
// still has open goals becomes open.
template<typename Descent, typename IStoreNodeDescent, typename IMarkLeafOpen, typename IRefuteNode>
struct pud_leaf_activator {
    pud_leaf_activator(
        IStoreNodeDescent& store_node_descent,
        IMarkLeafOpen& mark_leaf_open,
        IRefuteNode& refute_node);
    void activate(pud_node_id leaf, Descent descent);
private:
    IStoreNodeDescent& store_node_descent_;
    IMarkLeafOpen& mark_leaf_open_;
    IRefuteNode& refute_node_;
};

template<typename D, typename ISND, typename IMLO, typename IRN>
pud_leaf_activator<D, ISND, IMLO, IRN>::pud_leaf_activator(
    ISND& store_node_descent,
    IMLO& mark_leaf_open,
    IRN& refute_node)
    : store_node_descent_(store_node_descent)
    , mark_leaf_open_(mark_leaf_open)
    , refute_node_(refute_node) {}

template<typename D, typename ISND, typename IMLO, typename IRN>
void pud_leaf_activator<D, ISND, IMLO, IRN>::activate(pud_node_id leaf, D descent) {
    const bool goal_refuted = !descent.body_goal_pq().refuted_goals().empty();
    const bool goals_open = !descent.body_goal_expr().body_goal_exprs().empty();
    store_node_descent_.store(leaf, std::move(descent));
    if (goal_refuted) {
        refute_node_.refute(leaf);
        return;
    }
    if (!goals_open)
        return;
    mark_leaf_open_.mark_leaf_open(leaf);
}

#endif
