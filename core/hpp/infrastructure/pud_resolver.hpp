#ifndef PUD_RESOLVER_HPP
#define PUD_RESOLVER_HPP

#include <cstdint>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_mhcs_head_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_ubs_state.hpp"

template<
    typename IGetNodeQueries,
    typename IGetNodeBindings,
    typename IGetNodeLvc,
    typename IGetNodeGoals,
    typename IGetQueryCandidates,
    typename IGetCandidatePath,
    typename IBuildInference>
struct pud_resolver {
    pud_resolver(
        IGetNodeQueries& get_node_queries,
        IGetNodeBindings& get_node_bindings,
        IGetNodeLvc& get_node_lvc,
        IGetNodeGoals& get_node_goals,
        IGetQueryCandidates& get_query_candidates,
        IGetCandidatePath& get_candidate_path,
        IBuildInference& build_inference);
    std::vector<pud_inference> resolve(pud_global_node_id leaf, pud_body_goal_id goal);
private:
    IGetNodeQueries& get_node_queries_;
    IGetNodeBindings& get_node_bindings_;
    IGetNodeLvc& get_node_lvc_;
    IGetNodeGoals& get_node_goals_;
    IGetQueryCandidates& get_query_candidates_;
    IGetCandidatePath& get_candidate_path_;
    IBuildInference& build_inference_;
};

#define PUD_RESOLVER_TEMPLATE                                                              \
    template<typename IGetNodeQueries, typename IGetNodeBindings, typename IGetNodeLvc,      \
             typename IGetNodeGoals, typename IGetQueryCandidates,                           \
             typename IGetCandidatePath, typename IBuildInference>
#define PUD_RESOLVER                                                                       \
    pud_resolver<IGetNodeQueries, IGetNodeBindings, IGetNodeLvc, IGetNodeGoals, IGetQueryCandidates, \
        IGetCandidatePath, IBuildInference>

PUD_RESOLVER_TEMPLATE
PUD_RESOLVER::pud_resolver(
    IGetNodeQueries& get_node_queries,
    IGetNodeBindings& get_node_bindings,
    IGetNodeLvc& get_node_lvc,
    IGetNodeGoals& get_node_goals,
    IGetQueryCandidates& get_query_candidates,
    IGetCandidatePath& get_candidate_path,
    IBuildInference& build_inference)
    : get_node_queries_(get_node_queries)
    , get_node_bindings_(get_node_bindings)
    , get_node_lvc_(get_node_lvc)
    , get_node_goals_(get_node_goals)
    , get_query_candidates_(get_query_candidates)
    , get_candidate_path_(get_candidate_path)
    , build_inference_(build_inference) {}

PUD_RESOLVER_TEMPLATE
std::vector<pud_inference> PUD_RESOLVER::resolve(pud_global_node_id leaf, pud_body_goal_id goal) {
    const pud_ubs_state& bindings = get_node_bindings_.get(leaf);
    const uint32_t lvc = get_node_lvc_.get(leaf);
    const expr* goal_expr = get_node_goals_.get(leaf).goals.at(goal);
    std::vector<pud_inference> inferences;
    const pud_mhq_head_id head = get_node_queries_.get(leaf).at(goal)->head_id();
    for (pud_mhcs_head_id candidate : get_query_candidates_.candidates(head)) {
        inferences.push_back(
            build_inference_.build(leaf, goal, bindings, lvc, goal_expr, get_candidate_path_.path(candidate)));
    }
    return inferences;
}

#undef PUD_RESOLVER
#undef PUD_RESOLVER_TEMPLATE

#endif
