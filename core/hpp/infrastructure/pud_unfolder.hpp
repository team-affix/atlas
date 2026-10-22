#ifndef PUD_UNFOLDER_HPP
#define PUD_UNFOLDER_HPP

#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_child_delta.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_node_id.hpp"

template<
    typename IGetNodeDescent,
    typename IAcceptCandidate,
    typename IMakeChild,
    typename IStoreCallSite,
    typename IStoreChildren,
    typename IUnsetLeaf>
struct pud_unfolder {
    pud_unfolder(
        IGetNodeDescent& get_node_descent,
        IAcceptCandidate& accept_candidate,
        IMakeChild& make_child,
        IStoreCallSite& store_call_site,
        IStoreChildren& store_children,
        IUnsetLeaf& unset_leaf);
    std::vector<pud_node_id> unfold(
        pud_node_id leaf,
        pud_body_goal_id goal,
        const std::vector<std::vector<pud_node_id>>& callees);
private:
    IGetNodeDescent& get_node_descent_;
    IAcceptCandidate& accept_candidate_;
    IMakeChild& make_child_;
    IStoreCallSite& store_call_site_;
    IStoreChildren& store_children_;
    IUnsetLeaf& unset_leaf_;
};

#define PUD_UNFOLDER_TEMPLATE                                                                 \
    template<typename IGetNodeDescent, typename IAcceptCandidate, typename IMakeChild,       \
             typename IStoreCallSite, typename IStoreChildren, typename IUnsetLeaf>
#define PUD_UNFOLDER                                                                          \
    pud_unfolder<IGetNodeDescent, IAcceptCandidate, IMakeChild, IStoreCallSite, IStoreChildren, IUnsetLeaf>

PUD_UNFOLDER_TEMPLATE
PUD_UNFOLDER::pud_unfolder(
    IGetNodeDescent& get_node_descent,
    IAcceptCandidate& accept_candidate,
    IMakeChild& make_child,
    IStoreCallSite& store_call_site,
    IStoreChildren& store_children,
    IUnsetLeaf& unset_leaf)
    : get_node_descent_(get_node_descent)
    , accept_candidate_(accept_candidate)
    , make_child_(make_child)
    , store_call_site_(store_call_site)
    , store_children_(store_children)
    , unset_leaf_(unset_leaf) {}

PUD_UNFOLDER_TEMPLATE
std::vector<pud_node_id> PUD_UNFOLDER::unfold(
    pud_node_id leaf,
    pud_body_goal_id goal,
    const std::vector<std::vector<pud_node_id>>& callees) {
    const auto& leaf_descent = get_node_descent_.get(leaf);
    const expr* goal_expr = leaf_descent.body_goal_expr().body_goal_exprs().at(goal);
    std::vector<pud_node_id> children;
    for (const std::vector<pud_node_id>& callee : callees) {
        const pud_child_delta delta = accept_candidate_.accept(
            leaf_descent.binding().ubs_state(),
            leaf_descent.lvc().lvc(),
            goal_expr,
            callee);
        children.push_back(make_child_.make(leaf, delta));
    }
    store_call_site_.store(leaf, goal);
    store_children_.store(leaf, children);
    unset_leaf_.unset_leaf(leaf);
    return children;
}

#undef PUD_UNFOLDER
#undef PUD_UNFOLDER_TEMPLATE

#endif
