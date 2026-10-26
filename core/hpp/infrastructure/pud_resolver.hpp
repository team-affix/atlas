#ifndef PUD_RESOLVER_HPP
#define PUD_RESOLVER_HPP

#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_active_state.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_mhcs_head_id.hpp"

template<
    typename IGetActiveState,
    typename IFindNodeGoal,
    typename IGetQueryCandidates,
    typename IGetCandidatePath,
    typename IAcceptCandidate>
struct pud_resolver {
    pud_resolver(
        IGetActiveState& get_active_state,
        IFindNodeGoal& find_node_goal,
        IGetQueryCandidates& get_query_candidates,
        IGetCandidatePath& get_candidate_path,
        IAcceptCandidate& accept_candidate);
    std::vector<pud_inference> resolve(pud_global_node_id leaf, pud_body_goal_id goal);
private:
    IGetActiveState& get_active_state_;
    IFindNodeGoal& find_node_goal_;
    IGetQueryCandidates& get_query_candidates_;
    IGetCandidatePath& get_candidate_path_;
    IAcceptCandidate& accept_candidate_;
};

#define PUD_RESOLVER_TEMPLATE                                                              \
    template<typename IGetActiveState, typename IFindNodeGoal, typename IGetQueryCandidates, \
             typename IGetCandidatePath, typename IAcceptCandidate>
#define PUD_RESOLVER                                                                       \
    pud_resolver<IGetActiveState, IFindNodeGoal, IGetQueryCandidates, IGetCandidatePath, IAcceptCandidate>

PUD_RESOLVER_TEMPLATE
PUD_RESOLVER::pud_resolver(
    IGetActiveState& get_active_state,
    IFindNodeGoal& find_node_goal,
    IGetQueryCandidates& get_query_candidates,
    IGetCandidatePath& get_candidate_path,
    IAcceptCandidate& accept_candidate)
    : get_active_state_(get_active_state)
    , find_node_goal_(find_node_goal)
    , get_query_candidates_(get_query_candidates)
    , get_candidate_path_(get_candidate_path)
    , accept_candidate_(accept_candidate) {}

PUD_RESOLVER_TEMPLATE
std::vector<pud_inference> PUD_RESOLVER::resolve(pud_global_node_id leaf, pud_body_goal_id goal) {
    const pud_active_state& state = get_active_state_.get(leaf);
    const expr* goal_expr = find_node_goal_.find(leaf, goal);
    std::vector<pud_inference> inferences;
    for (pud_mhcs_head_id candidate : get_query_candidates_.candidates(state.queries.at(goal))) {
        inferences.push_back(
            accept_candidate_.accept(leaf, goal, state.bindings, state.lvc, goal_expr, get_candidate_path_.path(candidate)));
    }
    return inferences;
}

#undef PUD_RESOLVER
#undef PUD_RESOLVER_TEMPLATE

#endif
