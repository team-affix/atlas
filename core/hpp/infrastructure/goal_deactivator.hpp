#ifndef GOAL_DEACTIVATOR_HPP
#define GOAL_DEACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IGoalExprs, typename IGoalCandidateRules, typename IActiveGoals>
struct goal_deactivator {
    goal_deactivator(IGoalExprs& ge, IGoalCandidateRules& gcr, IActiveGoals& ag);
    void deactivate(const goal_lineage*);
private:
    IGoalExprs& unset_goal_expr;
    IGoalCandidateRules& erase_goal_candidates;
    IActiveGoals& erase_active_goal;
};

template<typename IGE, typename IGCR, typename IAG>
goal_deactivator<IGE, IGCR, IAG>::goal_deactivator(IGE& ge, IGCR& gcr, IAG& ag)
    : unset_goal_expr(ge), erase_goal_candidates(gcr), erase_active_goal(ag) {}

template<typename IGE, typename IGCR, typename IAG>
void goal_deactivator<IGE, IGCR, IAG>::deactivate(const goal_lineage* gl) {
    erase_goal_candidates.erase(gl);
    unset_goal_expr.unset(gl);
    erase_active_goal.erase_active_goal(gl);
}

#endif
