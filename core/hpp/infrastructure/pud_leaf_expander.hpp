#ifndef PUD_LEAF_EXPANDER_HPP
#define PUD_LEAF_EXPANDER_HPP

#include <algorithm>
#include <optional>
#include <variant>
#include <vector>
#include "value_objects/body_goal_id.hpp"
#include "value_objects/pud_child_delta.hpp"
#include "value_objects/pud_forced_query_propagation.hpp"
#include "value_objects/pud_mhcs_head_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "debug_assert.hpp"

// Expands a live leaf at one of its open body goals: every candidate of that goal's query is
// accepted as a child. The leaf stops being a leaf before any child descent opens a query, and
// children whose descents arrive with a refuted goal are refuted right away.
template<
    typename IGetNodeDescent,
    typename IGetQueryCandidates,
    typename IGetCandidateDescent,
    typename IAcceptCandidate,
    typename IMakeChild,
    typename IStoreCallSite,
    typename IStoreChildren,
    typename IUnsetLeaf,
    typename IStoreLiveChildCount,
    typename IPropagateRefutation,
    typename IInvalidateLeaves,
    typename IGetQueryHeadNode,
    typename IRefuteNode,
    typename ICheckNodeRefuted,
    typename IStoreNodeDescent,
    typename IEraseNodeDescent>
struct pud_leaf_expander {
    pud_leaf_expander(
        IGetNodeDescent& get_node_descent,
        IGetQueryCandidates& get_query_candidates,
        IGetCandidateDescent& get_candidate_descent,
        IAcceptCandidate& accept_candidate,
        IMakeChild& make_child,
        IStoreCallSite& store_call_site,
        IStoreChildren& store_children,
        IUnsetLeaf& unset_leaf,
        IStoreLiveChildCount& store_live_child_count,
        IPropagateRefutation& propagate_refutation,
        IInvalidateLeaves& invalidate_leaves,
        IGetQueryHeadNode& get_query_head_node,
        IRefuteNode& refute_node,
        ICheckNodeRefuted& check_node_refuted,
        IStoreNodeDescent& store_node_descent,
        IEraseNodeDescent& erase_node_descent);
    void expand(pud_node_id leaf, body_goal_id goal);
private:
    IGetNodeDescent& get_node_descent_;
    IGetQueryCandidates& get_query_candidates_;
    IGetCandidateDescent& get_candidate_descent_;
    IAcceptCandidate& accept_candidate_;
    IMakeChild& make_child_;
    IStoreCallSite& store_call_site_;
    IStoreChildren& store_children_;
    IUnsetLeaf& unset_leaf_;
    IStoreLiveChildCount& store_live_child_count_;
    IPropagateRefutation& propagate_refutation_;
    IInvalidateLeaves& invalidate_leaves_;
    IGetQueryHeadNode& get_query_head_node_;
    IRefuteNode& refute_node_;
    ICheckNodeRefuted& check_node_refuted_;
    IStoreNodeDescent& store_node_descent_;
    IEraseNodeDescent& erase_node_descent_;
};

#define PUD_LEAF_EXPANDER_TEMPLATE                                                         \
    template<typename IGetNodeDescent, typename IGetQueryCandidates,                       \
             typename IGetCandidateDescent, typename IAcceptCandidate, typename IMakeChild, \
             typename IStoreCallSite, typename IStoreChildren, typename IUnsetLeaf,         \
             typename IStoreLiveChildCount, typename IPropagateRefutation,                  \
             typename IInvalidateLeaves, typename IGetQueryHeadNode, typename IRefuteNode,  \
             typename ICheckNodeRefuted, typename IStoreNodeDescent,                        \
             typename IEraseNodeDescent>
#define PUD_LEAF_EXPANDER                                                                  \
    pud_leaf_expander<IGetNodeDescent, IGetQueryCandidates, IGetCandidateDescent,          \
        IAcceptCandidate, IMakeChild, IStoreCallSite, IStoreChildren, IUnsetLeaf,          \
        IStoreLiveChildCount, IPropagateRefutation, IInvalidateLeaves, IGetQueryHeadNode,  \
        IRefuteNode, ICheckNodeRefuted, IStoreNodeDescent, IEraseNodeDescent>

