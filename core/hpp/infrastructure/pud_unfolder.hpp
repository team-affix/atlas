#ifndef PUD_UNFOLDER_HPP
#define PUD_UNFOLDER_HPP

#include <cstddef>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_candidate_search_result.hpp"
#include "value_objects/pud_db_node.hpp"
#include "value_objects/pud_forced_unfold.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "debug_assert.hpp"

template<typename IGetNode,
         typename IUnifyCallee,
         typename INormalize,
         typename IMakeVar,
         typename IAddInference,
         typename ILinkChildren,
         typename IEffectiveBody,
         typename IGetLeafQueries,
         typename IClearLeafQueries,
         typename IForkChild,
         typename IInvalidateLeaf,
         typename IResumeCandidateSearch,
         typename ITakeDirtyLeaves>
struct pud_unfolder {
    pud_unfolder(IGetNode& get_node,
                 IUnifyCallee& unify_callee,
                 INormalize& normalize,
                 IMakeVar& make_var,
                 IAddInference& add_inference,
                 ILinkChildren& link_children,
                 IEffectiveBody& effective_body,
                 IGetLeafQueries& get_leaf_queries,
                 IClearLeafQueries& clear_leaf_queries,
                 IForkChild& fork_child,
                 IInvalidateLeaf& invalidate_leaf,
                 IResumeCandidateSearch& resume_candidate_search,
                 ITakeDirtyLeaves& take_dirty_leaves);
    coroutine<pud_forced_unfold, std::vector<const pud_rule_id*>> unfold(
        const pud_rule_id* leaf,
        size_t body_goal_idx);
private:
    std::vector<const pud_rule_id*> find_live_callees(pud_query& query);
    const pud_rule_id* materialize_child(pud_query& parent_query,
                                         const pud_rule_id* leaf,
                                         size_t body_goal_idx,
                                         const pud_rule_id* callee,
                                         uint32_t parent_lvc,
                                         const om_interval& parent_interval);
    size_t live_axiom_count(pud_query& query);
    std::vector<pud_forced_unfold> collect_yields();

    IGetNode& get_node_;
    IUnifyCallee& unify_callee_;
    INormalize& normalize_;
    IMakeVar& make_var_;
    IAddInference& add_inference_;
    ILinkChildren& link_children_;
    IEffectiveBody& effective_body_;
    IGetLeafQueries& get_leaf_queries_;
    IClearLeafQueries& clear_leaf_queries_;
    IForkChild& fork_child_;
    IInvalidateLeaf& invalidate_leaf_;
    IResumeCandidateSearch& resume_candidate_search_;
    ITakeDirtyLeaves& take_dirty_leaves_;
};

template<typename IGN, typename IUC, typename IN, typename IMV, typename IAI,
         typename ILC, typename IEB, typename IGLQ, typename ICLQ, typename IFC,
         typename IIL, typename IRCS, typename ITDL>
pud_unfolder<IGN, IUC, IN, IMV, IAI, ILC, IEB, IGLQ, ICLQ, IFC, IIL, IRCS, ITDL>::
pud_unfolder(IGN& get_node,
             IUC& unify_callee,
             IN& normalize,
             IMV& make_var,
             IAI& add_inference,
             ILC& link_children,
             IEB& effective_body,
             IGLQ& get_leaf_queries,
             ICLQ& clear_leaf_queries,
             IFC& fork_child,
             IIL& invalidate_leaf,
             IRCS& resume_candidate_search,
             ITDL& take_dirty_leaves)
    : get_node_(get_node)
    , unify_callee_(unify_callee)
    , normalize_(normalize)
    , make_var_(make_var)
    , add_inference_(add_inference)
    , link_children_(link_children)
    , effective_body_(effective_body)
    , get_leaf_queries_(get_leaf_queries)
    , clear_leaf_queries_(clear_leaf_queries)
    , fork_child_(fork_child)
    , invalidate_leaf_(invalidate_leaf)
    , resume_candidate_search_(resume_candidate_search)
    , take_dirty_leaves_(take_dirty_leaves) {}

template<typename IGN, typename IUC, typename IN, typename IMV, typename IAI,
         typename ILC, typename IEB, typename IGLQ, typename ICLQ, typename IFC,
         typename IIL, typename IRCS, typename ITDL>
std::vector<const pud_rule_id*>
pud_unfolder<IGN, IUC, IN, IMV, IAI, ILC, IEB, IGLQ, ICLQ, IFC, IIL, IRCS, ITDL>::
find_live_callees(pud_query& query) {
    std::vector<const pud_rule_id*> callees;
    for (pud_candidate_search_context& axiom_ctx : query.axiom_contexts) {
        const pud_candidate_search_result result =
            resume_candidate_search_.resume(query, axiom_ctx);
        if (std::holds_alternative<pud_candidate_search_result::axiom_refuted>(
                result.content))
            continue;
        callees.push_back(axiom_ctx.cursor);
    }
    return callees;
}

template<typename IGN, typename IUC, typename IN, typename IMV, typename IAI,
         typename ILC, typename IEB, typename IGLQ, typename ICLQ, typename IFC,
         typename IIL, typename IRCS, typename ITDL>
