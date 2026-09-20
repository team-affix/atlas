#ifndef PUD_UNFOLDER_HPP
#define PUD_UNFOLDER_HPP

#include <cstddef>
#include <unordered_map>
#include <utility>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_added_unification.hpp"
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
         typename ILiveCallees,
         typename IGetLeafQueries,
         typename IReplaceUnfolded,
         typename ITakeForcedUnfolds>
struct pud_unfolder {
    pud_unfolder(IGetNode& get_node,
                 IUnifyCallee& unify_callee,
                 INormalize& normalize,
                 IMakeVar& make_var,
                 IAddInference& add_inference,
                 ILinkChildren& link_children,
                 IEffectiveBody& effective_body,
                 ILiveCallees& live_callees,
                 IGetLeafQueries& get_leaf_queries,
                 IReplaceUnfolded& replace_unfolded,
                 ITakeForcedUnfolds& take_forced_unfolds);
    coroutine<pud_forced_unfold, std::vector<const pud_rule_id*>> unfold(
        const pud_rule_id* leaf,
        size_t body_goal_idx);
private:
    const pud_rule_id* materialize_child(pud_query& parent_query,
                                         const pud_rule_id* leaf,
                                         size_t body_goal_idx,
                                         const pud_rule_id* callee,
                                         uint32_t parent_lvc);

    IGetNode& get_node_;
    IUnifyCallee& unify_callee_;
    INormalize& normalize_;
    IMakeVar& make_var_;
    IAddInference& add_inference_;
    ILinkChildren& link_children_;
    IEffectiveBody& effective_body_;
    ILiveCallees& live_callees_;
    IGetLeafQueries& get_leaf_queries_;
    IReplaceUnfolded& replace_unfolded_;
    ITakeForcedUnfolds& take_forced_unfolds_;
};

template<typename IGN, typename IUC, typename IN, typename IMV, typename IAI,
         typename ILC, typename IEB, typename ILV, typename IGLQ, typename IRU,
         typename ITFU>
pud_unfolder<IGN, IUC, IN, IMV, IAI, ILC, IEB, ILV, IGLQ, IRU, ITFU>::
pud_unfolder(IGN& get_node,
             IUC& unify_callee,
             IN& normalize,
             IMV& make_var,
             IAI& add_inference,
             ILC& link_children,
             IEB& effective_body,
             ILV& live_callees,
             IGLQ& get_leaf_queries,
             IRU& replace_unfolded,
             ITFU& take_forced_unfolds)
    : get_node_(get_node)
    , unify_callee_(unify_callee)
    , normalize_(normalize)
    , make_var_(make_var)
    , add_inference_(add_inference)
    , link_children_(link_children)
    , effective_body_(effective_body)
    , live_callees_(live_callees)
    , get_leaf_queries_(get_leaf_queries)
    , replace_unfolded_(replace_unfolded)
    , take_forced_unfolds_(take_forced_unfolds) {}

template<typename IGN, typename IUC, typename IN, typename IMV, typename IAI,
         typename ILC, typename IEB, typename ILV, typename IGLQ, typename IRU,
         typename ITFU>
const pud_rule_id*
pud_unfolder<IGN, IUC, IN, IMV, IAI, ILC, IEB, ILV, IGLQ, IRU, ITFU>::
materialize_child(pud_query& parent_query,
                  const pud_rule_id* leaf,
                  size_t body_goal_idx,
                  const pud_rule_id* callee,
                  uint32_t parent_lvc) {
    std::vector<uint32_t> touched_reps;
    om_interval env{parent_query.interval};
    const bool unified = unify_callee_.unify_callee(
        parent_query, callee, touched_reps, env);
    DEBUG_ASSERT(unified);

    std::unordered_map<uint32_t, uint32_t> translation;
    std::vector<pud_added_unification> added_unifications;
    for (uint32_t rep : touched_reps) {
        const expr* value = normalize_.normalize(
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
            env, framed_expr{goal, 0}, parent_lvc, translation));
    }
    const uint32_t child_lvc = parent_lvc
        + static_cast<uint32_t>(translation.size());

    return add_inference_.add_inference(
        leaf, body_goal_idx, callee,
        std::move(added_unifications),
        std::move(added_body_goals),
        child_lvc);
}

template<typename IGN, typename IUC, typename IN, typename IMV, typename IAI,
         typename ILC, typename IEB, typename ILV, typename IGLQ, typename IRU,
         typename ITFU>
coroutine<pud_forced_unfold, std::vector<const pud_rule_id*>>
pud_unfolder<IGN, IUC, IN, IMV, IAI, ILC, IEB, ILV, IGLQ, IRU, ITFU>::
unfold(const pud_rule_id* leaf, size_t body_goal_idx) {
    const pud_db_node& parent_node = get_node_.get_node(leaf);
    const uint32_t parent_lvc = parent_node.lvc;
    const std::vector<const pud_rule_id*> callees =
        live_callees_.live_callees(leaf, body_goal_idx);
    DEBUG_ASSERT(!callees.empty());
    const std::vector<pud_query*>& parent_queries = get_leaf_queries_.get(leaf);
    DEBUG_ASSERT(body_goal_idx < parent_queries.size());
    pud_query* parent_query = parent_queries[body_goal_idx];

    std::vector<const pud_rule_id*> children;
    for (const pud_rule_id* callee : callees)
        children.push_back(materialize_child(
            *parent_query, leaf, body_goal_idx, callee, parent_lvc));

    link_children_.link_children(leaf, children);
    replace_unfolded_.replace_unfolded(leaf, body_goal_idx, children);

    const std::vector<pud_forced_unfold> yields = take_forced_unfolds_.take_forced_unfolds();
    for (const pud_forced_unfold& yield : yields)
        co_yield yield;
    co_return children;
}

#endif
