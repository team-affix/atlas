#ifndef GOAL_DEACTIVATOR_HPP
#define GOAL_DEACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IUnsetGoalExpr, typename IEraseGoalCandidates, typename IEraseActiveGoal>
struct goal_deactivator {
    goal_deactivator(IUnsetGoalExpr& ge, IEraseGoalCandidates& gcr, IEraseActiveGoal& ag);
    void deactivate(const goal_lineage*);
private:
    IUnsetGoalExpr& unset_goal_expr;
    IEraseGoalCandidates& erase_goal_candidates;
    IEraseActiveGoal& erase_active_goal;
};

template<typename IUGE, typename IEGC, typename IEAG>
goal_deactivator<IUGE, IEGC, IEAG>::goal_deactivator(IUGE& ge, IEGC& gcr, IEAG& ag)
    : unset_goal_expr(ge), erase_goal_candidates(gcr), erase_active_goal(ag) {}

template<typename IUGE, typename IEGC, typename IEAG>
void goal_deactivator<IUGE, IEGC, IEAG>::deactivate(const goal_lineage* gl) {
    erase_goal_candidates.erase(gl);
    unset_goal_expr.unset(gl);
    erase_active_goal.erase_active_goal(gl);
}

#endif
