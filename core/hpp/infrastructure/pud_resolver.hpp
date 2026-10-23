#ifndef PUD_RESOLVER_HPP
#define PUD_RESOLVER_HPP

#include <cstdint>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_child_delta.hpp"
#include "value_objects/pud_leaf_state.hpp"
#include "value_objects/pud_mhcs_head_id.hpp"
#include "value_objects/pud_node_id.hpp"

// Resolves a leaf's goal against every candidate of the goal's query, in axiom order: one child delta
// per candidate.
template<
    typename IGetLeafState,
    typename IFindNodeGoal,
    typename IGetNodeLvc,
    typename IGetQueryCandidates,
    typename IGetCandidatePath,
    typename IAcceptCandidate>
struct pud_resolver {
    pud_resolver(
        IGetLeafState& get_leaf_state,
        IFindNodeGoal& find_node_goal,
        IGetNodeLvc& get_node_lvc,
        IGetQueryCandidates& get_query_candidates,
        IGetCandidatePath& get_candidate_path,
        IAcceptCandidate& accept_candidate);
    std::vector<pud_child_delta> resolve(pud_node_id leaf, pud_body_goal_id goal);
private:
    IGetLeafState& get_leaf_state_;
    IFindNodeGoal& find_node_goal_;
    IGetNodeLvc& get_node_lvc_;
    IGetQueryCandidates& get_query_candidates_;
    IGetCandidatePath& get_candidate_path_;
    IAcceptCandidate& accept_candidate_;
};

#define PUD_RESOLVER_TEMPLATE                                                              \
    template<typename IGetLeafState, typename IFindNodeGoal, typename IGetNodeLvc,         \
             typename IGetQueryCandidates, typename IGetCandidatePath, typename IAcceptCandidate>
#define PUD_RESOLVER                                                                       \
    pud_resolver<IGetLeafState, IFindNodeGoal, IGetNodeLvc, IGetQueryCandidates,           \
        IGetCandidatePath, IAcceptCandidate>

PUD_RESOLVER_TEMPLATE
PUD_RESOLVER::pud_resolver(
    IGetLeafState& get_leaf_state,
    IFindNodeGoal& find_node_goal,
    IGetNodeLvc& get_node_lvc,
    IGetQueryCandidates& get_query_candidates,
    IGetCandidatePath& get_candidate_path,
    IAcceptCandidate& accept_candidate)
    : get_leaf_state_(get_leaf_state)
    , find_node_goal_(find_node_goal)
    , get_node_lvc_(get_node_lvc)
    , get_query_candidates_(get_query_candidates)
    , get_candidate_path_(get_candidate_path)
    , accept_candidate_(accept_candidate) {}

PUD_RESOLVER_TEMPLATE
std::vector<pud_child_delta> PUD_RESOLVER::resolve(pud_node_id leaf, pud_body_goal_id goal) {
    const pud_leaf_state& state = get_leaf_state_.get(leaf);
    const expr* goal_expr = find_node_goal_.find(leaf, goal);
    const uint32_t lvc = get_node_lvc_.get(leaf);
    std::vector<pud_child_delta> deltas;
    for (pud_mhcs_head_id candidate : get_query_candidates_.candidates(state.queries.at(goal)))
        deltas.push_back(accept_candidate_.accept(state.bindings, lvc, goal_expr, get_candidate_path_.path(candidate)));
    return deltas;
}

#undef PUD_RESOLVER
#undef PUD_RESOLVER_TEMPLATE

#endif
