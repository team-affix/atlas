#ifndef GOAL_DEACTIVATOR_HPP
#define GOAL_DEACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IUnsetGoalExpr, typename IEraseGoalCandidates, typename IEraseActiveGoal>
struct goal_deactivator {
    goal_deactivator(IUnsetGoalExpr& ge, IEraseGoalCandidates& gcr, IEraseActiveGoal& ag);
    void deactivate(const goal_lineage*);
private:
    IUnsetGoalExpr& unset_goal_expr_;
    IEraseGoalCandidates& erase_goal_candidates_;
    IEraseActiveGoal& erase_active_goal_;
};

template<typename IUGE, typename IEGC, typename IEAG>
goal_deactivator<IUGE, IEGC, IEAG>::goal_deactivator(IUGE& ge, IEGC& gcr, IEAG& ag)
    : unset_goal_expr_(ge), erase_goal_candidates_(gcr), erase_active_goal_(ag) {}

template<typename IUGE, typename IEGC, typename IEAG>
void goal_deactivator<IUGE, IEGC, IEAG>::deactivate(const goal_lineage* gl) {
    erase_goal_candidates_.erase(gl);
    unset_goal_expr_.unset(gl);
    erase_active_goal_.erase_active_goal(gl);
}

#endif
