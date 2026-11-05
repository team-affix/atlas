#ifndef PUD_TRAIL_LEAF_EVALUATOR_HPP
#define PUD_TRAIL_LEAF_EVALUATOR_HPP

#include <cstdint>
#include <utility>
#include <vector>
#include <immer/box.hpp>
#include "value_objects/pud_candidate_trail.hpp"
#include "value_objects/pud_eager_candidate.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_goal_trails.hpp"
#include "value_objects/pud_leaf_evaluation.hpp"
#include "value_objects/pud_node_trails.hpp"
#include "value_objects/pud_ubs_state.hpp"

// Queries every open goal of a leaf now, in goal order, from the trails the leaf holds, and stops at the first goal
// without a candidate: the leaf is refuted. The leaf then holds the trails this evaluation leaves, for its children
// to inherit, in place of those it held; a goal whose trails came out unchanged keeps its box, still shared with its
// relatives.
template<
    typename IGetNodeGoals,
    typename IGetNodeBindings,
    typename IGetNodeLvc,
    typename IGetNodeTrails,
    typename IEraseNodeTrails,
    typename IStoreNodeTrails,
    typename IQueryGoal>
struct pud_trail_leaf_evaluator {
    pud_trail_leaf_evaluator(
        IGetNodeGoals& get_node_goals,
        IGetNodeBindings& get_node_bindings,
        IGetNodeLvc& get_node_lvc,
        IGetNodeTrails& get_node_trails,
        IEraseNodeTrails& erase_node_trails,
        IStoreNodeTrails& store_node_trails,
        IQueryGoal& query_goal);
    pud_leaf_evaluation evaluate(pud_global_node_id leaf) const;
private:
    IGetNodeGoals& get_node_goals_;
    IGetNodeBindings& get_node_bindings_;
    IGetNodeLvc& get_node_lvc_;
    IGetNodeTrails& get_node_trails_;
    IEraseNodeTrails& erase_node_trails_;
    IStoreNodeTrails& store_node_trails_;
    IQueryGoal& query_goal_;
};

#define PUD_TRAIL_LEAF_EVALUATOR_TEMPLATE                                                                \
    template<typename IGetNodeGoals, typename IGetNodeBindings, typename IGetNodeLvc,                    \
             typename IGetNodeTrails, typename IEraseNodeTrails, typename IStoreNodeTrails, typename IQueryGoal>
#define PUD_TRAIL_LEAF_EVALUATOR                                                                         \
    pud_trail_leaf_evaluator<IGetNodeGoals, IGetNodeBindings, IGetNodeLvc, IGetNodeTrails,               \
        IEraseNodeTrails, IStoreNodeTrails, IQueryGoal>

PUD_TRAIL_LEAF_EVALUATOR_TEMPLATE
PUD_TRAIL_LEAF_EVALUATOR::pud_trail_leaf_evaluator(
    IGetNodeGoals& get_node_goals,
    IGetNodeBindings& get_node_bindings,
    IGetNodeLvc& get_node_lvc,
    IGetNodeTrails& get_node_trails,
    IEraseNodeTrails& erase_node_trails,
    IStoreNodeTrails& store_node_trails,
    IQueryGoal& query_goal)
    : get_node_goals_(get_node_goals)
    , get_node_bindings_(get_node_bindings)
    , get_node_lvc_(get_node_lvc)
    , get_node_trails_(get_node_trails)
    , erase_node_trails_(erase_node_trails)
    , store_node_trails_(store_node_trails)
    , query_goal_(query_goal) {}

PUD_TRAIL_LEAF_EVALUATOR_TEMPLATE
pud_leaf_evaluation PUD_TRAIL_LEAF_EVALUATOR::evaluate(pud_global_node_id leaf) const {
    const pud_ubs_state& bindings = get_node_bindings_.get(leaf);
    const uint32_t lvc = get_node_lvc_.get(leaf);
    const pud_node_trails held = get_node_trails_.get(leaf);
    pud_node_trails left;
    pud_leaf_evaluation evaluation{.leaf = leaf, .refuted = false, .goals = {}};
    for (const auto& [goal, goal_expr] : get_node_goals_.get(leaf).goals) {
        const immer::box<pud_goal_trails>* trails = held.find(goal);
        std::vector<pud_eager_candidate> candidates =
            query_goal_.query(leaf.module, bindings, goal_expr, lvc, trails == nullptr ? nullptr : &trails->get());
        pud_goal_trails found{.candidates = {}, .forked = false};
        for (const pud_eager_candidate& candidate : candidates)
            found.candidates.push_back(
                pud_candidate_trail{.candidate = candidate.path.back(), .witness_leaves = candidate.witness_leaves});
        const bool unchanged = trails != nullptr && trails->get() == found;
        left = left.set(goal, unchanged ? *trails : immer::box<pud_goal_trails>{std::move(found)});
        evaluation.refuted = candidates.empty();
        evaluation.goals.emplace(goal, std::move(candidates));
        if (evaluation.refuted)
            break;
    }
    erase_node_trails_.erase(leaf);
    store_node_trails_.store(leaf, std::move(left));
    return evaluation;
}

#undef PUD_TRAIL_LEAF_EVALUATOR
#undef PUD_TRAIL_LEAF_EVALUATOR_TEMPLATE

#endif
