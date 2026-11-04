#ifndef PUD_EVALUATION_RESOLVER_HPP
#define PUD_EVALUATION_RESOLVER_HPP

#include <cstdint>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_eager_candidate.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_leaf_evaluation.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "debug_assert.hpp"

// Resolves a leaf's goal against the candidates the leaf's evaluation found for it: one inference per candidate,
// built along its specialized path.
template<
    typename IGetEvaluation,
    typename IGetNodeBindings,
    typename IGetNodeLvc,
    typename IGetNodeGoals,
    typename IBuildInference>
struct pud_evaluation_resolver {
    pud_evaluation_resolver(
        IGetEvaluation& get_evaluation,
        IGetNodeBindings& get_node_bindings,
        IGetNodeLvc& get_node_lvc,
        IGetNodeGoals& get_node_goals,
        IBuildInference& build_inference);
    std::vector<pud_inference> resolve(pud_global_node_id leaf, pud_body_goal_id goal);
private:
    IGetEvaluation& get_evaluation_;
    IGetNodeBindings& get_node_bindings_;
    IGetNodeLvc& get_node_lvc_;
    IGetNodeGoals& get_node_goals_;
    IBuildInference& build_inference_;
};

#define PUD_EVALUATION_RESOLVER_TEMPLATE                                                                 \
    template<typename IGetEvaluation, typename IGetNodeBindings, typename IGetNodeLvc,                   \
             typename IGetNodeGoals, typename IBuildInference>
#define PUD_EVALUATION_RESOLVER                                                                          \
    pud_evaluation_resolver<IGetEvaluation, IGetNodeBindings, IGetNodeLvc, IGetNodeGoals, IBuildInference>

PUD_EVALUATION_RESOLVER_TEMPLATE
PUD_EVALUATION_RESOLVER::pud_evaluation_resolver(
    IGetEvaluation& get_evaluation,
    IGetNodeBindings& get_node_bindings,
    IGetNodeLvc& get_node_lvc,
    IGetNodeGoals& get_node_goals,
    IBuildInference& build_inference)
    : get_evaluation_(get_evaluation)
    , get_node_bindings_(get_node_bindings)
    , get_node_lvc_(get_node_lvc)
    , get_node_goals_(get_node_goals)
    , build_inference_(build_inference) {}

PUD_EVALUATION_RESOLVER_TEMPLATE
std::vector<pud_inference> PUD_EVALUATION_RESOLVER::resolve(pud_global_node_id leaf, pud_body_goal_id goal) {
    const pud_leaf_evaluation& evaluation = get_evaluation_.get();
    DEBUG_ASSERT(evaluation.leaf == leaf);
    const pud_ubs_state& bindings = get_node_bindings_.get(leaf);
    const uint32_t lvc = get_node_lvc_.get(leaf);
    const expr* goal_expr = get_node_goals_.get(leaf).goals.at(goal);
    std::vector<pud_inference> inferences;
    for (const pud_eager_candidate& candidate : evaluation.goals.at(goal))
        inferences.push_back(build_inference_.build(leaf, goal, bindings, lvc, goal_expr, candidate.path));
    return inferences;
}

#undef PUD_EVALUATION_RESOLVER
#undef PUD_EVALUATION_RESOLVER_TEMPLATE

#endif
