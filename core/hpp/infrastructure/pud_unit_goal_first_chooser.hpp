#ifndef PUD_UNIT_GOAL_FIRST_CHOOSER_HPP
#define PUD_UNIT_GOAL_FIRST_CHOOSER_HPP

#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_leaf_evaluation.hpp"
#include "debug_assert.hpp"

// Chooses the visited leaf's smallest unit goal, one its evaluation found exactly one candidate for, and otherwise
// leaves the choice to the fallback chooser.
template<typename IGetEvaluation, typename IChooseGoal>
struct pud_unit_goal_first_chooser {
    pud_unit_goal_first_chooser(IGetEvaluation& get_evaluation, IChooseGoal& choose_goal);
    pud_body_goal_id choose(pud_global_node_id leaf) const;
private:
    IGetEvaluation& get_evaluation_;
    IChooseGoal& choose_goal_;
};

template<typename IGetEvaluation, typename IChooseGoal>
pud_unit_goal_first_chooser<IGetEvaluation, IChooseGoal>::pud_unit_goal_first_chooser(
    IGetEvaluation& get_evaluation, IChooseGoal& choose_goal)
    : get_evaluation_(get_evaluation)
    , choose_goal_(choose_goal) {}

template<typename IGetEvaluation, typename IChooseGoal>
pud_body_goal_id pud_unit_goal_first_chooser<IGetEvaluation, IChooseGoal>::choose(pud_global_node_id leaf) const {
    const pud_leaf_evaluation& evaluation = get_evaluation_.get();
    DEBUG_ASSERT(evaluation.leaf == leaf);
    for (const auto& [goal, candidates] : evaluation.goals) {
        const bool unit = candidates.size() == 1;
        if (unit)
            return goal;
    }
    return choose_goal_.choose(leaf);
}

#endif