const pud_rule_id*
pud_unfolder<IGN, IUC, IN, IMV, IAI, ILC, IEB, IGLQ, ICLQ, IFC, IIL, IRCS, ITDL>::
materialize_child(pud_query& parent_query,
                  const pud_rule_id* leaf,
                  size_t body_goal_idx,
                  const pud_rule_id* callee,
                  uint32_t parent_lvc,
                  const om_interval& parent_interval) {
    std::vector<uint32_t> touched_reps;
    om_interval env{parent_interval};
    const bool unified = unify_callee_.unify_callee(
        parent_query, callee, touched_reps, env);
    DEBUG_ASSERT(unified);

    std::unordered_map<uint32_t, uint32_t> translation;
    std::vector<pud_added_unification> added_unifications;
    for (uint32_t rep : touched_reps) {
        const expr* value = normalize_.normalize(
            parent_query,
            env,
            framed_expr{make_var_.make_var(rep), 0},
            parent_lvc,
            translation);
        added_unifications.push_back(pud_added_unification{rep, value});
    }

    const std::vector<const expr*> callee_bodies = effective_body_.effective_body(callee);
    std::vector<const expr*> added_body_goals;
    for (const expr* goal : callee_bodies) {
        added_body_goals.push_back(normalize_.normalize(
            parent_query, env, framed_expr{goal, 0}, parent_lvc, translation));
    }
    const uint32_t child_lvc = parent_lvc
        + static_cast<uint32_t>(translation.size());

    pud_db_node child_node{
        parent_interval,
        std::move(added_unifications),
        std::move(added_body_goals),
        child_lvc};
    return add_inference_.add_inference(leaf, body_goal_idx, callee, child_node);
}

template<typename IGN, typename IUC, typename IN, typename IMV, typename IAI,
         typename ILC, typename IEB, typename IGLQ, typename ICLQ, typename IFC,
         typename IIL, typename IRCS, typename ITDL>
size_t
pud_unfolder<IGN, IUC, IN, IMV, IAI, ILC, IEB, IGLQ, ICLQ, IFC, IIL, IRCS, ITDL>::
live_axiom_count(pud_query& query) {
    size_t live = 0;
    for (pud_candidate_search_context& axiom_ctx : query.axiom_contexts) {
        const pud_candidate_search_result result =
            resume_candidate_search_.resume(query, axiom_ctx);
        if (!std::holds_alternative<pud_candidate_search_result::axiom_refuted>(
                result.content))
            ++live;
    }
    return live;
}

template<typename IGN, typename IUC, typename IN, typename IMV, typename IAI,
         typename ILC, typename IEB, typename IGLQ, typename ICLQ, typename IFC,
         typename IIL, typename IRCS, typename ITDL>
std::vector<pud_forced_unfold>
pud_unfolder<IGN, IUC, IN, IMV, IAI, ILC, IEB, IGLQ, ICLQ, IFC, IIL, IRCS, ITDL>::
collect_yields() {
    std::vector<pud_forced_unfold> yields;
    const std::vector<const pud_rule_id*> dirty = take_dirty_leaves_.take_dirty_leaves();
    for (const pud_rule_id* leaf : dirty) {
        const std::vector<pud_query*>& queries = get_leaf_queries_.get(leaf);
        bool refuted = false;
        std::vector<size_t> unit_idxs;
        for (size_t idx = 0; idx < queries.size(); ++idx) {
            const size_t live = live_axiom_count(*queries[idx]);
            if (live == 0) {
                refuted = true;
                break;
            }
            if (live == 1)
                unit_idxs.push_back(idx);
        }
        if (refuted) {
            yields.push_back(pud_forced_unfold{pud_forced_unfold::refuted{leaf}});
            continue;
        }
        for (size_t idx : unit_idxs)
            yields.push_back(pud_forced_unfold{pud_forced_unfold::unit{leaf, idx}});
    }
    return yields;
}

template<typename IGN, typename IUC, typename IN, typename IMV, typename IAI,
         typename ILC, typename IEB, typename IGLQ, typename ICLQ, typename IFC,
         typename IIL, typename IRCS, typename ITDL>
coroutine<pud_forced_unfold, std::vector<const pud_rule_id*>>
pud_unfolder<IGN, IUC, IN, IMV, IAI, ILC, IEB, IGLQ, ICLQ, IFC, IIL, IRCS, ITDL>::
unfold(const pud_rule_id* leaf, size_t body_goal_idx) {
    const pud_db_node& parent_node = get_node_.get_node(leaf);
    const uint32_t parent_lvc = parent_node.lvc;
    const om_interval parent_interval = parent_node.interval;
    const std::vector<pud_query*>& parent_queries = get_leaf_queries_.get(leaf);
    DEBUG_ASSERT(body_goal_idx < parent_queries.size());
    pud_query* parent_query = parent_queries[body_goal_idx];

    const std::vector<const pud_rule_id*> callees = find_live_callees(*parent_query);
    DEBUG_ASSERT(!callees.empty());

    std::vector<pud_query> leftover_templates;
    for (size_t idx = 0; idx < parent_queries.size(); ++idx) {
        if (idx == body_goal_idx)
            continue;
        leftover_templates.push_back(*parent_queries[idx]);
    }

    std::vector<const pud_rule_id*> children;
    for (const pud_rule_id* callee : callees)
        children.push_back(materialize_child(
            *parent_query, leaf, body_goal_idx, callee, parent_lvc, parent_interval));

    link_children_.link_children(leaf, children);
    clear_leaf_queries_.clear(leaf);

    for (const pud_rule_id* child : children)
        fork_child_.fork_child(child, leftover_templates);

    invalidate_leaf_.invalidate_leaf(leaf);

    const std::vector<pud_forced_unfold> yields = collect_yields();
    for (const pud_forced_unfold& yield : yields)
        co_yield yield;
    co_return children;
}

#endif
