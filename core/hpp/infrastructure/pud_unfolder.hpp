#ifndef PUD_UNFOLDER_HPP
#define PUD_UNFOLDER_HPP

#include <algorithm>
#include <optional>
#include <variant>
#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_child_delta.hpp"
#include "value_objects/pud_forced_query_propagation.hpp"
#include "value_objects/pud_mhcs_head_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "debug_assert.hpp"

template<
    typename IGetNodeDescent,
    typename IGetQueryCandidates,
    typename IGetCandidatePath,
    typename IAcceptCandidate,
    typename IMakeChild,
    typename IStoreCallSite,
    typename IStoreChildren,
    typename IUnsetLeaf,
    typename IMarkLeafClosed,
    typename IStoreLiveChildCount,
    typename IPropagateRefutation,
    typename IInvalidateLeaves,
    typename IGetQueryHeadNode,
    typename IRefuteNode,
    typename ICheckNodeRefuted,
    typename IStoreNodeDescent,
    typename IMarkLeafOpen,
    typename IEraseNodeDescent>
struct pud_unfolder {
    pud_unfolder(
        IGetNodeDescent& get_node_descent,
        IGetQueryCandidates& get_query_candidates,
        IGetCandidatePath& get_candidate_path,
        IAcceptCandidate& accept_candidate,
        IMakeChild& make_child,
        IStoreCallSite& store_call_site,
        IStoreChildren& store_children,
        IUnsetLeaf& unset_leaf,
        IMarkLeafClosed& mark_leaf_closed,
        IStoreLiveChildCount& store_live_child_count,
        IPropagateRefutation& propagate_refutation,
        IInvalidateLeaves& invalidate_leaves,
        IGetQueryHeadNode& get_query_head_node,
        IRefuteNode& refute_node,
        ICheckNodeRefuted& check_node_refuted,
        IStoreNodeDescent& store_node_descent,
        IMarkLeafOpen& mark_leaf_open,
        IEraseNodeDescent& erase_node_descent);
    void unfold(pud_node_id leaf, pud_body_goal_id goal);
private:
    IGetNodeDescent& get_node_descent_;
    IGetQueryCandidates& get_query_candidates_;
    IGetCandidatePath& get_candidate_path_;
    IAcceptCandidate& accept_candidate_;
    IMakeChild& make_child_;
    IStoreCallSite& store_call_site_;
    IStoreChildren& store_children_;
    IUnsetLeaf& unset_leaf_;
    IMarkLeafClosed& mark_leaf_closed_;
    IStoreLiveChildCount& store_live_child_count_;
    IPropagateRefutation& propagate_refutation_;
    IInvalidateLeaves& invalidate_leaves_;
    IGetQueryHeadNode& get_query_head_node_;
    IRefuteNode& refute_node_;
    ICheckNodeRefuted& check_node_refuted_;
    IStoreNodeDescent& store_node_descent_;
    IMarkLeafOpen& mark_leaf_open_;
    IEraseNodeDescent& erase_node_descent_;
};

#define PUD_UNFOLDER_TEMPLATE                                                         \
    template<typename IGetNodeDescent, typename IGetQueryCandidates,                       \
             typename IGetCandidatePath, typename IAcceptCandidate, typename IMakeChild, \
             typename IStoreCallSite, typename IStoreChildren, typename IUnsetLeaf,         \
             typename IMarkLeafClosed, typename IStoreLiveChildCount,                       \
             typename IPropagateRefutation, typename IInvalidateLeaves,                     \
             typename IGetQueryHeadNode, typename IRefuteNode, typename ICheckNodeRefuted,  \
             typename IStoreNodeDescent, typename IMarkLeafOpen, typename IEraseNodeDescent>
#define PUD_UNFOLDER                                                                  \
    pud_unfolder<IGetNodeDescent, IGetQueryCandidates, IGetCandidatePath,          \
        IAcceptCandidate, IMakeChild, IStoreCallSite, IStoreChildren, IUnsetLeaf,          \
        IMarkLeafClosed, IStoreLiveChildCount, IPropagateRefutation, IInvalidateLeaves,    \
        IGetQueryHeadNode, IRefuteNode, ICheckNodeRefuted, IStoreNodeDescent,              \
        IMarkLeafOpen, IEraseNodeDescent>

PUD_UNFOLDER_TEMPLATE
PUD_UNFOLDER::pud_unfolder(
    IGetNodeDescent& get_node_descent,
    IGetQueryCandidates& get_query_candidates,
    IGetCandidatePath& get_candidate_path,
    IAcceptCandidate& accept_candidate,
    IMakeChild& make_child,
    IStoreCallSite& store_call_site,
    IStoreChildren& store_children,
    IUnsetLeaf& unset_leaf,
    IMarkLeafClosed& mark_leaf_closed,
    IStoreLiveChildCount& store_live_child_count,
    IPropagateRefutation& propagate_refutation,
    IInvalidateLeaves& invalidate_leaves,
    IGetQueryHeadNode& get_query_head_node,
    IRefuteNode& refute_node,
    ICheckNodeRefuted& check_node_refuted,
    IStoreNodeDescent& store_node_descent,
    IMarkLeafOpen& mark_leaf_open,
    IEraseNodeDescent& erase_node_descent)
    : get_node_descent_(get_node_descent)
    , get_query_candidates_(get_query_candidates)
    , get_candidate_path_(get_candidate_path)
    , accept_candidate_(accept_candidate)
    , make_child_(make_child)
    , store_call_site_(store_call_site)
    , store_children_(store_children)
    , unset_leaf_(unset_leaf)
    , mark_leaf_closed_(mark_leaf_closed)
    , store_live_child_count_(store_live_child_count)
    , propagate_refutation_(propagate_refutation)
    , invalidate_leaves_(invalidate_leaves)
    , get_query_head_node_(get_query_head_node)
    , refute_node_(refute_node)
    , check_node_refuted_(check_node_refuted)
    , store_node_descent_(store_node_descent)
    , mark_leaf_open_(mark_leaf_open)
    , erase_node_descent_(erase_node_descent) {}

PUD_UNFOLDER_TEMPLATE
void PUD_UNFOLDER::unfold(pud_node_id leaf, pud_body_goal_id goal) {
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
            get_candidate_path_.get_path(candidate));
        children.push_back(make_child_.make(leaf, delta));
    }

    store_call_site_.store(leaf, goal);
    store_children_.store(leaf, children);
    unset_leaf_.unset_leaf(leaf);
    mark_leaf_closed_.mark_leaf_closed(leaf);
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
        const bool goals_open = !child_descent.body_goal_expr().body_goal_exprs().empty();
        store_node_descent_.store(child, std::move(child_descent));
        if (goal_refuted) {
            children_with_refuted_goals.push_back(child);
            continue;
        }
        if (!goals_open)
            continue;
        mark_leaf_open_.mark_leaf_open(child);
    }
    erase_node_descent_.erase(leaf);
    for (pud_node_id child : children_with_refuted_goals)
        refute_node_.refute(child);
}

#undef PUD_UNFOLDER
#undef PUD_UNFOLDER_TEMPLATE

#endif