PUD_LEAF_EXPANDER_TEMPLATE
PUD_LEAF_EXPANDER::pud_leaf_expander(
    IGetNodeDescent& get_node_descent,
    IGetQueryCandidates& get_query_candidates,
    IGetCandidateDescent& get_candidate_descent,
    IAcceptCandidate& accept_candidate,
    IMakeChild& make_child,
    IStoreCallSite& store_call_site,
    IStoreChildren& store_children,
    IUnsetLeaf& unset_leaf,
    IStoreLiveChildCount& store_live_child_count,
    IPropagateRefutation& propagate_refutation,
    IInvalidateLeaves& invalidate_leaves,
    IGetQueryHeadNode& get_query_head_node,
    IRefuteNode& refute_node,
    ICheckNodeRefuted& check_node_refuted,
    IStoreNodeDescent& store_node_descent,
    IEraseNodeDescent& erase_node_descent)
    : get_node_descent_(get_node_descent)
    , get_query_candidates_(get_query_candidates)
    , get_candidate_descent_(get_candidate_descent)
    , accept_candidate_(accept_candidate)
    , make_child_(make_child)
    , store_call_site_(store_call_site)
    , store_children_(store_children)
    , unset_leaf_(unset_leaf)
    , store_live_child_count_(store_live_child_count)
    , propagate_refutation_(propagate_refutation)
    , invalidate_leaves_(invalidate_leaves)
    , get_query_head_node_(get_query_head_node)
    , refute_node_(refute_node)
    , check_node_refuted_(check_node_refuted)
    , store_node_descent_(store_node_descent)
    , erase_node_descent_(erase_node_descent) {}

PUD_LEAF_EXPANDER_TEMPLATE
void PUD_LEAF_EXPANDER::expand(pud_node_id leaf, body_goal_id goal) {
    const auto leaf_descent = get_node_descent_.get(leaf);
    const std::optional<pud_mhq_head_id> query_head = leaf_descent.body_goal_pq().find_query_head(goal);
    DEBUG_ASSERT(query_head.has_value());
    const auto& query_candidates = get_query_candidates_.get_candidates(query_head.value());
    std::vector<pud_mhcs_head_id> candidates(query_candidates.begin(), query_candidates.end());
    std::sort(candidates.begin(), candidates.end());

    const expr* goal_expr = leaf_descent.body_goal_expr().body_goal_exprs().at(goal);
    std::vector<pud_node_id> children;
    for (pud_mhcs_head_id candidate : candidates) {
        const pud_child_delta delta = accept_candidate_.accept(
            leaf_descent.binding().ubs_state(),
            leaf_descent.lvc().lvc(),
            goal_expr,
            get_candidate_descent_.get_descent(candidate).node);
        children.push_back(make_child_.make(leaf, delta));
    }

    store_call_site_.store(leaf, goal);
    store_children_.store(leaf, children);
    unset_leaf_.unset_leaf(leaf);
    store_live_child_count_.store(leaf, children.size());
    if (children.empty())
        propagate_refutation_.refute(leaf);

    for (const pud_forced_query_propagation& forced : invalidate_leaves_.invalidate({leaf})) {
        const pud_refuted_query* refuted_query = std::get_if<pud_refuted_query>(&forced);
        if (!refuted_query)
            continue;
        refute_node_.refute(get_query_head_node_.get(refuted_query->head_id));
    }

    std::vector<pud_node_id> children_with_refuted_goals;
    for (pud_node_id child : children) {
        if (check_node_refuted_.check_refuted(child))
            continue;
        auto child_descent = leaf_descent.descend(leaf, child);
        const bool goal_refuted = !child_descent.body_goal_pq().refuted_goals().empty();
        store_node_descent_.store(child, std::move(child_descent));
        if (!goal_refuted)
            continue;
        children_with_refuted_goals.push_back(child);
    }
    erase_node_descent_.erase(leaf);
    for (pud_node_id child : children_with_refuted_goals)
        refute_node_.refute(child);
}

#undef PUD_LEAF_EXPANDER
#undef PUD_LEAF_EXPANDER_TEMPLATE

#endif
