#ifndef PUD_EAGER_LEAF_EVALUATOR_HPP
#define PUD_EAGER_LEAF_EVALUATOR_HPP

#include <cstdint>
#include <vector>
#include "value_objects/pud_eager_candidate.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_leaf_evaluation.hpp"
#include "value_objects/pud_ubs_state.hpp"

// Queries every open goal of a leaf now, in goal order, under the leaf's bindings. It stops at the first goal that
// has no candidate: the leaf is refuted.
template<typename IGetNodeGoals, typename IGetNodeBindings, typename IGetNodeLvc, typename IQueryGoal>
struct pud_eager_leaf_evaluator {
    pud_eager_leaf_evaluator(
        IGetNodeGoals& get_node_goals,
        IGetNodeBindings& get_node_bindings,
        IGetNodeLvc& get_node_lvc,
        IQueryGoal& query_goal);
    pud_leaf_evaluation evaluate(pud_global_node_id leaf) const;
private:
    IGetNodeGoals& get_node_goals_;
    IGetNodeBindings& get_node_bindings_;
    IGetNodeLvc& get_node_lvc_;
    IQueryGoal& query_goal_;
};

#define PUD_EAGER_LEAF_EVALUATOR_TEMPLATE                                                                \
    template<typename IGetNodeGoals, typename IGetNodeBindings, typename IGetNodeLvc, typename IQueryGoal>
#define PUD_EAGER_LEAF_EVALUATOR pud_eager_leaf_evaluator<IGetNodeGoals, IGetNodeBindings, IGetNodeLvc, IQueryGoal>

PUD_EAGER_LEAF_EVALUATOR_TEMPLATE
PUD_EAGER_LEAF_EVALUATOR::pud_eager_leaf_evaluator(
    IGetNodeGoals& get_node_goals,
    IGetNodeBindings& get_node_bindings,
    IGetNodeLvc& get_node_lvc,
    IQueryGoal& query_goal)
    : get_node_goals_(get_node_goals)
    , get_node_bindings_(get_node_bindings)
    , get_node_lvc_(get_node_lvc)
    , query_goal_(query_goal) {}

PUD_EAGER_LEAF_EVALUATOR_TEMPLATE
pud_leaf_evaluation PUD_EAGER_LEAF_EVALUATOR::evaluate(pud_global_node_id leaf) const {
    const pud_ubs_state& bindings = get_node_bindings_.get(leaf);
    const uint32_t lvc = get_node_lvc_.get(leaf);
    pud_leaf_evaluation evaluation{.leaf = leaf, .refuted = false, .goals = {}};
    for (const auto& [goal, goal_expr] : get_node_goals_.get(leaf).goals) {
        std::vector<pud_eager_candidate> candidates = query_goal_.query(leaf.module, bindings, goal_expr, lvc);
        evaluation.refuted = candidates.empty();
        evaluation.goals.emplace(goal, std::move(candidates));
        if (evaluation.refuted)
            break;
    }
    return evaluation;
}

#undef PUD_EAGER_LEAF_EVALUATOR
#undef PUD_EAGER_LEAF_EVALUATOR_TEMPLATE

#endif
